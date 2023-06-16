cmake_minimum_required(VERSION 3.10)

# Project description
project(RedisFromScratch 
        VERSION 1.0
        LANGUAGES CXX
        )

# Process the src and apps directories
add_subdirectory(src)
add_subdirectory(apps)

# Force the generation of a compile_commands.json file to provide autocompletion for IDEs
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE CACHE BOOL "" FORCE)