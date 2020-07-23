#include "gametree.h"
#include "asepritereader.h"
#include "CLI11.hpp"
// png handling from lodepng so no requirement of libpng
#define LODEPNG_NO_COMPILE_CPP
#include "lodepng.h"

using namespace gt;
using namespace asepreader;

std::shared_ptr<GTSpriteBank> aseprite_to_gt_sprite(std::shared_ptr<AseSprite> psp, std::string input_dir, std::string output_dir) {

  std::shared_ptr<GTSpriteBank> spritely = std::shared_ptr<GTSpriteBank>(new GTSpriteBank());

  //aseprite2sfml has much of what I need
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
    printf("- Character: %s index %d\n",ch_it->first.c_str(), cur_char_ind);

    for(auto act_it = ch_it->second.begin(); act_it != ch_it->second.end(); act_it++) {
      if(spritely->info.action_to_index.find(act_it->first) != spritely->info.action_to_index.end()) {
        cur_act_ind = spritely->info.action_to_index[act_it->first];
      } else {
        cur_act_ind = spritely->info.action_to_index.size();     // - verify
        spritely->info.action_to_index[act_it->first] = cur_act_ind;
      }
      printf("  - Action: %s index %d\n",act_it->first.c_str(),cur_act_ind);

      for(auto dir_it = act_it->second.begin(); dir_it != act_it->second.end(); dir_it++) {
        if(spritely->info.direction_to_index.find(dir_it->first) != spritely->info.direction_to_index.end()) {
          cur_dir_ind = spritely->info.direction_to_index[dir_it->first];
        } else {
          cur_dir_ind = spritely->info.direction_to_index.size();     // - verify
          spritely->info.direction_to_index[dir_it->first] = cur_dir_ind;
        }
        printf("    - Direction: %s index %d\n",dir_it->first.c_str(),cur_dir_ind);

        for(auto fnum = 0; fnum < dir_it->second.size(); fnum++) {
          printf("      - Frame: %d\n",fnum);
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
  printf("Creating spriteframes...\n");
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
    printf("- Character: %s index %d\n",ch_it->first.c_str(), cur_char_ind);
    if(spritely->frames.size() <= cur_char_ind) 
      spritely->frames.resize(cur_char_ind + 1);

    for(auto act_it = ch_it->second.begin(); act_it != ch_it->second.end(); act_it++) {
      cur_act_ind = spritely->info.action_to_index[act_it->first];
      printf("  - Action: %s index %d\n",act_it->first.c_str(),cur_act_ind);
      if(spritely->frames[cur_char_ind].size() <= cur_act_ind) 
        spritely->frames[cur_char_ind].resize(cur_act_ind + 1);

      for(auto dir_it = act_it->second.begin(); dir_it != act_it->second.end(); dir_it++) {
        cur_dir_ind = spritely->info.direction_to_index[dir_it->first];
        printf("    - Direction: %s index %d\n",dir_it->first.c_str(),cur_dir_ind);
        if(spritely->frames[cur_char_ind][cur_act_ind].size() <= cur_dir_ind) 
          spritely->frames[cur_char_ind][cur_act_ind].resize(cur_dir_ind + 1);

        for(auto fnum = 0; fnum < dir_it->second.size(); fnum++) {
          printf("      - Frame: %d\n",fnum);
          auto framely = dir_it->second[fnum];

          //HERE SUBTRACT BBOX ULX OFF OF ALL THE FRAMES' ORIGINAL SHEET COORDS
          //but wait, where does it go?
          printf("        - about to push\n");

          // THIS IS DUMPING CORE

          spritely->frames[cur_char_ind][cur_act_ind][cur_dir_ind].push_back(
            GTSpriteFrame(
              framely->ulx - bound_ulx,
              framely->uly - bound_uly,
              framely->wid,
              framely->ht,
              -framely->off_x,    //I think these have the opposite sense of what the ripper does
              -framely->off_y,
              framely->dur
            )
          );
          printf("        - finished push\n");
        }
      }
    }
  }

  //now do vectors to convert index to char/dir/act
  spritely->info.index_to_character.resize(spritely->info.character_to_index.size());
  for(auto kv : spritely->info.character_to_index) spritely->info.index_to_character[kv.second] = kv.first;

  spritely->info.index_to_action.resize(spritely->info.action_to_index.size());
  for(auto kv : spritely->info.action_to_index) spritely->info.index_to_action[kv.second] = kv.first;

  spritely->info.index_to_direction.resize(spritely->info.direction_to_index.size());
  for(auto kv : spritely->info.direction_to_index) spritely->info.index_to_direction[kv.second] = kv.first;

  //write header here! if bothering
  // printf("Writing header...\n");
  // writeHeader(psp, spritely, "creedle.h", header_out_dir);      //HEY HAVE A REAL FILENAME

  printf("- Bounding box of imagery within sprite sheet: [%d, %d - %d, %d]\n",
      bound_ulx, bound_uly, bound_lrx, bound_lry);

  //**********************************************************************************************
  //**********************************************************************************************
  //**********************************************************************************************
  // Create texture for this sprite! Which we will encode to base64_url on json write
  // let's try this lodepng thing that was in crunch's bitmap.cpp - code nicked from there, qv.
  //**********************************************************************************************
  //**********************************************************************************************
  //**********************************************************************************************
  //Load the png file
  std::string pngpath = input_dir + psp->sheet->get_image_name();
  printf("- Reading png: %s\n",pngpath.c_str());
  unsigned char* pdata;
  unsigned int pw, ph;
  if (lodepng_decode32_file(&pdata, &pw, &ph, pngpath.c_str()))       //this does close its file, down in its guts
  {
      printf("*** ERROR: failed to load png file %s\n",pngpath.c_str());
      return nullptr;
  }
  int w = static_cast<int>(pw);
  int h = static_cast<int>(ph);
  uint32_t* pixels = reinterpret_cast<uint32_t*>(pdata);      //convenient, each pixel is a uint32

  // DO STUFF! Crop equivalent to this: 
  // cropsheet.create((bound_lrx-bound_ulx), (bound_lry-bound_uly), sf::Color(0,255,0));
  // //see if we need to add 1 to lrx/y - doesn't look like it!
  // cropsheet.copy(origsheet,0,0,sf::IntRect(bound_ulx, bound_uly, bound_lrx, bound_lry), false);
  // lodepng doesn't seem to have any cropping stuff, so just do it by hand
  int cropwid = (bound_lrx-bound_ulx);
  int cropht = (bound_lry-bound_uly);
  printf("- Cropping image to %d x %d\n",cropwid,cropht);
  uint32_t* crop_pixels = new uint32_t[cropwid * cropht];
  int croppix_index = 0;
  for(int y = bound_uly; y < bound_lry; y++) {
    for(int x = bound_ulx; x < bound_lrx; x++) {
      crop_pixels[croppix_index++] = pixels[(y * cropwid) + x];
    }
  }

  //Free the untrimmed pixels
  free(pixels);

  // so now encode the crop_pixels 
  unsigned char *crop_pixels_u8 = reinterpret_cast<unsigned char *>(crop_pixels);

  /*Same as lodepng_encode_memory, but always encodes from 32-bit RGBA raw image.*/
  //unsigned lodepng_encode32(unsigned char** out, size_t* outsize,
  //                        const unsigned char* image, unsigned w, unsigned h);
  // I think it has the png header on it - the functions that save pngs appear to just write this buffer
  unsigned char *crop_png_data;
  size_t out_size;      //size of output data

  printf("- Encoding cropped image to png format\n");
  if(lodepng_encode32(&crop_png_data,&out_size,crop_pixels_u8,cropwid,cropht)) {
      printf("*** ERROR: failed to compress cropped spritesheet\n");
      return nullptr;
  }

  printf("- worky! final size %lu bytes\n",out_size);

  // make the cropped png data into a base64_url string
  // or no wait that happens in writing to json, let's just copy it to the gt sprite image_data
  spritely->image_data.resize(out_size);
  for(int b = 0; b < out_size; b++) spritely->image_data[b] = crop_png_data[b];

  // free the trimmed pixels
  delete[] crop_pixels;

  // free the pngified data
  free(crop_png_data);

  // and that should be it

  //quick test: write out the png data - yay worky!
  // std::string temp_png = pngpath + ".crop.png";
  // printf("- writing debug png %s\n",temp_png.c_str());
  // FILE *fp = fopen(temp_png.c_str(),"wb");
  // fwrite(spritely->image_data.data(), 1, spritely->image_data.size(), fp);
  // fclose(fp);
  
  return spritely;
}

int main(int argc, char *argv[]) {
  CLI::App app{"Hello and welcome to Aseprite2GT, the asymptotic Aseprite to Gametree conversion utility."};

  std::string aseprite_input_file;
  std::string output_dir;
  bool emit_header = false;
  std::string header_output_file;
  bool verify_json = false;

  app.add_option("-i,--input", aseprite_input_file, "Aseprite file (.json)")
    ->required()
    ->check(CLI::ExistingFile);

  app.add_option("-o,--outputdir", output_dir, "Output directory for file(s), must already exist")
    ->required()
    ->check(CLI::ExistingPath);

  app.add_option("-c,--cheader", header_output_file, "Optional: emit c++ header of metadata to header. Filename only");


  CLI11_PARSE(app, argc, argv);

  //digest arguments

  // if a header was requested, note that we will emit one
  if(!header_output_file.empty()) emit_header = true;

  // make sure output dir has a slash on the end
  // MAKE THIS PLATFORM INDEPENDENT
  if(output_dir[output_dir.length()-1] != '/') {
    output_dir += "/";
  }

  // create output directory if it doesn't exist - LATER - compiler is barfing at this saying filesystem isn't a namespace,
  // even after #include<filesystem>, so I have a bad setting somewhere, I think
  // drat, can't get this to work
  // std::error_code err;
  // std::filesystem::create_directories(output_dir, err);
  // printf("Error was %s\n",err.message().c_str());

  AseSpriteReader ar;

  printf("About to read Aseprite file %s...\n",aseprite_input_file.c_str());

  // MAKE THIS NON_UNIX_CHAUVINIST
  auto pos=aseprite_input_file.find_last_of('/');
  std::string input_dir = aseprite_input_file.substr(0,pos);
  if(input_dir.empty()) input_dir = ".";
  input_dir += "/";

  std::shared_ptr<AseSprite> pts = ar.read_sprite_file(aseprite_input_file);

  printf("About to convert to GTSprite...\n");

  std::shared_ptr<GTSpriteBank> pgts = aseprite_to_gt_sprite(pts, input_dir, output_dir);

  //now write it out!
  // MAKE THIS NON-UNIX-CHAUVINIST
  pos=aseprite_input_file.find_last_of('/');
  std::string spritename = aseprite_input_file.substr(pos+1,aseprite_input_file.size()-1);

  std::string json_filename = output_dir + spritename + "_a2gt_out.json";
  printf("Writing json file %s\n",json_filename.c_str());
  json jsprit;
  pgts->add_to_json(jsprit);
  std::ofstream json_outstream(json_filename);
  //setw(4) does 4 space pretty print
  json_outstream << std::setw(4) << jsprit << std::endl;
  //see what it's like without - single giant line
  //json_outstream << jmap << std::endl;

  if(emit_header) {
    // EMIT HEADER!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  }


  //TEST: put a flag on this or just comment out 
  // read the json back in 
  if(verify_json) {
    printf("VERIFYING: reading json file back in\n");
    GTSpriteBank resprite;
    std::ifstream ifs(json_filename);
    json jresprite;
    ifs >> jresprite;
    
    if(resprite.get_from_json(jresprite) == false) {
      fprintf(stderr,"FAILED to read json file back in\n");
      exit(1);
    }

    std::string json_filename2 = json_filename + "2.json";
    printf("--- successfully read! writing to %s\n",json_filename2.c_str());
    json outjresprite;
    resprite.add_to_json(outjresprite);
    std::ofstream json_outstream2(json_filename2);
    json_outstream2 << std::setw(4) << outjresprite << std::endl;
    printf("*** go diff the 2 json files and make sure they're the same\n");
  }


}