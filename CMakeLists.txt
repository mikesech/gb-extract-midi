cmake_minimum_required (VERSION 2.6) 
project (gb_aif_to_mid)

set(CMAKE_BUILD_TYPE Release)

set(sources main.cpp)

add_executable(${PROJECT_NAME} ${sources})

# This will have to be changed for compilers that specify
# C++11 support differently. (I should try to automate that
# in the future.)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
