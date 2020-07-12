#include "aseprite2sfml.h"
#include <ctime>
#include <cctype>

using namespace asepreader;

// little indentation thing
std::string Aseprite2SFML::indent(int ntabs, int tab_size) {
  return std::string(ntabs * tab_size,' ');
}

//for creating identifiers ok to use as enum values - for now, just replace any non-alnums with _
std::string Aseprite2SFML::enum_id(std::string instr) {
  std::string outstr = instr;
  std::transform(instr.begin(), instr.end(), outstr.begin(), 
    [](char c) { return std::isalnum(c) ? c : '_'; });
  return outstr;
}

bool Aseprite2SFML::writeHeader(std::shared_ptr<AseSprite> psp, std::shared_ptr<SFMLSprite> psfp, std::string header_file, std::string header_out_dir) {

  auto frames = psp->frames;

  // *********************************************************************************************************
  // *********************************************************************************************************
  // *********************************************************************************************************
  // so as clever as this header-emitter is, it might be a bad idea bc we don't want to have all sprites
  // loaded all the time, do we. For some, like the main character, maybe this is useful, but others should
  // be loaded from files.
  // drat! This was fun tho
  // *********************************************************************************************************
  // *********************************************************************************************************
  // *********************************************************************************************************


  //emit mappings and frame data!
  //std::string header_file = "fiddle.h";
  std::string header_path = header_out_dir + "/" + header_file; //MAKE SYSTEM INDEPENDENT AND REAL NAME
  std::string header_namespace = header_file.substr(0,header_file.find_last_of('.'));
  // make namespace name out of header_file less extension, all alphanums lowercased and non-alphanums turned to _
  std::transform(header_namespace.begin(), header_namespace.end(), header_namespace.begin(), 
    [](char c) { return std::isalnum(c) ? std::tolower(c) : '_'; });

  //and of course include-guard
  std::string include_guard_var = header_file;
  // construct include guard var out of all-uppercased alphanum parts of header_file, any non-alphanum replaced with _
  std::transform(include_guard_var.begin(), include_guard_var.end(), include_guard_var.begin(), 
    [](char c) { return std::isalnum(c) ? std::toupper(c) : '_'; });

  int tab_size = 4;     //HEY PUT THIS SOMEWHERE GLOBAL

  FILE *fp = fopen(header_path.c_str(),"wt");

  //emit mappings from char/dir/act names to numbers?
  //or do enums?
  // build usable structures here so that aseprite->this is a usable, if kind of silly,
  // way to handle it in-game
  // think of other stuff to put in header - for now, filename, rip time (local)
  fprintf(fp,"// %s\n",header_file.c_str());
  std::time_t t = std::time(nullptr);
  std::tm *tm = std::localtime(&t);
  char date_buffer[128];
  if(std::strftime(date_buffer,sizeof(date_buffer),"%Y-%b-%d %T %z",tm))
    fprintf(fp,"// ripped at %s\n\n",date_buffer);


  fprintf(fp,"#ifndef %s_INCLUDED\n",include_guard_var.c_str());
  fprintf(fp,"#define %s_INCLUDED\n\n",include_guard_var.c_str());

  fprintf(fp,"#include <map>\n");
  fprintf(fp,"#include <vector>\n");

  fprintf(fp,"namespace %s {\n",header_namespace.c_str());
  
  // enums, say? swh if we do enum <name>:int - so not scoped, but typed
  // give them the same names for each sprite and let namespace disambiguate?
  fprintf(fp,"\n%s// character enum\n",indent(1,tab_size).c_str());
  fprintf(fp,"%senum Character : int {\n",indent(1,tab_size).c_str());
  for(auto j = 0; j < index_to_character.size(); j++) {
    fprintf(fp, "%s%s = %d%s\n",indent(2,tab_size).c_str(), enum_id(index_to_character[j]).c_str(), j, j == index_to_character.size()-1 ? "":",");
  }
  fprintf(fp,"%s}\n\n",indent(1,tab_size).c_str());

  fprintf(fp,"%s// action enum\n",indent(1,tab_size).c_str());
  fprintf(fp,"%senum Action : int {\n",indent(1,tab_size).c_str());
  for(auto j = 0; j < index_to_action.size(); j++) {
    fprintf(fp, "%s%s = %d%s\n",indent(2,tab_size).c_str(), enum_id(index_to_action[j]).c_str(), j, j == index_to_action.size()-1 ? "":",");
  }
  fprintf(fp,"%s}\n\n",indent(1,tab_size).c_str());

  fprintf(fp,"%s// direction enum\n",indent(1,tab_size).c_str());
  fprintf(fp,"%senum Direction : int {\n",indent(1,tab_size).c_str());
  for(auto j = 0; j < index_to_direction.size(); j++) {
    fprintf(fp, "%s%s = %d%s\n",indent(2,tab_size).c_str(), enum_id(index_to_direction[j]).c_str(), j, j == index_to_direction.size()-1 ?  "":",");
  }
  fprintf(fp,"%s}\n\n",indent(1,tab_size).c_str());

  //now for actual sprite object! Be very careful to keep all the initializers in the right order!

  fprintf(fp,"%s// sprite initialization\n",indent(1,tab_size).c_str());

  fprintf(fp,"%sSFMLSprite %s_sprite {\n",indent(1,tab_size).c_str(), header_namespace.c_str());

  // SPRITE INFO INITIALIZER
  fprintf(fp,"%s//SFMLSpriteInfo info\n",indent(2,tab_size).c_str());
  fprintf(fp,"%s{\n",indent(2,tab_size).c_str());

  fprintf(fp,"%s//std::map<std::string,int> character_to_index\n",indent(3,tab_size).c_str());
  fprintf(fp,"%s{\n",indent(3,tab_size).c_str());
  int q = 0;
  for(auto kv : psfp->info.character_to_index) {
    fprintf(fp,"%s\"%s\" : %d%s\n",indent(4,tab_size).c_str(),kv.first.c_str(), kv.second,q == psfp->info.character_to_index.size()-1 ? "":",");
    q++;
  }
  fprintf(fp,"%s},\n",indent(3,tab_size).c_str());

  fprintf(fp,"%s//std::map<std::string,int> action_to_index\n",indent(3,tab_size).c_str());
  fprintf(fp,"%s{\n",indent(3,tab_size).c_str());
  q = 0;
  for(auto kv : psfp->info.action_to_index) {
    fprintf(fp,"%s\"%s\" : %d%s\n",indent(4,tab_size).c_str(),kv.first.c_str(), kv.second,q == psfp->info.action_to_index.size()-1 ? "":",");
    q++;
  }
  fprintf(fp,"%s},\n",indent(3,tab_size).c_str());

  fprintf(fp,"%s//std::map<std::string,int> direction_to_index\n",indent(3,tab_size).c_str());
  fprintf(fp,"%s{\n",indent(3,tab_size).c_str());
  q = 0;
  for(auto kv : psfp->info.direction_to_index) {
    fprintf(fp,"%s\"%s\" : %d%s\n",indent(4,tab_size).c_str(),kv.first.c_str(), kv.second,q == psfp->info.direction_to_index.size()-1 ? "":",");
    q++;
  }
  fprintf(fp,"%s}\n",indent(3,tab_size).c_str());       //note no comma here bc last

  fprintf(fp,"%s}\n\n",indent(2,tab_size).c_str());


  // THEN FRAME DATA INITIALIZER
  fprintf(fp,"%s//std::map<int, std::map<int, std::map<int, std::vector<SFMLSpriteFrame>>>> framedata\n",indent(2,tab_size).c_str());
  fprintf(fp,"%s{\n",indent(2,tab_size).c_str());

  //character level - all characters have something in them, presumably, so list all
  for(auto chr = 0; chr < index_to_character.size(); chr++) {
    fprintf(fp,"%s%s : {\n",indent(3,tab_size).c_str(), enum_id(index_to_character[chr]).c_str());

    auto charlev = (*frames)[index_to_character[chr]];

    //action level - ONLY ACTIONS THAT EXIST FOR THIS CHARACTER
    for(auto act = 0; act < index_to_action.size(); act++) {
      if(charlev.find(index_to_action[act]) != charlev.end()) {
        fprintf(fp,"%s%s : {\n",indent(4,tab_size).c_str(), enum_id(index_to_action[act]).c_str());

          //direction level ONLY DO THE ONES THAT ACTUALLY EXIST FOR THIS ACTION LIKE THERE IS NO A-DIRECTION FOR IDLE OR WALK!
          auto actlev = (charlev[index_to_action[act]]);
          for(auto dir = 0; dir < index_to_direction.size(); dir++) {
            if(actlev.find(index_to_direction[dir]) != actlev.end()) {
              fprintf(fp,"%s%s : {\n",indent(5,tab_size).c_str(), enum_id(index_to_direction[dir]).c_str());

              //frame level - all exist
              auto dirlev = actlev[index_to_direction[dir]];
              for(auto fr = 0; fr < dirlev.size(); fr++) {
                //figure out how to keep comments lined up or don't have them
                //be sure to get the initializing values in the right order!
                fprintf(fp,"%s{ %3d, %3d, %3d, %3d, %4.2f, %4.2f, %4d }%s // frame %d\n",indent(6,tab_size).c_str(), 
                  psfp->frames[chr][act][dir][fr].ulx,
                  psfp->frames[chr][act][dir][fr].uly,
                  psfp->frames[chr][act][dir][fr].wid,
                  psfp->frames[chr][act][dir][fr].ht,
                  psfp->frames[chr][act][dir][fr].offx,
                  psfp->frames[chr][act][dir][fr].offy,
                  psfp->frames[chr][act][dir][fr].dur,
                  fr == dirlev.size()-1 ? " ":",", fr);
              }

              fprintf(fp,"%s}%s\n",indent(5,tab_size).c_str(),dir == index_to_direction.size()-1 ?  "":",");
            }
          }

        fprintf(fp,"%s}%s\n",indent(4,tab_size).c_str(),act == index_to_action.size()-1 ?  "":",");
      }
    }

    fprintf(fp,"%s}%s\n",indent(3,tab_size).c_str(),chr == index_to_character.size()-1 ?  "":",");
  }
  fprintf(fp,"%s}\n\n",indent(2,tab_size).c_str());


  // AND THAT IS IT FOR THE INITIALIZER
  fprintf(fp,"%s} //SFMLSprite %s_sprite {\n",indent(1,tab_size).c_str(), header_namespace.c_str());


  //emit namespace and include guard enders and wrap up
  fprintf(fp,"} // namespace %s\n\n",header_namespace.c_str());
  fprintf(fp,"#endif // %s_INCLUDED\n",include_guard_var.c_str());
  fclose(fp);


  return true;
}

//takes an AseSprite pointer presumably created by asespritereader, converts it to platform-specific needs of sfml
std::shared_ptr<SFMLSprite> Aseprite2SFML::ConvertAsepriteSpriteToSFMLSprite(std::shared_ptr<AseSprite> psp, std::string spritesheet_dir, std::string header_out_dir, std::string sprite_out_dir) {

  printf("Converting to SFML sprite...\n");

  std::shared_ptr<SFMLSprite> spritely = std::shared_ptr<SFMLSprite>(new SFMLSprite);

  /* what we're getting as input
  class AseSprite {
    public:
      //has a sprite sheet
      std::shared_ptr<AseSpriteSheet> sheet;

      //has a collection of frames
      //SUPER GROSS. indexed by: 
      std::shared_ptr<std::map<std::string,                                             //character,
                        std::map<std::string,                                           //action,
                          std::map<std::string,                                         //direction,
                            std::vector<std::shared_ptr<AseSpriteFrame>>>>>> frames;    //frame number

      //might have a palette
      //multiple actors in the game could use the same Sprite - is this a good name for it?
      //so each of those would maintain its own timing counters, current action and direction
  };
  */

  // TO DO *************************************************************************************************************************************************
  // * For this platform, we don't need to do much with the sprite sheet - however, should crop off the origin dots and be sure it's in the format the game
  //   assumes. Other platforms that aren't going to scale and have a premium on space, e.g. Arduino, might tighten up imagery to exclude inter-frame transparency
  //   * for that, we can traverse the input AseSprite's frames and build a cropping box
  //   * that would need to take padding into account, yes? How do we know the pad size? It's recorded in each frame - a bit wasteful, :P
  // * Need to build a SFMLSpriteInfo and write it out as a header file to header_out_dir, should just be the declaration of a sprite variable
  //   with the initializations... 
  //   * should it be a subclass for that character? That makes some sense. 
  //     * That way could have enums for directions and actions and stuff...or would it be better if those were universals?
  //     * bull on with this and see what occurs. So far SFMLMap doesn't write anything out either and could do these in tandem
  //     * I could see the ardy version being like paletted RLE but that's for another class.

  //I love you, auto, for my not having to type out
  //std::shared_ptr<std::map<std::string,std::map<std::string,std::map<std::string,std::vector<std::shared_ptr<AseSpriteFrame>>>>>> frames
  auto frames = psp->frames;

  // bounding box of imagery (for cropping out the origin dots)
  int32_t bound_ulx = INT32_MAX;
  int32_t bound_uly = INT32_MAX;
  int32_t bound_lrx = -1;
  int32_t bound_lry = -1;

  //traverse and discover!
  //- bounding box around imagery within sprite sheet, leaving out origin dots
  //  assuming that the imagery in the sprite sheet is all used, we don't need to do an
  //  extract and "crunch"

  //traverse input frames, finding the bounding box of imagery on the sprite sheet
  int cur_char_ind;
  int cur_act_ind;
  int cur_dir_ind;

  for (auto ch_it = frames->begin(); ch_it != frames->end(); ch_it++) {
    //get character index if already exists, else create it
    if(spritely->info.character_to_index.find(ch_it->first) != spritely->info.character_to_index.end()) {
      cur_char_ind = spritely->info.character_to_index[ch_it->first];
    } else {
      cur_char_ind = spritely->info.character_to_index.size();     // - verify
      spritely->info.character_to_index[ch_it->first] = cur_char_ind;
    }
    //printf("- Character: %s index %d\n",ch_it->first.c_str(), cur_char_ind);

    for(auto act_it = ch_it->second.begin(); act_it != ch_it->second.end(); act_it++) {
      if(spritely->info.action_to_index.find(act_it->first) != spritely->info.action_to_index.end()) {
        cur_act_ind = spritely->info.action_to_index[act_it->first];
      } else {
        cur_act_ind = spritely->info.action_to_index.size();     // - verify
        spritely->info.action_to_index[act_it->first] = cur_act_ind;
      }
      //printf("  - Action: %s index %d\n",act_it->first.c_str(),cur_act_ind);

      for(auto dir_it = act_it->second.begin(); dir_it != act_it->second.end(); dir_it++) {
        if(spritely->info.direction_to_index.find(dir_it->first) != spritely->info.direction_to_index.end()) {
          cur_dir_ind = spritely->info.direction_to_index[dir_it->first];
        } else {
          cur_dir_ind = spritely->info.direction_to_index.size();     // - verify
          spritely->info.direction_to_index[dir_it->first] = cur_dir_ind;
        }
        //printf("    - Direction: %s index %d\n",dir_it->first.c_str(),cur_dir_ind);

        for(auto fnum = 0; fnum < dir_it->second.size(); fnum++) {
          //printf("      - Frame: %d\n",fnum);
          auto framely = dir_it->second[fnum];
          //figure out if it nudges the bounding box out. Don't forget to account for pad
          bound_ulx = std::min(bound_ulx, framely->ulx - framely->pad);
          bound_uly = std::min(bound_uly, framely->uly - framely->pad);
          bound_lrx = std::max(bound_lrx, framely->ulx + framely->wid + framely->pad);
          bound_lry = std::max(bound_lry, framely->uly + framely->ht + framely->pad);
        }
      }
    }
  }

  //traverse again, creating spriteframes
  //printf("Creating spriteframes...\n");
  // class SFMLSpriteFrame {
  //   public:
  //     //data members
  //     int ulx, uly;         //texture coordinates, pixels, within sprite sheet texture
  //     int wid, ht;          //extents of rectangle within sprite sheet texture
  //     float offx, offy;       //offsets to give to sprite.setOrigin(sf::Vector2f(offx, offy));
  // };

  //**********************************************************************************************
  //**********************************************************************************************
  //**********************************************************************************************
  // SO HERE SUBTRACT BBOX ULX OFF OF ALL THE FRAMES' SHEET COORDS - OR RATHER BE SURE TO DO THAT
  // FOR THE OUTPUT FORM!!!!!!!!!!!!!!!
  //**********************************************************************************************
  //**********************************************************************************************
  //**********************************************************************************************
  for (auto ch_it = frames->begin(); ch_it != frames->end(); ch_it++) {
    cur_char_ind = spritely->info.character_to_index[ch_it->first];
    //printf("- Character: %s index %d\n",ch_it->first.c_str(), cur_char_ind);

    for(auto act_it = ch_it->second.begin(); act_it != ch_it->second.end(); act_it++) {
      cur_act_ind = spritely->info.action_to_index[act_it->first];
      //printf("  - Action: %s index %d\n",act_it->first.c_str(),cur_act_ind);

      for(auto dir_it = act_it->second.begin(); dir_it != act_it->second.end(); dir_it++) {
        cur_dir_ind = spritely->info.direction_to_index[dir_it->first];
        //printf("    - Direction: %s index %d\n",dir_it->first.c_str(),cur_dir_ind);

        for(auto fnum = 0; fnum < dir_it->second.size(); fnum++) {
          //printf("      - Frame: %d\n",fnum);
          auto framely = dir_it->second[fnum];

          //HERE SUBTRACT BBOX ULX OFF OF ALL THE FRAMES' ORIGINAL SHEET COORDS
          //but wait, where does it go?
          spritely->frames[cur_char_ind][cur_act_ind][cur_dir_ind].push_back(
            SFMLSpriteFrame(
              framely->ulx - bound_ulx,
              framely->uly - bound_uly,
              framely->wid,
              framely->ht,
              -framely->off_x,    //I think these have the opposite sense of what the ripper does
              -framely->off_y,
              framely->dur
            )
          );
        }
      }
    }
  }



  //now do vectors to convert index to char/dir/act
  index_to_character.resize(spritely->info.character_to_index.size());
  for(auto kv : spritely->info.character_to_index) index_to_character[kv.second] = kv.first;

  index_to_action.resize(spritely->info.action_to_index.size());
  for(auto kv : spritely->info.action_to_index) index_to_action[kv.second] = kv.first;

  index_to_direction.resize(spritely->info.direction_to_index.size());
  for(auto kv : spritely->info.direction_to_index) index_to_direction[kv.second] = kv.first;

  //write header here! if bothering
  printf("Writing header...\n");
  writeHeader(psp, spritely, "creedle.h", header_out_dir);      //HEY HAVE A REAL FILENAME

  printf("- Bounding box of imagery within sprite sheet: [%d, %d - %d, %d]\n",
      bound_ulx, bound_uly, bound_lrx, bound_lry);

  //**********************************************************************************************
  //**********************************************************************************************
  //**********************************************************************************************
  // Write out texture for this sprite!
  //**********************************************************************************************
  //**********************************************************************************************
  //**********************************************************************************************
  //this should be pretty easy - load up the png, crop to bounding box, write out png, read in again as texture?
  //HOWEVER WE NEED THE RIGHT PATH TO THE ORIGINAL SHEET
  sf::Image origsheet;
  std::string origsheet_path = spritesheet_dir + "/" + psp->sheet->get_image_name();
  printf("- Loading original spritesheet image %s\n", origsheet_path.c_str());
  if(origsheet.loadFromFile(origsheet_path) == false) {
    printf("*** ERROR: failed to load original sprite sheet!\n");
    return nullptr;
  }

  //so now crop
  printf("- Cropping spritesheet image to [%d, %d - %d, %d]\n", bound_ulx, bound_uly, bound_lrx, bound_lry);
  sf::Image cropsheet;
  cropsheet.create((bound_lrx-bound_ulx), (bound_lry-bound_uly), sf::Color(0,255,0));
  //see if we need to add 1 to lrx/y - doesn't look like it!
  cropsheet.copy(origsheet,0,0,sf::IntRect(bound_ulx, bound_uly, bound_lrx, bound_lry), false);
  std::string outpng = "/home/sean/tmp/fiddle.png";
  printf("- saving png %s\n",outpng.c_str());
  cropsheet.saveToFile(outpng);  //PUT IN A REAL NAME
  printf("- creating texture\n");
  spritely->spritesheet.loadFromImage(cropsheet);
  
  return spritely;
}