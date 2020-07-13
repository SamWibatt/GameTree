//tiled2gt - converts Tiled map + assets to gametree format
// let's make a main that can read a tiled map and emit it in nice game-usable form
// or gametree form, really, either a metadata file readable straight into gametree objects
// plus png files for tilesets, or a single file with all those, or metadata as a cpp header,
// or whatever

#include <cstdio>
//#include <filesystem>     //this requires C++17 and I can't get stuff to compile with it in clang or gcc no matter what intellisense says, going back to 11
#include <system_error>
#include "tiledreader.h"
#include "CLI11.hpp"

using namespace tiledreader;


int main(int argc, char *argv[]) {
  //printf("Hello and welcome to Tiled2GT, the tenacious Tiled to Gametree conversion utility.\n");

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

  // Time for arg parsing
  // https://github.com/CLIUtils/CLI11#adding-options

  CLI::App app{"Hello and welcome to Tiled2GT, the tenacious Tiled to Gametree conversion utility."};

  // if(argc < 2) {
  //   fprintf(stderr,"Usage: Tiled2GT [OPTIONS] <Tiled file>\n");
  //   fprintf(stderr,"need:\n");
  //   fprintf(stderr,"- input map file, tmx (later maybe json too) \n");
  //   fprintf(stderr,"- output directory if we're writing tile sheets and metadata separately\n");
  //   fprintf(stderr,"- output format (separate files, glommed metadata & pngs to be read with loadFromMemory or \n");
  //   fprintf(stderr,"  similar)\n");
  //   fprintf(stderr,"- whether we want a header file made (or more generally output format - bin, json)\n");
  //   exit(1);
  // }

  std::string tiled_input_file;
  std::string output_dir;
  bool emit_header = false;
  std::string header_output_file;

  app.add_option("-i,--input", tiled_input_file, "Tiled map file (.tmx, soon .json)")
    ->required()
    ->check(CLI::ExistingFile);

  app.add_option("-o,--outputdir", output_dir, "Output directory for file(s), must already exist")
    ->required()
    ->check(CLI::ExistingPath);

  app.add_option("-c,--cheader", header_output_file, "Optional: emit c++ header of metadata to header. Filename only");


  CLI11_PARSE(app, argc, argv);

  //digest arguments

  // if a header was requested, note that we will emit one
  if(!header_output_file.empty()) emit_header = true;

  // create output directory if it doesn't exist - LATER - compiler is barfing at this saying filesystem isn't a namespace,
  // even after #include<filesystem>, so I have a bad setting somewhere, I think
  // drat, can't get this to work
  // std::error_code err;
  // std::filesystem::create_directories(output_dir, err);
  // printf("Error was %s\n",err.message().c_str());
  


  TiledReader tr;
  // //looks like a weird path like this works
  // std::string mappath = "assets/tiled_map/../../assets/./tiled_map/singlescreen.tmx";
  // printf("Mappath = %s\n",mappath.c_str());
  // std::shared_ptr<TiledMap> ptm = tr.read_map_file(mappath);
  // ******************************** YAY this is worky on entrapta with
  // /home/sean/dev/cpp/GameTree/build/tiledreader/tiled2gt -i ~/dev/GameNoodles/sfml/hello/assets/tiled_map/singlescreen.tmx -o ~/tmp/tiled2gt/
  // writes the tileset pngs and json files for the tile atlas!
  // didn't write the pngs on takkun, hm
  // aha, bc I didn't put the slash on the end of output dir, fix
  printf("About to convert Tiled file %s...\n",tiled_input_file.c_str());
  tr.read_map_file(tiled_input_file, output_dir);  

  

}