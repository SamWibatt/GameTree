#ifndef GTREE_MAP_H_INCLUDED
#define GTREE_MAP_H_INCLUDED

namespace gt {

  // MAP =========================================================================================
  // Will be organized similarly to a Tiled map, bc why not, but streamlined to contain only
  // what's necessary at runtime.

  // GTTile is the texture vertex info for a tile in a given tile sheet
  class GTTile {
    public:
      GTcoord_t ulx;
      GTcoord_t uly; 
      GTcoord_t wid;
      GTcoord_t ht;

    public:
      GTTile() {}
      GTTile(GTcoord_t ux, GTcoord_t uy, GTcoord_t w, GTcoord_t h) {
        ulx = ux;
        uly = uy;
        wid = w;
        ht = h;
      }
      virtual ~GTTile() {}

      // for writing to json file - should this be here?
      // assuming that e.g. j[layer_atlas_name] is a json and we hand in that
      public:
        virtual bool add_to_json(json& j) {
          j.push_back(std::map<std::string, GTcoord_t>{
            {"ulx", ulx},
            {"uly", uly},
            {"wid", wid},
            {"ht", ht}
          });
          return true;
        }

        // assuming that e.g. j[layer_atlas_name][tile index]
        // is usable as a json itself, and that's jt
        // FIND OUT IF THIS IS A GOOD WAY TO DO THIS 
        virtual bool get_from_json(json& jt) {
          if(jt.count("ulx") == 0 || jt.count("uly") == 0 ||
              jt.count("wid") == 0 || jt.count("ht") == 0) {
            //error! missing a field
            fprintf(stderr,"*** ERROR: tile needs ulx, uly, wid, ht, got %s\n",jt.dump().c_str());
            return false;
          }
          ulx = jt["ulx"];
          uly = jt["uly"];
          wid = jt["wid"];
          ht = jt["ht"];
          return true;
        }
  };

  //base class for shaped regions on map, for things like terrain type, triggers, etc.
  //strings are fairly expensive, 32 bytes on entrapta (intel core i5/64 bit linux)
  //16 bytes for a smart pointer, which makes sense - pointer + ref count + maybe other little stuff
  //would be nice to build app-specific lookup table for names, then could use an index value
  //how many of these do we want to keep around?
  typedef enum : GTindex_t {
    GTAT_Unknown = 0,
    GTAT_NoGo,
    GTAT_Slow,
    GTAT_Trigger
  } GTArea_type;

  typedef enum : GTindex_t {
    GTST_Unknown = 0,
    GTST_Rectangle,
    GTST_Ellipse,
    GTST_Polygon,
    GTST_Point
  } GTShape_type;


  class GTShape {
    public:
      // data members
      GTArea_type purpose;
      GTindex_t name_index;     // indexes into a list of strings; probably not have lots of same names but few shapes will have names and this is smaller than a string
      GTPoint position;         // we may want to move these around
      GTPoint bbox_ul;          // bounding box upper left, relative to position
      GTPoint bbox_lr;          // bounding box lower right, ""

    public:
      GTShape() {
        purpose = GTAT_Unknown;
        name_index = -1;
        position = {0,0};
        bbox_ul = {0,0};
        bbox_lr = {-1,-1};
      }
      GTShape(GTArea_type pur, GTPoint pos, GTPoint bul, GTPoint blr, GTindex_t ni) {
        purpose = pur;
        position = pos;
        bbox_ul = bul;
        bbox_lr = blr;
        name_index = ni;
      }
      virtual ~GTShape() {}

    public:
      //member functions
      // say bbox_check is end-exclusive in both dimensions?
      inline bool bbox_check(GTPoint& pt) {
        return(pt.x >= bbox_ul.x + position.x && pt.x < bbox_lr.x + position.x && 
                pt.y >= bbox_ul.y + position.y && pt.y < bbox_lr.y + position.y);
      }
      virtual bool inside_shape_if_inside_bbox(GTPoint& pt) = 0;      //each shape must define; doesn't do bbox check, inside() does
      bool inside(GTPoint& pt) {
        return bbox_check(pt) && inside_shape_if_inside_bbox(pt);
      }
      bool inside(GTcoord_t x, GTcoord_t y) {
        GTPoint pt(x,y);
        return inside(pt);
      }

      virtual GTShape_type get_shape_type() = 0;
      virtual std::shared_ptr<std::vector<GTPoint>> get_geometry() = 0;

      virtual bool add_to_json(json& j);
      virtual bool get_from_json(json& jt);
  };

  class GTRectangle : public GTShape {
    public:
      //no data members other than base class

    public:
      GTRectangle() {}
      GTRectangle(GTArea_type pur, GTPoint pos, GTPoint bul, GTPoint blr, GTindex_t ni) : GTShape(pur, pos, bul, blr, ni) {}
      virtual ~GTRectangle() {}

    public:
      //trivial: if pt is inside bbox, it's inside the rectangle.
      virtual bool inside_shape_if_inside_bbox(GTPoint& pt) override { return true; }
      virtual bool add_to_json(json& j) override;
      virtual bool get_from_json(json& jt) override;

      virtual GTShape_type get_shape_type() override { return GTST_Rectangle; }
      virtual std::shared_ptr<std::vector<GTPoint>> get_geometry() override {
        auto vec = std::shared_ptr<std::vector<GTPoint>>(new std::vector<GTPoint>(2));
        (*vec)[0] = bbox_ul;
        (*vec)[1] = bbox_lr;
        return vec;
      }
  };

  class GTEllipse : public GTShape {
    public:
      //no data members other than base class

    public:
      GTEllipse() {}
      GTEllipse(GTArea_type pur, GTPoint pos, GTPoint bul, GTPoint blr, GTindex_t ni) : GTShape(pur, pos, bul, blr, ni) {}
      virtual ~GTEllipse() {}

    public:
      //remember to allow for "position"
      virtual bool inside_shape_if_inside_bbox(GTPoint& pt) override;
      virtual bool add_to_json(json& j) override;
      virtual bool get_from_json(json& jt) override;

      virtual GTShape_type get_shape_type() override { return GTST_Ellipse; }
      virtual std::shared_ptr<std::vector<GTPoint>> get_geometry() override {
        auto vec = std::shared_ptr<std::vector<GTPoint>>(new std::vector<GTPoint>(2));
        (*vec)[0] = bbox_ul;
        (*vec)[1] = bbox_lr;
        return vec;
      }
  };

  //assumed to be a simple polygon! I think it's ok if there isn't a duplicate first point after last
  class GTPolygon : public GTShape {
    public:
      std::vector<GTPoint> points;

    public:
      GTPolygon() {}
      GTPolygon(GTArea_type pur, GTPoint pos, GTPoint bul, GTPoint blr, GTindex_t ni) : GTShape(pur, pos, bul, blr, ni) {}
      virtual ~GTPolygon() {}

    public:
      //remember to allow for "position"
      // HERE USE BOURKE'S ROUTINE AND CREDIT IT
      virtual bool inside_shape_if_inside_bbox(GTPoint& pt) override;
      virtual bool add_to_json(json& j) override;
      virtual bool get_from_json(json& jt) override;

      virtual GTShape_type get_shape_type() override { return GTST_Polygon; }
      virtual std::shared_ptr<std::vector<GTPoint>> get_geometry() override {
        auto vec = std::shared_ptr<std::vector<GTPoint>>(new std::vector<GTPoint>(points.size()));
        std::copy(points.begin(), points.end(), vec->begin());
        return vec;
      }
  };




  //**********************************************************************************************
  //**********************************************************************************************
  //**********************************************************************************************
  // ANIMATED TILE SUBCLASS OF THAT WHICH EVENTS CAN ADVANCE
  // Multiple inheritance: can make it so GTTile is not an Entity but GTAnimatedTile is both
  // that and an event entity
  // not sure this is the level at which tile animation will happen, but.
  //**********************************************************************************************
  //**********************************************************************************************
  //**********************************************************************************************
  class GTEventEntity;
  
  class GTAnimatedTile: public GTTile, public GTEventEntity {
    public:
      GTAnimatedTile() {}
      virtual ~GTAnimatedTile() {}
  };


  //location / extent of a free-floating tile as opposed to TiledMapLayer
  //grid-determined location
  //It's not an event entity but subclasses might be, consider
  class GTObjectTile {
    public:
      GTtile_index_t tile;       //index into tile_atlas
      GTcoord_t orx;        //origin x
      GTcoord_t ory;        //origin y
      GTcoord_t wid;        //width 
      GTcoord_t ht;         //height

    public:
      GTObjectTile(); 
      GTObjectTile(GTtile_index_t t, GTcoord_t ox, GTcoord_t oy, GTcoord_t w, GTcoord_t h);
      virtual ~GTObjectTile(); 

    // json i/o
    public:
      virtual bool add_to_json(json& j);
      virtual bool get_from_json(json& jt);
  };

  //base class for layers - assuming that events will occur on them
  class GTMapLayer : public GTEventEntity {
    public:
      //data members
      std::string name;       //we'll want a way to choose these by name e.g. for interleaving sprites in the scene graph
      //single source of tile imagery - 
      //bytes that are just a png file in memory. May later have other formats
      std::vector<uint8_t> image_data;
      //tile atlas - texture coordinates within image for tile n
      //tile 0 is always the no-tile, but put an entry in here for it
      //to keep the map indexing simple
      std::vector<GTTile> tile_atlas;

      GTBBox bounding_box;

      // merging tiled and objects layers so both have all the stuff
      GTcoord_t layer_tilewid;   //layer width in tiles
      GTcoord_t layer_tileht;    //height
      GTcoord_t tile_pixwid;     //tile grid width in pixels
      GTcoord_t tile_pixht;      //height
      
      //tilemap entries are indices into tile_atlas; 0 means no tile in
      //that grid location
      std::vector<GTtile_index_t> tile_map;
      //tile_objects are tiles not aligned to grid but drawing from the tilesheet texture
      std::vector<std::shared_ptr<GTObjectTile>> tile_objects;
      //shapes are (typically not drawn) areas e.g. rectangle, ellipse, polygon used for triggering, no-go, etc.
      std::vector<std::shared_ptr<GTShape>> shapes;


    public:
      void init() {
        name.clear();
        image_data.clear();
        tile_atlas.clear();
        //add a dummy tile for tile 0, which should never get rendered
        tile_atlas.push_back(GTTile(0,0,0,0));
        bounding_box.ulx = bounding_box.uly = bounding_box.wid = bounding_box.ht = 0;
      }
      GTMapLayer() {
        init();
      }
      virtual ~GTMapLayer() {}

      //call calculate_bounding_box after getting from json or other source in order to re-reckon bounding box
      virtual void calculate_bounding_box();
      virtual GTBBox get_bounding_box() { return bounding_box; };

      virtual bool add_to_json(json& basej);
      virtual bool get_from_json(json& jt);
  };

  class GTMap : public GTEventEntity {
    public:
      // Tiled map has only lists of tilesets and layers.
      // do we need the notion of a tileset?
      // each layer has a tile sheet and atlas, yes?
      std::vector<std::shared_ptr<GTMapLayer>> layers;
      std::shared_ptr<GTMapLayer> get_layer_by_name(std::string nm) {
        for(auto plyr : layers) {
          //printf("Checking \"%s\" against layer name \"%s\"\n",nm.c_str(),plyr->name.c_str());
          if(plyr->name == nm) return plyr;       //dumb search but we're not likely to have lots of these
        }
        return nullptr; //didn't find it
      }

    public:
      GTMap();
      virtual ~GTMap();

    public:
      virtual bool add_to_json(json& j);
      virtual bool get_from_json(json& jt);

      // this worked for SFMLMap too :P
      virtual bool load_from_json_file(std::string map_filename) {
        std::ifstream ifs(map_filename);
        if(ifs.fail()) {
            printf("*** ERROR: couldn't open %s\n",map_filename.c_str());
            return false;
        }
        json jmap;
        ifs >> jmap;
        ifs.close();

        if(!get_from_json(jmap)) {
            printf("*** ERROR: failed to load SFMLMap from json file \"%s\"\n",map_filename.c_str());
            return false;
        }
        return true;
      }
  };

}

#endif
