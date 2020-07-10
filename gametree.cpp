#include "gametree.h"

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

  void Time::advance_time(timestamp_t delta) {

    //if we're paused, bail
    if(paused) return;

    current_time += delta;

    // step through clients and have them happen their events
    for(auto cli : clients) {
      cli->handle_events();
    }
  }

  bool Time::add_client(EventEntity *cli) {
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

  
  bool Time::remove_client(id_t client_id) {
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

  EventEntity::EventEntity() {
    clock = nullptr;
    printf("Creating event entity, id %u\n",id);
  }

  EventEntity::~EventEntity() {
    //remove from clock's client list
    if(clock != nullptr) {
      printf("Event entity %u removing itself from clock client list\n",id);
      clock->remove_client(id);
    }

    printf("Destroying event entity, id %u\n",id);
  }

  void EventEntity::handle_events() {
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
  Actor::Actor() {
    // id should be filled in by Entity ctor
    printf("Creating actor id %u\n",id);
    sprite = nullptr;
    current_character = -1;
    current_action = -1;
    current_direction = -1;
    current_frame = -1;
  }

  Actor::~Actor() {
    //sprite is a smart pointer, just let it go?
    printf("Destroying Actor id %u\n",id);
  }

  // MIGHT ALSO PASS IN WHETHER THE ANIMATION REPEATS
  int Actor::set_action(std::string actname) {
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
  int Actor::set_direction(std::string dirname) {
    if(sprite == nullptr) return -1;
    int dirind = sprite->info.get_index_for_direction(dirname);
    if(dirind == -1) return -1;
    if(dirind == current_direction) return 1;   //already in this direction!
    current_direction = dirind;
    //return true;      //debug
    // keep action as is but set frame to 0
    return set_frame(0);
  }

  int Actor::set_frame(int fr) {
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

    event_func_t nuev = std::bind(&Actor::advance_frame_event, this, _1);
    add_event(clock->get_current_time() + fram->dur, nuev);

    return 1;
  }


  // default implementation adds 1 to frame, mods by # frames in current action/direction
  // returns number of millis to wait until next frame, 0 if the animation is ending
  // NOT ANYMORE does like others and returns -1 on error, 1 on success
  int Actor::advance_frame_event(timestamp_t lts) {

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
    event_func_t nuev = std::bind(&Actor::advance_frame_event, this, _1);
    add_event(lts + fram->dur, nuev);

    return 1;     //success
  }

}