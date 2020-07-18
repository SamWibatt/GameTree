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

using namespace gt;

int main()
{
    //really dumb little test - how big is an empty string? yikes, 32 bytes
    // std::string s;
    // printf("sizeof empty std::string is %lu or %lu\n",sizeof(std::string), sizeof(s));

    // s = "Jorkpork!";
    // printf("sizeof nonempty std::string is %lu or %lu\n",sizeof(std::string), sizeof(s));

    // std::shared_ptr<std::string> ps = nullptr;
    // printf("sizeof nullptr std::shared_ptr<std::string> is %lu or %lu\n",sizeof(std::shared_ptr<std::string>), sizeof(ps));

    // ps = std::shared_ptr<std::string>(new std::string("Jorkpork!"));
    // printf("sizeof non-nullptr std::shared_ptr<std::string> is %lu or %lu\n",sizeof(std::shared_ptr<std::string>), sizeof(ps));

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
    // min_x -1.000000 min_y -26.000000 max_x 113.833000 max_y 134.082993
    // // origin 1.333330 27.000000 bounding box -1.000000,-26.000000 - 113.833000,134.082993
    std::vector<std::vector<float>> polyverts = {
      { -1.000000, 0.000000 }, 
      { 6.666670, 7.333330 }, 
      { 6.666670, 13.333300 }, 
      { 19.333300, 19.333300 }, 
      { 20.333300, 38.333302 }, 
      { 53.750000, 71.416702 }, 
      { 53.541698, 88.916702 }, 
      { 62.333302, 98.791702 }, 
      { 79.708298, 99.583298 }, 
      { 113.833000, 134.082993 }, 
      { 112.875000, 62.625000 }, 
      { 91.000000, 63.000000 }, 
      { 87.000000, 53.666698 }, 
      { 87.666702, 39.666698 }, 
      { 56.666698, 11.000000 }, 
      { 55.666698, -5.333330 }, 
      { 36.666698, -25.666700 }, 
      { -0.666667, -26.000000 }, 
    };
    // //polygon has 18 verts
    // - Found a polygon of type nogo, name "", id 2!
    // min_x 0.000000 min_y -11.666700 max_x 144.332993 max_y 217.000000
    // // origin 125.000000 87.666702 bounding box 0.000000,-11.666700 - 144.332993,217.000000
    // std::vector<std::vector<float>> polyverts = {
    //   { 0.000000, 0.000000 }, 
    //   { 0.000000, 69.333298 }, 
    //   { 19.666700, 68.333298 }, 
    //   { 34.666698, 87.333298 }, 
    //   { 57.666698, 87.000000 }, 
    //   { 57.333302, 112.000000 }, 
    //   { 42.333302, 120.667000 }, 
    //   { 40.333302, 129.332993 }, 
    //   { 28.333300, 137.332993 }, 
    //   { 23.333300, 145.000000 }, 
    //   { 9.000000, 155.000000 }, 
    //   { 9.000000, 186.667007 }, 
    //   { 35.666698, 216.667007 }, 
    //   { 129.000000, 217.000000 }, 
    //   { 125.667000, 197.000000 }, 
    //   { 144.332993, 189.667007 }, 
    //   { 140.332993, 161.000000 }, 
    //   { 97.666702, 161.000000 }, 
    //   { 93.000000, 136.332993 }, 
    //   { 123.667000, 106.333000 }, 
    //   { 121.667000, 57.666698 }, 
    //   { 105.667000, 47.666698 }, 
    //   { 107.000000, 23.666700 }, 
    //   { 65.000000, -11.666700 }, 
    //   { 29.000000, -11.666700 }, 
    //   { 24.333300, 3.666670 }, 
    // };
    // //polygon has 26 verts
    // - Found a polygon of type nogo, name "", id 3!
    // min_x -21.375000 min_y -2.056820 max_x 99.000000 max_y 159.250000
    // // origin 216.000000 353.500000 bounding box -21.375000,-2.056820 - 99.000000,159.250000
    // std::vector<std::vector<float>> polyverts = {
    //   { -6.090910, -2.056820 }, 
    //   { 84.272697, -1.954550 }, 
    //   { 84.454498, 21.227301 }, 
    //   { 99.000000, 30.500000 }, 
    //   { 97.000000, 55.000000 }, 
    //   { 84.000000, 60.500000 }, 
    //   { 85.000000, 79.500000 }, 
    //   { 57.500000, 108.500000 }, 
    //   { 36.500000, 109.250000 }, 
    //   { 35.625000, 131.250000 }, 
    //   { 20.750000, 141.750000 }, 
    //   { 20.250000, 159.250000 }, 
    //   { -21.375000, 159.000000 }, 
    //   { -21.000000, 113.000000 }, 
    //   { -4.875000, 99.500000 }, 
    //   { -3.625000, 76.625000 }, 
    //   { 43.000000, 55.000000 }, 
    //   { 44.250000, 29.875000 }, 
    //   { 27.000000, 13.000000 }, 
    //   { 7.375000, 12.750000 }, 
    // };

    GTPolygon poly;
    poly.position = { 50, 50 };

    // COME UP WITH A WAY TO DO THIS BETTER
    // like a setpoints
    poly.points.resize(polyverts.size());       
    std::transform(polyverts.begin(), polyverts.end(),poly.points.begin(), 
            [](std::vector<float> pt){ GTPoint p; p.x = int(pt[0] + 0.5); p.y = int(pt[1] + 0.5); return p; });
    //let's derive the bounding box - THIS SHOULD BE ADDED TO GTPolygon
    int min_x = 999999;
    int min_y = 999999;
    int max_x = -999999;
    int max_y = -999999;
    for(auto pt: poly.points) {
      if (pt.x < min_x) min_x = pt.x;
      if (pt.y < min_y) min_y = pt.y;
      if (pt.x > max_x) max_x = pt.x;
      if (pt.y > max_y) max_y = pt.y;
    }
    poly.bbox_ul = { min_x, min_y };
    poly.bbox_lr = { poly.bbox_ul.x + (max_x - min_x), poly.bbox_ul.y + (max_y - min_y) };
    // END COME UP WITH A WAY TO DO THIS BETTER


    //purple outline for when point is outside the outline
    std::shared_ptr<sf::VertexArray> outliney = std::shared_ptr<sf::VertexArray>(new sf::VertexArray(sf::LineStrip,polyverts.size()+1));
    //white outline for when point is inside the outline
    std::shared_ptr<sf::VertexArray> inliney = std::shared_ptr<sf::VertexArray>(new sf::VertexArray(sf::LineStrip,polyverts.size()+1));
    for(auto j=0;j<polyverts.size();j++) {
        (*outliney)[j] = sf::Vertex(sf::Vector2f((poly.points[j].x + poly.position.x) * globalScaleX, 
                                                  (poly.points[j].y + poly.position.y) * globalScaleY),sf::Color(128,0,255));
        (*inliney)[j] = sf::Vertex(sf::Vector2f((poly.points[j].x + poly.position.x) * globalScaleX, 
                                                  (poly.points[j].y + poly.position.y) * globalScaleY),sf::Color(255,255,255));
    }
    (*outliney)[polyverts.size()] = (*outliney)[0];     //extra segment to close the polyline
    (*inliney)[polyverts.size()] = (*inliney)[0];     //extra segment to close the polyline
    // not always drawing them so don't add to scene_objects

    // location of the point we want to test for inside the polygon
    // window size given in absolute pixels so account for global scale
    // should be sf::Vector2f the_point = sf::Vector2f(window.getSize().x / (2 * globalScaleX), window.getSize().y / (2 * globalScaleY));
    // for bad pt-in-poly test
    sf::Vector2f the_point = sf::Vector2f(0.0, 0.0);



    printf("ThePoint is at %f, %f\n",the_point.x, the_point.y);
    //printf("TheBBox is top %d left %d width %d height %d\n",the_bbox.top, the_bbox.left, the_bbox.width, the_bbox.height);

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

        //if(!bbox_check(the_point, the_bbox) || !InsidePolygon(d_polyverts, gtPoint)) {
        if(!poly.inside(gtPoint)) {
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
