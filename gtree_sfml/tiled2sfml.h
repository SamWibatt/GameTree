//utility to turn tiledreader maps into sfml maps
//shouldn't be part of either library - though if it were to be, I'd put it in with tiledreader
//and keep separate from sfml bc sfml is the gameside

#ifndef TILED2SFML_H_INCLUDED
#define TILED2SFML_H_INCLUDED

#include<memory>
#include<unordered_set>
#include "tiledreader.h"
#include "sfmlmap.h"


using namespace tiledreader;
using namespace sfml_assets;


//atlas record is used in the mapping from original tile gid to subregion of the sf::Texture associated with it
//or for gid to the same in a list of images
// relocated to tiledreader
// class atlas_record {
//   public:
//     tile_index_t gid;
//     int tileset_index;    // index into map's list of tilesets, 0 for outward
//     int image_index;      // index into a given list of Bitmap images; for single texture sfml map layers, always 0
//     int ulx;
//     int uly; 
//     int wid;
//     int ht;

//   public:
//     atlas_record() {}
//     atlas_record(tile_index_t g, int ts, int ii, int ux, int uy, int w, int h) {
//       gid = g;
//       tileset_index = ts;
//       image_index = ii;
//       ulx = ux;
//       uly = uy;
//       wid = w;
//       ht = h;
//     }
// };

class Tiled2SFML {
  public:
    //data members

  public:
    Tiled2SFML(){}
    virtual ~Tiled2SFML() {}

  public:
    //member functions
    std::string get_name_for_bmp(tile_index_t gid);
    bool do_crunch(std::string name,std::string outputDir);
    std::pair<sf::Texture,std::unordered_map<tile_index_t, atlas_record>> MakeLayerTextureAndAtlas(std::shared_ptr<TiledMap> sptm, int layer_num);
    sf::VertexArray MakeLayerVertexArray(std::shared_ptr<TiledMap> sptm, int layer_num, sf::Texture layertex,
              std::unordered_map<tile_index_t,atlas_record> atlas);
    std::shared_ptr<SFMLMap> ConvertTiledMapToSFMLMap(std::shared_ptr<TiledMap> ptm, std::string outputDir);
};


#endif