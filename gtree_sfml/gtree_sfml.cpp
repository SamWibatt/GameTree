#include "gtree_sfml.h"

using namespace gt;

namespace gtree_sfml {

  //SFMLActor ===================================================================================


  //SFMLSpriteBank ==============================================================================

  bool SFMLSpriteBank::get_from_json(json& jt) {
    if(!GTSpriteBank::get_from_json(jt)) {
      //failed to read!
      return false;
    }

    //turn the imagery into a Texture and discard it
    spritesheet.loadFromMemory(image_data.data(), image_data.size());
    image_data.clear();

    return true;
  }


  //SFMLMap =====================================================================================

  bool SFMLMapLayer::build_tile_object_vertarrays(std::vector<std::shared_ptr<GTObjectTile>>& tile_objects, 
                std::vector<GTTile>& tile_atlas, sf::Texture *tx) {
    // iterate over tile_objects and build a little quadlike thing for each. 
    if(!tile_objects.empty()) {
      this->layer_vertarrays = std::shared_ptr<std::vector<SFMLVertArray>>(new std::vector<SFMLVertArray>());
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
        // looks like Tiled positions object tiles by lower left, tho - so try bumping y up by object's height - YAY

        SFMLVertArray xva(sf::PrimitiveType::Quads,4,tx);
        xva.va[0] = (sf::Vertex(sf::Vector2f(tob->orx,tob->ory - tob->ht),
            sf::Vector2f(tile_atlas[tob->tile].ulx,tile_atlas[tob->tile].uly)));
        //printf("Appending vertex: pos %f, %f tex %f, %f\n", xva.va[0].position.x,xva.va[0].position.y,xva.va[0].texCoords.x,xva.va[0].texCoords.y);
        xva.va[1] = (sf::Vertex(sf::Vector2f(tob->orx + tob->wid, tob->ory - tob->ht),
            sf::Vector2f(tile_atlas[tob->tile].ulx+tile_atlas[tob->tile].wid, 
              tile_atlas[tob->tile].uly)));
        //printf("Appending vertex: pos %f, %f tex %f, %f\n", xva.va[1].position.x,xva.va[1].position.y,xva.va[1].texCoords.x,xva.va[1].texCoords.y);
        xva.va[2] = (sf::Vertex(sf::Vector2f(tob->orx + tob->wid, tob->ory),
            sf::Vector2f(tile_atlas[tob->tile].ulx+tile_atlas[tob->tile].wid, 
            tile_atlas[tob->tile].uly+tile_atlas[tob->tile].ht)));
        //printf("Appending vertex: pos %f, %f tex %f, %f\n", xva.va[2].position.x,xva.va[2].position.y,xva.va[2].texCoords.x,xva.va[2].texCoords.y);
        xva.va[3] = (sf::Vertex(sf::Vector2f(tob->orx, tob->ory),
            sf::Vector2f(tile_atlas[tob->tile].ulx, tile_atlas[tob->tile].uly+tile_atlas[tob->tile].ht)));
        //printf("Appending vertex: pos %f, %f tex %f, %f\n", xva.va[3].position.x,xva.va[3].position.y,xva.va[3].texCoords.x,xva.va[3].texCoords.y);
        layer_vertarrays->push_back(xva);
      }

      //should we discard tile_objects like we did with png imagery? yeah let's do
      tile_objects.clear();
    }
    return true;
  }

  void SFMLTiledMapLayer::calculate_bounding_box() {
    //factor in both tiled map and object tiles, if any
    //start with tiled map as initial values, assuming origin is 0,0 at upper left of map
    int ulx = 0;
    int uly = 0;
    int lrx = layer_tilewid * tile_pixwid;
    int lry = layer_tileht * tile_pixht;

    //then do tile_objects - remember they're oriented around lower left, not upper left
    for(auto tob : tile_objects) {
      if(tob->orx < ulx) ulx = tob->orx;
      if(tob->ory - tob->ht < uly) uly = tob->ory - tob->ht;
      if(tob->orx + tob->wid > lrx) lrx = tob->orx + tob->wid;
      if(tob->ory > lry) lry = tob->ory;
    }

    bounding_box.left = ulx;
    bounding_box.top = uly;
    bounding_box.width = (lrx - ulx);
    bounding_box.height = (lry - uly);
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

    // FIRST TILE MAP
    layer_vertarrays = std::shared_ptr<std::vector<SFMLVertArray>>(new std::vector<SFMLVertArray>());
    SFMLVertArray xva(sf::PrimitiveType::Quads,0,layer_tex.get());
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

    // then tile objects, because they should be later in the list than the other tiles (drawn over them)
    build_tile_object_vertarrays(tile_objects, tile_atlas, layer_tex.get());

    // and find bounding box
    calculate_bounding_box();

    return true;
  }

  void SFMLObjectsMapLayer::calculate_bounding_box() {
    //factor in both tiled map and object tiles, if any
    //start with tiled map as initial values, assuming origin is 0,0 at upper left of map
    // init with suitable values - will we get past 1M pixels? probably not
    int ulx = 999999;
    int uly = 999999;
    int lrx = -999999;
    int lry = -999999;

    //then do tile_objects - remember they're oriented around lower left, not upper left
    for(auto tob : tile_objects) {
      if(tob->orx < ulx) ulx = tob->orx;
      if(tob->ory - tob->ht < uly) uly = tob->ory - tob->ht;
      if(tob->orx + tob->wid > lrx) lrx = tob->orx + tob->wid;
      if(tob->ory > lry) lry = tob->ory;
    }

    bounding_box.left = ulx;
    bounding_box.top = uly;
    bounding_box.width = (lrx - ulx);
    bounding_box.height = (lry - uly);
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

    // and find bounding box
    calculate_bounding_box();

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