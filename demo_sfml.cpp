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

// point-in-poly from auld website ===================================================================================================================================
// https://web.archive.org/web/20150301001957/http://geomalgorithms.com/a03-_inclusion.html
// has both crossing number and winding number tests!

typedef struct {
    int x, y;
} Point;

// Copyright 2000 softSurfer, 2012 Dan Sunday
// This code may be freely used and modified for any purpose
// providing that this copyright notice is included with it.
// SoftSurfer makes no warranty for this code, and cannot be held
// liable for any real or imagined damage resulting from its use.
// Users of this code must verify correctness for their application.
 

// a Point is defined by its coordinates {int x, y;}
//===================================================================
 

// isLeft(): tests if a point is Left|On|Right of an infinite line.
//    Input:  three points P0, P1, and P2
//    Return: >0 for P2 left of the line through P0 and P1
//            =0 for P2  on the line
//            <0 for P2  right of the line
//    See: Algorithm 1 "Area of Triangles and Polygons"
inline int
isLeft( Point P0, Point P1, Point P2 )
{
    return ( (P1.x - P0.x) * (P2.y - P0.y)
            - (P2.x -  P0.x) * (P1.y - P0.y) );
}
//===================================================================

// cn_PnPoly(): crossing number test for a point in a polygon
//      Input:   P = a point,
//               V[] = vertex points of a polygon V[n+1] with V[n]=V[0]
//      Return:  0 = outside, 1 = inside
// This code is patterned after [Franklin, 2000]
int
cn_PnPoly( Point P, Point* V, int n )
{
    int    cn = 0;    // the  crossing number counter

    // loop through all edges of the polygon
    for (int i=0; i<n; i++) {    // edge from V[i]  to V[i+1]
       if (((V[i].y <= P.y) && (V[i+1].y > P.y))     // an upward crossing
        || ((V[i].y > P.y) && (V[i+1].y <=  P.y))) { // a downward crossing
            // compute  the actual edge-ray intersect x-coordinate
            float vt = (float)(P.y  - V[i].y) / (V[i+1].y - V[i].y);
            if (P.x <  V[i].x + vt * (V[i+1].x - V[i].x)) // P.x < intersect
                 ++cn;   // a valid crossing of y=P.y right of P.x
        }
    }
    return (cn&1);    // 0 if even (out), and 1 if  odd (in)

}
//===================================================================


// wn_PnPoly(): winding number test for a point in a polygon
//      Input:   P = a point,
//               V[] = vertex points of a polygon V[n+1] with V[n]=V[0]
//      Return:  wn = the winding number (=0 only when P is outside)
int
wn_PnPoly( Point P, Point* V, int n )
{
    int    wn = 0;    // the  winding number counter

    // loop through all edges of the polygon
    for (int i=0; i<n; i++) {   // edge from V[i] to  V[i+1]
        if (V[i].y <= P.y) {          // start y <= P.y
            if (V[i+1].y  > P.y)      // an upward crossing
                 if (isLeft( V[i], V[i+1], P) > 0)  // P left of  edge
                     ++wn;            // have  a valid up intersect
        }
        else {                        // start y > P.y (no test needed)
            if (V[i+1].y  <= P.y)     // a downward crossing
                 if (isLeft( V[i], V[i+1], P) < 0)  // P right of  edge
                     --wn;            // have  a valid down intersect
        }
    }
    return wn;
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
    sf::VideoMode screenMode = sf::VideoMode(1024, 768);

    sf::Transform globalTransform = sf::Transform();
    float globalScaleX = 3.0;
    float globalScaleY = 3.0;
    globalTransform.scale(sf::Vector2f(globalScaleX,globalScaleY));
    bool fullscreen = false;         // can derive from whether any valid mode was found, etc.

    //CREATE WINDOW
    sf::RenderWindow window(screenMode, "Hello, GameTree!",fullscreen ? sf::Style::Fullscreen : sf::Style::Default);

    //how do you make vsync actually work? see https://www.maketecheasier.com/get-rid-screen-tearing-linux/ - yup, like that, the intel
    //section worked on entrapta
    window.setVerticalSyncEnabled(true); // call it once, after creating the window

    // KLUDGY WAY TO DO A SCENE GRAPH
    std::vector<std::shared_ptr<sf::Drawable>> scene_objects;

    // set up some stuff to draw
    // std::shared_ptr<sf::CircleShape> circy = std::shared_ptr<sf::CircleShape>(new sf::CircleShape(50.f));
    // circy->setFillColor(sf::Color(100, 250, 50));
    // circy->setOrigin(0,0);
    // scene_objects.push_back(circy);

    //try some tesselated triangles - damn, zigzag tesselation fails for river 3 in demo map
// //Our triangles! ==================
// std::vector<std::vector<std::pair<float,float>>> triverts = {
//     { {-6.090910, -2.056820}, {7.375000, 12.750000}, {84.272697, -1.954550} },
//     { {7.375000, 12.750000}, {84.272697, -1.954550}, {27.000000, 13.000000} },
//     { {84.272697, -1.954550}, {27.000000, 13.000000}, {84.454498, 21.227301} },
//     { {27.000000, 13.000000}, {84.454498, 21.227301}, {44.250000, 29.875000} },
//     { {84.454498, 21.227301}, {44.250000, 29.875000}, {99.000000, 30.500000} },
//     { {44.250000, 29.875000}, {99.000000, 30.500000}, {43.000000, 55.000000} },
//     { {99.000000, 30.500000}, {43.000000, 55.000000}, {97.000000, 55.000000} },
//     { {43.000000, 55.000000}, {97.000000, 55.000000}, {-3.625000, 76.625000} },
//     { {97.000000, 55.000000}, {-3.625000, 76.625000}, {84.000000, 60.500000} },
//     { {-3.625000, 76.625000}, {84.000000, 60.500000}, {-4.875000, 99.500000} },
//     { {84.000000, 60.500000}, {-4.875000, 99.500000}, {85.000000, 79.500000} },
//     { {-4.875000, 99.500000}, {85.000000, 79.500000}, {-21.000000, 113.000000} },
//     { {85.000000, 79.500000}, {-21.000000, 113.000000}, {57.500000, 108.500000} },
//     { {-21.000000, 113.000000}, {57.500000, 108.500000}, {-21.375000, 159.000000} },
//     { {57.500000, 108.500000}, {-21.375000, 159.000000}, {36.500000, 109.250000} },
//     { {-21.375000, 159.000000}, {36.500000, 109.250000}, {20.250000, 159.250000} },
//     { {36.500000, 109.250000}, {20.250000, 159.250000}, {35.625000, 131.250000} },
//     { {20.250000, 159.250000}, {35.625000, 131.250000}, {20.750000, 141.750000} },
// };
//     std::vector<sf::Color> colors = {
//         sf::Color(0,0,255),
//         sf::Color(0,255,0),
//         sf::Color(255,0,0),
//         sf::Color(0,255,255),
//         sf::Color(255,0,255),
//         sf::Color(255,255,0)
//     };

//     int colnum = 0;
//     for(auto tri : triverts) {
//         std::shared_ptr<sf::ConvexShape> triggy = std::shared_ptr<sf::ConvexShape>(new sf::ConvexShape());
//         triggy->setPosition(sf::Vector2f(30.0, 30.0));
//         triggy->setPointCount(3);
//         for(auto i = 0; i < 3; i++) {
//             triggy->setPoint(i,sf::Vector2f(tri[i].first,tri[i].second));
//         }
//         triggy->setFillColor(sf::Color(colors[colnum].r,colors[colnum].g,colors[colnum].b,96));           //semi-transparent fill
//         triggy->setOutlineColor(colors[colnum]);                //opaque cycling color lines
//         colnum = (colnum + 1) % (colors.size());
//         triggy->setOutlineThickness(-1);                        //-1 means inside the shape
//         scene_objects.push_back(triggy);
//     }

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


    //last river
    std::vector<std::vector<float>> polyverts = {
    { -6.090910, -2.056820 }, 
    { 84.272697, -1.954550 }, 
    { 84.454498, 21.227301 }, 
    { 99.000000, 30.500000 }, 
    { 97.000000, 55.000000 }, 
    { 84.000000, 60.500000 }, 
    { 85.000000, 79.500000 }, 
    { 57.500000, 108.500000 }, 
    { 36.500000, 109.250000 }, 
    { 35.625000, 131.250000 }, 
    { 20.750000, 141.750000 }, 
    { 20.250000, 159.250000 }, 
    { -21.375000, 159.000000 }, 
    { -21.000000, 113.000000 }, 
    { -4.875000, 99.500000 }, 
    { -3.625000, 76.625000 }, 
    { 43.000000, 55.000000 }, 
    { 44.250000, 29.875000 }, 
    { 27.000000, 13.000000 }, 
    { 7.375000, 12.750000 }, 
    };   

    //let's convert those to ints, rounding, to test out the point-in-poly thing I have above
    std::vector<Point> i_polyverts(polyverts.size());

    int polygonOffsetX = 50;
    int polygonOffsetY = 50;

    std::transform(polyverts.begin(), polyverts.end(),i_polyverts.begin(), 
            [globalScaleX, globalScaleY, polygonOffsetX, polygonOffsetY]
            (std::vector<float> pt){ Point p; p.x = int((pt[0] + 0.5) + polygonOffsetX); 
                                        p.y = int((pt[1]+0.5) + polygonOffsetY); return p; });
    

    //blue outline for when point is outside the outline
    std::shared_ptr<sf::VertexArray> outliney = std::shared_ptr<sf::VertexArray>(new sf::VertexArray(sf::LineStrip,polyverts.size()+1));
    for(auto j=0;j<polyverts.size();j++) {
        (*outliney)[j] = sf::Vertex(sf::Vector2f(i_polyverts[j].x * globalScaleX, i_polyverts[j].y * globalScaleY),sf::Color(0,0,255));
    }
    (*outliney)[polyverts.size()] = (*outliney)[0];     //extra segment to close the polyline
    //not always drawing it scene_objects.push_back(outliney);

    //white outline for when point is inside the outline
    std::shared_ptr<sf::VertexArray> inliney = std::shared_ptr<sf::VertexArray>(new sf::VertexArray(sf::LineStrip,polyverts.size()+1));
    for(auto j=0;j<polyverts.size();j++) {
        (*inliney)[j] = sf::Vertex(sf::Vector2f(i_polyverts[j].x * globalScaleX, i_polyverts[j].y * globalScaleY),sf::Color(255,255,255));
    }
    (*inliney)[polyverts.size()] = (*inliney)[0];     //extra segment to close the polyline
    // not always drawing it scene_objects.push_back(outliney);

    // location of the point we want to test for inside the polygon
    // window size given in absolute pixels so account for global scale
    sf::Vector2f the_point = sf::Vector2f(window.getSize().x / (2 * globalScaleX), window.getSize().y / (2 * globalScaleY));

    printf("ThePoint is at %f, %f\n",the_point.x, the_point.y);

    float point_velocity = 1.0;

    //crosshair for showing the point
    std::shared_ptr<sf::RectangleShape> xhair1 = std::shared_ptr<sf::RectangleShape>(new sf::RectangleShape(sf::Vector2f(1,11)));
    xhair1->setOrigin(sf::Vector2f(0.0,5.0));
    xhair1->setFillColor(sf::Color(0,255,0));
    scene_objects.push_back(xhair1);
    std::shared_ptr<sf::RectangleShape> xhair2 = std::shared_ptr<sf::RectangleShape>(new sf::RectangleShape(sf::Vector2f(11,1)));
    xhair2->setOrigin(sf::Vector2f(5.0,0.0));
    xhair2->setFillColor(sf::Color(0,255,0));
    scene_objects.push_back(xhair2);

    // MAIN LOOP =============================================================================================

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

        // move the point!
        float deltaX, deltaY;
        deltaX = 0.0;
        deltaY = 0.0;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) deltaX -= point_velocity;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) deltaX += point_velocity;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) deltaY -= point_velocity;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) deltaY += point_velocity;

        the_point.x += deltaX;
        the_point.y += deltaY;

        if(the_point.x < 0) the_point.x = 0;
        if(the_point.x > window.getSize().x / globalScaleX) the_point.x = window.getSize().x / globalScaleX;
        if(the_point.y < 0) the_point.y = 0;
        if(the_point.y > window.getSize().y / globalScaleY) the_point.y = window.getSize().y / globalScaleX;


        // clear the window with black color
        window.clear(sf::Color::Black);

        // make sure point is onscreen
        if(the_point.x < 0.0) the_point.x = 0.0;
        if(the_point.x > (window.getSize().x / globalScaleX)) the_point.x = (window.getSize().x / globalScaleX);
        if(the_point.y < 0.0) the_point.y = 0.0;
        if(the_point.y > (window.getSize().y / globalScaleY)) the_point.y = (window.getSize().y / globalScaleY);

        // draw crosshair of some sort at the_point
        xhair1->setPosition(sf::Vector2f(the_point.x,the_point.y));
        xhair2->setPosition(sf::Vector2f(the_point.x,the_point.y));



        // draw everything here...
        // again, simple kludgy way to do a scene graph
        for(auto sobj : scene_objects) {
            // apply global transform to everything
            window.draw(*sobj,globalTransform);
        }

        //if point is inside polygon, draw inliney, else outliney
        Point intpoint;
        intpoint.x = int(the_point.x + 0.5);       //need to do our own scaling bc it doesn't get xformed ... no we don't
        intpoint.y = int(the_point.y + 0.5);

        if(wn_PnPoly(intpoint, i_polyverts.data(), i_polyverts.size())) {
            //inside!
            window.draw(*inliney);
        } else {
            //outside!
            window.draw(*outliney);
        }




        // end the current frame
        window.display();
    }

    return 0;
}
