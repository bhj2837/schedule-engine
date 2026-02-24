cmake_minimum_required(VERSION 3.15)
project(schedule-engine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Crow 헤더 경로
include_directories(include)

add_executable(schedule-engine
    src/main.cpp
    src/scheduler.cpp
)

# Windows 소켓 라이브러리 (Crow 필수)
if(WIN32)
    target_link_libraries(schedule-engine ws2_32)
endif()