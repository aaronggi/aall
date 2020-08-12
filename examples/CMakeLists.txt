project(examples)
find_package(cppzmq)
#ignore this for now pls
add_subdirectory(/users/aarongilbert/devel/cpp/aall/ ./build)
add_executable(consumer consumer.cpp)
add_executable(producer producer.cpp)
target_link_libraries(consumer logging cppzmq)
target_link_libraries(producer logging cppzmq)