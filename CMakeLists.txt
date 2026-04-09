cmake_minimum_required(VERSION 3.20)
project(BuildALang LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(Includes)
file(GLOB_RECURSE SOURCES "Src/*.cpp" "main.cpp")

add_executable(lpp ${SOURCES})

# --- Profile Management ---
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Mode: DEBUG (Sanitizers + Symbols)")
    target_compile_options(lpp PRIVATE -g -O0 -Wall -Wextra -fsanitize=address,undefined)
    target_link_options(lpp PRIVATE -fsanitize=address,undefined)
else()
    message(STATUS "Mode: RELEASE (Optimized)")
    target_compile_options(lpp PRIVATE -O3 -march=native -Wall)
endif()

# --- The Automation Target (NO EXTERNAL SCRIPT NEEDED) ---
add_custom_target(run
    COMMAND ./lpp ../../test.lpp
    COMMAND nasm -f elf64 out.s -o out.o
    COMMAND ld out.o -o out
    COMMAND ./out || true
    DEPENDS lpp
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Executing: Compiler -> Assembler -> Linker -> Execution"
)