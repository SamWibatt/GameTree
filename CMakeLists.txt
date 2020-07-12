cmake_minimum_required(VERSION 3.14)
#CMAKE_CXX_STANDARD_REQUIRED 
project(GameTreeProject)
# need one of these for a demo executable add_executable(gametree main.cpp tiled2sfml.cpp aseprite2sfml.cpp)
add_library(gametree gametree.cpp)
set_property(TARGET gametree PROPERTY CXX_STANDARD 11)

add_subdirectory("tiledreader")
target_link_directories(gametree PUBLIC "tiledreader")

add_subdirectory("asepritereader")
target_include_directories(gametree PUBLIC "asepritereader")
target_link_directories(gametree PUBLIC "asepritereader")

# platform-specific subdirectory for using gametree with sfml - need to reorganize
add_subdirectory("gtree_sfml")
target_include_directories(gametree PUBLIC "gtree_sfml")
target_link_directories(gametree PUBLIC "gtree_sfml")



# put demo stuff here - we will want the gametree library
add_executable(demosfml demo_sfml.cpp)
#  -lsfml-graphics -lsfml-window -lsfml-system
# used to have hardcoded sfml paths - this is from https://github.com/Bromeon/Thor/blob/master/CMakeLists.txt#L110-L120
# see https://en.sfml-dev.org/forums/index.php?topic=26160.0
find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)       # at some point audio too

if(NOT SFML_FOUND)
	set(SFML_DIR "" CACHE PATH "SFML top-level directory")
	message("\n-> SFML directory not found. Set SFML_DIR (not SFML_ROOT) to SFML's top-level path (containing \"include\" and \"lib\" directories).")
	message("-> Make sure the SFML libraries >= 2.5 with the same configuration (Release/Debug, Static/Dynamic) exist.\n")
endif()

# TEMPORARY!!!!!!!!!!!!!!!!!!!!!!!!!! CURRENTLY GAMETREE DEPENDS ON SFML!
target_link_libraries(gametree PUBLIC sfml-graphics sfml-window sfml-system)

# demosfml will always depend on sfml
target_link_libraries(demosfml PUBLIC gametree sfml-graphics sfml-window sfml-system)



