#ifndef SFMLSPRITE_H_INCLUDED
#define SFMLSPRITE_H_INCLUDED

#include <SFML/Graphics.hpp>

namespace sfml_assets {

  // single frame
  class SFMLSpriteFrame {
    public:
      //data members
      int ulx, uly;         //texture coordinates, pixels, within sprite sheet texture
      int wid, ht;          //extents of rectangle within sprite sheet texture
      float offx, offy;     //offsets to give to sprite.setOrigin(sf::Vector2f(offx, offy));
      int dur;              //duration, millis

    public:
      SFMLSpriteFrame() {}
      SFMLSpriteFrame(int ux, int uy, int w, int h, int ox, int oy, int dr) {
        ulx = ux; 
        uly = uy;
        wid = w;
        ht = h;
        offx = ox;
        offy = oy;
        dur = dr;
      }
      ~SFMLSpriteFrame() {}
  };

  //There needs to be a class that's like a "directory" for these
  //contains inventory of actions, # directions & such for each of those
  //can be defined in a header emitted by asesprite2sfml
  class SFMLSpriteInfo {
    public:
      //data members
      // these map character name, action name, direction name to indices into frame map below
      std::map<std::string,int> character_to_index;
      std::map<std::string,int> action_to_index;
      std::map<std::string,int> direction_to_index;

    public:
      SFMLSpriteInfo(){}
      virtual ~SFMLSpriteInfo() {}

    public:
      //member functions
      int get_index_for_character(std::string chname) {
        auto it = character_to_index.find(chname);
        if(it == character_to_index.end()) return -1;
        return it->second;
      }
      int get_index_for_action(std::string actname) {
        auto it = action_to_index.find(actname);
        if(it == action_to_index.end()) return -1;
        return it->second;
      }
      int get_index_for_direction(std::string dirname) {
        auto it = direction_to_index.find(dirname);
        if(it == direction_to_index.end()) return -1;
        return it->second;
      }
  };

  class SFMLSprite {
    public:
      //data members
      SFMLSpriteInfo info;
      std::map<int, std::map<int, std::map<int, std::vector<SFMLSpriteFrame>>>> frames;
      sf::Texture spritesheet;

    public:
      SFMLSprite(){}
      virtual ~SFMLSprite() {}

    public:
      //member functions
  };
};


#endif

