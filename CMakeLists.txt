cmake_minimum_required(VERSION 3.26)
project(chat_sockets)

set(CMAKE_CXX_STANDARD 17)


add_executable(server server.cpp)
add_executable(client client.cpp)
target_link_libraries(server pthread)
target_link_libraries(client pthread)