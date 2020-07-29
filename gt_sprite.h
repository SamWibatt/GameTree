#ifndef GTREE_SPRITE_H_INCLUDED
#define GTREE_SPRITE_H_INCLUDED


namespace gt{

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

}

#endif