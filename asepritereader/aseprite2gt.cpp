#include "gametree.h"
#include "asepritereader.h"
#include "CLI11.hpp"

using namespace gt;
using namespace asepreader;

int main(int argc, char *argv[]) {
  CLI::App app{"Hello and welcome to Aseprite2GT, the asymptotic Aseprite to Gametree conversion utility."};

  std::string aseprite_input_file;
  std::string output_dir;
  bool emit_header = false;
  std::string header_output_file;
  bool verify_json = true;

  app.add_option("-i,--input", aseprite_input_file, "Aseprite map file (.json)")
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

  // make sure output dir has a slash on the end
  // MAKE THIS PLATFORM INDEPENDENT
  if(output_dir[output_dir.length()-1] != '/') {
    output_dir += "/";
  }

  // create output directory if it doesn't exist - LATER - compiler is barfing at this saying filesystem isn't a namespace,
  // even after #include<filesystem>, so I have a bad setting somewhere, I think
  // drat, can't get this to work
  // std::error_code err;
  // std::filesystem::create_directories(output_dir, err);
  // printf("Error was %s\n",err.message().c_str());

  AseSpriteReader ar;

  printf("About to read Aseprite file %s...\n",aseprite_input_file.c_str());

  std::shared_ptr<AseSprite> pts = ar.read_sprite_file(aseprite_input_file);

  // FINISH THIS!!!!!!!!!!!!!!!!!!!!!!
  // then we need something analogous to this
  // so: now to convert TiledReader objects into GT objects.
  // that should all be in this file.
  //std::shared_ptr<GTMap> pmap = tiled_map_to_gt_map(ptm, output_dir);

}