cmake_minimum_required(VERSION 3.14)
add_library(asepritereader asepritereader.cpp)
set_property(TARGET asepritereader PROPERTY CXX_STANDARD 11)
# set JSON_BuildTests to OFF so that it doesn't build the unit tests - don't need them, and they're kind of big
# https://stackoverflow.com/questions/20239334/cmake-set-subdirectory-options
# CMake's option command more or less adds a boolean variable to the cache.
# If you want to override the default value of an option, simply add a variable of the same name to the cache yourself before pulling in the subproject:
# set(WITH_FUNCTION_X OFF CACHE BOOL "enable X functionality")
# add_subdirectory(subproject)
# Note that the set command does nothing if there is already a value of that name in the cache. If you want to overwrite any existing value, add the FORCE option to that command.
# Sample with FORCE
# set(WITH_FUNCTION_X OFF CACHE BOOL "enable X functionality" FORCE)
# add_subdirectory(subproject)
# Then LOL I see I had this solution showing in json's own readme - https://github.com/nlohmann/json#embedded
# set(JSON_BuildTests OFF CACHE BOOL "Build the unit tests when BUILD_TESTING is enabled.")
set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory("json")
target_include_directories(asepritereader PUBLIC "json/single_include") 
