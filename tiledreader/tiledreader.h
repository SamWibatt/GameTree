#ifndef TILEDREADER_H_INCLUDED
#define TILEDREADER_H_INCLUDED

#include <memory>
#include <png.h>     // need to have libpng
#include <string.h>   //for old-school libpng memory stuff
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <sstream>
#include <set>
#include <map>
#include "pugixml.hpp"
//set up include dirs to have json/single_include
#include "nlohmann/json.hpp"


using namespace pugi;
using json = nlohmann::json;


namespace tiledreader {

  typedef uint32_t tile_index_t;

  //atlas record is used in the mapping from original tile gid to subregion of the sf::Texture associated with it
  //or for gid to the same in a list of images
  class atlas_record {
    public:
      tile_index_t gid;
      int tileset_index;    // index into map's list of tilesets, 0 for outward
      int image_index;      // index into a given list of Bitmap images; for single texture sfml map layers, always 0
      int ulx;
      int uly; 
      int wid;
      int ht;

    public:
      atlas_record() {}
      atlas_record(tile_index_t g, int ts, int ii, int ux, int uy, int w, int h) {
        gid = g;
        tileset_index = ts;
        image_index = ii;
        ulx = ux;
        uly = uy;
        wid = w;
        ht = h;
      }

      public:
        void add_to_json(json& j, std::string layer_atlas_name) {
          j[layer_atlas_name].push_back(std::map<std::string, int>{
            {"gid", int(gid) },
            {"tileset_index", tileset_index },
            {"image_index", image_index },
            {"ulx", ulx},
            {"uly", uly},
            {"wid", wid},
            {"ht", ht}
          });
        }
  };


  class TilesetImage {
    public:
      //properties about source image - can they have more than one?
      int image_width;
      int image_height;
      std::string image_trans;    //HTML-type color of transparency
      std::string image_source;   //path to (png) file with tile imagery

    public:
      TilesetImage(int iw, int ih, std::string it, std::string is) {
        image_width = iw;
        image_height = ih;
        image_trans = it;
        image_source = is;
      }
  };

  typedef enum {
    TS_TilesetImage,
    TS_CollectionOfImages,
    TS_Unknown
  } tileset_type;

  class Tileset {
      //follow tiled's naming
      public:
        //type of tileset - currently either a single image that gets ripped into same-size tiles or a collection of separate images
        tileset_type type = TS_Unknown;

        //these are for "tileset image" tilesets - but get supplied for collections of images too
        int first_gid;        //from tiled, if a map uses multiple tilesets, the tile_count indices after this refer to tiles in this tileset
        std::string source;   //the tileset file path, not image source
        int tile_width; //width of an individual tile
        int tile_height;
        int tile_count;
        int margin;
        int spacing;    //inter-tile gap, in pixels
        int columns;
        //LATER HERE GO MEMBERS FOR "COLLECTION OF IMAGES" tilesets if there are any special


        //images that participate in this tileset
        std::vector<TilesetImage> images;

        //actual tiles. Currently uncompressed. 
        //vector of little snips of data, which can be any size.
        //index into this vector is what's stored in a TiledReader's mapdata.
        //and those little hunks are from PNGImage, uncompressed RGBA data for now
        //this is the sort of thing that changes on an arduino version
        std::vector<std::shared_ptr<uint8_t>> tiledata;

        //little helpy functions for converters
        bool contains_gid(tile_index_t gid) { return (gid >= first_gid && gid < (first_gid + tile_count)); }
        //returns a tuple of (image #, ulx, uly, wid, ht) or nullptr if the gid isn't in this one
        std::tuple<int, int, int, int, int> image_num_and_coords_for_gid(tile_index_t gid);

  };

  typedef enum : int {
    TL_TiledLayer,
    TL_ObjectLayer,
    TL_Unknown
  } tilemaplayer_type;

  //shape subclass for containing shape objects: point, polygon, rectangle, ellipse
  typedef enum : int {
    TOS_Point,
    TOS_Polygon,
    TOS_Rectangle,
    TOS_Ellipse,
    TOS_Unknown
  } tileobjshape_type;

  class TiledObjectShape {
    public:
      tileobjshape_type shape_type;
      float origin_x, origin_y;
      float bbox_ulx, bbox_uly;
      float bbox_lrx, bbox_lry;
      std::string name;
      std::string type;       // assigned in Tiled in object properties when shape is highlighted, meaning assigned by game
      std::vector<std::pair<float,float>> polypoints;     // polygon points for polygons

    public:
      TiledObjectShape() {
        type = TOS_Unknown;
        origin_x = origin_y = 0.0;
        bbox_ulx = bbox_uly = bbox_lrx = bbox_lry = 0.0;
        polypoints.clear();
      }

      TiledObjectShape(tileobjshape_type sty, float orx, float ory, float bbulx, float bbuly, float bblrx, float bblry, std::string& nm, std::string& ty) {
        shape_type = sty;
        origin_x = orx;
        origin_y = ory;
        bbox_ulx = bbulx;
        bbox_uly = bbuly;
        bbox_lrx = (sty == TOS_Point) ? bbox_ulx : bblrx;     //force points to have 0 w/h
        bbox_lry = (sty == TOS_Point) ? bbox_uly : bblry;
        name = nm; 
        type = ty;
        polypoints.clear();
      }

      ~TiledObjectShape() {}

      void add_polypoint(float x1, float y1) {
        polypoints.push_back(std::pair<float,float>(x1,y1));
      }
  };

  //location of a tile listed in an object layer, e.g.
  //  <object id="3" gid="86" x="196.859" y="120.71" width="16" height="16"/>
  class TiledMapObjectTileLocation {
    public:
      int id;
      tile_index_t gid;
      float orx;      //origin x/y, not upper left - can be other locations
      float ory;
      int wid;
      int ht;

    public:
      TiledMapObjectTileLocation(int i, tile_index_t gd, float x, float y, int w, int h) {
        id = i;
        gid = gd;
        orx = x;
        ory = y;
        wid = w;
        ht = h;
      }
  };

  class TiledMapLayer {
    public:
      tilemaplayer_type type;
      int layer_id; // = grandkid.attribute("id").as_int();
      std::string layer_name; // = grandkid.attribute("name").as_string();
      int layer_w; // = grandkid.attribute("width").as_int();
      int layer_h; // = grandkid.attribute("height").as_int();
      std::vector<tile_index_t> mapcells;
      std::string encoding;
      int tile_width;   //pixel width of tiles, if applicable
      int tile_height;
      //for object group layers:
      std::vector<std::shared_ptr<TiledMapObjectTileLocation>> tile_objects;
      // here's the tile atlas recording where the post-crunch texture vertices are per tile
      std::shared_ptr<std::unordered_map<tile_index_t, atlas_record>> layer_atlas;
      //need something for shapes like circles and polygons
      std::vector<TiledObjectShape> shapes;


    public:
      TiledMapLayer() {}
      TiledMapLayer(tilemaplayer_type typ, int lid, std::string nam, int lw, int lh) {
        type = typ;
        layer_id = lid;
        layer_name = nam;
        layer_w = lw;
        layer_h = lh;
      }

    public:

      void add_to_json(json& j, std::string layer_metadata_name) {
        j[layer_metadata_name]["layer_id"] = layer_id;
        j[layer_metadata_name]["layer_name"] = layer_name;

        if(type == TL_TiledLayer) {
          j[layer_metadata_name]["type"] = "tiled";

          // int layer_w; // = grandkid.attribute("width").as_int();
          // int layer_h; // = grandkid.attribute("height").as_int();
          // std::vector<tile_index_t> mapcells;
          // std::string encoding;
          // int tile_width;   //pixel width of tiles, if applicable
          // int tile_height;

          j[layer_metadata_name]["layer_w"] = layer_w;
          j[layer_metadata_name]["layer_h"] = layer_h;
          j[layer_metadata_name]["encoding"] = encoding;
          j[layer_metadata_name]["tile_width"] = tile_width;
          j[layer_metadata_name]["tile_height"] = tile_height;

          //can you do this?
          j[layer_metadata_name]["mapcells"] = mapcells;

          // these are for animated and odd-size tiles & c? ... how DOES that work? Oh, it's in the atlas
          for(auto bjecto : tile_objects) {
            j[layer_metadata_name]["tile_objects"].push_back(std::map<std::string,int>{
              {"id", bjecto->id},
              {"gid", int(bjecto->gid)},
              {"orx", bjecto->orx},
              {"ory", bjecto->ory},
              {"wid", bjecto->wid},
              {"ht", bjecto->ht}
            });
          }

        } else if(type == TL_ObjectLayer) { 
          j[layer_metadata_name]["type"] = "object";

          for(auto bjecto : tile_objects) {
            j[layer_metadata_name]["tile_objects"].push_back(std::map<std::string,int>{
              {"id", bjecto->id},
              {"gid", int(bjecto->gid)},
              {"orx", bjecto->orx},
              {"ory", bjecto->ory},
              {"wid", bjecto->wid},
              {"ht", bjecto->ht}
            });
          }
        } else {
          j[layer_metadata_name]["type"] = "unknown";
        }

      }
  };

  class TiledMap {
    public:
      std::vector<std::shared_ptr<Tileset>> tilesets;
      std::vector<std::shared_ptr<TiledMapLayer>> layers;
  };

  class TiledReader {
    public:
      //actual map, currently uncompressed indices into tiledata, no flags or anything yet
      //when it gets to that let's just use a struct
      std::vector<tile_index_t> mapdata; 

    public:
      TiledReader() {}
      virtual ~TiledReader() {}

    public:
      virtual std::shared_ptr<std::unordered_map<tile_index_t, atlas_record>> MakeTilesheetPNGAndAtlas(std::shared_ptr<TiledMap> sptm, int layer_num, 
                                                                                                        std::string outputDir);
      std::string get_name_for_bmp(tile_index_t gid);
      bool do_crunch(std::string name,std::string outputDir);
      virtual std::shared_ptr<TiledMap> read_map_file(std::string filename, std::string outputDir);
  };


  class TiledReaderTilesetReader{
    public:
      //data members

    public:
      TiledReaderTilesetReader() {}
      ~TiledReaderTilesetReader() {}

    public:
      //hand in root node of a <tileset> and this returns a smart pointer to our tileset object
      //parent file is the path to the tmx file this is reading from (with filename still on it)
      std::shared_ptr<Tileset> handle(pugi::xml_node &tsroot, std::string parentfile);
      bool build_from_png_and_map(std::string filename, std::vector<tile_index_t> &mapdata);
  };

  class TiledReaderLayerReader{
    public:
      //data members

    public:
      TiledReaderLayerReader() {}
      ~TiledReaderLayerReader() {}

    public:
      TiledObjectShape MakePolygon(pugi::xml_node& polyroot, float origin_x, float origin_y, std::string area_name, std::string area_type);
      //hand in root node of a <layer> and this returns a smart pointer to our layer object
      //parent file is the path to the tmx file this is reading from (with filename still on it)
      std::shared_ptr<TiledMapLayer> handle(pugi::xml_node &lyrroot, std::string parentfile);
  };
}

#endif