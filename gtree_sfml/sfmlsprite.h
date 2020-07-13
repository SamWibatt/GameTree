#ifndef SFMLSPRITE_H_INCLUDED
#define SFMLSPRITE_H_INCLUDED

#include "gametree.h"
#include <SFML/Graphics.hpp>

using namespace gt;

namespace gtree_sfml {

  // SpriteFrame and SpriteFrameInfo are non-sfml-dependent, let's move them to gametree

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
};


#endif

