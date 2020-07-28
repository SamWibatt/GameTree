#include "gtree_sfml.h"

using namespace gt;

namespace gtree_sfml {

  //GTSFMapLayer ================================================================================

  bool GTSFMapLayer::build_drawables(GTMapLayer *lyr) {

    //image data is a temporary in-memory png. create layer_tex from that.
    // only exists if there is a tile map and/or a tile objects 
    if(!lyr->tile_map.empty() || !lyr->tile_objects.empty()) {
      if(layer_tex == nullptr) {
        layer_tex = std::shared_ptr<sf::Texture>(new sf::Texture());
        layer_tex->loadFromMemory(lyr->image_data.data(), lyr->image_data.size());
      }

      // DANGER: THIS DESTRUCTIVELY MODIFIES IMAGE_DATA (GETS RID OF IT IN FAVOR OF THE TEXTURE!!!!!!!!!!!!!!!!!!!!!!!!!)
      // likewise tile_objects below
      lyr->image_data.clear();
    }

    //first tiled map, if any
    layer_vertarrays = std::shared_ptr<std::vector<GTSFVertArray>>(new std::vector<GTSFVertArray>());
    if(!lyr->tile_map.empty()) {
      GTSFVertArray xva(nullptr, sf::PrimitiveType::Quads,0,layer_tex.get());
      for(auto i = 0; i < lyr->tile_map.size(); i++) {
        if(lyr->tile_map[i] != 0) {
          GTindex_t tcol = i % lyr->layer_tilewid;
          GTindex_t trow = i / lyr->layer_tilewid;

          GTcoord_t x = tcol * lyr->tile_pixwid;
          GTcoord_t y = trow * lyr->tile_pixht;
          
          //ADD TO VERTEX ARRAY
          // add all 4 points for this in the order ulx, uly; ulx, lry; lrx, rly, ulx, lry
          // remember all tiles in the tilemap are the tile_pixwd x tile_pixht size;
          // any oddsize are put in the tile_objects list
          xva.append(sf::Vertex(sf::Vector2f(x,y),sf::Vector2f(lyr->tile_atlas[lyr->tile_map[i]].ulx,
              lyr->tile_atlas[lyr->tile_map[i]].uly)));
          xva.append(sf::Vertex(sf::Vector2f(x + lyr->tile_pixwid, y),
              sf::Vector2f(lyr->tile_atlas[lyr->tile_map[i]].ulx + lyr->tile_pixwid, lyr->tile_atlas[lyr->tile_map[i]].uly)));
          xva.append(sf::Vertex(sf::Vector2f(x + lyr->tile_pixwid, y + lyr->tile_pixht),
              sf::Vector2f(lyr->tile_atlas[lyr->tile_map[i]].ulx + lyr->tile_pixwid, 
              lyr->tile_atlas[lyr->tile_map[i]].uly + lyr->tile_pixht)));
          xva.append(sf::Vertex(sf::Vector2f(x, y + lyr->tile_pixht),
              sf::Vector2f(lyr->tile_atlas[lyr->tile_map[i]].ulx, lyr->tile_atlas[lyr->tile_map[i]].uly + lyr->tile_pixht)));

        } 
        // else it's a blank, skip
        
      }
      layer_vertarrays->push_back(xva);
    }

    // iterate over tile_objects and build a little quadlike thing for each. 
    if(!lyr->tile_objects.empty()) {

      if(layer_vertarrays == nullptr || layer_vertarrays->empty())
        layer_vertarrays = std::shared_ptr<std::vector<GTSFVertArray>>(new std::vector<GTSFVertArray>());
      for(auto tob : lyr->tile_objects) {
        //here's what we have to work with
        //tob->orx, tob->ory, tob->tile, tob->wid, tob->ht
        //tile_atlas[tob->tile].ulx, tile_atlas[tob->tile].uly, tile_atlas[tob->tile].wid, tile_atlas[tob->tile].ht
        // the atlas is coords into the texture sheet image
        // texture verts similar but tile_atlas[tob->tile].wid/ht instead, yes? 
        // do a quad, clockwise so 0,0 - w, 0, - w, h - 0, h
        // should I be doing a separate vertex array for each?
        // Let's go with that; likely the tile map will emit subsets too, in a refactor
        // wait, need to give the actual position for each quad, not relative to a "position"
        // looks like Tiled positions object tiles by lower left, tho - so try bumping y up by object's height - YAY

        GTSFVertArray xva(nullptr, sf::PrimitiveType::Quads,4,layer_tex.get());
        xva.va[0] = (sf::Vertex(sf::Vector2f(tob->orx,tob->ory - tob->ht),
            sf::Vector2f(lyr->tile_atlas[tob->tile].ulx, lyr->tile_atlas[tob->tile].uly)));
        //printf("Appending vertex: pos %f, %f tex %f, %f\n", xva.va[0].position.x,xva.va[0].position.y,xva.va[0].texCoords.x,xva.va[0].texCoords.y);
        xva.va[1] = (sf::Vertex(sf::Vector2f(tob->orx + tob->wid, tob->ory - tob->ht),
            sf::Vector2f(lyr->tile_atlas[tob->tile].ulx + lyr->tile_atlas[tob->tile].wid, 
              lyr->tile_atlas[tob->tile].uly)));
        //printf("Appending vertex: pos %f, %f tex %f, %f\n", xva.va[1].position.x,xva.va[1].position.y,xva.va[1].texCoords.x,xva.va[1].texCoords.y);
        xva.va[2] = (sf::Vertex(sf::Vector2f(tob->orx + tob->wid, tob->ory),
            sf::Vector2f(lyr->tile_atlas[tob->tile].ulx + lyr->tile_atlas[tob->tile].wid, 
            lyr->tile_atlas[tob->tile].uly + lyr->tile_atlas[tob->tile].ht)));
        //printf("Appending vertex: pos %f, %f tex %f, %f\n", xva.va[2].position.x,xva.va[2].position.y,xva.va[2].texCoords.x,xva.va[2].texCoords.y);
        xva.va[3] = (sf::Vertex(sf::Vector2f(tob->orx, tob->ory),
            sf::Vector2f(lyr->tile_atlas[tob->tile].ulx, lyr->tile_atlas[tob->tile].uly + lyr->tile_atlas[tob->tile].ht)));
        //printf("Appending vertex: pos %f, %f tex %f, %f\n", xva.va[3].position.x,xva.va[3].position.y,xva.va[3].texCoords.x,xva.va[3].texCoords.y);
        layer_vertarrays->push_back(xva);
      }

      //should we discard tile_objects like we did with png imagery? yeah let's do
      lyr->tile_objects.clear();
    }

    // shapes!
    layer_shapes.clear();
    if(!lyr->shapes.empty()) {
      for(auto shap: lyr->shapes) {
          //get geometry from the shape - bounding box, list of points, etc.
          std::shared_ptr<std::vector<GTPoint>> geo = shap->get_geometry();
          std::shared_ptr<GTSFShape> nu_sfmlshape = nullptr;
          
          if(shap->get_shape_type() == GTST_Rectangle) {
              auto nu_rect = std::shared_ptr<sf::RectangleShape>(
                  new sf::RectangleShape(
                      // SHOULD THIS ALLOW FOR AN OFFSET OF SOME KIND? I guess only screen position needs to, qv.
                      sf::Vector2f((*geo)[1].x - (*geo)[0].x, (*geo)[1].y - (*geo)[0].y)
                  )
              );
              nu_rect->setFillColor(get_color_for_purpose(shap->purpose, 64));       //low-opacity fill
              nu_rect->setOutlineColor(get_color_for_purpose(shap->purpose, 255));   //opaque outline
              nu_rect->setOutlineThickness(-1.0);    //thin outline, inside shape
              //this is screen position! nu_shape->setPosition(shap->position.x, shap->position.y);
              nu_sfmlshape = std::shared_ptr<GTSFRectangle>(new GTSFRectangle(shap.get(), nu_rect));
          } else if(shap->get_shape_type() == GTST_Ellipse) {
              //make a circle s.t. its scale is 1 in the major axis and
              //minor/major in the minor axis
              float major_axis = std::max((*geo)[1].x,(*geo)[1].y);
              auto nu_circ = std::shared_ptr<sf::CircleShape>(new sf::CircleShape(major_axis/2.0));
              if((*geo)[1].x > (*geo)[1].y) {
                  // x larger, so set Y scale to y/x
                  nu_circ->scale(1.0, float((*geo)[1].y) / major_axis);
              } else {
                  // y larger, so set X scale to x/y
                  nu_circ->scale(float((*geo)[1].x) / major_axis,1.0);
              }
              nu_circ->setFillColor(get_color_for_purpose(shap->purpose, 64));       //low-opacity fill
              nu_circ->setOutlineColor(get_color_for_purpose(shap->purpose, 255));   //opaque outline
              nu_circ->setOutlineThickness(-1.0);    //thin outline, inside shape
              //this is screen position! nu_shape->setPosition(shap->position.x, shap->position.y);
              nu_sfmlshape = std::shared_ptr<GTSFCircle>(new GTSFCircle(shap.get(), nu_circ));
          } else if(shap->get_shape_type() == GTST_Polygon) {
              //argh, how to do this? I guess just connected lines atm
              //I don't want to do the whole triangulating thing again
              //sf::PrimitiveType pty, size_t nPoints, sf::Texture *tx
              auto nu_poly = std::shared_ptr<GTSFVertArray>(new GTSFVertArray(shap.get(), 
                  sf::PrimitiveType::LineStrip,geo->size()+1,nullptr));
              //fill in all the vertices from geometry, with color determined by shape's purpose
              sf::Color col = get_color_for_purpose(shap->purpose,255);       //opaque outline
              for(int i = 0; i < geo->size(); i++) {
                  //let's just put all the points in verbatim
                  nu_poly->set_vertex(i,sf::Vertex(sf::Vector2f((*geo)[i].x, (*geo)[i].y),col));
              }
              // one last vertex to close up the polyline
              nu_poly->set_vertex(geo->size(),sf::Vertex(sf::Vector2f((*geo)[0].x, (*geo)[0].y),col));
              nu_sfmlshape = nu_poly;
          } else if(shap->get_shape_type() == GTST_Point) {
              printf("*** WARNING: Point not yet supported, skipping\n");
              nu_sfmlshape = nullptr;
          } else {
              printf("*** WARNING: unknown shape type, skipping\n");
              nu_sfmlshape = nullptr;
          }

          //add  to scene graph
          if(nu_sfmlshape != nullptr) {
              layer_shapes.push_back(nu_sfmlshape);           // do we even need this? Yes, for adjusting positions onscreen
          }
      }

    }



    return true;
  }
  
}