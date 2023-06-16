add_executable(RedisServer ${CMAKE_CURRENT_SOURCE_DIR}/server/main.cpp)
add_executable(RedisClient ${CMAKE_CURRENT_SOURCE_DIR}/client/client.cpp)

target_link_libraries(RedisServer PUBLIC Redis)