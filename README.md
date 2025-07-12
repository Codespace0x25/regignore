# 🧹 regignore

**regignore** is a CLI tool that generates a `.gitignore` file from a more flexible `.regignore` file that supports both static glob-like patterns and full regular expressions.

---

## 📦 Features

- 🔹 **Static pattern matching** (like `.gitignore`)
- 🔹 **Regex support** using `r:<pattern>` syntax
- 🔹 **Multithreaded scanning** for fast performance
- 🔹 **Easy CLI interface** via [CLI11](https://github.com/CLIUtils/CLI11)
- 🔹 **Auto-generates `.gitignore`** from `.regignore`
- 🔹 **Directory selection** and file initialization with flags

---

## 📁 What is `.regignore`?

A more powerful alternative to `.gitignore`, where you can use:

- Static patterns:
  ```txt
  build/
  *.tmp```

* Regex patterns:

  ```txt
  r:.*\.(log|tmp|bak)$
  r:node_modules/.*\.map$
  ```

Regex lines must begin with `r:` to be treated as regular expressions.

---

## ⚙️ Usage

```bash
regignore [OPTIONS]
```

### 🔧 Options

| Flag              | Description                                   |
| ----------------- | --------------------------------------------- |
| `-i, --input`     | input .regignore file (default: .regignore)   |
| `-d, --directory` | Target directory (default: current directory) |
| `--init`          | Creates a new `.regignore` file and exits     |
| `-h, --help`      | Show usage information                        |
| `-v, --verbose`   | Show verbose logs                             |


## 📝 Example `.regignore`

```txt
# Regex pattern: ignore all logs and tmp files
r:.*\.(log|tmp)$

# Static patterns
build/
*.swp
```

---

## 📄 Output `.gitignore`

The generated `.gitignore` includes:

* Comments indicating source and origin
* Separation of static and regex-based entries

> ⚠️ Don't edit `.gitignore` directly — changes should be made in `.regignore`.

---

## 🛠 Build Instructions

### 📁 Dependencies

* C++20 compiler
* [CMake](https://cmake.org/) ≥ 3.20
* [CLI11](https://github.com/CLIUtils/CLI11) (included via `deps/`)

### 🔨 Build (Example)

```bash
git clone --recursive https://github.com/bonsall2004/regignore.git
cd regignore
mkdir build && cd build
cmake ..
cmake --build .
```

---

## 📬 Contributing

Pull requests and suggestions are welcome!
If you have ideas, bug reports, or feature requests, feel free to [open an issue](https://github.com/bonsall2004/regignore/issues).
