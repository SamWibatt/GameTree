cmake_minimum_required(VERSION 3.14)

add_library(tiledreader tiledreader.cpp) 
set_property(TARGET tiledreader PROPERTY CXX_STANDARD 11)

add_subdirectory("pugixml")
target_include_directories(tiledreader PUBLIC "pugixml/src") 
target_link_directories(tiledreader PUBLIC "pugixml/")

# the library this emits is just called libcrunch.a
add_subdirectory("crunch_modified")
target_include_directories(tiledreader PUBLIC "crunch_modified")
target_link_directories(tiledreader PUBLIC "crunch_modified")

target_link_libraries(tiledreader pugixml-static crunch)

add_executable(tiled2gt tiled2gt.cpp)
target_include_directories(tiled2gt PUBLIC "pugixml/src") 
target_link_libraries(tiled2gt tiledreader)
