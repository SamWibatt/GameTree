#include "gtree_sfml.h"

using namespace gt;

namespace gtree_sfml {

  bool SFMLMapLayer::build_tile_object_vertarrays(std::vector<std::shared_ptr<GTObjectTile>>& tile_objects, 
                std::vector<GTTile>& tile_atlas, sf::Texture *tx) {
    // iterate over tile_objects and build a little quadlike thing for each. 
    if(!tile_objects.empty()) {
      this->layer_vertarrays = std::shared_ptr<std::vector<xfVertArray>>(new std::vector<xfVertArray>());
      for(auto tob : tile_objects) {
        //here's what we have to work with
        //tob->orx, tob->ory, tob->tile, tob->wid, tob->ht
        //tile_atlas[tob->tile].ulx, tile_atlas[tob->tile].uly, tile_atlas[tob->tile].wid, tile_atlas[tob->tile].ht
        // the atlas is coords into the texture sheet image
        // texture verts similar but tile_atlas[tob->tile].wid/ht instead, yes? 
        // do a quad, clockwise so 0,0 - w, 0, - w, h - 0, h
        // should I be doing a separate vertex array for each?
        // Let's go with that; likely the tile map will emit subsets too, in a refactor
        // wait, need to give the actual position for each quad, not relative to a "position"
        xfVertArray xva(sf::PrimitiveType::Quads,4,tx);
        xva.va[0] = (sf::Vertex(sf::Vector2f(tob->orx,tob->ory),
            sf::Vector2f(tile_atlas[tob->tile].ulx,tile_atlas[tob->tile].uly)));
        xva.va[1] = (sf::Vertex(sf::Vector2f(tob->orx + tob->wid, tob->ory),
            sf::Vector2f(tile_atlas[tob->tile].ulx+tile_atlas[tob->tile].wid, 
              tile_atlas[tob->tile].uly)));
        xva.va[2] = (sf::Vertex(sf::Vector2f(tob->orx + tob->wid, tob->orx + tob->ht),
            sf::Vector2f(tile_atlas[tob->tile].ulx+tile_atlas[tob->tile].wid, 
            tile_atlas[tob->tile].uly+tile_atlas[tob->tile].ht)));
        xva.va[3] = (sf::Vertex(sf::Vector2f(tob->orx, tob->ory + tob->ht),
            sf::Vector2f(tile_atlas[tob->tile].ulx, tile_atlas[tob->tile].uly+tile_atlas[tob->tile].ht)));
        layer_vertarrays->push_back(xva);
      }

      //should we discard tile_objects like we did with png imagery? yeah let's do
      tile_objects.clear();
    }
    return true;
  }

  bool SFMLTiledMapLayer::get_from_json(json& jt) {
    if(!GTTiledMapLayer::get_from_json(jt)) {
      //failed to read!
      return false;
    }

    //image data is a temporary in-memory png. create layer_tex from that.
    layer_tex = std::shared_ptr<sf::Texture>(new sf::Texture());
    layer_tex->loadFromMemory(image_data.data(), image_data.size());

    // DANGER: THIS DESTRUCTIVELY MODIFIES IMAGE_DATA (GETS RID OF IT IN FAVOR OF THE TEXTURE!!!!!!!!!!!!!!!!!!!!!!!!!)
    // likewise tile_objects below
    image_data.clear();

    // NOW BUILD VERTEX ARRAYS
    // ******************************************************** WRITE THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // ******************************************************** WRITE THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // ******************************************************** WRITE THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //this->tile_map and this->tile_objects need to be accounted for, recall
    build_tile_object_vertarrays(tile_objects, tile_atlas, layer_tex.get());

    // DO TILE MAP
    layer_vertarrays = std::shared_ptr<std::vector<xfVertArray>>(new std::vector<xfVertArray>());
    xfVertArray xva(sf::PrimitiveType::Quads,0,layer_tex.get());
    for(auto i = 0; i < tile_map.size(); i++) {
      if(tile_map[i] != 0) {
        GTindex_t tcol = i % layer_tilewid;
        GTindex_t trow = i / layer_tilewid;

        GTcoord_t x = tcol * tile_pixwid;
        GTcoord_t y = trow * tile_pixht;
        
        //ADD TO VERTEX ARRAY
        // add all 4 points for this in the order ulx, uly; ulx, lry; lrx, rly, ulx, lry
        // remember all tiles in the tilemap are the tile_pixwd x tile_pixht size;
        // any oddsize are put in the tile_objects list
        xva.append(sf::Vertex(sf::Vector2f(x,y),sf::Vector2f(tile_atlas[tile_map[i]].ulx,
            tile_atlas[tile_map[i]].uly)));
        xva.append(sf::Vertex(sf::Vector2f(x+tile_pixwid,y),
            sf::Vector2f(tile_atlas[tile_map[i]].ulx+tile_pixwid, tile_atlas[tile_map[i]].uly)));
        xva.append(sf::Vertex(sf::Vector2f(x+tile_pixwid,y+tile_pixht),
            sf::Vector2f(tile_atlas[tile_map[i]].ulx+tile_pixwid, 
            tile_atlas[tile_map[i]].uly+tile_pixht)));
        xva.append(sf::Vertex(sf::Vector2f(x,y+tile_pixht),
            sf::Vector2f(tile_atlas[tile_map[i]].ulx, tile_atlas[tile_map[i]].uly+tile_pixht)));
      } 
      // else it's a blank, skip
      
    }
    layer_vertarrays->push_back(xva);

    return true;
  }

  bool SFMLObjectsMapLayer::get_from_json(json& jt) {
    if(!GTObjectsMapLayer::get_from_json(jt)) {
      //failed to read!
      return false;
    }

    //if there's imagery, make it a texture
    layer_tex = nullptr;
    layer_vertarrays = nullptr;
    if(!image_data.empty()) {
      //image data is a temporary in-memory png. create layer_tex from that.
      layer_tex = std::shared_ptr<sf::Texture>(new sf::Texture());
      layer_tex->loadFromMemory(image_data.data(), image_data.size());

      // DANGER: THIS DESTRUCTIVELY MODIFIES IMAGE_DATA (GETS RID OF IT IN FAVOR OF THE TEXTURE!!!!!!!!!!!!!!!!!!!!!!!!!)
      image_data.clear();

      // NOW BUILD VERTEX ARRAYS
      build_tile_object_vertarrays(tile_objects, tile_atlas, layer_tex.get());
    }


    return true;
  }

  // ick, had to reconstruct the whole GTMap reader bc it has to allocate the right subclasses of layer
  // may need to refactor some
  bool SFMLMap::get_from_json(json& j) {
    //we expect there to be an array of layers at the top level
    // or no wait, "layers" : []
    if(j.contains("layers")) {

      for(json jlyr : j["layers"]) {
        if(jlyr.contains("type")) {
          if(jlyr["type"] == "tiled") {
            auto lyr = std::shared_ptr<SFMLTiledMapLayer>(new SFMLTiledMapLayer());
            if(lyr->get_from_json(jlyr) == true) {
              layers.push_back(lyr);
              slayers.push_back(lyr); //GROSS KLUDGE TO AVOID SLICING BC I SCREWED UP SOMEWHERE
            } else {
              fprintf(stderr,"*** ERROR: failed to read tiled map layer\n");
              return false;
            }
          } else if(jlyr["type"] == "objects") {
            auto lyr = std::shared_ptr<SFMLObjectsMapLayer>(new SFMLObjectsMapLayer());
            if(lyr->get_from_json(jlyr) == true) {
              layers.push_back(lyr);
              slayers.push_back(lyr); //GROSS KLUDGE TO AVOID SLICING BC I SCREWED UP SOMEWHERE
            } else {
              fprintf(stderr,"*** ERROR: failed to read objects map layer\n");
              return false;
            }
          } else {
            fprintf(stderr,"*** ERROR: unknown layer type %s\n",std::string(jlyr["type"]).c_str());
            return false;
          } 
        } else {
          fprintf(stderr,"*** ERROR: no layer type given\n");
          return false;
        } 
      }

      return true;
    } else {

      fprintf(stderr,"*** ERROR: no 'layers' element at top level\n");
      return false;       // there was no layers field! Fail!
    }

    return true;
  }

  
}