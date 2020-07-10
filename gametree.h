//GameTree is a game-library-independent minimalist hierarchy of C++ classes for writing games.
//so far, 2D only.
//intended for use anywhere C++11 compiles, such as Linux / Mac / Windows computers,
//sufficiently powerful microcontrollers, etc.
//requires a 64 bit source of milliseconds?

#ifndef GAMETREE_H_INCLUDED
#define GAMETREE_H_INCLUDED

#include <cstdio>
#include <cstdint>
#include <memory>
#include <vector>
#include <map>
#include <functional>

//TEMP! for platform-specific sfml stuff
#include "sfmlsprite.h"
using namespace sfml_assets;

namespace gt {

  // GLOBALS AND TYPEDEFS ===========================================================================

  // unique ID for every GameTree instance
  typedef uint32_t id_t;

  // function for getting next ID globally (put a mutex around this or something)
  id_t get_next_id();

  // 64 bit millisecond counter for timestamps.
  // 32 bit rolls over in 49.7 days, which is probably sufficient, and in Win9x times
  // there was no danger of any machine staying up that long without a restart. Now, though.
  typedef uint64_t timestamp_t;

  class Event;
  class EventEntity;

  class Time {
    public:
      timestamp_t current_time;

      // global pause flags - bitfield - if a bit is set, that pause group is paused
      // starts out with everything paused
      // let's redo this so that we have multiple timers and they're just paused or not
      // each has its own list
      bool paused;

      // TEST: relocating this to EventEntity - so each entity will have an event list
      // should make it easier to manage their upcoming events
      // event list, ordered by timestamp
      //std::map<timestamp_t, std::shared_ptr<Event>> ev_list;

      // instead, here are the entities registered against this Time
      std::vector<EventEntity *> clients;


    public:
      Time() { 
        //start at time 0 with everything paused
        current_time = 0;
        paused = true; 
      }

      ~Time() {
        printf("Freeing Time object\n");
      }

    public:
      // *** TO DO: global clock functions
      // wrt pausing and stuff

      //*** TO DO: Pause and Unpause

      // per-loop time advance that triggers all events 
      void advance_time(timestamp_t delta);
      timestamp_t get_current_time() { return current_time; }

      // managing clients
      bool add_client(EventEntity  *cli); 
      bool remove_client(id_t client_id);


      // *** TO DO: Event list functions
      // - pause: have pause groups so some things can keep going while other stuff is paused - bitfield
      //   when unpaused, all the timestamps need to be updated
      //void update_timestamps_for_pause(timestamp_t delta);

    // *****************************************************************************************
    // *****************************************************************************************
    // *****************************************************************************************
    // TO DO: Cancel event function - happenings may have contributed new events that shouldn't
    // happen after all
    // *****************************************************************************************
    // *****************************************************************************************
    // *****************************************************************************************

  };


  // ENTITY ======================================================================================
  // root of the tree! the Thing. Everybody calls it an Object.
  // I suppose I could call it a Noun: Person, place, thing, event, idea, heh
  // bc it will be the root of person and it's not nice to call people Things
  // though Noun has grammar baggage. How about Entity?
  // Entity should be minimal: id and children(kids)
  // might not even keep the kids, might shfufle them into a subclass
  class Entity {
    //data members
    protected:
      id_t id;
      // pointers to avoid object slicing, smart pointers to avoid memory allocation hassles
      std::vector<std::shared_ptr<Entity>> kids;

    public:
      void init() { 
        id = get_next_id();
        printf("- entity init, id %u\n",id); 
      }
      Entity() { init(); }
      virtual ~Entity() { 
        //does this need to be explicit? kids.clear();
        printf("Deleting entity id %u\n",id); 
      }

    public:
      id_t get_id() { return id; }
      void set_id(id_t i) { id = i; }

      std::vector<std::shared_ptr<Entity>> &get_kids() { return kids; }
      void add_kid(std::shared_ptr<Entity> ent) { kids.push_back(ent); }
      std::shared_ptr<Entity> get_kid_by_index(int i) {
        if(kids.size() <= i) return nullptr;
        return kids[i];
      }
      std::shared_ptr<Entity> get_kid_by_id(id_t i) {
        for(auto kid: kids) {
          if(kid->get_id() == i) return kid;
        }
        return nullptr;
      }

  };

  //Tabling "graphical entity"
  // comments from it:
      // How do you draw? has sf::Drawable and sf::Transformable...
      // allegro handles drawing quite differently...
      // I don't want to get that far into things.
      // Do I even want to bother with the drawing? 
      // Should this just be a container for storing stuff like e.g. sprite frame indices?
      // You have the action, like "walk", then a direction, then the frames.
      // Each frame is something like index into an imagery pool and a duration in ms.
      // There you have it, still useful but not platform specific. Could subclass into plat-spec.
      // I suppose it could also contain info about bounding box and hotspot for the frames?
      // to do this in a node-based way... do I? the children of an animated sprite node would be
      // Action nodes, whose children would be Direction nodes (action over direction bc some actions
      // like dying would prolly be directionless)
      // need to think re: all this

  // EVENT ENTITY ================================================================================

  // MAKE THIS VISIBLE?
  // signature for an event function. Can either be a std::<function<int()>> of a literal int()
  // or a std::bind<int-returning-non-intvoid, all params bound> YAY
  // OK, it needs to be a bit richer than this - std::function<int()> works with free functions,
  // but if we want to use member functions of event entity, you can do like this, 
  // where virtbind is the base class (EventEntity), and virtbindkid is the child class that overrides
  // its narg function, just to see if that works right - and it does.
  // if the type is declared with a base class raw pointer argument, can use either a raw pointer ("this")
  // or a shared_ptr<>.get()

  // std::function<int(virtbindkid *)> rpvkn = &virtbindkid::narg;
  // int res = rpvkn(rpvk);
  // std::function<int(virtbindkid *)> rpvkns = std::bind(&virtbindkid::strarg, rpvk, "raw_fiddle");
  // res = rpvkns(rpvk);

  // //ok, so now the big test - can you use the base class in the std::function type? YES!
  // std::function<int(virtbind *)> rpvns = std::bind(&virtbindkid::strarg, rpvk, "raw_hovbart");
  // res = rpvns(rpvk);

  // + it works with a smart pointer get if the function object was created with a raw pointer
  // auto pvk = std::shared_ptr<virtbindkid>(new virtbindkid);
  // res = rpvns(pvk.get());

  // and can create from a shared_ptr<> get() too
  // std::function<int(virtbind *)> rpvnsg = std::bind(&virtbindkid::strarg, pvk.get(), "tligbot");
  // res = rpvnsg(pvk.get());

  // even works grossly with free functions:
  //can you use that signature with a free function and just pass in nullptr?
  // std::function<int(virtbind *)> rpvf = std::bind(stringarg, "furious_ugly");
  // res = rpvf(nullptr);

  // HOWEVER the way it's called in event entity's handle_events currently is just to pass "this" so raw ptr is great

  // IS THERE ANY STANDARD TO WHAT THE RETURN VALUES MEAN? 
  // so far I'm coding -1 = error, 0 = no error but nothing happened, 1 = ok... but not using it anywhere

  // OK how about also passing in the timestamp at which it was called, for calculating next one, if needed

  // changes the signature a bit, actually makes it a bit easier:
  // using std::placeholders::_1;
  // std::function<int(std::string)> rpvf2 = std::bind(&virtbindkid::strarg, pvk.get(), _1);
  // res = rpvf2("chunkbunk");
  // printf("result of rpvf2(pvk,\"chunkbunk\") was %d\n",res);


  //was typedef std::function<int(EventEntity *, timestamp_t)> event_func_t;
  typedef std::function<int(timestamp_t)> event_func_t;

  class EventEntity : public Entity {
    public:
      //data members
      // clock that governs this entity
      // since we're not creating it, don't use a smart ptr!
      Time *clock;
      // event list, ordered by timestamp
      // this works(ish), but it's complicated and gross
      //std::map<timestamp_t, std::shared_ptr<Event>> ev_list;
      //let's use a std::function instead. Default signature int(), can change later;
      //remember we can use std::bind to have this call into functions that have more params - ?
      // YES YOU CAN DO THAT
      std::map<timestamp_t,event_func_t> ev_map;

      // active means it's getting updated?
      bool active;


    public:
      EventEntity();
      virtual ~EventEntity();

    public:
      void register_clock(Time &tim) { 
        clock = &tim; 
        tim.add_client(this);
      }

      // mechanism to add new events - and events can use this too! as can other objects. It's public
      // I think it's safe to use std::function here... it amounts to a pointer, yes? could be bad if it points
      // to a function in an object that has been deleted, hm, so don't do that
      // what's the return value?
      virtual int add_event(timestamp_t ts, event_func_t evf) {
        ev_map[ts] = evf;
        return 0;
      }

      //and the loop that makes events happen!
      virtual void handle_events();
  };

  // ACTOR =======================================================================================

  // stepping ahead, let's make one work, and then refactor.
  class Actor : public EventEntity {
    public:
      //data members
      std::shared_ptr<SFMLSprite> sprite;
      int current_character;
      int current_direction;
      int current_action;
      int current_frame;

      // visible means it's drawn - can be active but not visible. Can it be visible but not active?
      bool visible;

    public:
      Actor();
      virtual ~Actor();

      const SFMLSpriteFrame *get_current_spriteframe() {
        //sanity check: should we do all this for every frame? on a "real computer" it's no problem
        //once per sprite per frame advance is not terrible overhead, but damn
        if(sprite == nullptr || 
          current_character == -1 || sprite->frames.find(current_character) == sprite->frames.end() ||
          current_action == -1 || sprite->frames[current_character].find(current_action) == sprite->frames[current_character].end() ||
          current_direction == -1 || sprite->frames[current_character][current_action].find(current_direction) == sprite->frames[current_character][current_action].end() ||
          current_frame == -1 || current_frame >= sprite->frames[current_character][current_action][current_direction].size()) {
          return nullptr;
        }
        //was return std::shared_ptr<SFMLSpriteFrame>(&(sprite->frames[current_character][current_action][current_direction][current_frame]));
        return &(sprite->frames[current_character][current_action][current_direction][current_frame]);
      }

    public:
      // member functions: returning int bc can use std::bind to hand in params such as timestamp, so long as return type is right!
      // ... will have to think re, might need wrappers
      // see above with declaration of event_func_t
      virtual int set_action(std::string actname);
      virtual int set_direction(std::string dirname);
      virtual int set_frame(int fr);

      // functions intended to be called by ev_map - signature is int(), maybe with bind we can 
      // overrideable, default implementation just does current_frame +1 mod # frames in current action
      // returns number of millis to wait until next frame, 0 if the animation is ending, -1 on error
      // let's say we add _event to functions intended to be called as events
      virtual int advance_frame_event(timestamp_t lts);

  };
}

#endif
