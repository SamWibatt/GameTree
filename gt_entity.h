#ifndef GTREE_ENTITY_H_INCLUDED
#define GTREE_ENTITY_H_INCLUDED

namespace gt{

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
      GTActor(std::shared_ptr<GTSpriteBank> nsb) {
        sbank = nsb;
      }
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
}

#endif
