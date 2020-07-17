#include <cstdio>
#include <SFML/Graphics.hpp>
#include "gametree.h"


// temp!
// #include <cerrno>
// #include <system_error>
// #include <cstring>
// #include "cppcodec/base64_url.hpp"
// using base64 = cppcodec::base64_url;
// end temp!

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
    float globalScaleX = 2.0;
    float globalScaleY = 2.0;
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

    //try some tesselated triangles
    //Our triangles! ==================
    // std::vector<std::vector<std::pair<float,float>>> triverts = {
    //     { {36.666698, -25.666700}, {53.750000, 71.416702}, {55.666698, -5.333330} },
    //     { {36.666698, -25.666700}, {20.333300, 38.333302}, {53.750000, 71.416702} },
    //     { {36.666698, -25.666700}, {19.333300, 19.333300}, {20.333300, 38.333302} },
    //     { {36.666698, -25.666700}, {6.666670, 7.333330}, {19.333300, 19.333300} },
    //     { {36.666698, -25.666700}, {0.000038, 0.000003}, {6.666670, 7.333330} },
    //     { {-0.000001, 0.000000}, {36.666698, -25.666700}, {-0.666667, -26.000000} },
    //     { {87.666702, 39.666698}, {79.708298, 99.583298}, {87.000000, 53.666698} },
    //     { {87.666702, 39.666698}, {62.333302, 98.791702}, {79.708298, 99.583298} },
    //     { {56.666698, 11.000000}, {62.333302, 98.791702}, {87.666702, 39.666698} },
    //     { {53.750000, 71.416702}, {56.666698, 11.000000}, {55.666698, -5.333330} },
    //     { {53.750000, 71.416702}, {62.333302, 98.791702}, {56.666698, 11.000000} },
    //     { {62.333302, 98.791702}, {53.750000, 71.416702}, {53.541698, 88.916702} },
    //     { {91.000000, 63.000000}, {113.833000, 134.082993}, {112.875000, 62.625000} },
    //     { {87.000000, 53.666698}, {113.833000, 134.082993}, {91.000000, 63.000000} },
    //     { {113.833000, 134.082993}, {87.000000, 53.666698}, {79.708298, 99.583298} },
    //     { {19.333300, 19.333300}, {6.666670, 7.333330}, {6.666670, 13.333300} },
    // };
    std::vector<sf::Color> colors = {
        sf::Color(0,0,255),
        sf::Color(0,255,0),
        sf::Color(255,0,0),
        sf::Color(0,255,255),
        sf::Color(255,0,255),
        sf::Color(255,255,0),
        sf::Color(255,255,255)
    };

    // int colnum = 0;
    // for(auto tri : triverts) {
    //     std::shared_ptr<sf::ConvexShape> triggy = std::shared_ptr<sf::ConvexShape>(new sf::ConvexShape());
    //     triggy->setPosition(sf::Vector2f(30.0, 30.0));
    //     triggy->setPointCount(3);
    //     for(auto i = 0; i < 3; i++) {
    //         triggy->setPoint(i,sf::Vector2f(tri[i].first,tri[i].second));
    //     }
    //     triggy->setFillColor(sf::Color(255,0,0,128));           //semi-transparent red
    //     triggy->setOutlineColor(colors[colnum]);                //opaque cycling color lines
    //     colnum = (colnum + 1) % (colors.size());
    //     triggy->setOutlineThickness(-1);                        //-1 means inside the shape
    //     scene_objects.push_back(triggy);
    // }

    // verify we read polygon right
    // lil tree poly
    // std::vector<std::vector<float>> polyverts = {
    // { 0.000000, 0.000000 }, 
    // { 31.879499, -0.125510 }, 
    // { 32.005001, 15.437700 }, 
    // { 21.713200, 20.834600 }, 
    // { 16.190800, 21.462200 }, 
    // { 8.158140, 19.579500 }, 
    // { 0.125510, 15.437700 }, 
    // };

    //1st river
    // std::vector<std::vector<float>> polyverts = {
    // { -1.000000, 0.000000 }, 
    // { 6.666670, 7.333330 }, 
    // { 6.666670, 13.333300 }, 
    // { 19.333300, 19.333300 }, 
    // { 20.333300, 38.333302 }, 
    // { 53.750000, 71.416702 }, 
    // { 53.541698, 88.916702 }, 
    // { 62.333302, 98.791702 }, 
    // { 79.708298, 99.583298 }, 
    // { 113.833000, 134.082993 }, 
    // { 112.875000, 62.625000 }, 
    // { 91.000000, 63.000000 }, 
    // { 87.000000, 53.666698 }, 
    // { 87.666702, 39.666698 }, 
    // { 56.666698, 11.000000 }, 
    // { 55.666698, -5.333330 }, 
    // { 36.666698, -25.666700 }, 
    // { -0.666667, -26.000000 }, 
    // };

    //2nd river
    // std::vector<std::vector<float>> polyverts = {
    // { 0.000000, 0.000000 }, 
    // { 0.000000, 69.333298 }, 
    // { 19.666700, 68.333298 }, 
    // { 34.666698, 87.333298 }, 
    // { 57.666698, 87.000000 }, 
    // { 57.333302, 112.000000 }, 
    // { 42.333302, 120.667000 }, 
    // { 40.333302, 129.332993 }, 
    // { 28.333300, 137.332993 }, 
    // { 23.333300, 145.000000 }, 
    // { 9.000000, 155.000000 }, 
    // { 9.000000, 186.667007 }, 
    // { 35.666698, 216.667007 }, 
    // { 129.000000, 217.000000 }, 
    // { 125.667000, 197.000000 }, 
    // { 144.332993, 189.667007 }, 
    // { 140.332993, 161.000000 }, 
    // { 97.666702, 161.000000 }, 
    // { 93.000000, 136.332993 }, 
    // { 123.667000, 106.333000 }, 
    // { 121.667000, 57.666698 }, 
    // { 105.667000, 47.666698 }, 
    // { 107.000000, 23.666700 }, 
    // { 65.000000, -11.666700 }, 
    // { 29.000000, -11.666700 }, 
    // { 24.333300, 3.666670 }, 
    // };

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
    std::shared_ptr<sf::VertexArray> outliney = std::shared_ptr<sf::VertexArray>(new sf::VertexArray(sf::LineStrip,polyverts.size()+1));
    for(auto j=0;j<polyverts.size();j++) {
        //should be 
        (*outliney)[j] = sf::Vertex(sf::Vector2f(polyverts[j][0] + 30, polyverts[j][1] + 30),sf::Color(255,255,255));
        //magnified x 3 (*outliney)[j] = sf::Vertex(sf::Vector2f((polyverts[j][0] * 3) + 30, (polyverts[j][1] * 3) + 30),sf::Color(255,255,255));
    }
    (*outliney)[polyverts.size()] = (*outliney)[0];     //extra segment to close the polyline
    scene_objects.push_back(outliney);

    // try a test where we shuffle up the points - right now assuming they go around the perimeter of the polygon,
    // instead do 0, 1, 2nd to last, 2, 3rd to last, ...
    // see how it looks for triangles - draw that as a linestrip and it should be a nice zigzag
    // let's see - so if you have
    // 0 1 2 3 4 5 6 - size = 7
    // you want
    // 0 1 6 2 5 3 4, yes? 5 iterations after the 0 1
    // if you have 
    // 0 1 2 3 4 5 6 7, - size = 8 you want
    // 0 1 7 2 6 3 5 4, yes? 6 iterations after the 0 2
    // what about
    // 0 6 1 5 2 4 3 - size 7
    std::vector<std::vector<float>> polyverts2;
    polyverts2.resize(polyverts.size());
    polyverts2[0] = polyverts[0];
    //polyverts2[1] = polyverts[1];
    //printf("0 1 ");
    printf("0 ");
    int offset = 0;
    for(int j=0;j<polyverts.size()-1;j++) {
        if((j%2) == 0) {
            //even j, get (size()-1)-offset
            printf("%lu ",(polyverts.size()-1) - offset);
            polyverts2[1+j] = polyverts[(polyverts.size()-1) - offset]; 
        } else {
            //odd j, get 1+ offset, advance offset
            printf("%d ",1 + offset);
            polyverts2[1+j] = polyverts[1+offset];
            offset++;
        }
    }
    printf("\n");
    std::shared_ptr<sf::VertexArray> outliney2 = std::shared_ptr<sf::VertexArray>(new sf::VertexArray(sf::TriangleStrip,polyverts2.size()));
    for(auto j=0;j<polyverts2.size();j++) {
        (*outliney2)[j] = sf::Vertex(sf::Vector2f(polyverts2[j][0] + 30, polyverts2[j][1] + 30),sf::Color(255,0,255));
        // magnified x 3 
        //(*outliney2)[j] = sf::Vertex(sf::Vector2f((polyverts2[j][0] * 3) + 30, (polyverts2[j][1] * 3) + 30),sf::Color(0,255,255));
    }
    //(*outliney2)[polyverts2.size()] = (*outliney2)[0];
    //outliney2->resize(polyverts2.size());   //try snipping off a vert or two - knocking one off makes it work, so left it off above
    scene_objects.push_back(outliney2);


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
            } else if ((event.type == sf::Event::KeyPressed) && 
                        (event.key.code == sf::Keyboard::Escape)) {
                window.close();
            }                
        }

        // clear the window with black color
        window.clear(sf::Color::Black);

        // draw everything here...
        // again, simple kludgy way to do a scene graph
        for(auto sobj : scene_objects) {
            // apply global transform to everything
            window.draw(*sobj,globalTransform);
        }

        // TEMP!
        //window.draw(hortsprit);

        // end the current frame
        window.display();
    }

    return 0;
}
