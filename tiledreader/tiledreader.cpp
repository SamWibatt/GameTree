#include <cstdio>
#include <string>
#include "tiledreader.h"

using namespace pugi;

namespace tiledreader {

  // TILESET =============================================================================================  
  //returns a tuple of (image #, ulx, uly, wid, ht) or -1s if the gid isn't in this one
  std::tuple<int, int, int, int, int> Tileset::image_num_and_coords_for_gid(tile_index_t gid) {
    int imagenum = -1;
    int ulx = -1;
    int uly = -1;
    int wid = -1;
    int ht = -1;
    if(!contains_gid(gid))
      return std::make_tuple(imagenum, ulx, uly, wid, ht);    //all -1s

    // HEY FILL THESE IN
    // if this is a single-image tileset
    if(type == TS_TilesetImage) {
      imagenum = 0;
      //deduce coordinates from margin, spacing, tile width and tile height
      //MARGIN NOT YET SUPPORTED
      int ind = gid - first_gid;
      int tile_col = ind % columns;
      int tile_row = ind / columns;
      ulx = tile_col * (tile_width + spacing);
      uly = tile_row * (tile_height + spacing);
      wid = tile_width;
      ht = tile_height;
    } else if(type == TS_CollectionOfImages) {
      //image number is just gid - first_gid; upper left x and y are 0, extents are image's full size
      imagenum = gid - first_gid;
      ulx = 0;
      uly = 0;
      wid = images[imagenum].image_width;
      ht = images[imagenum].image_height;
    } else {
      printf("WARNING: tileset type %d unknown\n",type);
        return std::make_tuple(-1, -1, -1, -1, -1);
    }

    return std::make_tuple(imagenum, ulx, uly, wid, ht);
  }


  // TILEDREADERTILESETREADER ============================================================================  

  // WHAT DO WE WANT TO END UP WITH FROM A PNG?
  // I could see a couple different things - tile set, single image, sprite sheet with
  // arbitrarily positioned sprites (kenney xml, crunch)

  //let's just make a class to encapsulate a png, can break it out into a library later


  //so let's say the way we want to handle top-level objects is each with their own subclass 
  std::shared_ptr<Tileset> TiledReaderTilesetReader::handle(pugi::xml_node &tsroot, std::string parentfile) {
    //this will create a std::shared_ptr<Tileset> and return it if successful
    //otherwise return nullptr
    // <tileset firstgid="1" source="canari_8bit_topdown.tsx"/>
    // <tileset firstgid="481" source="KenneyUIPixelPack.tsx"/>
    //printf("    Fiddle! Tileset! firstgid %d source %s\n",
    //        std::stoi(tsroot.attribute("firstgid").value()),tsroot.attribute("source").value());

    std::shared_ptr<Tileset> ts = std::shared_ptr<Tileset>(new Tileset);
    ts->first_gid = tsroot.attribute("firstgid").as_int();
    ts->source = tsroot.attribute("source").as_string();

    //so THIS needs to pop open the file in "source", normalizing its path wrt the tmx file
    //libpng 1.6.37 or newer installed - looks like focal has it
    //and looks like we can just stick paths together if they're relative
    //let's assume source is relative...?
    auto const pos=parentfile.find_last_of('/');
    std::string parentpath = parentfile.substr(0,pos);
    std::string sourcetsx = parentpath + "/" + tsroot.attribute("source").value();
    //printf("    tsx file is %s\n",sourcetsx.c_str());

    //need to parse that one as xml too!
    xml_document tsxdoc;
    xml_parse_result result = tsxdoc.load_file(sourcetsx.c_str());
    if(result.status != status_ok) {
      printf("    TSX read faily!\n");
      return nullptr;
    }

    //printf("    YAY Loaded! Here are the tsx's top-level children:\n");
    for(auto kid : tsxdoc.children()) {
      //printf("    - |%s|\n",kid.name());      //debug
      //toplevel tileset looks like
      //<tileset version="1.4" tiledversion="1.4.0" name="canari_8bit_topdown" 
      //    tilewidth="16" tileheight="16" tilecount="480" columns="16">
      //with space in between
      //<tileset version="1.4" tiledversion="1.4.0" name="KenneyUIPixelPack" 
      //    tilewidth="16" tileheight="16" spacing="2" tilecount="990" columns="30">
      /*
      */
      ts->tile_width = kid.attribute("tilewidth").as_int();
      ts->tile_height = kid.attribute("tileheight").as_int();
      ts->tile_count = kid.attribute("tilecount").as_int();
      ts->margin = kid.attribute("margin").as_int();
      ts->spacing = kid.attribute("spacing").as_int();    //inter-tile gap, in pixels
      ts->columns = kid.attribute("columns").as_int();


      //printf("      - firstgid %d tilewd %d tileht %d margin %d spacing %d tilect %d cols %d\n",
      //        ts->first_gid, ts->tile_width, ts->tile_height, ts->margin, ts->spacing, ts->tile_count, ts->columns);

      //ok could step back here and check to see if this is a "tileset image" tileset or a "collection of images" tileset.
      //I think the quick way to tell is that if kid.children has an "image" child, it's a tileset image, otherwise it's not.
      xml_node imgnode = kid.first_element_by_path("image");
      if(imgnode.type() == node_null) {
        //printf("      YAY it is a collection of images tileset!\n");
        ts->type = TS_CollectionOfImages;
        //so let's just shovel images in there
        //printf("      its children:\n");
        for (auto grandkid : kid.children()) {
          if(std::string(grandkid.name()) == "tile") {
            //check to see if it's an image tile - could also have these types
            //  <tile id="95" terrain="2,3,3,3"/>
            //  <tile id="112">
            //   <animation>
            //    <frame tileid="112" duration="150"/>
            //    <frame tileid="113" duration="150"/>
            //    <frame tileid="114" duration="150"/>
            //    <frame tileid="115" duration="150"/>
            //   </animation>
            //  </tile>
            // and what we want is e.g.
            // <tile id="3">
            //   <image width="68" height="93" source="../Kenney_Assets_1/Generic_Items/genericItem_color_105.png"/>
            // </tile>
            // SOOOO we need to know how to map the tile ID to the source - can we count on them being in order?
            // looks like it, even if they have other baggage like terrain types or collision shapes. But let's enforce it
            //  <tile id="8" terrain=",,0,0">
            //   <image width="117" height="113" source="../Kenney_Assets_1/Generic_Items/genericItem_color_124.png"/>
            //   <objectgroup draworder="index" id="2">
            //    <object id="1" x="1.72697" y="35.6908">
            //     <polygon points="0,0 11.5132,12.6645 0,69.6546 19.5724,76.5625 89.227,74.8355 97.8618,59.2928 93.8322,48.9309 116.859,35.1151 98.4375,-14.3914 73.6842,-18.9967 72.5329,-25.3289 55.8388,-25.3289 56.4145,-30.5099 37.4178,-35.1151 33.9638,-23.602 18.9967,-21.875 18.4211,-8.63487"/>
            //    </object>
            //   </objectgroup>
            //  </tile>
            // NOTE THAT THERE IS A NON-BLANK TILE 0 IN COLLECTION OF IMAGES TILESETS!
            xml_node imgnode = grandkid.first_element_by_path("image");
            if(imgnode.type() != node_null) {
              //yay we have an image source to append 
              tile_index_t id = tile_index_t(grandkid.attribute("id").as_ullong());

              //check to see that the size of 
              if(ts->images.size() != id) {
                //mismatch in tile index to ts->images index, which we currently can't handle
                printf("ERROR: tile id %u doesn't match images index %u\n",id,uint32_t(ts->images.size()));
                return nullptr; 
              }

              auto const pngpos=sourcetsx.find_last_of('/');
              std::string tsxpath = sourcetsx.substr(0,pngpos);
              std::string sourcepng = tsxpath + "/" + imgnode.attribute("source").as_string();
              //printf("        png file is %s\n",sourcepng.c_str());
              ts->images.push_back(TilesetImage(imgnode.attribute("width").as_int(), 
                                                imgnode.attribute("height").as_int(), 
                                                imgnode.attribute("trans").as_string(),
                                                sourcepng));
              //printf("        image tile: imgwd %d imght %d transp |%s| src %s\n", ts->images.back().image_width, ts->images.back().image_height,
              //ts->images.back().image_trans.c_str(), ts->images.back().image_source.c_str());
            }
          }
        }
      } else {
        //printf("      YAY it is a single image tileset\n");
        ts->type = TS_TilesetImage;
        //printf("      its children:\n");
        for (auto grandkid : kid.children()) {
          //image child looks like
          //<image source="../8BIT_CanariPack_TopDown/TILESET/PixelPackTOPDOWN8BIT.png" width="256" height="480"/>
          // with a transparent color
          //<image source="../Kenney_Assets_1/UI_Pixel_Pack/UIpackSheet_magenta.png" trans="ff00ff" width="538" height="592"/>
          if(std::string(grandkid.name()) == "image") {
            //memorize for later png ripping
            auto const pngpos=sourcetsx.find_last_of('/');
            std::string tsxpath = sourcetsx.substr(0,pngpos);
            std::string sourcepng = tsxpath + "/" + grandkid.attribute("source").as_string();
            //printf("        png file is %s\n",sourcepng.c_str());

            ts->images.push_back(TilesetImage(grandkid.attribute("width").as_int(), grandkid.attribute("height").as_int(),
                              grandkid.attribute("trans").as_string(), 
                              sourcepng));
            
            //printf("        - imgwd %d imght %d transp |%s| src %s\n", ts->images.back().image_width, ts->images.back().image_height,
            //  ts->images.back().image_trans.c_str(), ts->images.back().image_source.c_str());

          } else if(std::string(grandkid.name()) == "tile") {
            //************************* THIS WILL END UP HANDLING THINGS LIKE ANIMATED TILES, TERRAINS, COLLISION
            //************************* THIS WILL END UP HANDLING THINGS LIKE ANIMATED TILES, TERRAINS, COLLISION
            //************************* THIS WILL END UP HANDLING THINGS LIKE ANIMATED TILES, TERRAINS, COLLISION
            //************************* THIS WILL END UP HANDLING THINGS LIKE ANIMATED TILES, TERRAINS, COLLISION
            //************************* THIS WILL END UP HANDLING THINGS LIKE ANIMATED TILES, TERRAINS, COLLISION
            //************************* THIS WILL END UP HANDLING THINGS LIKE ANIMATED TILES, TERRAINS, COLLISION
            //************************* THIS WILL END UP HANDLING THINGS LIKE ANIMATED TILES, TERRAINS, COLLISION
          } else {
            printf("- INFO: element not supported %s\n",grandkid.name());
          }
          //debug
          //if(std::string(grandkid.name()) != "tile")    //there are lots of these, verbose
          //  printf("      - |%s|\n",grandkid.name());
        }
      }
    }

    return ts;
  }

  // TILEDREADERLAYERREADER ==============================================================================

  std::shared_ptr<TiledMapLayer> TiledReaderLayerReader::handle(pugi::xml_node &lyrroot, 
      std::string parentfile) {

    std::shared_ptr<TiledMapLayer> tl;

    if(std::string(lyrroot.name()) == "layer") {
      //layer
      //<layer id="1" name="Background" width="29" height="16">
      //<data encoding="csv"> <<-- data is a child of layer
      // and if its encoding is csv the csv data is the text child
      // of that data element, I believe. 
      tl = std::shared_ptr<TiledMapLayer>(new TiledMapLayer(TL_TiledLayer,
          lyrroot.attribute("id").as_int(),
          lyrroot.attribute("name").as_string(), lyrroot.attribute("width").as_int(),
          lyrroot.attribute("height").as_int()));

      //printf("    Rad! Layer! ID %d name %s width %d height %d\n", tl->layer_id,
      //  tl->layer_name.c_str(), tl->layer_w, tl->layer_h); 
      
      //now get the map cells
      tl->mapcells.clear();
      xml_node datanode = lyrroot.first_element_by_path("data");
      tl->encoding = datanode.attribute("encoding").as_string();
      if(tl->encoding == "csv") {
        std::string maptext = datanode.text().as_string();
        //strip whitespace and we should have only csv numbers
        maptext.erase(std::remove_if(maptext.begin(), maptext.end(), ::isspace), maptext.end());
        //then just split by commas - https://www.fluentcpp.com/2017/04/21/how-to-split-a-string-in-c/
        tl->mapcells.reserve(tl->layer_w * tl->layer_h);
        std::istringstream iss(maptext);
        std::string token;
        while (std::getline(iss, token, ','))
            tl->mapcells.push_back(tile_index_t(std::stoul(token)));
        //printf("    Hey found data node with encoding %s and %d entries\n",
        //  datanode.attribute("encoding").as_string(), int(tl->mapcells.size()));
      } else {
        printf("Unsupported layer data encoding %s\n", datanode.attribute("encoding").as_string());
        //Looks like there's sample code for the other formats here:
        //https://doc.mapeditor.org/en/stable/reference/tmx-map-format/
      }  
    } else if(std::string(lyrroot.name()) == "objectgroup") {
      //printf("Oh hey an ogject layer!\n");
      //so... how do we differentiat an ogject layer from a tile layer?
      //should probably make it explicit, as with tileset types
      //for the moment, tile height and width of 0 will do
      //but there are other types
      //so we'll do that
      tl = std::shared_ptr<TiledMapLayer>(new TiledMapLayer(TL_ObjectLayer,
        std::stoi(lyrroot.attribute("id").as_string()),
        lyrroot.name(),0,0));
      /* let us read this as an ogject group layer!
      <objectgroup id="5" name="Ogjects">
        //so - "object" children can be a tile off in a random spot: look for the "gid" attribute
        <object id="3" gid="86" x="196.859" y="120.71" width="16" height="16"/>
        <object id="4" gid="86" x="280.63" y="181.635" width="16" height="16"/>
        <object id="5" gid="86" x="330.893" y="95.579" width="16" height="16"/>
        <object id="10" gid="1479" x="214.426" y="139.214" width="117" height="113"/>
        // can just put those gids in mapcells

        //or they can be shapes: I suppose we ignore these for now
        //but we do need some kind of mechanism for them - like add a shapes list to the layer object?
        <object id="6" name="LArrow" x="33.2984" y="168.092">
        <polygon points="0,0 4.50765,-4.50765 12.7959,-4.21683 12.9413,4.65306 4.07142,4.65306"/>
        </object>
      </objectgroup>
      */
      for(auto kid : lyrroot.children()) {
        if(std::string(kid.name()) == "object") {
          int gidley = kid.attribute("gid").as_int();
          if(gidley != 0) {
            //printf("Hey found ogject with gid %d, will put in mapcells\n",gidley);
            //oh wait need to make sure we don't do duplicates - or does that matter? The converter to sfml can handle it
            tl->mapcells.push_back(tile_index_t(gidley));
            //SAVE OFF THE POSITIONS! bc there isn't a grid to determine it
            //<object id="10" gid="1479" x="214.426" y="139.214" width="117" height="113"/>
            tl->tile_objects.push_back(
              std::shared_ptr<TiledMapObjectTileLocation>(new TiledMapObjectTileLocation(
                kid.attribute("id").as_int(), 
                gidley, 
                kid.attribute("x").as_float(), 
                kid.attribute("y").as_float(),
                kid.attribute("width").as_int(), 
                kid.attribute("height").as_int())));
            // printf("Saved off tile object: id %d gid %u orx %f ory %f wid %d ht %d\n",
            //         tl->tile_objects.back()->id,
            //         tl->tile_objects.back()->gid,
            //         tl->tile_objects.back()->ory,
            //         tl->tile_objects.back()->ory,
            //         tl->tile_objects.back()->wid,
            //         tl->tile_objects.back()->ht);
          }
        }
      }
    }
    return tl;
  }

  // TILEDREADER =========================================================================================

  std::shared_ptr<TiledMap> TiledReader::read_map_file(std::string filename) {
    printf("Reading Tiled map %s\n",filename.c_str());
    xml_document doc;
    xml_parse_result result = doc.load_file(filename.c_str());
    if(result.status != status_ok) {
      printf("Read faily!\n");
      return nullptr;
    }

    //Map object we're building
    std::shared_ptr<TiledMap> tm = std::shared_ptr<TiledMap>(new TiledMap);

    //get some top level element readers ready
    TiledReaderTilesetReader tsr;
    TiledReaderLayerReader tlr;

    //ok, do something with it!
    //printf("Loaded!\n");
    for(auto kid : doc.children()) {
      //printf("- |%s|\n",kid.name());
      //let's do one more level down - here is where tilesets and layers live!
      for(auto grandkid : kid.children()) {
        //printf("  - |%s|\n",grandkid.name());
        // handle according to name!
        if(std::string(grandkid.name()) == "tileset") {
          //read the tileset info and add to map object
          std::shared_ptr<Tileset> tsp = tsr.handle(grandkid,filename);   //can return null for unsupported type
          if(tsp != nullptr) tm->tilesets.push_back(tsp);
          else printf("WARNING: found unsupported / erroneous tileset |%s|\n",filename.c_str());
        } else if(std::string(grandkid.name()) == "layer") {
          std::shared_ptr<TiledMapLayer> tlp = tlr.handle(grandkid,filename);
          if(tm != nullptr) tm->layers.push_back(tlp);
          else printf("WARNING: found unsupported / erroneous map |%s|\n",filename.c_str());
        } else if(std::string(grandkid.name()) == "objectgroup") {
          //aha, an ogject layer!
          std::shared_ptr<TiledMapLayer> tlp = tlr.handle(grandkid,filename);
          if(tm != nullptr) tm->layers.push_back(tlp);
          else printf("WARNING: found unsupported / erroneous map |%s|\n",filename.c_str());
        } else {
          //unknown top level node, warn that we don't handle it
          //and then implement the handling if it's important!
          printf("    Regretfully unsupported: %s\n",grandkid.name());
        }
      }
    }

    //set layers' tile sizes. FOR NOW JUST USE THE SAME SIZE FOR ALL, TAKEN FROM
    //FIRST TILESET AND THIS NEEDS TO BE FIDDLED IF I HAVE DIFFERENT TILE SIZES ON DIFFT LAYERS
    for(auto layer : tm->layers) {
      layer->tile_height = tm->tilesets[0]->tile_height;
      layer->tile_width = tm->tilesets[0]->tile_width;
    }

    //so now we have a map with layers and tilesets. Is that all we need?
    //for now, yes, just need to throw it back.
    return tm;

    //second pass..... go see e.g. sfmlmap
  }
}