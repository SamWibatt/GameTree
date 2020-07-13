//sfmlmap is a tiled map background usable by SFML.
//closely aligned to the Tiled map editor https://www.mapeditor.org/

#ifndef SFMLMAP_H_INCLUDED
#define SFMLMAP_H_INCLUDED

#include <vector>
#include <SFML/Graphics.hpp>

namespace sfml_assets {

  class SFMLMap {
    public:
      //data members
      //what do we need?
      //single bank of tile data, let's say
      //or should these be Textures? Or one big image?
      //I think the way to render is a vertex array https://www.sfml-dev.org/tutorials/2.5/graphics-vertex-array.php
      //esp see https://www.sfml-dev.org/tutorials/2.5/graphics-vertex-array.php#example-tile-map
      //but I don't think we draw the whole thing if it's representing a giant map
      //my guess is we just make one slightly larger than the screen and shift all the indices when scrolled all the way to one edge...?
      //is that necessary? Does it just clip?
      //alternately, a bunch of screen-sized sub-maps, and just draw a few at a time, I think it would always be 1-4 of them
      //1 if it's perfectly centered in xy, 2 if it's centered in x or y and not in the other, 4 if it's off center in both
      //also for packing stuff into non-uniform single textures, the crunch program Clarissa pointed out at 
      //https://github.com/ChevyRay/crunch could work! Different style of atlas from Kenney but I could fiddle it up
      //it's now a subdirectory of hello project and should be easy to incorporate in other stuff - put everything but its main in a library
      //std::vector<sf::Image> tiles;
      //vertexarray of sf::LineStrip does look like the way to draw the polygon objects
      //let's start with one texture per layer
      std::vector<sf::Texture> layer_tiletexs;

      //need some kind of atlases for those - should there be a tileset class with an atlas and a texture?
      //or do we? Would it work just to have a vertex array for each submap? Atlases more a sprites thing?
      //animated tiles present a problem there but could just do minor surgery on submaps to make it happen
      //how do we know which submap corresponds to what area? 
      //let's just start with one vertex array per layer. 
      // HEY LET'S DO TRIANGLE STRIPS
      // EACH SHOULD HAVE A BOUNDING BOX, ACCOUNTING FOR UNUSUALLY SIZED TILES
      std::vector<sf::VertexArray> layer_vert_arrays;

    public:
      SFMLMap(){}
      virtual ~SFMLMap() {}

    public:
      //member functions
  };
}

#endif