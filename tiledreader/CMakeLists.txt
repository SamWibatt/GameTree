cmake_minimum_required(VERSION 3.14)

add_library(tiledreader tiledreader.cpp) 
set_property(TARGET tiledreader PROPERTY CXX_STANDARD 11)

add_subdirectory("pugixml")
target_include_directories(tiledreader PUBLIC "pugixml/src") 
target_link_directories(tiledreader PUBLIC "pugixml/")

add_subdirectory("crunch_modified")
target_include_directories(tiledreader PUBLIC "crunch_modified")
target_link_directories(tiledreader PUBLIC "crunch_modified")

target_link_libraries(tiledreader pugixml-static crunch_modified)

