// reader for JSON sprite metadata from aseprite
// assumes sheet-type export from command line, split by tags into different sprite actions / directions
// e.g.
// aseprite -b -v --trim --inner-padding 1 --sheet PrincessAll.png --sheet-columns 4 --filename-format 'c{title}*a{outertag}*d{innertag}*f{tagframe00}*p{layer}' --format json-hash --data PrincessAll.json --split-layers PrincessAll.aseprite
// note "filename-format" is for the name of each frame, not the sheet filename
// "title" is the sheet filename minus the extension; using the * as delimiter so that it's unlikely to show up in filenames.
// sheet-columns to taste, if left off, you get one long row of images
// so here is a requirement that an action have an outer tag around its directions: the form is
// c<character/item name>*a<action>*d<direction>*f<frame #>*p<purpose>
// direction can be: U for up, D for down, L for left, R for right, H for horizontal (left and right), V for vertical (up and down)
// or whatever you want. if there is no direction (d<tag> is the same as a<tag>?) it's for all directions - this is bc that's what you
// get if the outer and inner tags cover the same range of frames anyway :P FIGURE OUT SOME OTHER WAY TO DO THAT
// p<purpose> is the function of the layer - so far, "Imagery" or "Origin". Imagery is the normal sprite images. Each frame in Origin should have only one pixel occupied,
// at the point relative to which the sprite should be drawn in-game. When sprites are trimmed, this makes registration possible. Origin frames shouldn't
// be included in actual game asset builds or drawn in game unless they're used for debugging.
// if they're not trimmed, ripper will have to go find the one pixel in the transparent frame - trim is to be preferred.
// --inner-padding 1 puts a border of transparency around each image in the spritesheet; it's possible when certain scaling or image processing algorithms operate
// on subsets of the sprite sheet, they drag in some fraction of the pixels surrounding the image, so you want a bit of harmless margin.

#ifndef ASESPRITEREADER_H_INCLUDED
#define ASESPRITEREADER_H_INCLUDED

#include <memory>
#include <vector>
//set up include dirs to have json/single_include
#include "nlohmann/json.hpp"

using json = nlohmann::json;

namespace asepreader {

  //represents an image from which sprite frames are pulled
  //also contains metadata re: image dimensions, format, scale
  class AseSpriteSheet {
    private:
      int image_wid;
      int image_ht;
      std::string image_name;

    public:
      int get_image_wid() { return image_wid; }
      void set_image_wid(int iw) { image_wid = iw; }
      int get_image_ht() { return image_ht; }
      void set_image_ht(int ih) { image_ht = ih; }
      std::string get_image_name() { return image_name; }
      void set_image_name(std::string iname) { image_name = iname; }
  };

  class AseSpriteFrame {
    //represents one rectangular subset of the sprite sheet
    //rectangle extent + hot spot + duration
    public:
      int ulx, uly, wid, ht;      // corner and extent in sprite sheet
      int off_x, off_y;           // offset from hotspot to upper left at render time - add these to game x/y
      int dur;                    // duration in milliseconds
      int pad;                  //width of transparent pad around imagery, pixels, in original sheet - better kept in frame, but easier to find here

    public:
      AseSpriteFrame() {}
      AseSpriteFrame(std::shared_ptr<AseSpriteFrame> other) {
        ulx = other->ulx;
        uly = other->uly;
        wid = other->wid;
        ht = other->ht;
        off_x = other->off_x;
        off_y = other->off_y;
        dur = other->dur;
        pad = other->pad;
      }
      ~AseSpriteFrame() {}
  }; 

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

  //here are things that read them from asesprite

  //analogous to TiledMapReader
  class AseSpriteReader {
    public:
      AseSpriteReader();
      ~AseSpriteReader(); 

    public:
      std::shared_ptr<AseSpriteSheet> handle_sprite_meta(json::iterator meta_it);
      //helper to parse frame names, broken out mainly for unit testing at some point
      std::tuple<std::string, std::string, std::string, int, std::string> parse_frame_name(std::string frname, char delim);
      std::string make_frame_name(std::string char_key, std::string act_key, std::string dir_key, int frame_num,
                                  std::string purp_key, char delim);
      std::shared_ptr<AseSpriteFrame> build_frame(json::iterator img_it, json::iterator org_it);
      std::shared_ptr<std::map<std::string,std::map<std::string,std::map<std::string,std::vector<std::shared_ptr<AseSpriteFrame>>>>>> handle_sprite_frames(json::iterator frames_it);
      std::shared_ptr<AseSprite> read_sprite_file(std::string filename);
  };

}

#endif
