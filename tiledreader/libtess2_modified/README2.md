# Overview

This is a reorganized and modified form of [Mikko Mononen's Libtess2 library](https://github.com/memononen/libtess2). 

`tesselator.h` is the header to include.

There is a nice explanation of how GLU tesselation works at [Song Ho Ahn (안성호)'s OpenGL Tesselation page](https://www.songho.ca/opengl/gl_tessellation.html).

Apparently using GLU code anymore is quaint and ancient and even in 2013 was getting jerky comments on Stack Overflow about "Oh my God, people still use that?" But sometimes you just need to break a polygon into triangles and don't want to rope in the entirety of OpenGL to do it.

ANYWAY Modified by Sean Igo to be CMake-specific, just by bringing all the sources (not necessary, but I like flat directories for libraries) and includes together and making a CMakeLists.txt.

