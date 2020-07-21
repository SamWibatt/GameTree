#include "gtree_sfml.h"

using namespace gt;

namespace gtree_sfml {

  bool SFMLMapLayer::build_tile_object_vertarrays(std::vector<std::shared_ptr<GTObjectTile>>& tile_objects, 
                std::vector<GTTile>& tile_atlas) {
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
        xfVertArray xva(sf::PrimitiveType::Quads,4);
        xva.append(sf::Vertex(sf::Vector2f(0.0,0.0),sf::Vector2f(tile_atlas[tob->tile].ulx,tile_atlas[tob->tile].uly)));
        xva.append(sf::Vertex(sf::Vector2f(tob->wid,0.0),
            sf::Vector2f(tile_atlas[tob->tile].ulx+tile_atlas[tob->tile].wid, tile_atlas[tob->tile].uly)));
        xva.append(sf::Vertex(sf::Vector2f(tob->wid,tob->ht),
            sf::Vector2f(tile_atlas[tob->tile].ulx+tile_atlas[tob->tile].wid, 
            tile_atlas[tob->tile].uly+tile_atlas[tob->tile].ht)));
        xva.append(sf::Vertex(sf::Vector2f(0.0,tob->ht),
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
    build_tile_object_vertarrays(tile_objects, tile_atlas);

    // DO TILE MAP
    for(auto i = 0; i < tile_map.size(); i++) {
      if(tile_map[i] != 0) {
        GTindex_t tcol = i % layer_tilewid;
        GTindex_t trow = i / layer_tilewid;

        GTcoord_t x = tcol * tile_pixwid;
        GTcoord_t y = trow * tile_pixht;
        
        // ************ WRITE THIS *********************************************************
        // ************ WRITE THIS *********************************************************
        // ************ WRITE THIS *********************************************************
        //ADD TO VERTEX ARRAY
      } else {
        // ************ WRITE THIS *********************************************************
        // ************ WRITE THIS *********************************************************
        // ************ WRITE THIS *********************************************************
        // no tile in this spot, emit any vertex array accumulated so far and
        // start a new one
      }
      
    }

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
      build_tile_object_vertarrays(tile_objects, tile_atlas);
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
            } else {
              fprintf(stderr,"*** ERROR: failed to read tiled map layer\n");
              return false;
            }
          } else if(jlyr["type"] == "objects") {
            auto lyr = std::shared_ptr<SFMLObjectsMapLayer>(new SFMLObjectsMapLayer());
            if(lyr->get_from_json(jlyr) == true) {
              layers.push_back(lyr);
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