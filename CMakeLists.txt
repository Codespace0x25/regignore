cmake_minimum_required(VERSION 3.20)
project(regignore)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(MINGW OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++ -D_GLIBCXX_USE_CXX11_ABI=1")
endif()

add_subdirectory(deps/CLI11)

add_executable(regignore main.cpp)
target_link_libraries(regignore PRIVATE CLI11::CLI11)

target_include_directories(regignore PRIVATE ${CMAKE_SOURCE_DIR}/deps/CLI11/include)

if(UNIX AND NOT APPLE)
  find_package(Threads REQUIRED)
  target_link_libraries(regignore PRIVATE Threads::Threads)
endif()
