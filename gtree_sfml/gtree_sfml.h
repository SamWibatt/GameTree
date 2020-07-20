//sfmlmap is a tiled map background usable by SFML.
//closely aligned to the Tiled map editor https://www.mapeditor.org/

#ifndef GTREE_SFML_H_INCLUDED
#define GTREE_SFML_H_INCLUDED

#include "gametree.h"
#include <vector>
#include <SFML/Graphics.hpp>

using namespace gt;

namespace gtree_sfml {

  //SFMLSprite ==================================================================================
  class SFMLSprite : public GTSprite {
    public:
      //data members
      sf::Texture spritesheet;

    public:
      SFMLSprite(){}
      virtual ~SFMLSprite() {}

    public:
      //member functions
  };


  //SFMLMap =====================================================================================

  // Transformable vertex array for building scrolly / rotatey / etc tile maps
  class xfVertArray : public sf::Drawable, public sf::Transformable {
    public:
      sf::VertexArray va;

    public:
      xfVertArray(sf::PrimitiveType pty, size_t nPoints) {
        va = sf::VertexArray(pty, nPoints);
      }

      void append(sf::Vertex v) {
        va.append(v);
      }

      void draw(sf::RenderTarget &target, sf::RenderStates states) const {
        sf::RenderStates rs = states;
        rs.transform = states.transform * getTransform();
        target.draw(va, rs);
      }
  };


  class SFMLMap : public GTMap {
    public:
      //Textures from the map layers
      std::vector<sf::Texture> layer_tiletexs;

      // vertex arrays - not sure it will be organized like this
      std::vector<xfVertArray> layer_vert_arrays;

    public:
      SFMLMap(){}
      virtual ~SFMLMap() {}

    public:
      //member functions
      // - reading from json also does the step where it constructs vertex arrays and textures
      virtual bool get_from_json(json& jt) override;
  };
}

#endif