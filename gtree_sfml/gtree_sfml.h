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

        // Do I need to compose states's transform with anything? If I do:
        sf::RenderStates rs = states;
        rs.transform = states.transform * getTransform();
        target.draw(*spr,rs);
      }
  };

  //GTSFMapLayer ================================================================================

  //base class so I can store these things in a vector
  class GTSFShape : public sf::Drawable, public sf::Transformable {
    public:
      GTShape *sh;

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
      //GTSFRectangle() {}
      GTSFRectangle(GTShape *s, std::shared_ptr<sf::RectangleShape>& rec) { 
        sh = s;
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
      GTSFCircle(GTShape *s, std::shared_ptr<sf::CircleShape>& circ) { 
        sh = s;
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
      GTSFVertArray(GTShape *s, sf::PrimitiveType pty, size_t nPoints, sf::Texture *tx) {
        sh = s;
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
      GTMapLayer *ly;             //dumb pointer assumes GT version will outlive GTSF version - keep an eye on this
      std::shared_ptr<sf::Texture> layer_tex;                       //tilesheet, if any, nullptr if not
      std::shared_ptr<std::vector<GTSFVertArray>> layer_vertarrays;   //vertex arrays, if any,  nullptr if not
      std::vector<std::shared_ptr<GTSFShape>> layer_shapes;

    public:
      virtual bool build_drawables(GTMapLayer *lyr);

      //GTSFMapLayer() { ly = nullptr; }

      GTSFMapLayer(GTMapLayer *nlyr) {
        ly = nlyr;
        build_drawables(ly);
      }

      virtual ~GTSFMapLayer() {}

    public:
      // colors for filling or outlining collision shapes
      sf::Color get_color_for_purpose(GTArea_type t, sf::Uint8 alpha) {
          switch(t) {
              case GTAT_NoGo: return sf::Color(255,0,0,alpha);      //red for nogo
              case GTAT_Slow: return sf::Color(0,0,255,alpha);      //blue for slow
              case GTAT_Trigger: return sf::Color(0,255,0,alpha);   //green for trigger
              default: return sf::Color(128,128,128,alpha);         //grey for unk
          }
      }

      virtual void draw(sf::RenderTarget &target, sf::RenderStates states) const {
        // I don't think I need to compose states's transform with anything? no, I think I do
        if(layer_vertarrays != nullptr) {
          sf::RenderStates rs = states;
          rs.transform = states.transform * getTransform();
          for(auto j = 0; j < layer_vertarrays->size(); j++) {
            (*layer_vertarrays)[j].draw(target,rs);
          }
        }
        if(!layer_shapes.empty()) {
          sf::RenderStates rs = states;
          rs.transform = states.transform * getTransform();
          for(auto j = 0; j < layer_shapes.size(); j++) {
            target.draw(*(layer_shapes[j].get()),rs);
          }
        }
      }


  };
}

#endif