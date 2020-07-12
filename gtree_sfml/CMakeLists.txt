cmake_minimum_required(VERSION 3.14)
add_library(gtree_sfml sfmlmap.cpp sfmlsprite.cpp)
set_property(TARGET gtree_sfml PROPERTY CXX_STANDARD 11)

# needs to be able to see gametree - this is gross
target_include_directories(gtree_sfml PUBLIC "..")

# used to have hardcoded sfml paths - this is from https://github.com/Bromeon/Thor/blob/master/CMakeLists.txt#L110-L120
# see https://en.sfml-dev.org/forums/index.php?topic=26160.0
find_package(SFML 2.5 COMPONENTS graphics REQUIRED)

if(NOT SFML_FOUND)
	set(SFML_DIR "" CACHE PATH "SFML top-level directory")
	message("\n-> SFML directory not found. Set SFML_DIR (not SFML_ROOT) to SFML's top-level path (containing \"include\" and \"lib\" directories).")
	message("-> Make sure the SFML libraries >= 2.5 with the same configuration (Release/Debug, Static/Dynamic) exist.\n")
endif()

target_link_libraries(gtree_sfml PUBLIC sfml-graphics)
