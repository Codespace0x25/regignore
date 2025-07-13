#include <ctime>
#include <filesystem>
#include <fstream>
#include <future>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <regex>
#include <set>
#include <string>
#include <vector>

#include <CLI/CLI.hpp>

namespace fs = std::filesystem;

struct RegignoreEntry {
  bool isRegex;
  std::string rawLine;
  std::regex regexPattern;
  /* what dose `explicit` do here */
  explicit RegignoreEntry(const std::string &line) {
    rawLine = line;
    if (line.starts_with("r:")) {
      isRegex = true;
      std::string expr = line.substr(2);
      try {
        regexPattern = std::regex(expr, std::regex::ECMAScript);
      } catch (const std::regex_error &e) {
        std::cerr << "Invalid regex pattern: " << expr << "\n";
        isRegex = false;
      }
    } else {
      isRegex = false;
    }
  }
};

std::vector<RegignoreEntry> loadRegignoreFile(const std::string &filename,
                                              bool verbose) {
  std::vector<RegignoreEntry> entries;
  std::ifstream file(filename);
  std::string line;

  if (!file.is_open()) {
    std::cerr << "Failed to open " << filename << "\n";
    return entries;
  }

  if (verbose)
    std::cout << "Loading patterns from " << filename << "...\n";

  while (std::getline(file, line)) {
    if (line.empty() || line[0] == '#')
      continue;
    entries.emplace_back(line);
  }

  if (verbose)
    std::cout << "Loaded " << entries.size() << " patterns.\n";

  return entries;
}

bool matchesRegex(const std::string &pathStr,
                  const std::vector<RegignoreEntry> &entries) {
  for (const auto &entry : entries) {
    if (entry.isRegex && std::regex_search(pathStr, entry.regexPattern)) {
      return true;
    }
  }
  return false;
}

std::string gitignorePatternToRegex(const std::string &pattern) {
  std::string regexStr;
  regexStr.reserve(pattern.size() * 2);

  size_t i = 0;
  while (i < pattern.size()) {
    if (pattern[i] == '*') {
      if (i + 1 < pattern.size() && pattern[i + 1] == '*') {
        regexStr += ".*";
        i += 2;
      } else {
        regexStr += "[^/]*";
        i++;
      }
    } else if (pattern[i] == '?') {
      regexStr += "[^/]";
      i++;
    } else if (pattern[i] == '.') {
      regexStr += "\\.";
      i++;
    } else if (pattern[i] == '/') {
      regexStr += "/";
      i++;
    } else {
      static const std::string regexSpecialChars = R"(\^$+()[]{}|.)";
      if (regexSpecialChars.find(pattern[i]) != std::string::npos) {
        regexStr += '\\';
      }
      regexStr += pattern[i++];
    }
  }

  return regexStr;
}

bool isUnderStaticIgnore(const std::string &path,
                         const std::set<std::string> &staticLines) {
  fs::path fsPath(path);

  for (const auto &pattern : staticLines) {
    if (pattern.empty())
      continue;

    bool isDirPattern = pattern.ends_with('/');

    std::string trimmedPattern = pattern;
    if (isDirPattern) {
      trimmedPattern.pop_back();
    }

    std::string regexPatternStr = gitignorePatternToRegex(trimmedPattern);

    bool hasSlash = (trimmedPattern.find('/') != std::string::npos);

    std::regex regexPattern;

    if (hasSlash) {
      regexPattern =
          std::regex("^" + regexPatternStr + (isDirPattern ? "(/.*)?$" : "$"));
      if (std::regex_match(path, regexPattern)) {
        if (isDirPattern) {
          if (fs::is_directory(fsPath) || path.size() > trimmedPattern.size()) {
            return true;
          }
        } else {
          return true;
        }
      }
    } else {
      std::regex baseRegex("^" + regexPatternStr + "$");

      for (auto &part : fsPath) {
        std::string partStr = part.generic_string();
        if (std::regex_match(partStr, baseRegex)) {
          if (isDirPattern) {
            auto subpath = fsPath.lexically_relative(part);
            if (fs::is_directory(part) || !subpath.empty()) {
              return true;
            }
          } else {
            return true;
          }
        }
      }
    }
  }

  return false;
}

int generateGitignore(const std::string &regignoreFile,
                      const std::string &outputFile, const std::string &scanDir,
                      bool verbose) {
  if (!fs::exists(regignoreFile)) {
    std::cerr << regignoreFile << " file not found.\n";
    return 1;
  }

  auto entries = loadRegignoreFile(regignoreFile, verbose);
  std::set<std::string> staticLines;
  std::vector<RegignoreEntry> regexEntries;

  for (const auto &entry : entries) {
    if (entry.isRegex) {
      regexEntries.push_back(entry);
    } else {
      staticLines.insert(entry.rawLine);
    }
  }

  std::ofstream gitignore(outputFile);
  if (!gitignore.is_open()) {
    std::cerr << "Failed to write " << outputFile << "\n";
    return 1;
  }

  std::mutex mtx;
  std::set<std::string> matchedFiles;
  /*
  this is here as i trued to do it in the same line. but i woudl error as it did nto like

  `std::gmtime(&std::time(nullptr))`
  in the steaming to the filein the std::put_time(std::gmtime(&std::time(nullptr)), "%Y-%m-%d %H:%M:%S UTC")
                                                              ^ this was the problem but when i removed it,
							      it was being pasted by valuse and it did not
							      like that so i had to extrat it to by its self so
							      i can pass by refrentc and also do it with out error
							      here was comp error
							      
  ```
    regignore/main.cpp:195:52: error: lvalue required as unary ‘&’ operand
    195 |             << std::put_time(std::gmtime(&std::time(nullptr)),
        |                                           ~~~~~~~~~^~~~~~~~~
  ```
   */
  std::time_t curintTime = std::time(nullptr); 

  gitignore << "# This file was generated from " << regignoreFile << " at "
            << std::put_time(std::gmtime(&curintTime), "%Y-%m-%d %H:%M:%S UTC") << "\n";
  gitignore << "# Any changes must be made from the " << regignoreFile
            << " file otherwise they will not persist\n";

  gitignore << "\n# Static patterns:\n";
  for (const auto &line : staticLines) {
    gitignore << line << '\n';
  }

  if (verbose)
    std::cout << "Scanning directory " << scanDir << " recursively...\n";

  std::vector<fs::path> files;
  for (const auto &file : fs::recursive_directory_iterator(fs::path(scanDir))) {
    if (!file.is_regular_file())
      continue;

    fs::path rel = fs::relative(file.path(), fs::path(scanDir));
    std::string relStr = rel.generic_string();

    if (relStr == outputFile || relStr == regignoreFile)
      continue;
    if (isUnderStaticIgnore(relStr, staticLines))
      continue;

    files.push_back(rel);
  }

  if (verbose)
    std::cout << "Found " << files.size()
              << " files to check against regex patterns.\n";

  std::vector<std::future<void>> futures;
  for (const auto &file : files) {
    futures.push_back(std::async(std::launch::async,
                                 [&regexEntries, &matchedFiles, &mtx, file]() {
                                   std::string relStr = file.generic_string();
                                   if (matchesRegex(relStr, regexEntries)) {
                                     std::lock_guard<std::mutex> lock(mtx);
                                     matchedFiles.insert(relStr);
                                   }
                                 }));
  }

  for (auto &fut : futures)
    fut.get();

  if (!matchedFiles.empty()) {
    gitignore << "\n# Files matched by regex patterns:\n";
    for (const auto &line : matchedFiles) {
      if (!staticLines.contains(line)) {
        gitignore << line << '\n';
      }
    }
  }

  if (verbose)
    std::cout << outputFile << " generated successfully.\n";

  return 0;
}

int main(int argc, char **argv) {
  CLI::App app{"Regignore to Gitignore converter CLI tool"};

  std::string regignoreFile = ".regignore";
  std::string outputFile = ".gitignore";
  std::string scanDir = ".";
  bool verbose = false;
  bool init = false;

  app.add_option("-i,--input", regignoreFile, "Input .regignore filename")
      ->check(CLI::ExistingFile);
  app.add_option("-o,--output", outputFile, "Output .gitignore filename");
  app.add_option("-d,--directory", scanDir,
                 "Directory to scan (default: current directory)")
      ->check(CLI::ExistingDirectory);
  app.add_flag("-v,--verbose", verbose, "Enable verbose output");
  app.add_flag("-I,--init", init, "Create a new regignore file");

  CLI11_PARSE(app, argc, argv);
  if (init) {
    fs::path targetDir = fs::absolute(scanDir);
    fs::path regignorePath = targetDir / regignoreFile;
    if (fs::exists(regignorePath)) {
      std::cerr << "Warning: " << regignoreFile << " already exists in "
                << targetDir << "\n";
      return 1;
    }

    std::ofstream regignore(regignorePath);
    if (!regignore.is_open()) {
      std::cerr << "Failed to create " << regignoreFile << " file in "
                << targetDir << "\n";
      return 1;
    }

    regignore << "# Lines prefixed with r: will be interpreted as regex\n";
    regignore << "# Other lines will be treated as gitignore-style patterns\n";
    regignore << "# Example:\n";
    regignore << "# r:^build/.*\\.o$\n";
    regignore << "# *.log\n";

    std::cout << ".regignore initialized at: " << regignorePath << "\n";
    return 0;
  } else {
    return generateGitignore(regignoreFile, outputFile, scanDir, verbose);
  }
}
