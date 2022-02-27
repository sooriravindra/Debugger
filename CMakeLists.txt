cmake_minimum_required(VERSION 3.1)
project(Debugger)

file(GLOB SRC_FILES src/*.cpp)
file(GLOB HEADER_FILES src/include/*.h)
set(LIB_FILES lib/linenoise/linenoise.c)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

# Clang format
add_custom_target(format COMMAND clang-format 
  -style=Google -i ${SRC_FILES} ${HEADER_FILES})

# Clang tidy
add_custom_target(lint COMMAND clang-tidy -p=${CMAKE_BINARY_DIR} 
  --checks=readability-*,google-* ${SRC_FILES} ${HEADER_FILES})

# Libelfin
add_custom_target(Libelfin COMMAND make 
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/lib/libelfin)


include_directories(src/include lib/linenoise lib/libelfin)

add_executable(Debugger ${SRC_FILES} ${LIB_FILES})

add_dependencies(Debugger Libelfin)
