#include <cstdio>
#include <SFML/Graphics.hpp>
#include "gametree.h"
#include <algorithm>        //for transform


// temp!
// #include <cerrno>
// #include <system_error>
// #include <cstring>
// #include "cppcodec/base64_url.hpp"
// using base64 = cppcodec::base64_url;
// end temp!


typedef struct {
   int32_t x,y;
} GTPoint;

//adapted from Paul Bourke, http://paulbourke.net/geometry/polygonmesh/
//Copyright notice on home page http://www.paulbourke.net/ reads
//"Any source code found here may be freely used provided credits are given to the author."
bool InsidePolygon(std::vector<GTPoint>& polygon, GTPoint p)
{
  int32_t counter = 0;
  int32_t i;
  int32_t xinters;      //should be float - originally was, see if works w/int - so far so good!
  GTPoint p1,p2;

  p1 = polygon[0];
  for (i=1;i<=polygon.size();i++) {
    p2 = polygon[i % polygon.size()];
    if (p.y > std::min(p1.y,p2.y)) {
      if (p.y <= std::max(p1.y,p2.y)) {
        if (p.x <= std::max(p1.x,p2.x)) {
          if (p1.y != p2.y) {
            xinters = (p.y-p1.y)*(p2.x-p1.x)/(p2.y-p1.y)+p1.x;
            if (p1.x == p2.x || p.x <= xinters)
              counter++;
          }
        }
      }
    }
    p1 = p2;
  }

  if (counter % 2 == 0)
    return(false);
  else
    return(true);
}

// bounding box check
int bbox_check(sf::Vector2f& the_point, sf::Rect<int>& the_bbox) {
    if(the_point.x < the_bbox.left || the_point.y < the_bbox.top) return 0;
    if(the_point.x > the_bbox.left + the_bbox.width || the_point.y > the_bbox.top + the_bbox.height) return 0;
    return 1;
}


int main()
{
    // //quick test of base64 stuff - let's do a little pnglet
    // std::string png_path = "/home/sean/dev/GameNoodles/sfml/hello/assets/8BIT_CanariPack_TopDown/SPRITES/ITEMS/item8BIT_heart.png";
    // std::vector<uint8_t> image_data;
    // printf("About to read png %s\n",png_path.c_str());
    // FILE * pngfp = std::fopen(png_path.c_str(), "rb");

    // if(pngfp == nullptr) {
    //     fprintf(stderr,"*** ERROR: failed to open png file %s - error %s\n",png_path.c_str(),std::strerror(errno));
    //     exit(1);
    // }
    // //get file size and allocate buffer for it
    // std::fseek(pngfp,0,SEEK_END);
    // auto pngsiz = std::ftell(pngfp);
    // std::fseek(pngfp,0,SEEK_SET);
    // printf("png size = %ld\n",pngsiz);
    // image_data.resize(pngsiz);    //see if this works; I think we want resize and not reserve
    // std::fread(image_data.data(), sizeof(uint8_t), image_data.size(), pngfp);
    // fclose(pngfp);

    // printf("Converting png to base64\n");
    // std::string encoded_png = base64::encode(image_data);

    // printf("Encoded png is: ------------------------\n%s\n-------------------------\n",encoded_png.c_str());

    // printf("Converting base64 back to png\n");
    // std::vector<uint8_t> decoded_png = base64::decode(encoded_png);

    // printf("Loading texture from decoded png\n");
    // sf::Texture tex;
    // tex.loadFromMemory(decoded_png.data(),decoded_png.size());

    // printf("texture size: %d x %d\n",tex.getSize().x, tex.getSize().y);
    
    // sf::Sprite hortsprit;
    // hortsprit.setTexture(tex,true);
    // hortsprit.setPosition(sf::Vector2f(10.0f, 10.0f));
    // hortsprit.setScale(5.0,5.0);

    // // WORKY!!!!!!!!!!!!!!!!!!!!

    // // end base64 test =================================================================================

    // this will get replaced with some utility sfml plat-spec that sees which modes are available and finds the most 
    // suitable one (and makes sure it can work - recall how enabling vsync on entrapta broke all the 4:3 modes)
    // it will also choose scale factor
    // look into https://www.sfml-dev.org/tutorials/2.5/graphics-transform.php#object-hierarchies-scene-graph
    // for how to have a global scale factor; that's for a bit later
    //should be sf::VideoMode screenMode = sf::VideoMode(720, 400);
    sf::VideoMode screenMode = sf::VideoMode(600, 600);

    sf::Transform globalTransform = sf::Transform();
    float globalScaleX = 2.0;
    float globalScaleY = 2.0;
    globalTransform.scale(sf::Vector2f(globalScaleX,globalScaleY));
    bool fullscreen = false;         // can derive from whether any valid mode was found, etc.

    //CREATE WINDOW
    sf::RenderWindow window(screenMode, "Hello, GameTree!",fullscreen ? sf::Style::Fullscreen : sf::Style::Default);

    //how do you make vsync actually work? see https://www.maketecheasier.com/get-rid-screen-tearing-linux/ - yup, like that, the intel
    //section worked on entrapta
    // disabling for point-in-poly test
    //window.setVerticalSyncEnabled(true); // call it once, after creating the window

    // KLUDGY WAY TO DO A SCENE GRAPH
    std::vector<std::shared_ptr<sf::Drawable>> scene_objects;

    // set up some stuff to draw
    // std::shared_ptr<sf::CircleShape> circy = std::shared_ptr<sf::CircleShape>(new sf::CircleShape(50.f));
    // circy->setFillColor(sf::Color(100, 250, 50));
    // circy->setOrigin(0,0);
    // scene_objects.push_back(circy);

    // - Found a polygon of type nogo, name "", id 1!
    // min_x 0.333330 min_y 1.000000 max_x 115.166328 max_y 161.082993
    // // origin 0.333330 1.000000 bounding box w 114.833000 h 160.082993
    // std::vector<std::vector<float>> polyverts = {
    // { 0.000000, 26.000000 }, 
    // { 7.666670, 33.333328 }, 
    // { 7.666670, 39.333298 }, 
    // { 20.333300, 45.333298 }, 
    // { 21.333300, 64.333298 }, 
    // { 54.750000, 97.416702 }, 
    // { 54.541698, 114.916702 }, 
    // { 63.333302, 124.791702 }, 
    // { 80.708298, 125.583298 }, 
    // { 114.833000, 160.082993 }, 
    // { 113.875000, 88.625000 }, 
    // { 92.000000, 89.000000 }, 
    // { 88.000000, 79.666702 }, 
    // { 88.666702, 65.666702 }, 
    // { 57.666698, 37.000000 }, 
    // { 56.666698, 20.666670 }, 
    // { 37.666698, 0.333300 }, 
    // { 0.333333, 0.000000 }, 
    // };

    // - Found a polygon of type nogo, name "", id 2!
    // min_x 125.000000 min_y 76.000000 max_x 269.333008 max_y 304.666687
    // // origin 125.000000 76.000000 bounding box w 144.333008 h 228.666687
    // std::vector<std::vector<float>> polyverts = {
    // { 0.000000, 11.666702 }, 
    // { 0.000000, 81.000000 }, 
    // { 19.666702, 80.000000 }, 
    // { 34.666702, 99.000000 }, 
    // { 57.666702, 98.666702 }, 
    // { 57.333298, 123.666702 }, 
    // { 42.333298, 132.333710 }, 
    // { 40.333298, 140.999695 }, 
    // { 28.333298, 148.999695 }, 
    // { 23.333298, 156.666702 }, 
    // { 9.000000, 166.666702 }, 
    // { 9.000000, 198.333710 }, 
    // { 35.666702, 228.333710 }, 
    // { 129.000000, 228.666687 }, 
    // { 125.666992, 208.666687 }, 
    // { 144.333008, 201.333710 }, 
    // { 140.333008, 172.666702 }, 
    // { 97.666702, 172.666702 }, 
    // { 93.000000, 147.999695 }, 
    // { 123.666992, 117.999695 }, 
    // { 121.666992, 69.333405 }, 
    // { 105.666992, 59.333405 }, 
    // { 107.000000, 35.333405 }, 
    // { 65.000000, 0.000000 }, 
    // { 29.000000, 0.000000 }, 
    // { 24.333298, 15.333374 }, 
    // };

    // - Found a polygon of type nogo, name "", id 3!
    // min_x 194.625000 min_y 351.443176 max_x 315.000000 max_y 512.750000
    // // origin 194.625000 351.443176 bounding box w 120.375000 h 161.306824
    std::vector<std::vector<float>> polyverts = {
    { 15.284088, 0.000000 }, 
    { 105.647705, 0.102264 }, 
    { 105.829498, 23.284119 }, 
    { 120.375000, 32.556824 }, 
    { 118.375000, 57.056824 }, 
    { 105.375000, 62.556824 }, 
    { 106.375000, 81.556824 }, 
    { 78.875000, 110.556824 }, 
    { 57.875000, 111.306824 }, 
    { 57.000000, 133.306824 }, 
    { 42.125000, 143.806824 }, 
    { 41.625000, 161.306824 }, 
    { 0.000000, 161.056824 }, 
    { 0.375000, 115.056824 }, 
    { 16.500000, 101.556824 }, 
    { 17.750000, 78.681824 }, 
    { 64.375000, 57.056824 }, 
    { 65.625000, 31.931824 }, 
    { 48.375000, 15.056824 }, 
    { 28.750000, 14.806824 }, 
    };

    // - Found a polygon of type nogo, name "treepoly", id 75!
    // min_x 367.994995 min_y 255.914505 max_x 400.000000 max_y 277.502197
    // origin 367.994995 255.914505 bounding box w 32.005005 h 21.587692
    // std::vector<std::vector<float>> polyverts = {
    // { 0.000000, 0.125504 }, 
    // { 31.879486, 0.000000 }, 
    // { 32.005005, 15.563217 }, 
    // { 21.713196, 20.960098 }, 
    // { 16.190796, 21.587692 }, 
    // { 8.158142, 19.705002 }, 
    // { 0.125519, 15.563217 }, 
    // };

    // representation for pt-in-poly
    std::vector<GTPoint> d_polyverts(polyverts.size()+1);

    int polygonOffsetX = 50;
    int polygonOffsetY = 50;

    std::transform(polyverts.begin(), polyverts.end(),d_polyverts.begin(), 
            [globalScaleX, globalScaleY, polygonOffsetX, polygonOffsetY]
            (std::vector<float> pt){ GTPoint p; p.x = int((pt[0] + 0.5) + polygonOffsetX); 
                                        p.y = int((pt[1] + 0.5) + polygonOffsetY); return p; });

    d_polyverts[polyverts.size()] = d_polyverts[0];         //close the polygon  - damn, that made it worse for 3rd river, better for 1st                            


    //purple outline for when point is outside the outline
    std::shared_ptr<sf::VertexArray> outliney = std::shared_ptr<sf::VertexArray>(new sf::VertexArray(sf::LineStrip,polyverts.size()+1));
    for(auto j=0;j<polyverts.size();j++) {
        (*outliney)[j] = sf::Vertex(sf::Vector2f(d_polyverts[j].x * globalScaleX, d_polyverts[j].y * globalScaleY),sf::Color(128,0,255));
    }
    (*outliney)[polyverts.size()] = (*outliney)[0];     //extra segment to close the polyline
    //not always drawing it scene_objects.push_back(outliney);

    //white outline for when point is inside the outline
    std::shared_ptr<sf::VertexArray> inliney = std::shared_ptr<sf::VertexArray>(new sf::VertexArray(sf::LineStrip,polyverts.size()+1));
    for(auto j=0;j<polyverts.size();j++) {
        (*inliney)[j] = sf::Vertex(sf::Vector2f(d_polyverts[j].x * globalScaleX, d_polyverts[j].y * globalScaleY),sf::Color(255,255,255));
    }
    (*inliney)[polyverts.size()] = (*inliney)[0];     //extra segment to close the polyline
    // not always drawing it scene_objects.push_back(outliney);

    // location of the point we want to test for inside the polygon
    // window size given in absolute pixels so account for global scale
    // should be sf::Vector2f the_point = sf::Vector2f(window.getSize().x / (2 * globalScaleX), window.getSize().y / (2 * globalScaleY));
    // for bad pt-in-poly test
    sf::Vector2f the_point = sf::Vector2f(0.0, 0.0);

    sf::Rect<int> the_bbox;

    //let's derive the bounding box
    int min_x = 999999;
    int min_y = 999999;
    int max_x = -999999;
    int max_y = -999999;
    for(auto pt: d_polyverts) {
      if (pt.x < min_x) min_x = pt.x;
      if (pt.y < min_y) min_y = pt.y;
      if (pt.x > max_x) max_x = pt.x;
      if (pt.y > max_y) max_y = pt.y;
    }
    the_bbox.left = min_x;
    the_bbox.top = min_y;
    the_bbox.width = (max_x - min_x);
    the_bbox.height = (max_y - min_y);


    printf("ThePoint is at %f, %f\n",the_point.x, the_point.y);
    printf("TheBBox is top %d left %d width %d height %d\n",the_bbox.top, the_bbox.left, the_bbox.width, the_bbox.height);

    float point_velocity = 1.0;

    //crosshair for showing the point
    // std::shared_ptr<sf::RectangleShape> xhair1 = std::shared_ptr<sf::RectangleShape>(new sf::RectangleShape(sf::Vector2f(1,11)));
    // xhair1->setOrigin(sf::Vector2f(0.0,5.0));
    // xhair1->setFillColor(sf::Color(0,255,0));
    // scene_objects.push_back(xhair1);
    // std::shared_ptr<sf::RectangleShape> xhair2 = std::shared_ptr<sf::RectangleShape>(new sf::RectangleShape(sf::Vector2f(11,1)));
    // xhair2->setOrigin(sf::Vector2f(5.0,0.0));
    // xhair2->setFillColor(sf::Color(0,255,0));
    // scene_objects.push_back(xhair2);

    // the point
    std::shared_ptr<sf::RectangleShape> punto_out = std::shared_ptr<sf::RectangleShape>(new sf::RectangleShape(sf::Vector2f(1,1)));
    punto_out->setFillColor(sf::Color(128,0,255));
    punto_out->setOrigin(sf::Vector2f(0.0,0.0));
    //scene_objects.push_back(punto);
    std::shared_ptr<sf::RectangleShape> punto_in = std::shared_ptr<sf::RectangleShape>(new sf::RectangleShape(sf::Vector2f(1,1)));
    punto_in->setFillColor(sf::Color(255,255,255));
    punto_in->setOrigin(sf::Vector2f(0.0,0.0));

    // MAIN LOOP =============================================================================================

    //initial clear for trails version
    window.clear(sf::Color::Black);

    // run the program as long as the window is open
    while (window.isOpen())
    {
        // check all the window's events that were triggered since the last iteration of the loop
        sf::Event event;
        while (window.pollEvent(event))
        {
            // "close requested" event: we close the window
            // can also press ESC to close window
            if (event.type == sf::Event::Closed) {
                window.close();
            } else if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Escape) {
                    window.close();
                }
            }
        }

        // // move the point!
        // float deltaX, deltaY;
        // deltaX = 0.0;
        // deltaY = 0.0;

        // if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) deltaX -= point_velocity;
        // if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) deltaX += point_velocity;

        // if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) deltaY -= point_velocity;
        // if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) deltaY += point_velocity;

        // the_point.x += deltaX;
        // the_point.y += deltaY;

        // if(the_point.x < 0) the_point.x = 0;
        // if(the_point.x > window.getSize().x / globalScaleX) the_point.x = window.getSize().x / globalScaleX;
        // if(the_point.y < 0) the_point.y = 0;
        // if(the_point.y > window.getSize().y / globalScaleY) the_point.y = window.getSize().y / globalScaleX;

        // let's just make the dot go from the upper left to the lower right
        the_point.x += 1.0;
        if(the_point.x > (window.getSize().x / globalScaleX)) {
            the_point.x = 0;
            the_point.y += 1.0;
            if(the_point.y > (window.getSize().y / globalScaleY)) {
                 the_point.y = (window.getSize().y / globalScaleY);
                 the_point.x = 0.0;
            }
        }
    


        // clear the window with black color
        //window.clear(sf::Color::Black);

        // make sure point is onscreen
        if(the_point.x < 0.0) the_point.x = 0.0;
        if(the_point.x > (window.getSize().x / globalScaleX)) the_point.x = (window.getSize().x / globalScaleX);
        if(the_point.y < 0.0) the_point.y = 0.0;
        if(the_point.y > (window.getSize().y / globalScaleY)) the_point.y = (window.getSize().y / globalScaleY);

        // draw crosshair of some sort at the_point
        // xhair1->setPosition(sf::Vector2f(the_point.x,the_point.y));
        // xhair2->setPosition(sf::Vector2f(the_point.x,the_point.y));

        // temp
        punto_in->setPosition(sf::Vector2f(the_point.x,the_point.y));
        punto_out->setPosition(sf::Vector2f(the_point.x,the_point.y));

        // draw everything here...
        // again, simple kludgy way to do a scene graph
        for(auto sobj : scene_objects) {
            // apply global transform to everything
            window.draw(*sobj,globalTransform);
        }

        //if point is inside polygon, draw inliney, else outliney
        // Point intpoint;
        // intpoint.x = int(the_point.x + 0.5);       //need to do our own scaling bc it doesn't get xformed ... no we don't
        // intpoint.y = int(the_point.y + 0.5);

        GTPoint gtPoint;
        gtPoint.x = the_point.x;
        gtPoint.y = the_point.y;

        //you know what, let's add a bounding box check 

        if(!bbox_check(the_point, the_bbox) || !InsidePolygon(d_polyverts, gtPoint)) {
            //outside!
            window.draw(*outliney);
            window.draw(*punto_out,globalTransform);
        } else {
            //inside!
            window.draw(*inliney);
            window.draw(*punto_in,globalTransform);
        }

        // end the current frame
        window.display();
    }

    return 0;
}
