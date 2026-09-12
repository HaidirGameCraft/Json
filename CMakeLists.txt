cmake_minimum_required(VERSION 3.22.1)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(program)

set(LIBJSON_NAME "json")

file(GLOB CXX_SOURCES "src/*.cpp")

add_library(${LIBJSON_NAME} SHARED ${CXX_SOURCES})

add_executable(${PROJECT_NAME} main.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC "./src")
target_link_directories(${PROJECT_NAME} PUBLIC "." )
target_link_libraries(${PROJECT_NAME} PUBLIC ${LIBJSON_NAME})
