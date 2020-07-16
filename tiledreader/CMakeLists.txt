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

# emits liblibtess2.a :P
add_subdirectory("libtess2_modified")
target_include_directories(tiledreader PUBLIC "libtess2_modified")
target_link_directories(tiledreader PUBLIC "libtess2_modified")

# for conversion to gametree - THIS IS GROSS
target_include_directories(tiledreader PUBLIC "..")
target_link_directories(tiledreader PUBLIC "..")

# is this even necessary if we're not doing add_subdir?
set(JSON_BuildTests OFF CACHE INTERNAL "")
# this is gross but it gets unhappy if I do 2 copies of the library
# do I need the subdir if I'm just going to use the header?
#add_subdirectory("../asepritereader/json")
target_include_directories(tiledreader PUBLIC "../asepritereader/json/single_include") 

target_link_libraries(tiledreader pugixml-static crunch libtess2)

add_executable(tiled2gt tiled2gt.cpp)
target_include_directories(tiled2gt PUBLIC "../cppcodec")
target_include_directories(tiled2gt PUBLIC "pugixml/src") 
target_include_directories(tiled2gt PUBLIC "libtess2_modified")
target_link_directories(tiled2gt PUBLIC "libtess2_modified")
target_link_libraries(tiled2gt tiledreader gametree libtess2)
