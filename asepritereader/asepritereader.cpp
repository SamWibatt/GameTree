#include "asepritereader.h"

#include<iostream>
#include<fstream>
#include <sstream>
#include <map>

using json = nlohmann::json;

namespace asepreader {
  AseSpriteReader::AseSpriteReader() {
  }

  AseSpriteReader::~AseSpriteReader() {
  } 


  std::shared_ptr<AseSpriteSheet> AseSpriteReader::handle_sprite_meta(json::iterator meta_it) {

    std::shared_ptr<AseSpriteSheet> sheetly = std::shared_ptr<AseSpriteSheet>(new AseSpriteSheet); 

    //printf("- Handling metadata block\n");

    //argh but then how do we use the input value? it's an iterator in the middle of stuff
    //let's try this - yay worky
    auto &meta_obj = *meta_it;
    // this works to iterate over items in the meta block
    //for (auto& el : meta_obj.items()) {
    //  std::cout << "  - " << el.key() << std::endl; // << " : " << el.value() << "\n";
    //}

    /* meta looks like
    "meta": {
      "app": "http://www.aseprite.org/",
      "version": "1.2.21-x64",
      "image": "PrincessAll.png",
      "format": "RGBA8888",
      "size": { "w": 64, "h": 64 },
      "scale": "1"
    }

    //which parts do we care about?
    //not app
    //version? Might at some point
    //everything else yes
    */

    std::string image_filename = "*NO*IMAGE*FOUND*";

    //get the sprite sheet image filename!
    json::iterator meta_img_it = meta_obj.find("image");
    if(meta_img_it != meta_obj.end()) {
      if(meta_img_it->is_string() == true) {
        image_filename = std::string(*meta_img_it);
        sheetly->set_image_name(image_filename);
        //printf("  - image filename %s\n", sheetly->get_image_name().c_str());
      } else {
        //error!
        printf("  - *** ERROR: image entry in meta block isn't a string\n");
        return nullptr;
      }
    } else {
      printf("  - *** ERROR: no image entry found in meta block\n");
      return nullptr;
    }

    //"size": { "w": 64, "h": 64 },
    json::iterator meta_siz_it = meta_obj.find("size");
    if(meta_siz_it != meta_obj.end()) {
      if(meta_siz_it->is_structured() == true && meta_siz_it->contains("w") && meta_siz_it->contains("h")) {
        sheetly->set_image_wid(meta_obj["size"]["w"]);
        sheetly->set_image_ht(meta_obj["size"]["h"]);
        //printf("  - spritesheet image width %d height %d\n",sheetly->get_image_wid(),sheetly->get_image_ht());
      } else {
        printf("  - *** ERROR: size entry in image block isn't a hash or is missing w or h key\n");
        return nullptr;
      }
    } else {
      printf("  - *** ERROR: no size entry found in meta block\n");
      return nullptr;
    }

    // "format": "RGBA8888",
    // ... do we care about this, really? I think the sfml import will handle it. We don't have to do bitmap packing

    // "scale": "1"
    // what does this mean? Once I find out, implement as needed. 

    return sheetly;
  }

  //parses a frame name of the form "cCharacter_aAction_dDirection_fFrame" where delimiter here shown as _ is given in delim
  //and returns a tuple of string, string, string, int representing
  //("Character","Action","Direction",Frame,"Purpose")
  //on error, returns ("","","",-1,"")
  std::tuple<std::string, std::string, std::string, int, std::string> AseSpriteReader::parse_frame_name(std::string frname, char delim) {
    // this clumsiness tokenizes a string by given delimiter
    std::istringstream iss(frname);
    std::string token;
    std::vector<std::string> segments;
    while (std::getline(iss, token, delim)) segments.push_back(token);
    //make sure we get everything
    std::string cur_character;
    std::string cur_action;
    std::string cur_direction;
    int cur_frame = -1;
    std::string cur_purpose;
    for(auto sg : segments) {
      //printf("  - %s\n",sg.c_str());
      if(sg[0] == 'c') {
        cur_character = sg.substr(1,sg.length()-1);    //lop 'c' off the beginning
        //printf("    - Character: %s\n",cur_character.c_str());
      } else if(sg[0] == 'a') {
        cur_action = sg.substr(1,sg.length()-1);    //lop 'a' off the beginning
        //printf("    - Action: %s\n",cur_action.c_str());
      } else if(sg[0] == 'd') {
        cur_direction = sg.substr(1,sg.length()-1);    //lop 'd' off the beginning
        //printf("    - Direction: %s\n",cur_direction.c_str());
      } else if(sg[0] == 'f') {
        try {
          cur_frame = std::stoi(sg.substr(1,sg.length()-1));    //lop 'f' off the beginning and make number
          //printf("    - Frame: %d\n",cur_frame);
        } catch (...) {
          printf("*** ERROR: could not extract frame number from %s\n",sg.c_str());
          return std::make_tuple("","","",-1,"");
        }
      } else if(sg[0] == 'p') {
        cur_purpose = sg.substr(1,sg.length()-1);    //lop 'p' off the beginning
        //printf("    - Purpose: %s\n",cur_purpose.c_str());
      } else {
        //don't recognize this piece of the name
        printf("    - *** WARNING: unsupported segment %s in frame name %s\n",sg.c_str(),frname.c_str());
      }
    }

    //make sure we got everything
    if(cur_character.empty()) {
      printf("*** ERROR: no character name in frame name %s\n",frname.c_str());
      return std::make_tuple("","","",-1,"");
    }
    if(cur_action.empty()) {
      printf("*** ERROR: no action name in frame name %s\n",frname.c_str());
      return std::make_tuple("","","",-1,"");
    }
    if(cur_direction.empty()) {
      printf("*** ERROR: no direction in frame name %s\n",frname.c_str());
      return std::make_tuple("","","",-1,"");
    }
    if(cur_frame == -1) {
      printf("*** ERROR: no frame number in frame name %s\n",frname.c_str());
      return std::make_tuple("","","",-1,"");
    }
    if(cur_purpose.empty()) {
      printf("*** ERROR: no purpose in frame name %s\n",frname.c_str());
      return std::make_tuple("","","",-1,"");
    }

    return std::make_tuple(cur_character, cur_action, cur_direction, cur_frame, cur_purpose);
  }




  std::string AseSpriteReader::make_frame_name(std::string char_key, std::string act_key, std::string dir_key, int frame_num,
                              std::string purp_key, char delim) {
    //std::string delim_str(1,delim);
    char frname[1024];
    //return "c" + char_key + delim_str + "a" + act_key + delim_str + "d" + dir_key + delim_str + 
    //    "f" + std::to_string(frame_num) + delim_str + "p" + purp_key;
    sprintf(frname,"c%s%ca%s%cd%s%cf%02d%cp%s",char_key.c_str(), delim, act_key.c_str(), delim, 
            dir_key.c_str(), delim, frame_num, delim, purp_key.c_str());
    
    return std::string(frname);
  }


  //We get two frames: Imagery and Origin.
    // Frames need the hotspot calculation to be done bt parallel Imagery and Origin frames
    // for efficiency, could just do a bunch of indexing, like have a big vector of frames, and figure out
    // the first index for each character/action/direction. find parallel copies of each frame in Imagery and Origin:
    //  "cPrincessAll*aWalk*dR*f02*pImagery": {
    //   "frame": { "x": 33, "y": 36, "w": 16, "h": 18 },
    //   "rotated": false,
    //   "trimmed": true,
    //   "spriteSourceSize": { "x": 1, "y": 0, "w": 14, "h": 16 },
    //   "sourceSize": { "w": 16, "h": 16 },
    //   "duration": 150
    //  },

    //  # ---snip---

    //  "cPrincessAll*aWalk*dR*f02*pOrigin": {
    //   "frame": { "x": 6, "y": 111, "w": 3, "h": 3 },
    //   "rotated": false,
    //   "trimmed": true,
    //   "spriteSourceSize": { "x": 7, "y": 15, "w": 1, "h": 1 },
    //   "sourceSize": { "w": 16, "h": 16 },
    //   "duration": 150
    //  },    
    // the Origin layer's "spriteSourceSize" x and y are the coordinates of the dot within the original 16x16 frame. Similarly, 
    // the imagery frame's spriteSourceSize has the same for it, so subtracting the imagery's value from the origin's 
    // should yield the origin relative to the trimmed image.
    // additional math to allow for the inner padding possible by examining the "frame" w/h vs. the spriteSourceSize w/h, if needed

  std::shared_ptr<AseSpriteFrame> AseSpriteReader::build_frame(json::iterator img_it, json::iterator org_it) {
    //printf("          - build_frame\n");

    //how about insisting that trimmed be true in both and sprite source size in origin have w and h of 1
    //becomes much easier to do our calcs
    auto& img_obj = *img_it;
    auto& org_obj = *org_it;

    if(!img_obj["trimmed"].is_boolean() || img_obj["trimmed"] != true || 
        !org_obj["trimmed"].is_boolean() || org_obj["trimmed"] != true) {
      printf("*** ERROR: frames must all be trimmed (trimmed entry must be boolean and true)\n");
      return nullptr;
    }
    //printf("            - OK frames are trimmed\n");

    //sanity check types for snip and origin calculations - we don't need all of these but check if file is well formed
    if(!img_obj["frame"].is_object() || !img_obj["frame"]["x"].is_number_integer() || !img_obj["frame"]["y"].is_number_integer() ||
        !img_obj["frame"]["w"].is_number_integer() ||!img_obj["frame"]["h"].is_number_integer()) {
      printf("*** ERROR: imagery frame entry should be an object with integer entries x, y, w, h\n");
      return nullptr;
    }
    if(!img_obj["spriteSourceSize"].is_object() || !img_obj["spriteSourceSize"]["x"].is_number_integer() || !img_obj["spriteSourceSize"]["y"].is_number_integer() ||
        !img_obj["spriteSourceSize"]["w"].is_number_integer() ||!img_obj["spriteSourceSize"]["h"].is_number_integer()) {
      printf("*** ERROR: imagery spriteSourceSize entry should be an object with integer entries x, y, w, h\n");
      return nullptr;
    }
    if(!org_obj["frame"].is_object() || !org_obj["frame"]["x"].is_number_integer() || !org_obj["frame"]["y"].is_number_integer() ||
        !org_obj["frame"]["w"].is_number_integer() ||!org_obj["frame"]["h"].is_number_integer()) {
      printf("*** ERROR: origin frame entry should be an object with integer entries x, y, w, h\n");
      return nullptr;
    }
    if(!org_obj["spriteSourceSize"].is_object() || !org_obj["spriteSourceSize"]["x"].is_number_integer() || !org_obj["spriteSourceSize"]["y"].is_number_integer() ||
        !org_obj["spriteSourceSize"]["w"].is_number_integer() ||!org_obj["spriteSourceSize"]["h"].is_number_integer()) {
      printf("*** ERROR: origin spriteSourceSize entry should be an object with integer entries x, y, w, h\n");
      return nullptr;
    }

    // find frame and spriteSourceSize values to derive ulx, uly, w/h for snipping this frame from sheet
    // should we sanity-check these for being 1x1 pixel?
    if(org_obj["spriteSourceSize"]["w"] != 1 || org_obj["spriteSourceSize"]["h"] != 1) {
      printf("*** WARNING: origin frame should only have 1x1 pixel content\n");
    }

    // printf("            - img frame (x%3d y%3d w%3d) source (x%3d y%3d w%3d h%3d) org source (x%3d y%3d)\n", 
    //       int(img_obj["frame"]["x"]), int(img_obj["frame"]["y"]), int(img_obj["frame"]["w"]),
    //       int(img_obj["spriteSourceSize"]["x"]), int(img_obj["spriteSourceSize"]["y"]), 
    //       int(img_obj["spriteSourceSize"]["w"]), int(img_obj["spriteSourceSize"]["h"]),
    //       int(org_obj["spriteSourceSize"]["x"]), int(org_obj["spriteSourceSize"]["y"]));


    //now we know we have what we need, build our frame
    std::shared_ptr<AseSpriteFrame> framely = std::shared_ptr<AseSpriteFrame>(new AseSpriteFrame);

    framely->pad = (int(img_obj["frame"]["w"]) - int(img_obj["spriteSourceSize"]["w"]))/2;     //calculate padding between images - same in w and h
    //printf("            - img padding %d pix each side\n", framely->pad);

    //so: we now have all the quantities we need to derive the sprite's rectangle in the sheet and its origin wrt that.
    // imagery sample
    //   "frame": { "x": 33, "y": 36, "w": 16, "h": 18 },     <==== note this includes padding for position and dimensions
    //   "spriteSourceSize": { "x": 1, "y": 0, "w": 14, "h": 16 }, <==== so adjust with the relative position and dimensions from here
    //                                                                   deduce padding from frame w/h - sourcesize w/h, padding size doesn't affect x/y here
    //   "sourceSize": { "w": 16, "h": 16 },  <=== not sure if this is useful
    // corresponding origin
    //   "frame": { "x": 6, "y": 111, "w": 3, "h": 3 },
    //   "spriteSourceSize": { "x": 7, "y": 15, "w": 1, "h": 1 },
    //   "sourceSize": { "w": 16, "h": 16 },

    framely->ulx = int(img_obj["frame"]["x"]) + framely->pad;             // upper left x of imagery within sheet 
    framely->uly = int(img_obj["frame"]["y"]) + framely->pad;             // upper left y "
    framely->wid = int(img_obj["spriteSourceSize"]["w"]);                 // width of imagery within sheet
    framely->ht = int(img_obj["spriteSourceSize"]["h"]);                  // height
    framely->off_x = int(img_obj["spriteSourceSize"]["x"]) - 
        int(org_obj["spriteSourceSize"]["x"]);   // add this to game's sprite x to get imagery upper left x
    framely->off_y = int(img_obj["spriteSourceSize"]["y"]) - 
        int(org_obj["spriteSourceSize"]["y"]);   // same for y

    // also want duration
    if(img_obj["duration"].is_number_integer() == false) {
      printf("*** ERROR: duration should be an integer\n");
    }
    framely->dur = img_obj["duration"];
    // printf("            - img on sheet (ulx %d uly %d w %d h %d) game coord offset (x %d y %d) dur: %d ms\n",
    //     framely->ulx, framely->uly, framely->wid, framely->ht, 
    //     framely->off_x, framely->off_y,
    //     framely->dur);

    return framely;
  }




  //returns nullptr on error
  std::shared_ptr<std::map<std::string,std::map<std::string,std::map<std::string,std::vector<std::shared_ptr<AseSpriteFrame>>>>>> AseSpriteReader::handle_sprite_frames(json::iterator frames_it) {
    //printf("- Handling frames block\n");

    auto &frames_obj = *frames_it;

    //I hope this is the worst declaration I have to do 
    std::shared_ptr<std::map<std::string,std::map<std::string,std::map<std::string,std::vector<std::shared_ptr<AseSpriteFrame>>>>>> allframes = 
      std::shared_ptr<std::map<std::string,std::map<std::string,std::map<std::string,std::vector<std::shared_ptr<AseSpriteFrame>>>>>>(
        new std::map<std::string,std::map<std::string,std::map<std::string,std::vector<std::shared_ptr<AseSpriteFrame>>>>>());

    //OK so we need to read all the frames and parse their name into character / action / direction / frame / purpose
    //We should only have one character... ???
    //I can imagine there would be items / icons / etc. collections that had a bunch of "characters"
    // like have a "StationaryObject" character

    //so THESE are appropriate for the looping-through approach.
    // + yay this works
    //for (auto& el : (*frames_it).items()) {
    //  std::cout << "  - frame: " << el.key() << "\n"; // " : " << el.value() << "\n";
    //}
    // special iterator member functions for objects
    char delim_char = '*';
    std::string cur_character, cur_action, cur_direction, cur_purpose;
    int cur_frame;
    for (json::iterator it = frames_it->begin(); it != frames_it->end(); ++it) {
      std::cout << "  - frame: " << it.key() << "\n"; // " : " << it.value() << "\n";
      // ok so parse out character, action, direction, and frame. Delimited by asterisks
      std::tie(cur_character,cur_action,cur_direction,cur_frame, cur_purpose) = parse_frame_name(std::string(it.key()),'*');
      // on error it returns ("","","",-1), just look for the -1
      if(cur_frame == -1) {
        printf("- *** ERROR parsing frame name %s\n",std::string(it.key()).c_str());
        return nullptr;
      }
      // printf("    - character %s action %s direction %s frame %d purpose %s\n",
      //         cur_character.c_str(),cur_action.c_str(),cur_direction.c_str(),cur_frame,cur_purpose.c_str());

      //allocate space. Assume frames can be found out of order. Not worrying about purpose yet
      if((*allframes)[cur_character][cur_action][cur_direction].size() < cur_frame + 1) {
        (*allframes)[cur_character][cur_action][cur_direction].resize(cur_frame + 1);
      }
    }


    // what can that look like?
    //map[character][action][direction] is a vector of frames - and that is allframes

    //because the loop above initialized allframes to have all the slots it's going to need, 
    //can iterate like this
    //printf("- building frames\n");
    for(auto char_it = allframes->begin(); char_it != allframes->end(); char_it++) {
      std::string char_key = char_it->first;
      //printf("- character: %s\n",char_key.c_str());
      for(auto act_it = char_it->second.begin(); act_it != char_it->second.end(); act_it++) {
        std::string act_key = act_it->first;
        //printf("  - action: %s\n",act_key.c_str());
        for(auto dir_it = act_it->second.begin(); dir_it != act_it->second.end(); dir_it++) {
          std::string dir_key = dir_it->first;
          //printf("    - direction: %s\n",dir_key.c_str());
            for(auto frame_num = 0; frame_num < dir_it->second.size(); frame_num++) {
              //printf("      - frame %d\n",frame_num);
              //so now we have everything we need to assemble our parallel Imagery and Origin keys
              std::string imagery_key = make_frame_name(char_key, act_key, dir_key, frame_num, "Imagery", delim_char);
              std::string origin_key = make_frame_name(char_key, act_key, dir_key, frame_num, "Origin", delim_char);
              //printf("        - Imagery key: |%s| Origin key: |%s|\n",imagery_key.c_str(), origin_key.c_str());
              json::iterator img_it = frames_obj.find(imagery_key);
              json::iterator org_it = frames_obj.find(origin_key);
              if(img_it == frames_obj.end()) {
                printf("*** ERROR: couldn't find imagery key %s\n",imagery_key.c_str());
                return nullptr;
              }
              if(org_it == frames_obj.end()) {
                printf("*** ERROR: couldn't find origin key %s\n",origin_key.c_str());
                return nullptr;
              }

              //OK SO FINALLY WE HAVE WHAT WE NEED TO BUILD A FRAME!
              std::shared_ptr<AseSpriteFrame> framey = build_frame(img_it,org_it);

              if(framey == nullptr) {
                printf("*** ERROR: failed to construct frame from Imagery key: |%s| Origin key: |%s|\n",imagery_key.c_str(), origin_key.c_str());
                return nullptr;
              }

              // OK for this data structure to be unwieldy, as this is an intermediate form; it's the conversion
              // to sfml or whatever that will neat it up for e.g. arduino, crop sprite sheet for arduino,
              // maybe emit a header file that does all the organizing e.g. indices into a single big vector of frames
              // so, this:
              (*allframes)[char_key][act_key][dir_key][frame_num] = framey;
            }
        }
      }
    }

    //KLUDGY post-process: if action and direction have the same name, assume it means there is only one direction
    //for that action - currently that's how aseprite behaves if you have an outer and inner tag of the same extent.
    // so replace the direction with "A" if that's the case
    //printf("Postprocessing direction names...\n");
    for(auto ckv : (*allframes)) {
      cur_character = ckv.first;
      for(auto akv: ckv.second) {
        cur_action = akv.first;
        for(auto dkv: akv.second) {
          cur_direction = dkv.first;
          bool dir_replaced = false;
          for(auto frame_num = 0; frame_num < dkv.second.size(); frame_num++) {
            if(cur_direction == cur_action) {
              //printf("      - *** NOTE: special case direction == action, for action %s, replacing direction with 'A' (all)\n",cur_action.c_str());
              if((*allframes)[cur_character][cur_action].find("A") == (*allframes)[cur_character][cur_action].end()) {
                //reserve space in an "A" direction if one doesn't exist
                (*allframes)[cur_character][cur_action]["A"].resize((*allframes)[cur_character][cur_action][cur_direction].size());
              }
              (*allframes)[cur_character][cur_action]["A"][frame_num] = 
                std::shared_ptr<AseSpriteFrame>(new AseSpriteFrame((*allframes)[cur_character][cur_action][cur_direction][frame_num]));

              (*allframes)[cur_character][cur_action][cur_direction][frame_num] = nullptr;

              dir_replaced = true;
            }              
          }
          // if the direction was replaced with "A", get rid of original
          if(dir_replaced)
            (*allframes)[cur_character][cur_action].erase(cur_direction);
        }
      }
    }




    return allframes;
  }

  std::shared_ptr<AseSprite> AseSpriteReader::read_sprite_file(std::string filename) {
    //printf("Reading sprite metadata file %s\n",filename.c_str());

    std::shared_ptr<AseSprite> spritely = std::shared_ptr<AseSprite>(new AseSprite());


    //from https://github.com/nlohmann/json#examples
    // read a JSON file
    std::ifstream infile(filename);
    json jsonfile_in;
    infile >> jsonfile_in;
    infile.close();

    //glean useful info from the file!
    //https://github.com/nlohmann/json#stl-like-access shows various methods of iterating / searching the tree
    json::iterator meta_it = jsonfile_in.find("meta");
    if(meta_it != jsonfile_in.end()) {
      spritely->sheet = handle_sprite_meta(meta_it);
      if(spritely->sheet == nullptr) {
        printf("*** ERROR: failed to read meta block\n");
      }
      //printf("Spritely sheet wd %d ht %d\n",spritely->sheet->GetImage_wid(), spritely->sheet->GetImage_ht());
    } else {
      printf("*** ERROR: aseprite json file has no meta block\n");
      return nullptr;
    }

    //now for frames!
    json::iterator frames_it = jsonfile_in.find("frames");
    if(frames_it != jsonfile_in.end()) {
      spritely->frames = handle_sprite_frames(frames_it);
      if(spritely->frames == nullptr) {
        printf("*** ERROR: failed to read frames block\n");
      }
    } else {
      printf("*** ERROR: aseprite json file has no frames block\n");
      return nullptr;
    }


    return spritely;
  }

}