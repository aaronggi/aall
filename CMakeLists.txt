cmake_minimum_required(VERSION 3.15)

project(logging LANGUAGES CXX)

message("${CMAKE_MODULE_PATH}")

set(CMAKE_CXX_STANDARD 17)
set(BUILD_SHARED_LIBS FALSE)


#find_package(ZeroMQ)
find_package(cppzmq)

#add_subdirectory(libzmq)
#add_subdirectory(cppzmq)
add_subdirectory(external/fmt EXCLUDE_FROM_ALL)
add_subdirectory(external/date)

set(${HEADERS} include/log.hpp)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE include/)

target_link_libraries(${PROJECT_NAME} INTERFACE
        fmt::fmt-header-only
        date
        cppzmq)
        #libzmq
        #cppzmq)


install (TARGETS ${PROJECT_NAME} DESTINATION /usr/local/include)