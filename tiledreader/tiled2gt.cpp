//tiled2gt - converts Tiled map + assets to gametree format
// let's make a main that can read a tiled map and emit it in nice game-usable form
// or gametree form, really, either a metadata file readable straight into gametree objects
// plus png files for tilesets, or a single file with all those, or metadata as a cpp header,
// or whatever

#include <cstdio>
#include <fstream>
#include <cerrno>
//#include <filesystem>     //this requires C++17 and I can't get stuff to compile with it in clang or gcc no matter what intellisense says, going back to 11
#include <system_error>
#include "tiledreader.h"
#include "gametree.h"
#include "CLI11.hpp"
#include "cppcodec/base64_rfc4648.hpp"
using base64 = cppcodec::base64_rfc4648;


using namespace tiledreader;
using namespace gt;
using json = nlohmann::json;


// CONVERSION ====================================================================================

std::shared_ptr<GTTiledMapLayer> tiled_tile_layer_to_gt_tile_layer(std::shared_ptr<TiledMapLayer> ptl, std::map<tile_index_t,GTtile_index_t>& gidmapping) {
  std::shared_ptr<GTTiledMapLayer> pgtml = std::shared_ptr<GTTiledMapLayer>(new GTTiledMapLayer());

  //ok now step through the mapcells and remap
  // * scan ptl->mapcells to produce pgtml->tile_map, copying any 0 across to 0, 
  //   any other number to the map entry for it in the map we built before
  //   OR could just put gidmapping[0] = 0
  pgtml->tile_map.resize(ptl->mapcells.size());
  std::transform(ptl->mapcells.begin(),ptl->mapcells.end(), pgtml->tile_map.begin(), [&gidmapping](tile_index_t i){ return gidmapping[i]; });

  //fill in simple data members
  pgtml->layer_tilewid = ptl->layer_w;
  pgtml->layer_tileht = ptl->layer_h;
  pgtml->tile_pixwid = ptl->tile_width;
  pgtml->tile_pixht = ptl->tile_height;

  return pgtml;
}

std::shared_ptr<GTObjectsMapLayer> tiled_obj_layer_to_gt_obj_layer(std::shared_ptr<TiledMapLayer> ptl, std::map<tile_index_t,GTtile_index_t>& gidmapping) {
  std::shared_ptr<GTObjectsMapLayer> pgoml = std::shared_ptr<GTObjectsMapLayer>(new GTObjectsMapLayer());

  //convert ptl->tile_objects to pgoml->tile_objects
  //ptl->tile_objects is these class TiledMapObjectTileLocation {
  // int id;
  // tile_index_t gid;
  // float orx;      //origin x/y, not upper left - can be other locations
  // float ory;
  // int wid;
  // int ht;
  pgoml->tile_objects.clear();

  // so the remapping here is to replace the original TiledMapObjectTileLocation's gid with its remapped index into tile_atlas.
  // **************** HOW DO WE GET OFFSET X AND Y? TILED DOESN'T PROVIDE THEM CURRENTLY 
  int offx = 0;
  int offy = 0; 
  for(auto tob : ptl->tile_objects) {
    pgoml->tile_objects.push_back(std::shared_ptr<GTObjectTile>(
      new GTObjectTile(gidmapping[tob->gid], tob->orx, tob->ory, tob->wid, tob->ht, offx, offy))
    );
  }

  return pgoml;
}

std::shared_ptr<GTMapLayer> tiled_layer_to_gt_layer(std::shared_ptr<TiledMap> ptm, int layer_num, std::string output_dir) {
  std::shared_ptr<GTMapLayer> plyr;

  //what type plyr ends up being depends on its tiled-map type
  std::shared_ptr<TiledMapLayer> ptl = ptm->layers[layer_num];

  // ptl->layer_atlas needs to be turned into pgtml->tile_atlas
  // so: 
  // * scan ptl->layer_atlas to build mapping of unique gids to contiguous numbers.
  //   remember to reserve tile 0 for no-tile
  std::map<tile_index_t,GTtile_index_t> gidmapping;
  gidmapping[0] = 0; //reserve the empty tile

  //drat, layer_atlas is an unordered map. Let's order it all
  std::set<tile_index_t> ordered_gids;
  for(auto tatl : *(ptl->layer_atlas)) ordered_gids.insert(tatl.first);
  GTtile_index_t j = 0;
  for(auto gid : ordered_gids) if(gid != 0) gidmapping[gid] = j++;

  //now for layer-type-specific stuff

  if(ptl->type == TL_TiledLayer) {
    plyr = tiled_tile_layer_to_gt_tile_layer(ptl, gidmapping);
  } else if(ptl->type == TL_ObjectLayer) {
    plyr = tiled_obj_layer_to_gt_obj_layer(ptl, gidmapping);
  } else {
    //unknown!
    fprintf(stderr, "*** ERROR: unknown layer type %d\n",ptl->type);
    return nullptr;
  }

  //in any case, we need the image
  //plyr->image_data is just the entire contents of the png from ptl
  //does ptl record that? no, but see tiledreader do_crunch
  //ASSUMING THERE IS ONLY ONE IMAGE FOR THE TILESET, NAMED LIKE THIS
  std::string png_path = output_dir + "Tileset_" + ptl->layer_name + "0.png";

  FILE * pngfp = std::fopen(png_path.c_str(), "rb");

  if(pngfp == nullptr) {
    fprintf(stderr,"*** ERROR: failed to open png file %s - error %s\n",png_path.c_str(),std::strerror(errno));
    return nullptr;
  }
  //get file size and allocate buffer for it
  std::fseek(pngfp,0,SEEK_END);
  auto pngsiz = std::ftell(pngfp);
  std::fseek(pngfp,0,SEEK_SET);
  plyr->image_data.resize(pngsiz);    //see if this works; I think we want resize and not reserve
  std::fread(plyr->image_data.data(), sizeof(uint8_t), plyr->image_data.size(), pngfp);
  fclose(pngfp);

  // now build the contiguous vector pgtml->tile_atlas. pre-insert 0, iterate over ordered_gids as before, get the same ordering.
  plyr->tile_atlas.clear();
  plyr->tile_atlas.push_back(GTTile(0,0,0,0));         // put in no-tile in index 0
  for(auto gid : ordered_gids) 
    if(gid != 0) {
      atlas_record &ar = (*(ptl->layer_atlas))[gid];
      plyr->tile_atlas.push_back(GTTile(ar.ulx, ar.uly, ar.wid, ar.ht));
    }

  return plyr;
}

std::shared_ptr<GTMap> tiled_map_to_gt_map(std::shared_ptr<TiledMap> ptm, std::string output_dir) {
  std::shared_ptr<GTMap> pmap = std::shared_ptr<GTMap>(new GTMap());

  // from the top. Step through layers, converting each.
  for(auto j = 0; j < ptm->layers.size(); j++) {
    std::shared_ptr<GTMapLayer> plyr = tiled_layer_to_gt_layer(ptm,j, output_dir);
    if(plyr != nullptr) {
      pmap->layers.push_back(plyr);
    }
    else {
      fprintf(stderr,"*** ERROR: failed to convert layer %d\n",j);
      return nullptr;
    }
  }

  return pmap;
}

// MAIN ==========================================================================================


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
  


  TiledReader tr;
  // //looks like a weird path like this works
  // std::string mappath = "assets/tiled_map/../../assets/./tiled_map/singlescreen.tmx";
  // printf("Mappath = %s\n",mappath.c_str());
  // std::shared_ptr<TiledMap> ptm = tr.read_map_file(mappath);
  // ******************************** YAY this is worky on entrapta with
  // /home/sean/dev/cpp/GameTree/build/tiledreader/tiled2gt -i ~/dev/GameNoodles/sfml/hello/assets/tiled_map/singlescreen.tmx -o ~/tmp/tiled2gt/
  // writes the tileset pngs!
  printf("About to read Tiled file %s...\n",tiled_input_file.c_str());

  // maybe rename this method bc it saves pngs out, too
  std::shared_ptr<TiledMap> ptm = tr.read_map_file(tiled_input_file, output_dir);  

  // so: now to convert TiledReader objects into GT objects.
  // that should all be in this file.
  std::shared_ptr<GTMap> pmap = tiled_map_to_gt_map(ptm, output_dir);

  // now emit it!!!!!!!!!!!!!!!!!!
  std::string json_filename = output_dir + "mapout.json";   //GET A REAL NAME  
  printf("Writing json file %s\n",json_filename.c_str());
  json jmap;
  pmap->add_to_json(jmap);
  std::ofstream json_outstream(json_filename);
  //setw(4) does 4 space pretty print
  json_outstream << std::setw(4) << jmap << std::endl;

  if(emit_header) {
    // EMIT HEADER!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  }

}