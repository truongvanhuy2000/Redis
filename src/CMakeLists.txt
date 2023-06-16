# Get all the C++ files composing the render library
file(GLOB redis_FILES ${CMAKE_CURRENT_SOURCE_DIR}/redis/*.cpp)

# Create a shared library using these files
add_library(Redis SHARED ${redis_FILES})

# Define the include directory for the library
target_include_directories(Redis PUBLIC ${CMAKE_SOURCE_DIR}/include)