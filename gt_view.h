// NOT INTENDED TO BE INCLUDED DIRECTLY - JUST INCLUDE gametree.h

#ifndef GTREE_VIEW_H_INCLUDED
#define GTREE_VIEW_H_INCLUDED


namespace gt{

  // GTViewport has width / height in pixels, upper-left location in world coordinates,
  // associaton with a GTMap?
  // can scroll just by changing the upper left world coord location
  // but there are min/max scroll x/y s.t. the viewport window must always be inside them
  class GTViewport {

  };

  //GTScrollBoxViewport has a "scroll box" in it and a tracking point s.t. 
  // the scroll box is smaller than the viewport and if the tracking point would move outside it,
  // the viewport scrolls to keep the tracking point at the edge of the scroll box 
  // UNLESS the viewport has reached its scroll min/max and then the tracking point can wander out
  class GTScrollBoxViewport : public GTViewport {

  };

}

#endif