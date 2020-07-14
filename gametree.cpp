#include "gametree.h"
#include "cppcodec/base64_url.hpp"
using base64 = cppcodec::base64_url;


using std::placeholders::_1;

namespace gt {
  
  // GLOBALS ========================================================================================
  //global var (will need to threadsafe it somehow)
  //init to 1 at run time so id 0 always means uninitialized
  id_t g_next_id = 1;
  id_t get_next_id() { return g_next_id++; }

  // TIME ========================================================================================

  // *****************************************************************************************
  // *****************************************************************************************
  // *****************************************************************************************
  // ADVANCE GLOBAL TIME

  void GTTime::advance_time(timestamp_t delta) {

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

  
  bool GTTime::remove_client(id_t client_id) {
    for(auto cit = clients.begin(); cit != clients.end(); cit++) {
      if((*cit)->get_id() == client_id) {
        //this might invalidate the iterator, but we're leaving anyway
        printf("Time found client %u, removing\n",client_id);
        clients.erase(cit);
        return true;
      }
    }
    return false;
  }

  // EVENTENTITY ====================================================================================

  GTEventEntity::GTEventEntity() {
    clock = nullptr;
    printf("Creating event entity, id %u\n",id);
  }

  GTEventEntity::~GTEventEntity() {
    //remove from clock's client list
    if(clock != nullptr) {
      printf("Event entity %u removing itself from clock client list\n",id);
      clock->remove_client(id);
    }

    printf("Destroying event entity, id %u\n",id);
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
    // now we have things that amount to calls to int() functions
    int evres;

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
    printf("Creating actor id %u\n",id);
    sprite = nullptr;
    current_character = -1;
    current_action = -1;
    current_direction = -1;
    current_frame = -1;
  }

  GTActor::~GTActor() {
    //sprite is a smart pointer, just let it go?
    printf("Destroying GTActor id %u\n",id);
  }

  // MIGHT ALSO PASS IN WHETHER THE ANIMATION REPEATS
  int GTActor::set_action(std::string actname) {
    if(sprite == nullptr) return -1;
    int actind = sprite->info.get_index_for_action(actname);
    if(actind == -1) return -1;
    if(actind == current_action) return 1;   //already in this action!
    current_action = actind;
    //return true;    //debug
    // keep direction as is but set frame to 0
    return set_frame(0);
  }

  // MIGHT ALSO PASS IN WHETHER THE ANIMATION REPEATS
  int GTActor::set_direction(std::string dirname) {
    if(sprite == nullptr) return -1;
    int dirind = sprite->info.get_index_for_direction(dirname);
    if(dirind == -1) return -1;
    if(dirind == current_direction) return 1;   //already in this direction!
    current_direction = dirind;
    //return true;      //debug
    // keep action as is but set frame to 0
    return set_frame(0);
  }

  int GTActor::set_frame(int fr) {
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
  int GTActor::advance_frame_event(timestamp_t lts) {

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
    // std::function<int(virtbindkid *)> rpvkn = &virtbindkid::narg;
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
    std::string encoded_png = base64::encode(image_data);

    j["image_data"] = encoded_png;

    for(auto t : tile_atlas) {
      t.add_to_json(j["tile_data"]);
    }

    return true;
  }

  bool GTMapLayer::get_from_json(json& jt) {
    return false;     //TEMP
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
      subj["tiled_map"].push_back(ti);
    }

    j.push_back(subj);

    return true;
  }

  bool GTTiledMapLayer::get_from_json(json& jt) {
    if(!GTMapLayer::get_from_json(jt)) return false; 
    return false;     //TEMP
  }

  // GTObjectTile --------------------------------------------------------------------------------

  GTObjectTile::GTObjectTile() {
  }

  GTObjectTile::GTObjectTile(int t, int ox, int oy, int w, int h, int fx, int fy) {
    tile = t;
    orx = ox;
    ory = oy;
    wid = w;
    ht = h;
    offx = fx;
    offy = fy;
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
    subj["offx"] = offx;
    subj["offy"] = offy;

    j.push_back(subj);

    return true;
  }

  bool GTObjectTile::get_from_json(json& jt) {
    return false; //temp!
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
    return false;     //TEMP
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
    return false;       //TEMP!
  }

}