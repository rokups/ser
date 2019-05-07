cmake_minimum_required(VERSION 3.13)
project(serialization)

set(CMAKE_CXX_STANDARD 14)

file (GLOB SOURCE_FILES *.cpp *.hpp *.h)
file (GLOB_RECURSE PUGIXML_SOURCE_FILES pugixml/*.cpp pugixml/*.hpp pugixml/*.h)
file (GLOB_RECURSE RAPODJSON_SOURCE_FILES rapidjson/*.cpp rapidjson/*.hpp rapidjson/*.h)

add_executable(serialization ${SOURCE_FILES} ${PUGIXML_SOURCE_FILES} ${RAPODJSON_SOURCE_FILES})
target_compile_options(serialization PRIVATE -fno-rtti)
