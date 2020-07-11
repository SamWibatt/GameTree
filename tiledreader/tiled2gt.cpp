//tiled2gt - converts Tiled map + assets to gametree format
// let's make a main that can read a tiled map and emit it in nice game-usable form
// or gametree form, really, either a metadata file readable straight into gametree objects
// plus png files for tilesets, or a single file with all those, or metadata as a cpp header,
// or whatever

#include <cstdio>
#include "tiledreader.h"
#include "CLI11.hpp"

using namespace tiledreader;

int main(int argc, char *argv[]) {
  printf("Hello and welcome to Tiled2GT, the tenacious Tiled to Gametree conversion utility.\n");

  //parse command line.
  //WRITE THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  // need:
  // - input map file, tmx (later maybe json too) 
  // - output directory if we're writing tile sheets and metadata separately
  // - output format (separate files, glommed metadata & pngs to be read with loadFromMemory or 
  //   similar)
  // - whether we want a header file made (or more generally output format - bin, json)
  // why not use https://github.com/CLIUtils/CLI11
  // got CLI11.hpp release 1.9.1 from https://github.com/CLIUtils/CLI11/releases

  // LEFT OFF HERE: PUT IN ARG PARSING, see
  // https://github.com/CLIUtils/CLI11#adding-options

  if(argc < 2) {
    fprintf(stderr,"Usage: Tiled2GT [OPTIONS] <Tiled file>\n");
    fprintf(stderr,"need:\n");
    fprintf(stderr,"- input map file, tmx (later maybe json too) \n");
    fprintf(stderr,"- output directory if we're writing tile sheets and metadata separately\n");
    fprintf(stderr,"- output format (separate files, glommed metadata & pngs to be read with loadFromMemory or \n");
    fprintf(stderr,"  similar)\n");
    fprintf(stderr,"- whether we want a header file made (or more generally output format - bin, json)\n");
    exit(1);
  }


  TiledReader tr;
  // //looks like a weird path like this works
  // std::string mappath = "assets/tiled_map/../../assets/./tiled_map/singlescreen.tmx";
  // printf("Mappath = %s\n",mappath.c_str());
  // std::shared_ptr<TiledMap> ptm = tr.read_map_file(mappath);


}