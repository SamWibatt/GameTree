// NOT INTENDED TO BE INCLUDED DIRECTLY - JUST INCLUDE gametree.h

#ifndef GTREE_VIEW_H_INCLUDED
#define GTREE_VIEW_H_INCLUDED


namespace gt{

  // GTViewport has width / height in pixels, upper-left location in world coordinates,
  // associaton with a GTMap?
  // can scroll just by changing the upper left world coord location
  // but there are min/max scroll x/y s.t. the viewport window must always be inside them
  class GTViewport {
    public:
      GTcoord_t world_ulx;      //world coordinate at the upper left of this viewport
      GTcoord_t world_uly;
      GTcoord_t wid;
      GTcoord_t ht;

      GTcoord_t world_wid;      //width of entire map
      GTcoord_t world_ht;

      GTcoord_t world_min_x;    // ulx can't be less than this
      GTcoord_t world_min_y;    // mm
      GTcoord_t world_max_x;    // (ulx + wid) can't be more than this
      GTcoord_t world_max_y;    // mm

    public:
      GTViewport() { world_ulx = world_uly = wid = ht = world_wid = world_ht = world_min_x = world_min_y = 0; }

      //ctor makes no effort to ensure that the coordinates are legal
      // this version assumes world_max/min are entire world extent
      GTViewport(GTcoord_t wux, GTcoord_t wuy, GTcoord_t w, GTcoord_t h, GTcoord_t ww, GTcoord_t wh) {
        world_ulx = wux; 
        world_uly = wuy; 
        wid = w;
        ht = h;
        world_wid = ww;
        world_ht = wh;
        world_min_x = 0; 
        world_min_y = 0;
        world_max_x = world_wid;
        world_max_y = world_ht;
      }

      GTViewport(GTcoord_t wux, GTcoord_t wuy, GTcoord_t w, GTcoord_t h, 
          GTcoord_t ww, GTcoord_t wh, 
          GTcoord_t wminx, GTcoord_t wminy, GTcoord_t wmaxx, GTcoord_t wmaxy) {
        world_ulx = wux; 
        world_uly = wuy; 
        wid = w;
        ht = h;
        world_wid = ww;
        world_ht = wh;
        world_min_x = wminx; 
        world_min_y = wminy;
        world_max_x = wmaxx; 
        world_max_y = wmaxy;
      }

        
      virtual ~GTViewport() {}


    public:
      // set_world_pos sets the world position of this viewport's upper left, returns true if that puts the 
      // viewport within the world
      // refuses to set the location and returns false if not
      // does allow negatives because world coords can be negative if they want
      virtual bool set_world_pos(GTcoord_t x, GTcoord_t y) {
        if(x < world_min_x || y < world_min_y || 
            x > (world_max_x - wid) || y > (world_max_y - ht)) {
          return false;
        }
        world_ulx = x;
        world_uly = y;
        return true;
      }

      // set_view_size changes the viewport dimensions, leaving the world_ulx/y in the same spot
      // unless that would make the view go out of bounds, in which case everything is unchanged and it returns false
      virtual bool set_view_size(GTcoord_t nw, GTcoord_t nh) {
        if(world_ulx < world_min_x || world_uly < world_min_y || 
            world_ulx > (world_max_x - nw) || world_uly > (world_max_y - nh)) {
          return false;
        }
        wid = nw;
        ht = nh;
        return true;
      }

      virtual bool set_view_size_and_world_pos(GTcoord_t nw, GTcoord_t nh, GTcoord_t x, GTcoord_t y) {
        if(x < world_min_x || y < world_min_y || 
            x > (world_max_x - nw) || y > (world_max_x - nh)) {
          return false;
        }
        world_ulx = x;
        world_uly = y;
        wid = nw;
        ht = nh;
        return true;
      }

  };

  //GTScrollBoxViewport has a "scroll box" in it and a tracking point s.t. 
  // the scroll box is smaller than the viewport and if the tracking point would move outside it,
  // the viewport scrolls to keep the tracking point at the edge of the scroll box 
  // UNLESS the viewport has reached its scroll min/max and then the tracking point can wander out
  class GTScrollBoxViewport : public GTViewport {
    public:
      // tracking point current location
      GTcoord_t track_x;
      GTcoord_t track_y;

      // tracking point maximum / minimum world coordinates bc tracked object has some extent we don't want to go off the edge of the world
      GTcoord_t track_x_min;
      GTcoord_t track_x_max;
      GTcoord_t track_y_min;
      GTcoord_t track_y_max;

      // distance from sides of viewport to scroll box corresponding side, all assumed to be nonnegative
      GTcoord_t margin_left;
      GTcoord_t margin_right;
      GTcoord_t margin_top;
      GTcoord_t margin_bottom;

    public:
      //no attempt made to verify legality
      GTScrollBoxViewport(GTcoord_t wux, GTcoord_t wuy, GTcoord_t w, GTcoord_t h, 
                          GTcoord_t ww, GTcoord_t wh, 
                          GTcoord_t wminx, GTcoord_t wminy, GTcoord_t wmaxx, GTcoord_t wmaxy,
                          GTcoord_t tx, GTcoord_t ty, 
                          GTcoord_t tminx, GTcoord_t tmaxx, GTcoord_t tminy, GTcoord_t tmaxy,
                          GTcoord_t ml, GTcoord_t mr, GTcoord_t mt, GTcoord_t mb ) : 
                          GTViewport(wux, wuy, w, h, ww, wh, wminx, wminy, wmaxx, wmaxy) {
        track_x = tx;
        track_y = ty;
        track_x_min = tminx;
        track_x_max = tmaxx;
        track_y_min = tminy;
        track_y_max = tmaxy;
        margin_left = ml;
        margin_right = mr;
        margin_top = mt;
        margin_bottom = mb;
      }

      // assumes world min/max are world extents
      GTScrollBoxViewport(GTcoord_t wux, GTcoord_t wuy, GTcoord_t w, GTcoord_t h, 
                          GTcoord_t ww, GTcoord_t wh,
                          GTcoord_t tx, GTcoord_t ty, 
                          GTcoord_t tminx, GTcoord_t tmaxx, GTcoord_t tminy, GTcoord_t tmaxy,
                          GTcoord_t ml, GTcoord_t mr, GTcoord_t mt, GTcoord_t mb ) : 
                          GTViewport(wux, wuy, w, h, ww, wh) {
        track_x = tx;
        track_y = ty;
        track_x_min = tminx;
        track_x_max = tmaxx;
        track_y_min = tminy;
        track_y_max = tmaxy;
        margin_left = ml;
        margin_right = mr;
        margin_top = mt;
        margin_bottom = mb;
      }

      GTScrollBoxViewport() : GTViewport() {
        track_x = track_y = track_x_min = track_x_max = track_y_min = 
          track_y_max = margin_left = margin_right = margin_top = margin_bottom = 0;
      } 

      virtual ~GTScrollBoxViewport() {}

    public:
      // ugh, overrides to set_world_pos, set_view_size, set_view_size_and_world_pos need to be done and account for tracking point
      // might be as simple as calling base class and if that fails, fail bc the viewport can't go there...
      // but then set_tracking_point wrt the new viewport? hm, not quite.
      // tracking point stays the same, if the new scroll box contains it, yay! If not, decide if the viewport needs to scroll.
      // WRITE THOSE!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      // WRITE THOSE!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      // WRITE THOSE!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      // How important is that stuff anyway? Am I just getting ahead of myself wrt features? Yes. Keep what I've got, see if it's ever used
      // set_world_pos will be, by set_tracking point

      // set_tracking_point updates the tracking point and causes the viewport to scroll, if necessary.
      // - if the new tracking point is outside tracking min/max, it's clamped to be within
      // - 
      virtual bool set_tracking_point(GTcoord_t ntx, GTcoord_t nty) {
        //ok, corner cases:
        // - tracking point out of bounds (bounds are assumed to be within the world)
        // - or should we just clamp it? Let's clamp it
        GTcoord_t adj_ntx = ntx;
        GTcoord_t adj_nty = nty;

        if(ntx < track_x_min) adj_ntx = track_x_min; 
        if(ntx > track_x_max) adj_ntx = track_x_max;
        if(nty < track_y_min) adj_nty = track_y_min;
        if(nty > track_y_max) adj_nty = track_y_max;

        // if that means the tracking point hasn't moved, just return
        if(adj_ntx == track_x && adj_nty == track_y) return true;

        // it's legal, set it
        track_x = adj_ntx;
        track_y = adj_nty;

        // k so check to see if it's outside the scroll box. If it's inside, we're good!
        // for convenience, find world-adjusted positions of the box margins:
        GTcoord_t sb_wleft = world_ulx + margin_left;
        GTcoord_t sb_wright = world_ulx + wid - margin_right;    // all margins positive so subtract
        GTcoord_t sb_wtop = world_uly + margin_top;
        GTcoord_t sb_wbottom = world_uly + ht - margin_bottom;

        //horizontal scroll check 
        if(track_x < sb_wleft) {
          //track x is off the left side, scroll left as far as possible
          // the ideal amount to scroll is # pixels track_x is to the left of sb_wleft
          // clamp to world_min_x
          world_ulx -= (sb_wleft - track_x);
          if(world_ulx < world_min_x) world_ulx = world_min_x;
        } else if(track_x > sb_wright) {
          // track x off to right, scroll right as far as possible, <= # pix track_x is
          // off to the right. Clamp to world_max_x - wid
          world_ulx += (track_x - sb_wright);
          if(world_ulx > world_max_x - wid) world_ulx = world_max_x - wid;
        }

        //vertical scroll check 
        if(track_y < sb_wtop) {
          //track y is off the top, scroll up as far as possible
          // the ideal amount to scroll is # pixels track_y is above sb_wtop
          // clamp to world_min_y
          world_uly -= (sb_wtop - track_y);
          if(world_uly < world_min_y) world_uly = world_min_y;
        } else if(track_y > sb_wbottom) {
          // track y off to bottom, scroll down as far as possible, <= # pix track_y is
          // below. Clamp to world_max_y - ht
          world_uly += (track_y - sb_wbottom);
          if(world_uly > world_max_y - ht) world_uly = world_max_y - ht;
        }

        return true;
      }
  };

}

#endif