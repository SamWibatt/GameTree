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
      sf::Texture *tex; 

    public:
      xfVertArray(sf::PrimitiveType pty, size_t nPoints, sf::Texture *tx) {
        va = sf::VertexArray(pty, nPoints);
        tex = tx;
      }

      void append(sf::Vertex v) {
        va.append(v);
      }

      void draw(sf::RenderTarget &target, sf::RenderStates states) const {
        sf::RenderStates rs = states;
        rs.transform = states.transform * getTransform();
        if(tex != nullptr) {
          rs.texture = tex;
        } 
        target.draw(va, rs);
      }
  };

  class SFMLMapLayer {
    public:
      std::shared_ptr<sf::Texture> layer_tex;                       //tilesheet, if any, nullptr if not
      std::shared_ptr<std::vector<xfVertArray>> layer_vertarrays;   //vertex arrays, if any,  nullptr if not

    public:
      SFMLMapLayer() {}
      virtual ~SFMLMapLayer() {}

    public:
      bool build_tile_object_vertarrays(std::vector<std::shared_ptr<GTObjectTile>>& tile_objects, 
                std::vector<GTTile>& tile_atlas, sf::Texture *tx);
  };

  // ok, these are kind of gross, inheriting them from the gametree versions so the parent map can
  // allocate them and handle them properly
  class SFMLTiledMapLayer : public GTTiledMapLayer, public SFMLMapLayer {
    public:

    public:
      virtual bool get_from_json(json& jt) override;

    public:
      bool build_tile_map_vertarrays();
  };

  class SFMLObjectsMapLayer : public GTObjectsMapLayer, public SFMLMapLayer {

    public:
      virtual bool get_from_json(json& jt) override;
  };


  class SFMLMap : public GTMap {
    public:
      //data members
      std::vector<std::shared_ptr<SFMLMapLayer>> slayers;   //...not happy with this but layers are getting sliced

    public:
      SFMLMap(){}
      virtual ~SFMLMap() {}

    public:
      //member functions
      // - reading from json also does the step where it constructs vertex arrays and textures
      //   handled by the layers' get_from_jsons
      virtual bool get_from_json(json& jt) override;
  };
}

#endif