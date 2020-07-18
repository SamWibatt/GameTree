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

// nother point in poly auld ===========================================================================================================
// http://www.eecs.umich.edu/courses/eecs380/HANDOUTS/PROJ2/InsidePoly.html

#define MIN(x,y) (x < y ? x : y)
#define MAX(x,y) (x > y ? x : y)
#define INSIDE 0
#define OUTSIDE 1

typedef struct {
   double x,y;
} dPoint;

int InsidePolygon(dPoint *polygon,int N, dPoint p)
{
  int counter = 0;
  int i;
  double xinters;
  dPoint p1,p2;

  p1 = polygon[0];
  for (i=1;i<=N;i++) {
    p2 = polygon[i % N];
    if (p.y > MIN(p1.y,p2.y)) {
      if (p.y <= MAX(p1.y,p2.y)) {
        if (p.x <= MAX(p1.x,p2.x)) {
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
    return(OUTSIDE);
  else
    return(INSIDE);
}

// bounding box check
int bbox_check(sf::Vector2f& the_point, sf::Rect<float>& the_bbox) {
    if(the_point.x < the_bbox.left || the_point.y < the_bbox.top) return 0;
    if(the_point.x > the_bbox.left + the_bbox.width || the_point.y > the_bbox.top + the_bbox.height) return 0;
    return 1;
}

// end point-in-poly from auld website ===============================================================================================================================

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


//     // verify we read polygon right
//     // lil tree poly
//     // std::vector<std::vector<float>> polyverts = {
//     // { 0.000000, 0.000000 }, 
//     // { 31.879499, -0.125510 }, 
//     // { 32.005001, 15.437700 }, 
//     // { 21.713200, 20.834600 }, 
//     // { 16.190800, 21.462200 }, 
//     // { 8.158140, 19.579500 }, 
//     // { 0.125510, 15.437700 }, 
//     // };

    // //first river, normalized - ccl - DOES NOT WORK EITHER DIRECTION
    // worky with bbox check! worky 2nd algo even wo bbox!
    std::vector<std::vector<float>> polyverts = {
    { 0.000000, 26.000000 }, 
    { 7.666670, 33.333328 }, 
    { 7.666670, 39.333298 }, 
    { 20.333300, 45.333298 }, 
    { 21.333300, 64.333298 }, 
    { 54.750000, 97.416702 }, 
    { 54.541698, 114.916702 }, 
    { 63.333302, 124.791702 }, 
    { 80.708298, 125.583298 }, 
    { 114.833000, 160.082993 }, 
    { 113.875000, 88.625000 }, 
    { 92.000000, 89.000000 }, 
    { 88.000000, 79.666702 }, 
    { 88.666702, 65.666702 }, 
    { 57.666698, 37.000000 }, 
    { 56.666698, 20.666670 }, 
    { 37.666698, 0.333300 }, 
    { 0.333333, 0.000000 }, 
    };    

    //last river, non-normalized - seems worky - haven't tried post-test
    // std::vector<std::vector<float>> polyverts = {
    // { -6.090910, -2.056820 }, 
    // { 84.272697, -1.954550 }, 
    // { 84.454498, 21.227301 }, 
    // { 99.000000, 30.500000 }, 
    // { 97.000000, 55.000000 }, 
    // { 84.000000, 60.500000 }, 
    // { 85.000000, 79.500000 }, 
    // { 57.500000, 108.500000 }, 
    // { 36.500000, 109.250000 }, 
    // { 35.625000, 131.250000 }, 
    // { 20.750000, 141.750000 }, 
    // { 20.250000, 159.250000 }, 
    // { -21.375000, 159.000000 }, 
    // { -21.000000, 113.000000 }, 
    // { -4.875000, 99.500000 }, 
    // { -3.625000, 76.625000 }, 
    // { 43.000000, 55.000000 }, 
    // { 44.250000, 29.875000 }, 
    // { 27.000000, 13.000000 }, 
    // { 7.375000, 12.750000 }, 
    // };   

    //last river, normalized - also has a problem with the upper left of the screen in poly test mode! I wonder if it's bc the poly isn't closed
    //nope, but works with bbox check
    // std::vector<std::vector<float>> polyverts = {
    // { 15.284088, 0.000000 }, 
    // { 105.647705, 0.102264 }, 
    // { 105.829498, 23.284119 }, 
    // { 120.375000, 32.556824 }, 
    // { 118.375000, 57.056824 }, 
    // { 105.375000, 62.556824 }, 
    // { 106.375000, 81.556824 }, 
    // { 78.875000, 110.556824 }, 
    // { 57.875000, 111.306824 }, 
    // { 57.000000, 133.306824 }, 
    // { 42.125000, 143.806824 }, 
    // { 41.625000, 161.306824 }, 
    // { 0.000000, 161.056824 }, 
    // { 0.375000, 115.056824 }, 
    // { 16.500000, 101.556824 }, 
    // { 17.750000, 78.681824 }, 
    // { 64.375000, 57.056824 }, 
    // { 65.625000, 31.931824 }, 
    // { 48.375000, 15.056824 }, 
    // { 28.750000, 14.806824 }, 
    // };    

    //lil tree polygon - works with bbox check - worky with second algorithm
    // std::vector<std::vector<float>> polyverts = {
    // { 0.000000, 0.125504 }, 
    // { 31.879486, 0.000000 }, 
    // { 32.005005, 15.563217 }, 
    // { 21.713196, 20.960098 }, 
    // { 16.190796, 21.587692 }, 
    // { 8.158142, 19.705002 }, 
    // { 0.125519, 15.563217 }, 
    // };    

    //second river normalized - drat, its error zone has some inside the bbox
    //worky with second algorithm
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

    // FOR CCL FIRST RIVER TEMP! turn it backwards & see if it fixes troubles
    //std::reverse(polyverts.begin(),polyverts.end());
    //printf("Reversed polyverts! first vert is now %f, %f\n",polyverts[0][0], polyverts[0][1]);

    //let's convert those to ints, rounding, to test out the point-in-poly thing I have above
    //std::vector<Point> i_polyverts(polyverts.size()+1);         //let's see if closing the polygon makes it work! was just size()

    // doubles for the other routine
    std::vector<dPoint> d_polyverts(polyverts.size()+1);

    int polygonOffsetX = 50;
    int polygonOffsetY = 50;

    // std::transform(polyverts.begin(), polyverts.end(),i_polyverts.begin(), 
    //         [globalScaleX, globalScaleY, polygonOffsetX, polygonOffsetY]
    //         (std::vector<float> pt){ Point p; p.x = int((pt[0] + 0.5) + polygonOffsetX); 
    //                                     p.y = int((pt[1]+0.5) + polygonOffsetY); return p; });

    // i_polyverts[polyverts.size()] = i_polyverts[0];         //close the polygon  - damn, that made it worse for 3rd river, better for 1st                            

    std::transform(polyverts.begin(), polyverts.end(),d_polyverts.begin(), 
            [globalScaleX, globalScaleY, polygonOffsetX, polygonOffsetY]
            (std::vector<float> pt){ dPoint p; p.x = double(pt[0] + polygonOffsetX); 
                                        p.y = double(pt[1] + polygonOffsetY); return p; });

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

    sf::Rect<float> the_bbox;

    //let's derive the bounding box
    float min_x = MAXFLOAT;
    float min_y = MAXFLOAT;
    float max_x = -MAXFLOAT;
    float max_y = -MAXFLOAT;
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
    printf("TheBBox is top %f left %f width %f height %f\n",the_bbox.top, the_bbox.left, the_bbox.width, the_bbox.height);

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

        dPoint dpoint;
        dpoint.x = double(the_point.x);
        dpoint.y = double(the_point.y);

        //you know what, let's add a bounding box check 

        if(!bbox_check(the_point, the_bbox) || InsidePolygon(d_polyverts.data(), d_polyverts.size(), dpoint) == OUTSIDE) {
            //outside!
            window.draw(*outliney);
            window.draw(*punto_out,globalTransform);
        } else {
            //inside!
            window.draw(*inliney);
            window.draw(*punto_in,globalTransform);
        }


        // if(!bbox_check(the_point, the_bbox) || !wn_PnPoly(intpoint, i_polyverts.data(), i_polyverts.size())) {
        //     //outside!
        //     window.draw(*outliney);
        //     window.draw(*punto_out,globalTransform);
        //     //punto->setFillColor(sf::Color(128,0,255,0));
        // } else {
        //     //inside!
        //     window.draw(*inliney);
        //     window.draw(*punto_in,globalTransform);
        //     //punto->setFillColor(sf::Color(255,255,255,0));
        // }




        // end the current frame
        window.display();
    }

    return 0;
}
