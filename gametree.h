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
//set up include dirs to have json/single_include
#include "nlohmann/json.hpp"
#include <fstream>

using json = nlohmann::json;

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


  // SPRITE ======================================================================================

    // single frame
  class GTSpriteFrame {
    public:
      //data members
      GTcoord_t ulx, uly;         //texture coordinates, pixels, within sprite sheet texture
      GTcoord_t wid, ht;          //extents of rectangle within sprite sheet texture
      GTcoord_t offx, offy;     //offsets to give to sprite.setOrigin(sf::Vector2f(offx, offy));
      GTdur_t dur;              //duration, millis

    public:
      GTSpriteFrame() {}
      GTSpriteFrame(GTcoord_t ux, GTcoord_t uy, GTcoord_t w, GTcoord_t h, GTcoord_t ox, GTcoord_t oy, GTdur_t dr) {
        ulx = ux; 
        uly = uy;
        wid = w;
        ht = h;
        offx = ox;
        offy = oy;
        dur = dr;
      }
      virtual ~GTSpriteFrame() {}

      public:
        // json i/o
        virtual bool add_to_json(json& j);
        virtual bool get_from_json(json& jt);
  };

  //There needs to be a class GTthat's like a "directory" for these
  //contains inventory of actions, # directions & such for each of those
  //can be defined in a header emitted by asesprite2sfml
  class GTSpriteInfo {
    public:
      //data members
      // these map character name, action name, direction name to indices into frame map below
      std::map<std::string,GTindex_t> character_to_index;
      std::map<std::string,GTindex_t> action_to_index;
      std::map<std::string,GTindex_t> direction_to_index;
      // and the other way around
      std::vector<std::string> index_to_character;
      std::vector<std::string> index_to_action;
      std::vector<std::string> index_to_direction;


    public:
      GTSpriteInfo(){}
      virtual ~GTSpriteInfo() {}

    public:
      //member functions
      GTindex_t get_index_for_character(std::string chname) {
        auto it = character_to_index.find(chname);
        if(it == character_to_index.end()) return -1;
        return it->second;
      }
      GTindex_t get_index_for_action(std::string actname) {
        auto it = action_to_index.find(actname);
        if(it == action_to_index.end()) return -1;
        return it->second;
      }
      GTindex_t get_index_for_direction(std::string dirname) {
        auto it = direction_to_index.find(dirname);
        if(it == direction_to_index.end()) return -1;
        return it->second;
      }

      // json i/o
      virtual bool add_to_json(json& j);
      virtual bool get_from_json(json& jt);
  };

  //platform-independent SpriteBank object that plat-spec ones will subclass
  class GTSpriteBank {
    public:
      //data members
      GTSpriteInfo info;
      //... why can't this just be a vector of vector of vectors? NOW IT IS!!!!!!!!!!!!!!
      // we went to the trouble of making the keys into numbers, and they're contiguous.
      //std::map<GTindex_t, std::map<GTindex_t, std::map<GTindex_t, std::vector<GTSpriteFrame>>>> frames;
      // character  |  action |   direction | frame #
      std::vector<std::vector<std::vector<std::vector<GTSpriteFrame>>>> frames;
      std::vector<uint8_t> image_data;      // png-formatted (i.e., written to disk would be a full .png file) image data of sprite sheet

    public:
      GTSpriteBank() {}
      virtual ~GTSpriteBank() {}

      // json i/o
      virtual bool add_to_json(json& j);
      virtual bool get_from_json(json& jt);

      virtual bool load_from_json_file(std::string sbank_filename) {
        std::ifstream ifs(sbank_filename);
        if(ifs.fail()) {
            printf("*** ERROR: couldn't open %s\n",sbank_filename.c_str());
            return false;
        }
        json jmap;
        ifs >> jmap;
        ifs.close();

        if(!get_from_json(jmap)) {
            printf("*** ERROR: failed to load SpriteBank from json file \"%s\"\n",sbank_filename.c_str());
            return false;
        }
        return true;
      }

  };


  // ENTITY ======================================================================================
  // root of the tree! the Thing. Everybody calls it an Object.
  // I suppose I could call it a Noun: Person, place, thing, event, idea, heh
  // bc it will be the root of person and it's not nice to call people Things
  // though Noun has grammar baggage. How about Entity?
  // Entity should be minimal: id and children(kids)
  // might not even keep the kids, might shfufle them into a subclass
  class GTEntity {
    //data members
    protected:
      GTid_t id;
      // pointers to avoid object slicing, smart pointers to avoid memory allocation hassles
      // do we need this?
      // std::vector<std::shared_ptr<GTEntity>> kids;

    public:
      void init() { 
        id = get_next_id();
        //printf("- entity init, id %u\n",id); 
      }
      GTEntity() { init(); }
      virtual ~GTEntity() { 
        //does this need to be explicit? kids.clear();
        //printf("Deleting entity id %u\n",id); 
      }

    public:
      GTid_t get_id() { return id; }
      void set_id(GTid_t i) { id = i; }

      // revive if we're going to have child pointers
      // std::vector<std::shared_ptr<GTEntity>> &get_kids() { return kids; }
      // void add_kid(std::shared_ptr<GTEntity> ent) { kids.push_back(ent); }
      // std::shared_ptr<GTEntity> get_kid_by_index(GTindex_t i) {
      //   if(kids.size() <= i) return nullptr;
      //   return kids[i];
      // }
      // std::shared_ptr<GTEntity> get_kid_by_id(GTid_t i) {
      //   for(auto kid: kids) {
      //     if(kid->get_id() == i) return kid;
      //   }
      //   return nullptr;
      // }

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
  // GTres_t res = rpvkn(rpvk);
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


  //was typedef std::function<GTres_t(EventEntity *, GTtimestamp_t)> event_func_t;
  typedef std::function<GTres_t(GTtimestamp_t)> event_func_t;

  class GTEventEntity : public GTEntity {
    public:
      //data members
      // clock that governs this entity
      // since we're not creating it, don't use a smart ptr!
      GTTime *clock;
      // event list, ordered by timestamp
      // this works(ish), but it's complicated and gross
      //std::map<GTtimestamp_t, std::shared_ptr<Event>> ev_list;
      //let's use a std::function instead. Default signature GTres_t(), can change later;
      //remember we can use std::bind to have this call into functions that have more params - ?
      // YES YOU CAN DO THAT
      std::map<GTtimestamp_t,event_func_t> ev_map;

      // active means it's getting updated?
      bool active;


    public:
      GTEventEntity();
      virtual ~GTEventEntity();

    public:
      void register_clock(GTTime &tim) { 
        clock = &tim; 
        tim.add_client(this);
      }

      // mechanism to add new events - and events can use this too! as can other objects. It's public
      // I think it's safe to use std::function here... it amounts to a pointer, yes? could be bad if it points
      // to a function in an object that has been deleted, hm, so don't do that
      // what's the return value?
      virtual GTres_t add_event(GTtimestamp_t ts, event_func_t evf) {
        ev_map[ts] = evf;
        return 0;
      }

      //and the loop that makes events happen!
      virtual void handle_events();
  };

  // ACTOR =======================================================================================

  // stepping ahead, let's make one work, and then refactor.
  class GTActor : public GTEventEntity {
    public:
      //data members
      std::shared_ptr<GTSpriteBank> sbank;
      GTindex_t current_character;
      GTindex_t current_direction;
      GTindex_t current_action;
      GTindex_t current_frame;

      // visible means it's drawn - can be active but not visible. Can it be visible but not active?
      bool visible;

    public:
      GTActor();
      virtual ~GTActor();

        GTSpriteFrame *get_current_spriteframe() const {
        //sanity check: should we do all this for every frame? on a "real computer" it's no problem
        //once per sprite per frame advance is not terrible overhead, but damn
        if(sbank == nullptr || 
          current_character == -1 || current_character >= sbank->frames.size() ||
          current_action == -1 || current_action >= sbank->frames[current_character].size() ||
          current_direction == -1 || current_direction >= sbank->frames[current_character][current_action].size() ||
          current_frame == -1 || current_frame >= sbank->frames[current_character][current_action][current_direction].size()) {
          return nullptr;
        }


        //was return std::shared_ptr<GTSpriteFrame>(&(sbank->frames[current_character][current_action][current_direction][current_frame]));
        return &(sbank->frames[current_character][current_action][current_direction][current_frame]);
      }

    public:
      // member functions: returning GTres_t bc can use std::bind to hand in params such as timestamp, so long as return type is right!
      // ... will have to think re, might need wrappers
      // see above with declaration of event_func_t
      virtual GTres_t set_action(std::string actname);
      virtual GTres_t set_direction(std::string dirname);
      virtual GTres_t set_frame(GTindex_t fr);

      // functions intended to be called by ev_map - signature is GTRes_t(), maybe with bind we can 
      // overrideable, default implementation just does current_frame +1 mod # frames in current action
      // returns number of millis to wait until next frame, 0 if the animation is ending, -1 on error
      // let's say we add _event to functions intended to be called as events
      virtual GTres_t advance_frame_event(GTdur_t lts);

  };

  // MAP =========================================================================================
  // Will be organized similarly to a Tiled map, bc why not, but streamlined to contain only
  // what's necessary at runtime.

  // GTTile is the texture vertex info for a tile in a given tile sheet
  class GTTile {
    public:
      GTcoord_t ulx;
      GTcoord_t uly; 
      GTcoord_t wid;
      GTcoord_t ht;

    public:
      GTTile() {}
      GTTile(GTcoord_t ux, GTcoord_t uy, GTcoord_t w, GTcoord_t h) {
        ulx = ux;
        uly = uy;
        wid = w;
        ht = h;
      }
      virtual ~GTTile() {}

      // for writing to json file - should this be here?
      // assuming that e.g. j[layer_atlas_name] is a json and we hand in that
      public:
        virtual bool add_to_json(json& j) {
          j.push_back(std::map<std::string, GTcoord_t>{
            {"ulx", ulx},
            {"uly", uly},
            {"wid", wid},
            {"ht", ht}
          });
          return true;
        }

        // assuming that e.g. j[layer_atlas_name][tile index]
        // is usable as a json itself, and that's jt
        // FIND OUT IF THIS IS A GOOD WAY TO DO THIS 
        virtual bool get_from_json(json& jt) {
          if(jt.count("ulx") == 0 || jt.count("uly") == 0 ||
              jt.count("wid") == 0 || jt.count("ht") == 0) {
            //error! missing a field
            fprintf(stderr,"*** ERROR: tile needs ulx, uly, wid, ht, got %s\n",jt.dump().c_str());
            return false;
          }
          ulx = jt["ulx"];
          uly = jt["uly"];
          wid = jt["wid"];
          ht = jt["ht"];
          return true;
        }
  };

  //base class for shaped regions on map, for things like terrain type, triggers, etc.
  //strings are fairly expensive, 32 bytes on entrapta (intel core i5/64 bit linux)
  //16 bytes for a smart pointer, which makes sense - pointer + ref count + maybe other little stuff
  //would be nice to build app-specific lookup table for names, then could use an index value
  //how many of these do we want to keep around?
  typedef enum : GTindex_t {
    GTAT_Unknown = 0,
    GTAT_NoGo,
    GTAT_Slow,
    GTAT_Trigger
  } GTArea_type;

  typedef enum : GTindex_t {
    GTST_Unknown = 0,
    GTST_Rectangle,
    GTST_Ellipse,
    GTST_Polygon,
    GTST_Point
  } GTShape_type;


  class GTShape {
    public:
      // data members
      GTArea_type purpose;
      GTindex_t name_index;     // indexes into a list of strings; probably not have lots of same names but few shapes will have names and this is smaller than a string
      GTPoint position;         // we may want to move these around
      GTPoint bbox_ul;          // bounding box upper left, relative to position
      GTPoint bbox_lr;          // bounding box lower right, ""

    public:
      GTShape() {
        purpose = GTAT_Unknown;
        name_index = -1;
        position = {0,0};
        bbox_ul = {0,0};
        bbox_lr = {-1,-1};
      }
      GTShape(GTArea_type pur, GTPoint pos, GTPoint bul, GTPoint blr, GTindex_t ni) {
        purpose = pur;
        position = pos;
        bbox_ul = bul;
        bbox_lr = blr;
        name_index = ni;
      }
      virtual ~GTShape() {}

    public:
      //member functions
      // say bbox_check is end-exclusive in both dimensions?
      inline bool bbox_check(GTPoint pt) {
        return(pt.x >= bbox_ul.x + position.x && pt.x < bbox_lr.x + position.x && 
                pt.y >= bbox_ul.y + position.y && pt.y < bbox_lr.y + position.y);
      }
      virtual bool inside_shape_if_inside_bbox(GTPoint pt) = 0;      //each shape must define; doesn't do bbox check, inside() does
      bool inside(GTPoint pt) {
        return bbox_check(pt) && inside_shape_if_inside_bbox(pt);
      }

      virtual GTShape_type get_shape_type() = 0;
      virtual std::shared_ptr<std::vector<GTPoint>> get_geometry() = 0;

      virtual bool add_to_json(json& j);
      virtual bool get_from_json(json& jt);
  };

  class GTRectangle : public GTShape {
    public:
      //no data members other than base class

    public:
      GTRectangle() {}
      GTRectangle(GTArea_type pur, GTPoint pos, GTPoint bul, GTPoint blr, GTindex_t ni) : GTShape(pur, pos, bul, blr, ni) {}
      virtual ~GTRectangle() {}

    public:
      //trivial: if pt is inside bbox, it's inside the rectangle.
      virtual bool inside_shape_if_inside_bbox(GTPoint pt) override { return true; }
      virtual bool add_to_json(json& j) override;
      virtual bool get_from_json(json& jt) override;

      virtual GTShape_type get_shape_type() override { return GTST_Rectangle; }
      virtual std::shared_ptr<std::vector<GTPoint>> get_geometry() override {
        auto vec = std::shared_ptr<std::vector<GTPoint>>(new std::vector<GTPoint>(2));
        (*vec)[0] = bbox_ul;
        (*vec)[1] = bbox_lr;
        return vec;
      }
  };

  class GTEllipse : public GTShape {
    public:
      //no data members other than base class

    public:
      GTEllipse() {}
      GTEllipse(GTArea_type pur, GTPoint pos, GTPoint bul, GTPoint blr, GTindex_t ni) : GTShape(pur, pos, bul, blr, ni) {}
      virtual ~GTEllipse() {}

    public:
      //remember to allow for "position"
      virtual bool inside_shape_if_inside_bbox(GTPoint pt) override;
      virtual bool add_to_json(json& j) override;
      virtual bool get_from_json(json& jt) override;

      virtual GTShape_type get_shape_type() override { return GTST_Ellipse; }
      virtual std::shared_ptr<std::vector<GTPoint>> get_geometry() override {
        auto vec = std::shared_ptr<std::vector<GTPoint>>(new std::vector<GTPoint>(2));
        (*vec)[0] = bbox_ul;
        (*vec)[1] = bbox_lr;
        return vec;
      }
  };

  //assumed to be a simple polygon! I think it's ok if there isn't a duplicate first point after last
  class GTPolygon : public GTShape {
    public:
      std::vector<GTPoint> points;

    public:
      GTPolygon() {}
      GTPolygon(GTArea_type pur, GTPoint pos, GTPoint bul, GTPoint blr, GTindex_t ni) : GTShape(pur, pos, bul, blr, ni) {}
      virtual ~GTPolygon() {}

    public:
      //remember to allow for "position"
      // HERE USE BOURKE'S ROUTINE AND CREDIT IT
      virtual bool inside_shape_if_inside_bbox(GTPoint pt) override;
      virtual bool add_to_json(json& j) override;
      virtual bool get_from_json(json& jt) override;

      virtual GTShape_type get_shape_type() override { return GTST_Polygon; }
      virtual std::shared_ptr<std::vector<GTPoint>> get_geometry() override {
        auto vec = std::shared_ptr<std::vector<GTPoint>>(new std::vector<GTPoint>(points.size()));
        std::copy(points.begin(), points.end(), vec->begin());
        return vec;
      }
  };




  //**********************************************************************************************
  //**********************************************************************************************
  //**********************************************************************************************
  // ANIMATED TILE SUBCLASS OF THAT WHICH EVENTS CAN ADVANCE
  // Multiple inheritance: can make it so GTTile is not an Entity but GTAnimatedTile is both
  // that and an event entity
  // not sure this is the level at which tile animation will happen, but.
  //**********************************************************************************************
  //**********************************************************************************************
  //**********************************************************************************************
  class GTAnimatedTile: public GTTile, public GTEventEntity {
    public:
      GTAnimatedTile() {}
      virtual ~GTAnimatedTile() {}
  };


  //base class for layers - assuming that events will occur on them
  class GTMapLayer : public GTEventEntity {
    public:
      //data members
      std::string name;       //we'll want a way to choose these by name e.g. for interleaving sprites in the scene graph
      //single source of tile imagery - 
      //bytes that are just a png file in memory. May later have other formats
      std::vector<uint8_t> image_data;
      //tile atlas - texture coordinates within image for tile n
      //tile 0 is always the no-tile, but put an entry in here for it
      //to keep the map indexing simple
      std::vector<GTTile> tile_atlas;

    public:
      void init() {
        name.clear();
        image_data.clear();
        tile_atlas.clear();
        //add a dummy tile for tile 0, which should never get rendered
        tile_atlas.push_back(GTTile(0,0,0,0));
      }
      GTMapLayer() {
        init();
      }
      virtual ~GTMapLayer() {}

      // ****************************************************************
      // ****************************************************************
      // ****************************************************************
      // JSON IN/OUT WILL NEED TO BE DONE BY SUBCLASSES.
      // HOW DO WE KNOW WHICH KIND TO INSTANTIATE ON READING?
      // DO WE NEED A TYPE FIELD?
      // well.... *do* we need to stub these? There are the image_data
      // and atlas to consider in each case.
      // write image data with https://github.com/tplgy/cppcodec
      // see https://github.com/tplgy/cppcodec#base64
      // ****************************************************************
      // ****************************************************************
      // ****************************************************************
      virtual bool add_to_json(json& j);
      virtual bool get_from_json(json& jt);
  };


  //location / extent of a free-floating tile as opposed to TiledMapLayer
  //grid-determined location
  //It's not an event entity but subclasses might be, consider
  class GTObjectTile {
    public:
      GTtile_index_t tile;       //index into tile_atlas
      GTcoord_t orx;        //origin x
      GTcoord_t ory;        //origin y
      GTcoord_t wid;        //width 
      GTcoord_t ht;         //height

    public:
      GTObjectTile(); 
      GTObjectTile(GTtile_index_t t, GTcoord_t ox, GTcoord_t oy, GTcoord_t w, GTcoord_t h);
      virtual ~GTObjectTile(); 

    // json i/o
    public:
      virtual bool add_to_json(json& j);
      virtual bool get_from_json(json& jt);
  };

  //layer that consists of tiles individually placed with no grid
  class GTObjectsMapLayer : public GTMapLayer {
    public:
      std::vector<std::shared_ptr<GTObjectTile>> tile_objects;
      std::vector<std::shared_ptr<GTShape>> shapes;

    public:
      // json i/o
      virtual bool add_to_json(json& j) override;
      virtual bool get_from_json(json& jt) override;
  };

  // layer that consists of a square grid of (at least mostly) same-size tiles
  // may have some odd-sized ones
  // the defining characteristic is that tile placement is
  // determined by the grid. 
  // Though the odd sized ones are handled by tile_objects list.
  class GTTiledMapLayer : public GTMapLayer {
    public:
      GTcoord_t layer_tilewid;   //layer width in tiles
      GTcoord_t layer_tileht;    //height
      GTcoord_t tile_pixwid;     //tile grid width in pixels
      GTcoord_t tile_pixht;      //height
      
      //tilemap entries are indices into tile_atlas; 0 means no tile in
      //that grid location
      std::vector<GTtile_index_t> tile_map;

      std::vector<std::shared_ptr<GTObjectTile>> tile_objects;

    public:
      // json i/o
      virtual bool add_to_json(json& j) override;
      virtual bool get_from_json(json& jt) override;
  };


  class GTMap : public GTEventEntity {
    public:
      // Tiled map has only lists of tilesets and layers.
      // do we need the notion of a tileset?
      // each layer has a tile sheet and atlas, yes?
      std::vector<std::shared_ptr<GTMapLayer>> layers;
      std::shared_ptr<GTMapLayer> get_layer_by_name(std::string nm) {
        for(auto plyr : layers) {
          //printf("Checking \"%s\" against layer name \"%s\"\n",nm.c_str(),plyr->name.c_str());
          if(plyr->name == nm) return plyr;       //dumb search but we're not likely to have lots of these
        }
        return nullptr; //didn't find it
      }

    public:
      GTMap();
      virtual ~GTMap();

    public:
      virtual bool add_to_json(json& j);
      virtual bool get_from_json(json& jt);

      // this worked for SFMLMap too :P
      virtual bool load_from_json_file(std::string map_filename) {
        std::ifstream ifs(map_filename);
        if(ifs.fail()) {
            printf("*** ERROR: couldn't open %s\n",map_filename.c_str());
            return false;
        }
        json jmap;
        ifs >> jmap;
        ifs.close();

        if(!get_from_json(jmap)) {
            printf("*** ERROR: failed to load SFMLMap from json file \"%s\"\n",map_filename.c_str());
            return false;
        }
        return true;
      }
  };

}

#endif
