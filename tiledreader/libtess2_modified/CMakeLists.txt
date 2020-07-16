cmake_minimum_required(VERSION 3.14)

add_library(libtess2 bucketalloc.c dict.c geom.c mesh.c priorityq.c sweep.c tess.c) 
set_property(TARGET tiledreader PROPERTY CXX_STANDARD 11)
