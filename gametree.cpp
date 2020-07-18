#include "gametree.h"
#include "cppcodec/base64_url.hpp"
using base64 = cppcodec::base64_url;


using std::placeholders::_1;

namespace gt {
  
  // GLOBALS ========================================================================================
  //global var (will need to threadsafe it somehow)
  //init to 1 at run time so id 0 always means uninitialized
  GTid_t g_next_id = 1;
  GTid_t get_next_id() { return g_next_id++; }

  // TIME ========================================================================================

  // *****************************************************************************************
  // *****************************************************************************************
  // *****************************************************************************************
  // ADVANCE GLOBAL TIME

  void GTTime::advance_time(GTdur_t delta) {

    //if we're paused, bail
    if(paused) return;

    current_time += delta;

    // step through clients and have them happen their events
    for(auto cli : clients) {
      cli->handle_events();
    }
  }

  bool GTTime::add_client(GTEventEntity *cli) {
      //what would make this fail? like, it's already there?
      if(cli == nullptr) return false;

      for(auto cit = clients.begin(); cit != clients.end(); cit++) {
        if((*cit)->get_id() == cli->get_id()) {
          //duplicate!
          printf("ERROR: duplicate client ID %u",cli->get_id());
          return false;
        }
      }

      clients.push_back(cli);
      return true;
  }

  
  bool GTTime::remove_client(GTid_t client_id) {
    for(auto cit = clients.begin(); cit != clients.end(); cit++) {
      if((*cit)->get_id() == client_id) {
        //this might invalidate the iterator, but we're leaving anyway
        //printf("Time found client %u, removing\n",client_id);
        clients.erase(cit);
        return true;
      }
    }
    return false;
  }

  // EVENTENTITY ====================================================================================

  GTEventEntity::GTEventEntity() {
    clock = nullptr;
    //printf("Creating event entity, id %u\n",id);
  }

  GTEventEntity::~GTEventEntity() {
    //remove from clock's client list
    if(clock != nullptr) {
      //printf("Event entity %u removing itself from clock client list\n",id);
      clock->remove_client(id);
    }

    //printf("Destroying event entity, id %u\n",id);
  }

  void GTEventEntity::handle_events() {
    // bail if we're not active
    if(!active) {
      return;
    }

    //bail if clock is paused or not set
    if(clock == nullptr || clock->paused) {
      return;
    }

    // so let's go through our map 
    // now we have things that amount to calls to GTres_t() functions
    GTres_t evres;

    for (auto it = ev_map.cbegin(), next_it = it; it != ev_map.cend(); it = next_it)
    {
      ++next_it;
      //for each event, trigger it if current time is >= its timestamp
      if (clock->current_time >= it->first)
      {
        //invoke the event function! It makes its own additions, if any, to the event map
        // based on the timestamp of this event
        evres = it->second(it->first);

        // what do we do with the result value?

        //erase existing version of event
        ev_map.erase(it);
      }

      if(it->first > clock->current_time) {
        // we've reached the events in the future, don't bother with them
        break;
      }
    }
  }

  // ACTOR ==========================================================================================
  GTActor::GTActor() {
    // id should be filled in by Entity ctor
    //printf("Creating actor id %u\n",id);
    sprite = nullptr;
    current_character = -1;
    current_action = -1;
    current_direction = -1;
    current_frame = -1;
  }

  GTActor::~GTActor() {
    //sprite is a smart pointer, just let it go?
    //printf("Destroying GTActor id %u\n",id);
  }

  // MIGHT ALSO PASS IN WHETHER THE ANIMATION REPEATS
  GTres_t GTActor::set_action(std::string actname) {
    if(sprite == nullptr) return -1;
    GTindex_t actind = sprite->info.get_index_for_action(actname);
    if(actind == -1) return -1;
    if(actind == current_action) return 1;   //already in this action!
    current_action = actind;
    //return true;    //debug
    // keep direction as is but set frame to 0
    return set_frame(0);
  }

  // MIGHT ALSO PASS IN WHETHER THE ANIMATION REPEATS
  GTres_t GTActor::set_direction(std::string dirname) {
    if(sprite == nullptr) return -1;
    GTindex_t dirind = sprite->info.get_index_for_direction(dirname);
    if(dirind == -1) return -1;
    if(dirind == current_direction) return 1;   //already in this direction!
    current_direction = dirind;
    //return true;      //debug
    // keep action as is but set frame to 0
    return set_frame(0);
  }

  GTres_t GTActor::set_frame(GTindex_t fr) {
    // bail if duplicate? not necessarily, is this only called by set_action and set_direction?
    // if that's so, and we happen to be on the same frame they want to set, upcoming events could be wrong
    // needs a better name
    
    current_frame = fr;
    // SANITY CHECK
    auto fram = get_current_spriteframe();
    if(fram == nullptr) return -1;        //illegal!
    if(clock == nullptr) return 1;        //...stalled, but ok?

    //revoke pending animation events - CURRENTLY REMOVE ALL EVENTS - move to advance_frame?
    ev_map.clear();

    // if it's a cyclic animation (or at least has frames after this one) add next advance frame event...
    // ???? Let's move that to advance_frame
    // timestamp_t ts = clock->get_current_time()+fram->dur;
    // //THIS BREAKS STUFF
    // ev_list[ts] = std::shared_ptr<Event>(new AdvanceFrameEvent(ts,this));
    // return true;

    event_func_t nuev = std::bind(&GTActor::advance_frame_event, this, _1);
    add_event(clock->get_current_time() + fram->dur, nuev);

    return 1;
  }


  // default implementation adds 1 to frame, mods by # frames in current action/direction
  // returns number of millis to wait until next frame, 0 if the animation is ending
  // NOT ANYMORE does like others and returns -1 on error, 1 on success
  GTres_t GTActor::advance_frame_event(GTdur_t lts) {

    //RIDICULOUSLY VERBOSE DEBUG
    //printf("advance_frame_event at %lu called at %lu\n",lts,clock->get_current_time());

    // if this actor isn't active, do nothing
    if(!active) return 0;

    auto fram = get_current_spriteframe();
    if(fram == nullptr) return -1;

    current_frame = 
      (current_frame + 1) % sprite->frames[current_character][current_action][current_direction].size();

    //return number of millis until next frame, or 0 if the animation is done
    // CURRENTLY ALWAYS CYCLES put in a flag about this - in aseprite or elsewhere
    // next frame should be fram->dur after the event that called this - which is what?
    // handed in as lts.
    event_func_t nuev = std::bind(&GTActor::advance_frame_event, this, _1);
    add_event(lts + fram->dur, nuev);

    return 1;     //success
  }

  // MAP =========================================================================================

  // GTMapLayer ----------------------------------------------------------------------------------
  // so this takes a json that is a sub-json created by one of the subclasses, qv.
  bool GTMapLayer::add_to_json(json& j) {
    // base class handles image_data and tile_atlas
    // write image data in base64_url - https://github.com/tplgy/cppcodec#base64

    //OBJECT LAYERS MAY NOT HAVE IMAGE DATA! don't emit an image_data or tile_atlas if they don't!
    if(!image_data.empty()) {
      std::string encoded_png = base64::encode(image_data);

      j["image_data"] = encoded_png;

      for(auto t : tile_atlas) {
        t.add_to_json(j["tile_atlas"]);
      }
    } else {
      fprintf(stderr,"+++ WARNING: map layer doesn't have any image data. OK if it's a polygons-only object layer or something\n");
    }

    return true;
  }

  bool GTMapLayer::get_from_json(json& jt) {
    if(!jt.contains("image_data") || !jt.contains("tile_atlas")) {
      fprintf(stderr,"+++ WARNING: map layer has no image_data or tile_atlas - ok if it's an object layer with no imagery\n");
      return true;
    }

    // get the .png data
    image_data = base64::decode(std::string(jt["image_data"]));
    
    //then the tile atlas!
    tile_atlas.resize(jt["tile_atlas"].size());
    for(auto j = 0; j < tile_atlas.size(); j++) {
      tile_atlas[j].get_from_json(jt["tile_atlas"][j]);
    }


    return true;
  }


  // GTObjectTile --------------------------------------------------------------------------------

  GTObjectTile::GTObjectTile() {
  }

  GTObjectTile::GTObjectTile(GTtile_index_t t, GTcoord_t ox, GTcoord_t oy, GTcoord_t w, GTcoord_t h) {
    tile = t;
    orx = ox;
    ory = oy;
    wid = w;
    ht = h;
  }

  GTObjectTile::~GTObjectTile() {
  }

  bool GTObjectTile::add_to_json(json& j) {
    json subj;
    subj["tile"] = tile;
    subj["orx"] = orx;
    subj["ory"] = ory;
    subj["wid"] = wid;
    subj["ht"] = ht;

    j.push_back(subj);

    return true;
  }

  bool GTObjectTile::get_from_json(json& jt) {

    if(!jt.contains("tile") || !jt.contains("orx") || !jt.contains("ory") || !jt.contains("wid") || !jt.contains("ht")) {
      fprintf(stderr,"*** ERROR: tile object requires all of tile, orx, ory, wid, ht\n");
      return false;
    }

    tile = jt["tile"];
    orx = jt["orx"];
    ory = jt["ory"];
    wid = jt["wid"];
    ht = jt["ht"];

    return true;
  }

  // GTObjectsMapLayer ---------------------------------------------------------------------------

  bool GTObjectsMapLayer::add_to_json(json& j) {
    json subj;
    if(!GTMapLayer::add_to_json(subj)) return false; 

    //emit type
    subj["type"] = "objects";

    //emit tile_objects
    for(auto tob : tile_objects) {
      tob->add_to_json(subj["tile_objects"]);
    }

    j.push_back(subj);

    return true;
  }

  bool GTObjectsMapLayer::get_from_json(json& jt) {
    if(!GTMapLayer::get_from_json(jt)) return false; 

    if(jt.contains("tile_objects")) {
      tile_objects.resize(jt["tile_objects"].size());
      for(auto j = 0; j <  jt["tile_objects"].size(); j++) {
        tile_objects[j] = std::shared_ptr<GTObjectTile>(new GTObjectTile());
        tile_objects[j]->get_from_json(jt["tile_objects"][j]);
      }
    } else {
      // ACTUALLY THIS IS OK - there could just be polygons and stuff that I haven't supported yet
      //fprintf(stderr,"*** ERROR: no tile_objects in objects layer\n");
      //return false;
    }

    return true;
  }

    // GTTiledMapLayer -----------------------------------------------------------------------------

  //subclass overrides need to call the base class one to add/get image data and tile atlas
  bool GTTiledMapLayer::add_to_json(json& j) {
    json subj;

    if(!GTMapLayer::add_to_json(subj)) return false; 

    // put in a type field so reader knows how to handle - or should we just look for fields we know will be there?
    // let's make it explicit
    subj["type"] = "tiled";

    // so for tiled map add the tile map specific stuff
    subj["layer_tilewid"] = layer_tilewid;
    subj["layer_tileht"] = layer_tileht;
    subj["layer_pixwid"] = tile_pixwid;
    subj["layer_pixht"] = tile_pixht;

    // then emit tile_map;
    for(auto ti : tile_map) {
      subj["tile_map"].push_back(ti);
    }

    //emit tile_objects, if any - if not, json won't have a "tile_objects"
    if(tile_objects.size() > 0) {
      for(auto tob : tile_objects) {
        tob->add_to_json(subj["tile_objects"]);
      }
    }

    j.push_back(subj);

    return true;
  }

  bool GTTiledMapLayer::get_from_json(json& jt) {
    if(!GTMapLayer::get_from_json(jt)) return false; 

    //sanity check - for now just look for required fields' presence. 
    if(!jt.contains("layer_tilewid") || !jt.contains("layer_tileht") ||
        !jt.contains("layer_pixwid") || !jt.contains("layer_pixht") ||
        !jt.contains("tile_map")) {
      fprintf(stderr,"*** ERROR: tiled layer requires layer_tilewid, layer_tileht, layer_pixwd, layer_pixht, and tile_map\n");
      return false;
    }

    //read the simple ones
    layer_tilewid = jt["layer_tilewid"];
    layer_tileht = jt["layer_tileht"];
    tile_pixwid = jt["layer_pixwid"];
    tile_pixht = jt["layer_pixht"];

    //then the tile map
    tile_map.clear();
    for(auto ind : jt["tile_map"]) {
      tile_map.push_back(GTtile_index_t(ind));
    }

    // there might also be a tile_objects
    if(jt.contains("tile_objects")) {
      tile_objects.resize(jt["tile_objects"].size());
      for(auto j = 0; j <  jt["tile_objects"].size(); j++) {
        tile_objects[j] = std::shared_ptr<GTObjectTile>(new GTObjectTile());
        tile_objects[j]->get_from_json(jt["tile_objects"][j]);
      }
    }

    return true;
  }


  // GTMap ---------------------------------------------------------------------------------------

  GTMap::GTMap() {
  }

  GTMap::~GTMap() {
  }

  bool GTMap::add_to_json(json& j) {

    //emit layers
    for(auto lyr : layers) {
      lyr->add_to_json(j["layers"]);
    }

    return true;
  }

  bool GTMap::get_from_json(json& j) {
    //we expect there to be an array of layers at the top level
    // or no wait, "layers" : []
    if(j.contains("layers")) {

      for(json jlyr : j["layers"]) {
        if(jlyr.contains("type")) {
          if(jlyr["type"] == "tiled") {
            auto lyr = std::shared_ptr<GTTiledMapLayer>(new GTTiledMapLayer());
            if(lyr->get_from_json(jlyr) == true) {
              layers.push_back(lyr);
            } else {
              fprintf(stderr,"*** ERROR: failed to read tiled map layer\n");
              return false;
            }
          } else if(jlyr["type"] == "objects") {
            auto lyr = std::shared_ptr<GTObjectsMapLayer>(new GTObjectsMapLayer());
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
    }

    fprintf(stderr,"*** ERROR: no 'layers' element at top level\n");
    return false;       // there was no layers field! Fail!
  }

}