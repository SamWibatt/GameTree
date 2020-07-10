cmake_minimum_required(VERSION 3.14)
add_library(crunch binary.cpp bitmap.cpp GuillotineBinPack.cpp hash.cpp 
    lodepng.cpp MaxRectsBinPack.cpp packer.cpp Rect.cpp str.cpp crunch_main.cpp)
set_property(TARGET crunch PROPERTY CXX_STANDARD 11)
