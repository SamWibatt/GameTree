//sfmlmap is a tiled map background usable by SFML.
//closely aligned to the Tiled map editor https://www.mapeditor.org/

#ifndef GTREE_SFML_H_INCLUDED
#define GTREE_SFML_H_INCLUDED

#include "gametree.h"
#include <vector>
#include <SFML/Graphics.hpp>

using namespace gt;
using namespace sf;

namespace gtree_sfml {


  //GTSFSpriteBank ==============================================================================

  class GTSFSpriteBank {
    public:
      //data members
      GTSpriteBank *sb;         //dumb pointer assumes GT version will outlive GTSF version - keep an eye on this
      sf::Texture spritesheet;

    public:
      GTSFSpriteBank(){
        sb = nullptr;
      }
      GTSFSpriteBank(GTSpriteBank *newsb) {
        if(newsb != nullptr) {
          sb = newsb;
          //ICKY SIDE EFFECT: turn the imagery into a Texture and discard it
          spritesheet.loadFromMemory(sb->image_data.data(), sb->image_data.size());
          sb->image_data.clear();
        }
      }
      virtual ~GTSFSpriteBank() {}

    public:
      //member functions
  };

  //GTSFActor ===================================================================================

  // assume a typical actor is a sprite - can use other drawables to derive from otherwise
  // OR because Sprite's draw is private, make this Transformable and Drawable and have a Sprite member :|
  class GTSFActor : public Transformable, public Drawable {
    public:
      GTActor *ac;
      std::shared_ptr<GTSFSpriteBank> sfsb;
      std::shared_ptr<Sprite> spr;

    public:
      //if you just assign sprite bank and try to set the texture anywhere else, it gets all object-sliced :P
      virtual void set_sprite_bank(std::shared_ptr<GTSFSpriteBank> sb) {
        this->sfsb = sb;
        spr = std::shared_ptr<sf::Sprite>(new Sprite());
        spr->setTexture(sfsb->spritesheet);
      };

      GTSFActor() { 
        spr = nullptr;
        sfsb = nullptr;
        ac = nullptr; 
      }

      GTSFActor(GTActor *nac) {
        std::shared_ptr<GTSFSpriteBank> nsfsb = std::shared_ptr<GTSFSpriteBank>(new GTSFSpriteBank(nac->sbank.get()));       //gross side effect clears samurai_sbank's image data
        ac = nac;
        set_sprite_bank(nsfsb);
      }

      GTSFActor(GTActor *nac, std::shared_ptr<GTSFSpriteBank> nsfsb) {
        ac = nac;
        set_sprite_bank(nsfsb);
      }

      virtual ~GTSFActor() {}

    public:
      virtual void draw(sf::RenderTarget &target, sf::RenderStates states) const {
        if(sfsb == nullptr) return;

        //but I think I can just set the texture rectangle appropriately and draw it?
        GTSpriteFrame  *frm = ac->get_current_spriteframe();
        if(frm == nullptr) return;

        //set texture rectangle and hotspot offset; position is set in our transform, yes?
        // drat, gets object-sliced but shouldn't do here anyway
        //spr->setTexture(sbank->spritesheet);
        spr->setTextureRect(sf::IntRect(frm->ulx,frm->uly,frm->wid,frm->ht));
        spr->setOrigin(frm->offx, frm->offy);

        // for Invisible Samurai bug this is showing getTexture() as 0 - and aha, that is actually shewing a pointer, so it's nullptr by here
        // printf("spr texture: %lu tr: (%d,%d - w%d,h%d) org: %f,%f\n",spr->getTexture(),
        //   spr->getTextureRect().left, spr->getTextureRect().top, spr->getTextureRect().width, spr->getTextureRect().height,
        //   spr->getOrigin().x, spr->getOrigin().y);

        // Do I need to compose states's transform with anything? If I do:
        sf::RenderStates rs = states;
        rs.transform = states.transform * getTransform();
        target.draw(*spr,rs);
      }
  };

  // HEREAFTER UNREFACTORED *******************************************************************************************************************************************
  // HEREAFTER UNREFACTORED *******************************************************************************************************************************************
  // HEREAFTER UNREFACTORED *******************************************************************************************************************************************
  // HEREAFTER UNREFACTORED *******************************************************************************************************************************************
  // HEREAFTER UNREFACTORED *******************************************************************************************************************************************
  // HEREAFTER UNREFACTORED *******************************************************************************************************************************************
  // HEREAFTER UNREFACTORED *******************************************************************************************************************************************

  //GTSFMap =====================================================================================

  //base class so I can store these things in a vector
  class GTSFShape : public sf::Drawable, public sf::Transformable {
    public:
      GTSFShape() {}
      virtual ~GTSFShape() {}

      // for those classes that are wrappers around sfml shapes, return shape pointer, eww, shudder
      // this way you can do stuff like set position
      virtual sf::Shape *get_shape() = 0;
  };

  //wrapper for SFML rectangle :P 
  class GTSFRectangle : public GTSFShape {
    public:
      std::shared_ptr<sf::RectangleShape> r;

    public:
      GTSFRectangle() {}
      GTSFRectangle(std::shared_ptr<sf::RectangleShape>& rec) { 
        r = rec; 
      }
      virtual ~GTSFRectangle() {}
      virtual sf::Shape *get_shape() override { return r.get(); }

      virtual void draw(sf::RenderTarget &target, sf::RenderStates states) const override {
        // I don't think I need to compose states's transform with anything? no, I think I do
        if(r != nullptr) {
          sf::RenderStates rs = states;
          rs.transform = states.transform * getTransform();
          target.draw(*(r.get()), rs);
        }
      }
  };

  //wrapper for SFML circle :P 
  class GTSFCircle : public GTSFShape {
    public:
      std::shared_ptr<sf::CircleShape> c;

    public:
      GTSFCircle() {}
      GTSFCircle(std::shared_ptr<sf::CircleShape>& circ) { 
        c = circ; 
      }
      virtual ~GTSFCircle() {}
      virtual sf::Shape *get_shape() override { return c.get(); }

      virtual void draw(sf::RenderTarget &target, sf::RenderStates states) const override {
        // I don't think I need to compose states's transform with anything? no, I think I do
        if(c != nullptr) {
          sf::RenderStates rs = states;
          rs.transform = states.transform * getTransform();
          target.draw(*(c.get()), rs);
        }
      }
  };


  // Transformable vertex array for building scrolly / rotatey / etc tile maps
  class GTSFVertArray : public GTSFShape {
    public:
      sf::VertexArray va;
      sf::Texture *tex; 

    public:
      GTSFVertArray(sf::PrimitiveType pty, size_t nPoints, sf::Texture *tx) {
        va = sf::VertexArray(pty, nPoints);
        tex = tx;
      }

      void append(sf::Vertex v) {
        va.append(v);
      }

      void set_vertex(int index, sf::Vertex v) {
        if(index < 0 || index >= va.getVertexCount()) return;     //illegal index, do nothing
        va[index] = v;
      }

      void draw(sf::RenderTarget &target, sf::RenderStates states) const override {
        sf::RenderStates rs = states;
        rs.transform = states.transform * getTransform();
        if(tex != nullptr) {
          rs.texture = tex;
        } 
        target.draw(va, rs);
      }

      //nastiness - this class isn't a sf::Shape wrapper, so can't do a get_shape
      virtual sf::Shape *get_shape() override { return nullptr; }
  };

  class GTSFMapLayer : public sf::Drawable, public sf::Transformable {
    public:
      std::shared_ptr<sf::Texture> layer_tex;                       //tilesheet, if any, nullptr if not
      std::shared_ptr<std::vector<GTSFVertArray>> layer_vertarrays;   //vertex arrays, if any,  nullptr if not
      // box containing the min/max in each dimension
      sf::Rect<int> bounding_box;

    public:
      GTSFMapLayer() {}
      virtual ~GTSFMapLayer() {}

    public:
      virtual bool build_tile_object_vertarrays(std::vector<std::shared_ptr<GTObjectTile>>& tile_objects, 
                std::vector<GTTile>& tile_atlas, sf::Texture *tx);

      virtual void draw(sf::RenderTarget &target, sf::RenderStates states) const {
        // I don't think I need to compose states's transform with anything? no, I think I do
        if(layer_vertarrays != nullptr) {
          sf::RenderStates rs = states;
          rs.transform = states.transform * getTransform();
          for(auto j = 0; j < layer_vertarrays->size(); j++) {
            (*layer_vertarrays)[j].draw(target,rs);
          }
        }
      }

      //call calculate_bounding_box after getting from json or other source in order to re-reckon bounding box
      virtual void calculate_bounding_box() = 0;
      virtual sf::Rect<int> get_bounding_box() { return bounding_box; };

  };

  // ok, these are kind of gross, inheriting them from the gametree versions so the parent map can
  // allocate them and handle them properly
  class GTSFTiledMapLayer : public GTTiledMapLayer, public GTSFMapLayer {
    public:

    public:
      virtual bool get_from_json(json& jt) override;

    public:
      bool build_tile_map_vertarrays();
      void calculate_bounding_box() override;
  };

  class GTSFObjectsMapLayer : public GTObjectsMapLayer, public GTSFMapLayer {

    public:
      virtual bool get_from_json(json& jt) override;
      void calculate_bounding_box() override;
  };


  class GTSFMap : public GTMap {
    public:
      //data members
      std::vector<std::shared_ptr<GTSFMapLayer>> slayers;   //...not happy with this but layers are getting sliced
      // and similar anti-slicer
      std::shared_ptr<GTSFMapLayer> get_sfml_layer_by_name(std::string nm) {
        int j = 0;
        for(auto plyr : layers) {
          if(plyr->name == nm) return slayers[j];       //dumb search but we're not likely to have lots of these
          j++;
        }
        return nullptr; //didn't find it
      }


    public:
      GTSFMap(){}
      virtual ~GTSFMap() {}

    public:
      //member functions
      // - reading from json also does the step where it constructs vertex arrays and textures
      //   handled by the layers' get_from_jsons
      virtual bool get_from_json(json& jt) override;
  };
}

#endif