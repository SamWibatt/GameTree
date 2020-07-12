#ifndef ASEPRITE2SFML_H_INCLUDED
#define ASEPRITE2SFML_H_INCLUDED

#include <memory>
#include <map>
#include "asepritereader.h"
#include "sfmlsprite.h"

using namespace asepreader;
using namespace sfml_assets;

class Aseprite2SFML {
  public:
    //data_members
    //quick test: seems to compile fine. Works, too, though keys end up in alpha order
    //std::map<std::string,int> map_s_to_i = { {"chunk", 99}, {"bunk", 111},{"hideyunk", -467}};
    std::vector<std::string> index_to_character;
    std::vector<std::string> index_to_action;
    std::vector<std::string> index_to_direction;

  public:
    Aseprite2SFML() {}
    ~Aseprite2SFML() {}

  public:
    //member functions
    // dumb indent thing
    std::string indent(int ntabs, int tab_size);
    // makes a string suitable for use as a C++ enum identifier
    std::string enum_id(std::string instr);  
    bool writeHeader(std::shared_ptr<AseSprite> psp, std::shared_ptr<SFMLSprite> psfp, std::string header_file, std::string header_out_dir);
    std::shared_ptr<SFMLSprite> ConvertAsepriteSpriteToSFMLSprite(std::shared_ptr<AseSprite> psp, std::string spritesheet_dir, std::string header_out_dir, std::string sprite_out_dir);
};

#endif
