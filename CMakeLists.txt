cmake_minimum_required(VERSION 3.12)
project(dbpool)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include_directories("/usr/include/mysql")
link_directories("/usr/lib64/mysql")
include_directories(pool)

add_executable(tp test/tp.cpp)
target_link_libraries(tp pthread)

add_executable(db test/db.cpp pool/db.cpp)
target_link_libraries(db mysqlclient pthread)

