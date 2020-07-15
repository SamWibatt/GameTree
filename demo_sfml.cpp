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
    sf::VideoMode screenMode = sf::VideoMode(720, 400);
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
    std::shared_ptr<sf::CircleShape> circy = std::shared_ptr<sf::CircleShape>(new sf::CircleShape(50.f));
    circy->setFillColor(sf::Color(100, 250, 50));
    circy->setOrigin(0,0);
    scene_objects.push_back(circy);


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
