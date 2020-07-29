// NOT INTENDED TO BE INCLUDED DIRECTLY - JUST INCLUDE gametree.h

#ifndef GTREE_BASE_H_INCLUDED
#define GTREE_BASE_H_INCLUDED

namespace gt {

  // GLOBALS AND TYPEDEFS ===========================================================================

  // type for array indices & similar
  typedef int32_t GTindex_t;

  //tile index type - i.e., in tile maps.
  //CUT THIS DOWN as possible, Tiled uses 32 bit, I'm targeting smaller.
  //256 tiles seems too few to allow for, in real games...?
  typedef uint16_t GTtile_index_t;

  // type for millisecond durations
  typedef uint32_t GTdur_t;

  // type for integer results, esp. event callbacks
  typedef int32_t GTres_t;

  // integer coordinate type
  typedef int32_t GTcoord_t;

  class GTPoint {
    public:
      GTcoord_t x,y;

    public:
      GTPoint() { x = y = 0; }
      GTPoint(int nx, int ny) { x = nx; y = ny; }
      ~GTPoint() {}
  };

  //bounding box class
  class GTBBox {
    public:
      GTcoord_t ulx;
      GTcoord_t uly;
      GTcoord_t wid;
      GTcoord_t ht;

    public:
      GTBBox() { ulx = uly = wid = ht = 0; }
      GTBBox(GTcoord_t nx, GTcoord_t ny, GTcoord_t nw, GTcoord_t nh) { 
        ulx = nx;
        uly = ny;
        wid = nw;
        ht = nh; 
      }
      ~GTBBox() {}
  };

  // unique ID for every GameTree instance
  typedef uint32_t GTid_t;

  // function for getting next ID globally (put a mutex around this or something)
  GTid_t get_next_id();

  // 64 bit millisecond counter for timestamps.
  // 32 bit rolls over in 49.7 days, which is probably sufficient, and in Win9x times
  // there was no danger of any machine staying up that long without a restart. Now, though.
  typedef uint64_t GTtimestamp_t;

  class GTEvent;
  class GTEntity;
  class GTEventEntity;

  class GTTime {
    public:
      GTtimestamp_t current_time;

      // global pause flags - bitfield - if a bit is set, that pause group is paused
      // starts out with everything paused
      // let's redo this so that we have multiple timers and they're just paused or not
      // each has its own list
      bool paused;

      // TEST: relocating this to EventEntity - so each entity will have an event list
      // should make it easier to manage their upcoming events
      // event list, ordered by timestamp
      //std::map<GTtimestamp_t, std::shared_ptr<Event>> ev_list;

      // instead, here are the entities registered against this Time
      std::vector<GTEventEntity *> clients;


    public:
      GTTime() { 
        //start at time 0 with everything paused
        current_time = 0;
        paused = true; 
      }

      ~GTTime() {
        printf("Freeing GTTime object\n");
      }

    public:
      // *** TO DO: global clock functions
      // wrt pausing and stuff

      //*** TO DO: Pause and Unpause

      // per-loop time advance that triggers all events 
      void advance_time(GTdur_t delta);
      GTtimestamp_t get_current_time() { return current_time; }

      // managing clients
      bool add_client(GTEventEntity  *cli); 
      bool remove_client(GTid_t client_id);


      // *** TO DO: Event list functions
      // - pause: have pause groups so some things can keep going while other stuff is paused - bitfield
      //   when unpaused, all the timestamps need to be updated
      //void update_timestamps_for_pause(GTtimestamp_t delta);

    // *****************************************************************************************
    // *****************************************************************************************
    // *****************************************************************************************
    // TO DO: Cancel event function - happenings may have contributed new events that shouldn't
    // happen after all
    // *****************************************************************************************
    // *****************************************************************************************
    // *****************************************************************************************

  };
}

#endif