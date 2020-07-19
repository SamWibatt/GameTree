#include <cstdio>
#include <string>
#include "tiledreader.h"
//stuff from crunch
#include "crunch_main.h"
#include <fstream>
#include <iomanip>

using namespace pugi;
using json = nlohmann::json;


namespace tiledreader {

  // HEY MAKE THIS OS INDEPENDENT
  std::string path_sep = "/";

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
    //DO THE PATHS IN THIS USE THE OS-SPEC?
    auto const pos=parentfile.find_last_of(path_sep[0]);
    std::string parentpath = parentfile.substr(0,pos);
    std::string sourcetsx = parentpath + path_sep + tsroot.attribute("source").value();
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

              auto const pngpos=sourcetsx.find_last_of(path_sep[0]);
              std::string tsxpath = sourcetsx.substr(0,pngpos);
              std::string sourcepng = tsxpath + path_sep + imgnode.attribute("source").as_string();
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
            auto const pngpos=sourcetsx.find_last_of(path_sep[0]);
            std::string tsxpath = sourcetsx.substr(0,pngpos);
            std::string sourcepng = tsxpath + path_sep + grandkid.attribute("source").as_string();
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

  TiledObjectShape TiledReaderLayerReader::MakePolygon(pugi::xml_node& polyroot, float origin_x, float origin_y, std::string area_name, std::string area_type) {
    // polygons look like this:
    // <object id="1" type="nogo" x="1.33333" y="27">
    //    <polygon points="-1,0 6.66667,7.33333 6.66667,13.3333 19.3333,19.3333 20.3333,38.3333 53.75,71.4167 53.5417,88.9167 62.3333,98.7917 79.7083,99.5833 113.833,134.083 112.875,62.625 91,63 87,53.6667 87.6667,39.6667 56.6667,11 55.6667,-5.33333 36.6667,-25.6667 -0.666667,-26"/>
    // </object>
    // polyroot is pointing at the polygon object
    TiledObjectShape poly;

    poly.shape_type = TOS_Polygon;      //maybe we should have a better name for a list of triangles
    poly.name = area_name;
    poly.type = area_type;

    // <polygon points="0,0 0,69.3333 19.6667,68.3333 34.6667,87.3333 57.6667,87"/>
    // break it up into tokens by spaces
    std::istringstream iss(polyroot.child("polygon").attribute("points").as_string());
    std::string token;
    while (std::getline(iss, token, ' ')) {
      //then split token over comma and take the resulting numbers as x, y
      auto commapos = token.find_first_of(',');
      if(commapos == std::string::npos) {
        printf("- WARNING: found a polygon with point coordinates with no comma \"%s\", skipping, id %d, name \"%s\"\n",token.c_str(),
          polyroot.attribute("id").as_int(),polyroot.attribute("name").as_string());
        poly.type = TOS_Unknown;
        poly.polypoints.clear();
        return poly;
      }
      float ptx = std::stof(token.substr(0,commapos));
      float pty = std::stof(token.substr(commapos+1,token.length()));
      //poly.add_polypoint(ptx+origin_x,pty+origin_y);    // get absolute location of points
      //let's go back to just ptx and pty, non-absoluted, bc that's how GTPolygon will want it?
      poly.add_polypoint(ptx,pty);
    }

    //work out the bounding box - it'll look something like this
    //float bbox_ulx = std::min_element(v.begin(), v.end());
    // should we normalize it so the furthest-left point is at 0? I like that idea
    float min_x = MAXFLOAT;
    float min_y = MAXFLOAT;
    float max_x = -MAXFLOAT;
    float max_y = -MAXFLOAT;
    for(auto pt:poly.polypoints) {
      if (pt.first < min_x) min_x = pt.first;
      if (pt.second < min_y) min_y = pt.second;
      if (pt.first > max_x) max_x = pt.first;
      if (pt.second > max_y) max_y = pt.second;
    }

    printf("min_x %f min_y %f max_x %f max_y %f\n",min_x,min_y,max_x,max_y);

    //then set it bbox in poly
    poly.bbox_ulx = min_x;
    poly.bbox_uly = min_y;
    poly.bbox_lrx = max_x;
    poly.bbox_lry = max_y;

    //so I have some confusion when the origin is not the same as the top left of the bounding box.
    //as is the case with First River. oh hey and it's entered ccl and last point is in upper left of image!
    // I guess we could just leave the origin and points alone - but then the bounding box is kind of wrong? We do know its
    // width and height but not how to position it.
    poly.origin_x = origin_x;
    poly.origin_y = origin_y;

    printf("// origin %f %f bounding box %f,%f - %f,%f\n",poly.origin_x,poly.origin_y,poly.bbox_ulx,poly.bbox_uly,poly.bbox_lrx,poly.bbox_lry);

    // pre-normalize points debug
    printf("std::vector<std::vector<float>> polyverts = {\n");      
    for(auto pt: poly.polypoints) 
      printf("  { %f, %f }, \n", pt.first, pt.second);
    printf("};\n");
    printf("//polygon has %lu verts\n",poly.polypoints.size());

    return poly;
  }

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
        //lyrroot.name(),   //this isn't the name! It just names it objectgroup :P
        lyrroot.attribute("name").as_string(),
        0,0));
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
          if(kid.attribute("gid")) {
            // only tile objects have gids, so this is a tile object
            int gidley = kid.attribute("gid").as_int();
            if(gidley != 0) {
              // tile 0 is no-tile but you also get it if as_int() didn't work, so skip zeros
              //printf("Hey found tile ogject with gid %d, will put in mapcells\n",gidley);
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
            } //else warn got a tile 0?
          } else if(kid.attribute("type")) {
            // polygon / rect / ellipse spotter
            // all objects have x and y, can have type, name, width, height
            // we're looking for *meaningful* shapes here, which I'm going to determine by "type" - DOCUMENT THAT!
            // are there illegal values for these we can sanity check?
            float origin_x = kid.attribute("x").as_float();
            float origin_y = kid.attribute("y").as_float();
            std::string area_type = std::string(kid.attribute("type").as_string());
            std::string area_name = std::string(kid.attribute("name").as_string());   //not required, things like triggers might use

            //determine what we're dealing with - 
            if(kid.child("point")) {
              //point object has a point child
              // We SHOULD handle it - type "start_point" could be where the character spawns into the level
              printf("- Found a point of type %s, name \"%s\", id %d!\n",area_type.c_str(), area_name.c_str(), kid.attribute("id").as_int());
              tl->shapes.push_back(TiledObjectShape(TOS_Point,origin_x,origin_y,0.0,0.0,0.0,0.0,area_name,area_type));
            } else if(kid.child("text")) {
              //text object has a text child
              printf("- Warning: text objects not yet supported - type %s, name \"%s\", id %d\n",area_type.c_str(), area_name.c_str(), kid.attribute("id").as_int());
              continue;
            } else if(kid.child("ellipse")) {
              //ellipse object has an ellipse child
              printf("- Found an ellipse of type %s, name \"%s\", id %d!\n",area_type.c_str(), area_name.c_str(), kid.attribute("id").as_int());
              tl->shapes.push_back(TiledObjectShape(TOS_Ellipse,origin_x,origin_y,0.0,0.0,
                                                    kid.attribute("width").as_float(),
                                                    kid.attribute("height").as_float(),
                                                    area_name,area_type));
            } else if(kid.child("polygon")) {
              //polygon object has a polygon child
              printf("- Found a polygon of type %s, name \"%s\", id %d!\n",area_type.c_str(), area_name.c_str(), kid.attribute("id").as_int());
              if(kid.child("polygon").attribute("points")) {
                TiledObjectShape poly = MakePolygon(kid,origin_x,origin_y,area_name,area_type);
                if(poly.shape_type == TOS_Polygon) {
                  tl->shapes.push_back(poly);
                } else {
                  printf("*** ERROR: polygon creation failed\n");
                  return nullptr;
                }
              } else {
                printf("- WARNING: found a polygon with no points attribute, skipping, id %d, name \"%s\"\n", kid.attribute("id").as_int(),
                  kid.attribute("name").as_string());
              }
            } else {
              //assume it's a rectangle!
              printf("- Found a rectangle of type %s, name \"%s\", id %d!\n",area_type.c_str(), area_name.c_str(), kid.attribute("id").as_int());
              tl->shapes.push_back(TiledObjectShape(TOS_Rectangle,origin_x,origin_y,0.0,0.0,
                                                    kid.attribute("width").as_float(),
                                                    kid.attribute("height").as_float(),
                                                    area_name,area_type));
            }
          } else {
            printf("- Warning: found a non-tile object, id %d first child (%s) name \"%s\" without a type, skipping\n",kid.attribute("id").as_int(),
              kid.first_child().name(), kid.attribute("name").as_string());
          }
        }
      }
    }
    return tl;
  }

  // TILEDREADER =========================================================================================


  // transplanted from tiled2sfml - this part isn't platform-specific.

  std::string TiledReader::get_name_for_bmp(tile_index_t gid) {
      char gidnum[9];
    sprintf(gidnum,"%04d",gid);
    return "g" + std::string(gidnum);    
  }

  // uses lots of crunch globals, ick - big chunk of its old main()
  bool TiledReader::do_crunch(std::string name,std::string outputDir) {
    //time for some raw crunch code
    //Sort the bitmaps by area
    //printf("- sorting bitmap list of %lu elements\n",bitmaps.size());
    sort(bitmaps.begin(), bitmaps.end(), [](const Bitmap* a, const Bitmap* b) {
        return (a->width * a->height) < (b->width * b->height);
    });
    
    //Pack the bitmaps
    //clear packers here in case we've done other layers

    packers.clear();
    //printf("- Packing bitmaps\n");
    while (!bitmaps.empty())
    {
        if (optVerbose)
            cout << "packing " << bitmaps.size() << " images..." << endl;
        auto packer = new Packer(optSize, optSize, optPadding);
        packer->Pack(bitmaps, optVerbose, optUnique, optRotate);
        packers.push_back(packer);
        if (optVerbose)
            cout << "finished packing: " << name << to_string(packers.size() - 1) << " (" << packer->width << " x " << packer->height << ')' << endl;
    
        if (packer->bitmaps.empty())
        {
            cerr << "packing failed, could not fit bitmap: " << (bitmaps.back())->name << endl;
            return false;
        }
    }
    
    //Save the atlas image
    //printf("- Saving atlas images\n");
    for (size_t i = 0; i < packers.size(); ++i)
    {
        if (optVerbose)
            cout << "writing png: " << outputDir << name << to_string(i) << ".png" << endl;
        packers[i]->SavePng(outputDir + name + to_string(i) + ".png");
    }

    //Save the atlas xml
    if (optXml)
    {
        //printf("- Saving atlas xml\n");
        if (optVerbose)
            cout << "writing xml: " << outputDir << name << ".xml" << endl;
        
        ofstream xml(outputDir + name + ".xml");
        xml << "<atlas>" << endl;
        for (size_t i = 0; i < packers.size(); ++i)
            packers[i]->SaveXml(name + to_string(i), xml, optTrim, optRotate);
        xml << "</atlas>";
    }
    
    //Save the atlas json
    if (optJson)
    {
        //printf("- Saving atlas json\n");

        if (optVerbose)
            cout << "writing json: " << outputDir << name << ".json" << endl;
        
        ofstream json(outputDir + name + ".json");
        json << '{' << endl;
        json << "\t\"textures\":[" << endl;
        for (size_t i = 0; i < packers.size(); ++i)
        {
            json << "\t\t{" << endl;
            packers[i]->SaveJson(name + to_string(i), json, optTrim, optRotate);
            json << "\t\t}";
            if (i + 1 < packers.size())
                json << ',';
            json << endl;
        }
        json << "\t]" << endl;
        json << '}';
    }
    return true;
  }


  //this saves the .png as a side effect, which I guess is ok
  std::shared_ptr<std::unordered_map<tile_index_t, atlas_record>> TiledReader::MakeTilesheetPNGAndAtlas(std::shared_ptr<TiledMap> sptm, int layer_num, std::string outputDir) {
    //  std::unordered_map<tile_index_t,atlas_record> new_atlas;      //atlas of gid->atlas record for result
    auto new_atlas = std::shared_ptr<std::unordered_map<tile_index_t, atlas_record>>(new std::unordered_map<tile_index_t, atlas_record>());
    std::unordered_map<tile_index_t,atlas_record> orig_atlas;     //gid -> atlas record for original
    std::shared_ptr<TiledMapLayer> sptml = sptm->layers[layer_num];

    // Steps appropriate to all layer types (so far):
    //Step 1: figure out which imagery we need. May not be all of it, esp if using 3rd party assets
    //        which gids participate in this layer? Easy enough, just build a set. std::set is sorted and iterable yay
    std::set<tile_index_t> unique_gids;
    //let's assume that layers encoded with ways other than csv have the same ultimate mapcells entry
    for(tile_index_t mc : sptml->mapcells) {
      //mask off rotation bits - they will still be there in the layer mapcells when we build the vertex array
      unique_gids.insert(mc & 0x00FFFFFF);
    }
    //debug
    //printf("Got these unique gids: "); for(tile_index_t gid : unique_gids) printf("%u ", gid); printf("\n");

    //Step 2. where does their imagery originate?
    //        need to map a gid to a region on a source image.
    //        read the images as crunch Bitmaps.
    // also need to build the set of original tileset images
    // ... only those we use, though.
    //indexed by pair(tileset number within map, image number within tileset)
    std::map<std::pair<int, int>,std::shared_ptr<Bitmap>> tileset_bitmaps;

    //printf("- Building bitmap list\n");

    for(tile_index_t gid:unique_gids) {
      if(gid != 0) {    //0 is special case blank tile, just skipped and no quad emitted, doesn't need imagery

        //so: we need to know which tileset it belongs to, which image within that, coords within the image.
        //sounds like a job for member functions of Tileset!
        bool foundgid = false;
        int imgnum = -1, ulx = -1, uly = -1, wid = -1, ht = -1;
        for(auto tsnum = 0; tsnum < sptm -> tilesets.size(); tsnum++) {
          if(sptm->tilesets[tsnum]->contains_gid(gid)) {
            // so tsnum is the tileset number within map that it belongs to
            foundgid = true;
            tie(imgnum, ulx, uly, wid, ht) = sptm->tilesets[tsnum]->image_num_and_coords_for_gid(gid);
            //printf("YAY gid %u is in tileset %u - image %d, ulx %d, uly %d, w %d, h %d\n",gid, tsnum,
            //        imgnum, ulx, uly, wid, ht);

            // so we need a registry of crunch Bitmaps that contain the entire tileset images from which these come, yes?
            // s.t. each is only loaded once.
            // so, map from tileset number/image number to Bitmap, yes? yes.
            std::pair<int,int> tsimg = std::make_pair(tsnum,imgnum);
            if(tileset_bitmaps.find(tsimg) == tileset_bitmaps.end()) {
              //hasn't been loaded yet, load that suckoir
              //hardcoding premultiply and trim to false
              string bmpname = "tileset_" + std::to_string(tsnum) + "_img_" + std::to_string(imgnum);
              //printf("Need to load bitmap %s from %s\n",bmpname.c_str(),sptm->tilesets[tsnum]->images[imgnum].image_source.c_str());
              tileset_bitmaps[tsimg] = 
                std::shared_ptr<Bitmap>(new Bitmap(sptm->tilesets[tsnum]->images[imgnum].image_source, bmpname, false, false));
              //Key-color transparentify it if it needs it!
              if(!sptm->tilesets[tsnum]->images[imgnum].image_trans.empty()) {
                tileset_bitmaps[tsimg]->Transparentify(sptm->tilesets[tsnum]->images[imgnum].image_trans);
              }
            }

            orig_atlas[gid] = atlas_record(gid, tsnum, imgnum, ulx, uly, wid, ht);

            break;
          }
        }
        if(!foundgid) {
          printf("ERROR! didn't find gid %u in any tileset! Dying!\n",gid);
          exit(1);
        }

      }
    }    




    //Step 3. Assemble those regions as "Bitmap" objects for crunch to arrange.
    bitmaps.clear();        //crunch's global 

    //set up crunch options
    init_crunch_options();
    //optVerbose = true;      //danger verbose lol
    //optXml = true;          //let's emit some xml
    //optJson = true;         //and INHERENTLY SUPERIOR LOL json

    //so what we need to put in there is one for every gid in unique_gids, yes?
    for(tile_index_t gid:unique_gids) {
      if(gid != 0) {      //skip the blank tile
        atlas_record oar = orig_atlas[gid];
        std::shared_ptr<Bitmap> bmp = tileset_bitmaps[std::make_pair(oar.tileset_index,oar.image_index)];

        //create bitmap from scratch
        Bitmap *nubmp = new Bitmap(oar.wid, oar.ht);
        nubmp->name = get_name_for_bmp(gid);    
        nubmp->CopyPixelRegion(bmp.get(), oar.ulx, oar.uly, oar.wid, oar.ht);
        bitmaps.push_back(nubmp);
        //printf("nubmp -> name = %s\n",bitmaps.back()->name.c_str());
      }
    }

    //Step 4. invoke crunch to produce a single bitmap we'll use as a texture for this layer.
    //        create texture from crunch's output bitmap
    //        say we die if there is more than one packer for now
    // HEY THIS NAME GETS BUILT IN DIFFERENT SPOTS, MAKE A FUNCTION?
    // see e.g. tiled2gt tiled_layer_to_gt_layer()
    std::string name = "Tileset_" + sptml->layer_name;
    if(!do_crunch(name,outputDir)) {
      printf("ERROR crunch failed\n");
      //return std::make_pair(nullptr,nullptr);
      //WHAT IS OUR RETURN ERROR VALUE?
    }
    if(packers.size() > 1) {
      //hey we don't support this yet!
      printf("ERROR: ended up with %zu packers, hoped for 1 - deal with!\n",packers.size());
    }

    //Step 5. create atlas from crunch's atlas: map gid to region in new layer texture
    //how do we know that? I guess by looking up gid names somehow?
    //oh yes, packer's int FindPointsAndBitmapsIndexByName(string &name);      //sean adds - what does the return value mean? index into bitmaps[]
    //new_atlas[0] = atlas_record();
    // To be consistent with the above writing out the .png, let's write out the atlas too!
    // what does that look like? FTM let's emit json - yay nother submodule? We want to be able to read json maps anyway
    // - moving json part down below so whole map has one 
    for(tile_index_t gid: unique_gids) {
      if(gid != 0) {
        atlas_record atrec;
        atrec.gid = gid;
        atrec.image_index = 0;        //each layer currently only has one texture, so this is 0
        atrec.tileset_index = 0;      //meaningless in output, make 0
        string gidname = get_name_for_bmp(gid); 
        //index into bitmaps/points within packer
        int pkrbmpind = packers[0]->FindPointsAndBitmapsIndexByName(gidname);
        atrec.ulx = packers[0]->points[pkrbmpind].x;
        atrec.uly = packers[0]->points[pkrbmpind].y;
        atrec.wid = packers[0]->bitmaps[pkrbmpind]->width;
        atrec.ht = packers[0]->bitmaps[pkrbmpind]->height;
        (*new_atlas)[gid] = atrec;


      }
    }

    
    return new_atlas;

  }

  std::shared_ptr<TiledMap> TiledReader::read_map_file(std::string filename, std::string outputDir) {
    printf("Reading Tiled map %s\n",filename.c_str());
    xml_document doc;
    xml_parse_result result = doc.load_file(filename.c_str());
    if(result.status != status_ok) {
      printf("Read faily!\n");
      return nullptr;
    }

    //fiddle output directory to have a slash on the end
    // HEY MAKE THIS OS-INDEPENDENT correctly
    if(outputDir[outputDir.length()-1] != path_sep[0])
      outputDir += path_sep;

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

    //so now we have a map with layers and tilesets. Is that all we need? NO! We need the .png files for the layers' tilesets, too!
    json json_map;

    //this used to be in tiled2sfml & now it's here!
    //so we need to call the png & atlas make on each layer
    for(int layer_num = 0; layer_num < tm->layers.size(); layer_num++) {
      std::shared_ptr<std::unordered_map<tile_index_t, atlas_record>> layer_atlas = MakeTilesheetPNGAndAtlas(tm, layer_num, outputDir);
      //DO SOMETHING WITH LAYER_ATLAS? I guess don't really need to just yet but could; maketilesheetpngandatlas emits file
      // - and actually it would be better to build the json here from the atlas
      //need to make legal json key somehow
      //THIS NEEDS TO BE THE SAME AS THE PNG NAME but for packer number then we can use the key to load the png

      //if we were to emit json at this level - 
      // std::string layer_atlas_name = "Tileset_" + tm->layers[layer_num]->layer_name;
      // std::string layer_metadata_name = "Layer_" + tm->layers[layer_num]->layer_name;
      //json_map[layer_metadata_name]["type"] = tm->layers[layer_num]->type;
      //tm->layers[layer_num]->add_to_json(json_map,layer_metadata_name);
      // tileset atlas add to map json 
      //for(auto atrec: layer_atlas) atrec.second.add_to_json(json_map, layer_atlas_name);
      //well, instead, let's just put them in the layer
      tm->layers[layer_num]->layer_atlas = layer_atlas;
    }

    // This is great! But misconceived. We don't want Tiled metadata, we want GT metadata, so we need to postprocess Tiled into that.
    // Is there any reason to emit metadata for this format? The tm data structure + the png files should be good.
    // //emit json map metadata
    // // outputDir has a slash on end so just concat
    // // MAKE A REAL NAME
    // std::string json_map_name = outputDir + "outmap.json"; 
    // printf("-- writing json map %s\n",json_map_name.c_str());
    // std::ofstream json_outstream(json_map_name);
    // //setw(4) does 4 space pretty print
    // json_outstream << std::setw(4) << json_map << endl;


    return tm;    
  }
}