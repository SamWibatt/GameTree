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

  // SPRITE =========================================================================================

  bool GTSpriteFrame::add_to_json(json& j) {
    json subj;
    // GTcoord_t ulx, uly;         //texture coordinates, pixels, within sprite sheet texture
    // GTcoord_t wid, ht;          //extents of rectangle within sprite sheet texture
    // GTcoord_t offx, offy;     //offsets to give to sprite.setOrigin(sf::Vector2f(offx, offy));
    // GTdur_t dur;              //duration, millis
    
    subj["ulx"] = ulx;
    subj["uly"] = uly;
    subj["wid"] = wid;
    subj["ht"] = ht;
    subj["offx"] = offx;
    subj["offy"] = offy;
    subj["dur"] = dur;

    j.push_back(subj);

    return true;
  }

  bool GTSpriteFrame::get_from_json(json& jt) {
    if(!jt.contains("dur") || !jt.contains("ulx") || !jt.contains("uly") || !jt.contains("wid") || !jt.contains("ht") ||
        !jt.contains("offx") || !jt.contains("offy")) {
      fprintf(stderr,"*** ERROR: GTSpriteFrame object requires all of dur, ulx, uly, wid, ht, offx, offy\n");
      return false;
    }

    ulx = jt["ulx"];
    uly = jt["uly"];
    wid = jt["wid"];
    ht = jt["ht"];
    offx = jt["offx"];
    offy = jt["offy"];
    dur = jt["dur"];

    return true;
  }

  bool GTSpriteInfo::add_to_json(json& j) {
    j["character_to_index"] = character_to_index;
    j["action_to_index"] = action_to_index;
    j["direction_to_index"] = direction_to_index;

    j["index_to_character"] = index_to_character;
    j["index_to_action"] = index_to_action;
    j["index_to_direction"] = index_to_direction;

    return true;
  }

  bool GTSpriteInfo::get_from_json(json& jt) {
    if(!jt.contains("character_to_index") || !jt.contains("action_to_index") || !jt.contains("direction_to_index") || 
        !jt.contains("index_to_character") || !jt.contains("index_to_action") || !jt.contains("index_to_direction")) {
      fprintf(stderr,"*** ERROR: GTSpriteInfo object requires all of character_to_index, action_to_index, direction_to_index, index_to_character, index_to_action, and index_to_direction\n");
      return false;
    }

    character_to_index = jt["character_to_index"].get<std::map<std::string,GTindex_t>>();
    action_to_index = jt["action_to_index"].get<std::map<std::string,GTindex_t>>();
    direction_to_index = jt["direction_to_index"].get<std::map<std::string,GTindex_t>>();

    index_to_character = jt["index_to_character"].get<std::vector<std::string>>();
    index_to_action = jt["index_to_action"].get<std::vector<std::string>>();
    index_to_direction = jt["index_to_direction"].get<std::vector<std::string>>();

    return true;
  }

  bool GTSpriteBank::add_to_json(json& j) {
    // GTSpriteInfo info;
    // std::map<GTindex_t, std::map<GTindex_t, std::map<GTindex_t, std::vector<GTSpriteFrame>>>> frames;
    // std::vector<uint8_t> image_data;      // png-formatted (i.e., written to disk would be a full .png file) image data of sprite sheet
    if(info.add_to_json(j["info"]) == false) {
      printf("*** ERROR writing json for GTSprite.info\n");
      return false;
    }

    //eep frames - how to do this? loopity loop loop?
    // wait, this isn't quite right. this is using indices, not keys.
    // but now they are!
    for(auto ch = 0; ch < frames.size(); ch++) {
      for(auto ac = 0; ac < frames[ch].size(); ac++) {
        for(auto di = 0; di < frames[ch][ac].size(); di++) {
          for(auto sf = 0; sf < frames[ch][ac][di].size(); sf++) {
            // try forcing the numbers to be strings - otherwise they're interpreted as vector indices
            // well, now we want them to be!
            //if(sf.add_to_json(j["frames"][std::to_string(ch.first)][std::to_string(ac.first)][std::to_string(di.first)]) == false) {
            if(frames[ch][ac][di][sf].add_to_json(j["frames"][ch][ac][di]) == false) {
              printf("*** ERROR writing json for GTSprite.frames\n");
              return false;
            }
          }
        }
      }
    }

    //then write out base64_url - encoded png imagery of sprite sheet
    std::string encoded_png = base64::encode(image_data);
    j["image_data"] = encoded_png;

    return true;
  }

  bool GTSpriteBank::get_from_json(json& jt) {

    printf("- Reading sprite info...\n");
    if(info.get_from_json(jt["info"]) == false) {
      printf("*** ERROR reading json for GTSprite.info\n");
      return false;
    }

    // how do we get the frames back out? TEST THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    printf("- Reading sprite frames...\n");
    frames.resize(jt["frames"].size());
    for(auto chi = 0; chi < jt["frames"].size(); chi++) {
      frames[chi].resize(jt["frames"][chi].size());
      for(auto aci = 0; aci < frames[chi].size(); aci++) {
        frames[chi][aci].resize(jt["frames"][chi][aci].size());
        for(auto dii = 0; dii < frames[chi][aci].size(); dii++) {
          frames[chi][aci][dii].resize(jt["frames"][chi][aci][dii].size());
          for(auto sfi = 0; sfi < frames[chi][aci][dii].size(); sfi++) {
            if(frames[chi][aci][dii][sfi].get_from_json(jt["frames"][chi][aci][dii][sfi]) == false) {
              printf("*** ERROR reading json for GTSprite.frames\n");
              return false;
            }
          }
        }
      }
    }

    // and finally, get the base64_url-encoded string of image_data decoded into image_data
    printf("- Decoding image data\n");
    image_data = base64::decode(std::string(jt["image_data"]));

    return true;
  }


  // ACTOR ==========================================================================================
  GTActor::GTActor() {
    // id should be filled in by Entity ctor
    //printf("Creating actor id %u\n",id);
    sbank = nullptr;
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
    if(sbank == nullptr) return -1;
    GTindex_t actind = sbank->info.get_index_for_action(actname);
    if(actind == -1) return -1;
    if(actind == current_action) return 1;   //already in this action!
    current_action = actind;
    //return true;    //debug
    // keep direction as is but set frame to 0
    return set_frame(0);
  }

  // MIGHT ALSO PASS IN WHETHER THE ANIMATION REPEATS
  GTres_t GTActor::set_direction(std::string dirname) {
    if(sbank == nullptr) return -1;
    GTindex_t dirind = sbank->info.get_index_for_direction(dirname);
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
      (current_frame + 1) % sbank->frames[current_character][current_action][current_direction].size();

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

    j["name"] = name;

    //OBJECT LAYERS MAY NOT HAVE IMAGE DATA! don't emit an image_data or tile_atlas if they don't!
    if(!image_data.empty()) {
      std::string encoded_png = base64::encode(image_data);

      j["image_data"] = encoded_png;

      for(auto t : tile_atlas) {
        t.add_to_json(j["tile_atlas"]);
      }
    } else {
      // is this even a problem?
      //fprintf(stderr,"+++ WARNING: map layer doesn't have any image data. OK if it's a polygons-only object layer or something\n");
    }

    return true;
  }

  bool GTMapLayer::get_from_json(json& jt) {
    // get the name
    name = jt["name"];

    if(!jt.contains("image_data") || !jt.contains("tile_atlas")) {
      //fprintf(stderr,"+++ WARNING: map layer has no image_data or tile_atlas - ok if it's an object layer with no imagery\n");
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

  // GTShape and subclasses ----------------------------------------------------------------------

  // subclasses call this first, like with MapLayer
  // so this takes a json that is a sub-json created by one of the subclasses, qv.
  bool GTShape::add_to_json(json& j) {
    // base class handles
    // GTArea_type purpose;
    // GTindex_t name_index;     // indexes into a list of strings; probably not have lots of same names but few shapes will have names and this is smaller than a string
    // GTPoint position;         // we may want to move these around
    // GTPoint bbox_ul;          // bounding box upper left, relative to position
    // GTPoint bbox_lr;          // bounding box lower right, ""

    j["purpose"] = purpose;           // will show up as a number, not the enum name, wev
    j["name_index"] = name_index;
    j["position"] = std::pair<int, int>(position.x, position.y);    //see if this works - doing GTPoint.add_to_json seems excessive
    j["bbox_ul"] = std::pair<int, int>(bbox_ul.x, bbox_ul.y);
    j["bbox_lr"] = std::pair<int, int>(bbox_lr.x, bbox_lr.y);

    return true;
  }

  bool GTShape::get_from_json(json& jt) {
    // sanity check - for now, presence only
    if(!jt.contains("purpose") || !jt.contains("name_index") ||
        !jt.contains("position") || !jt.contains("bbox_ul") ||
        !jt.contains("bbox_lr")) {
      fprintf(stderr,"*** ERROR: shape requires purpose, name_index, position, bbox_ul, and bbox_lr\n");
      return false;
    }

    // got all the pieces, read them in
    std::pair<float,float> perry;
    purpose = jt["purpose"];
    name_index = jt["name_index"];
    perry = jt["position"];
    position.x = perry.first;
    position.y = perry.second;
    perry = jt["bbox_ul"];
    bbox_ul.x = perry.first;
    bbox_ul.y = perry.second;
    perry = jt["bbox_lr"];
    bbox_lr.x = perry.first;
    bbox_lr.y = perry.second;

    return true;
  }

  bool GTRectangle::add_to_json(json& j) {
    //printf("--- add rectangle\n");
    json subj;
    if(!GTShape::add_to_json(subj)) return false;
    subj["type"] = "rectangle";
    j.push_back(subj);
    return true; 
  }

  bool GTRectangle::get_from_json(json& jt) {
    if(!GTShape::get_from_json(jt)) return false; 
    // I think that's all we need to do with a rectangle
    
    return true; 
  }

  //https://math.stackexchange.com/questions/76457/check-if-a-point-is-within-an-ellipse
  // answer by Srivatsan which uses no sqrts
  // ((((x-h) * (x-h)) / (rx * rx)) + (((y-k) * (y-k)) / (ry * ry))) <= 1
  // if that is true, x,y is inside the ellipse.
  // where h, k are center of ellipse
  // rx = semi-major axis (width?) of ellipse - no, just half of the longer axis, width or height, whichever is larger
  // ry = semi-minor axis (height?) of ellipse - half of shorter axis
  // though this looks like it's assuming the rx/ry are half of wid/ht resp
  // wiki page has the same formula under https://en.wikipedia.org/wiki/Semi-major_and_semi-minor_axes#Ellipse
  // I think bc I know the bounding box I can just use width or ht /2
  // so let's try that and see what we get
  // remember to allow for shape's "position"
  bool GTEllipse::inside_shape_if_inside_bbox(GTPoint pt) {
    // should I subtract the position off of the point to check, instead of adding it to bbox?
    // that way stuff is nearer to the origin and the squaring won't make it overflow?
    // ASSUME THE BOUNDING BOX UPPER LEFT IS WHERE THE ORIGIN IS
    GTcoord_t widh = (bbox_lr.x - bbox_ul.x) / 2;
    GTcoord_t hth = (bbox_lr.y - bbox_ul.y / 2);
    GTcoord_t h = widh;
    GTcoord_t k = hth;
    GTcoord_t rx = (widh > hth) ? widh : hth;
    GTcoord_t ry = (widh <= hth)? widh : hth;  
    GTcoord_t x = pt.x - position.x;
    GTcoord_t y = pt.y - position.y;

    //ick, result of 0..1 so it's all floaty? Well, I see it implemented as all ints here - https://www.geeksforgeeks.org/check-if-a-point-is-inside-outside-or-on-the-ellipse/
    // so let's try it
    // NEEDS TESTING
    return ( ( ((x-h) * (x-h)) / (rx * rx) ) + ( ((y-k) * (y-k)) / (ry * ry) ) ) <= 1;
  }

  bool GTEllipse::add_to_json(json& j) {
    //printf("--- add ellipse\n");
    json subj;
    if(!GTShape::add_to_json(subj)) return false;
    subj["type"] = "ellipse";
    j.push_back(subj);
    return true; 
  }

  bool GTEllipse::get_from_json(json& jt) {
    if(!GTShape::get_from_json(jt)) return false; 
    // I think that's all we need to do with an ellipse
    return true; 
  }

  //adapted from Paul Bourke, http://paulbourke.net/geometry/polygonmesh/
  //Copyright notice on home page http://www.paulbourke.net/ reads
  //"Any source code found here may be freely used provided credits are given to the author."
  //bool InsidePolygon(std::vector<GTPoint>& polygon, GTPoint p)

  bool GTPolygon::inside_shape_if_inside_bbox(GTPoint pt)
  {
    int32_t counter = 0;
    int32_t i;
    int32_t xinters;      //should be float - originally was, see if works w/int - so far so good!
    GTPoint p1,p2;

    p1 = points[0];
    p1.x += position.x;     //need to adjust for position
    p1.y += position.y;
    for (i=1;i<=points.size();i++) {
      p2 = points[i % points.size()];
      p2.x += position.x;
      p2.y += position.y;
      if (pt.y > std::min(p1.y,p2.y)) {
        if (pt.y <= std::max(p1.y,p2.y)) {
          if (pt.x <= std::max(p1.x,p2.x)) {
            if (p1.y != p2.y) {
              xinters = (pt.y-p1.y)*(p2.x-p1.x)/(p2.y-p1.y)+p1.x;
              if (p1.x == p2.x || pt.x <= xinters)
                counter++;
            }
          }
        }
      }
      p1 = p2;
    }

    if (counter % 2 == 0)
      return(false);
    else
      return(true);
  }

  bool GTPolygon::add_to_json(json& j) {
    //printf("--- add polygon\n");
    json subj;
    if(!GTShape::add_to_json(subj)) return false;
    subj["type"] = "polygon";

    for(auto pt : points) {
      subj["points"].push_back(std::pair<GTcoord_t, GTcoord_t>(pt.x,pt.y));
    }

    j.push_back(subj);
    return true; 
  }

  bool GTPolygon::get_from_json(json& jt) {
    if(!GTShape::get_from_json(jt)) return false; 

    if(!jt.contains("points")) {
      printf("*** ERROR: polygon has no points\n");
      return false;
    }

    points.clear();
    for(auto j = 0; j < jt["points"].size(); j++) {
      std::pair<GTcoord_t, GTcoord_t> perry;
      perry = jt["points"][j];      //see if works
      points.push_back(GTPoint(perry.first, perry.second));
    }
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

    // emit shapes
    printf("---- about to emit shapes\n");
    for(auto shap : shapes) {
      shap->add_to_json(subj["shapes"]);
    }

    j.push_back(subj);

    return true;
  }

  bool GTObjectsMapLayer::get_from_json(json& jt) {
    if(!GTMapLayer::get_from_json(jt)) return false; 

    //must have at least one of tile_objects or shapes
    if(!jt.contains("tile_objects") && !jt.contains("shapes")) {
      printf("*** ERROR: objects layer has no tile objects or shapes\n");
      return false;
    }

    if(jt.contains("tile_objects")) {
      tile_objects.resize(jt["tile_objects"].size());
      for(auto j = 0; j <  jt["tile_objects"].size(); j++) {
        tile_objects[j] = std::shared_ptr<GTObjectTile>(new GTObjectTile());
        if(!tile_objects[j]->get_from_json(jt["tile_objects"][j])) {
          printf("*** ERROR: failed to read tile object\n");
          return false;
        }
      }
    } 

    if(jt.contains("shapes")) {
      shapes.resize(jt["shapes"].size());
      for(auto j = 0; j <  jt["shapes"].size(); j++) {
        if(jt["shapes"][j].contains("type")) {
          if(jt["shapes"][j]["type"] == "rectangle") {
            shapes[j] = std::shared_ptr<GTRectangle>(new GTRectangle());
            if(!shapes[j]->get_from_json(jt["shapes"][j])) {
              printf("*** ERROR: failed to read Rectangle object\n");
            }
          } else if(jt["shapes"][j]["type"] == "ellipse") {
            shapes[j] = std::shared_ptr<GTEllipse>(new GTEllipse());
            if(!shapes[j]->get_from_json(jt["shapes"][j])) {
              printf("*** ERROR: failed to read Ellipse object\n");
            }
          } else if(jt["shapes"][j]["type"] == "polygon") {
            shapes[j] = std::shared_ptr<GTPolygon>(new GTPolygon());
            if(!shapes[j]->get_from_json(jt["shapes"][j])) {
              printf("*** ERROR: failed to read Polygon object\n");
            }
          } else {
            // HEY HANDLE POINTS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            printf("*** ERROR: unrecognized shape type \"%s\" in shape\n",std::string(jt["shapes"][j]["type"]).c_str());
          }
        } else {
          printf("*** ERROR: shape does not contain a type field\n");
        }
        // tile_objects[j] = std::shared_ptr<GTObjectTile>(new GTObjectTile());
        // tile_objects[j]->get_from_json(jt["tile_objects"][j]);
      }
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