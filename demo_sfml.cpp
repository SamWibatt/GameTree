//#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

// temp!
#include <cstdio>
#include <cerrno>
#include <system_error>
#include <cstring>
#include "cppcodec/base64_url.hpp"
using base64 = cppcodec::base64_url;
// end temp!

int main()
{
    //quick test of base64 stuff - let's do a little pnglet
    std::string png_path = "/home/sean/dev/GameNoodles/sfml/hello/assets/8BIT_CanariPack_TopDown/SPRITES/ITEMS/item8BIT_heart.png";
    std::vector<uint8_t> image_data;
    printf("About to read png %s\n",png_path.c_str());
    FILE * pngfp = std::fopen(png_path.c_str(), "rb");

    if(pngfp == nullptr) {
        fprintf(stderr,"*** ERROR: failed to open png file %s - error %s\n",png_path.c_str(),std::strerror(errno));
        exit(1);
    }
    //get file size and allocate buffer for it
    std::fseek(pngfp,0,SEEK_END);
    auto pngsiz = std::ftell(pngfp);
    std::fseek(pngfp,0,SEEK_SET);
    printf("png size = %ld\n",pngsiz);
    image_data.resize(pngsiz);    //see if this works; I think we want resize and not reserve
    std::fread(image_data.data(), sizeof(uint8_t), image_data.size(), pngfp);
    fclose(pngfp);

    printf("Converting png to base64\n");
    std::string encoded_png = base64::encode(image_data);

    printf("Encoded png is: ------------------------\n%s\n-------------------------\n",encoded_png.c_str());

    printf("Converting base64 back to png\n");
    std::vector<uint8_t> decoded_png = base64::decode(encoded_png);

    printf("Loading texture from decoded png\n");
    sf::Texture tex;
    tex.loadFromMemory(decoded_png.data(),decoded_png.size());

    printf("texture size: %d x %d\n",tex.getSize().x, tex.getSize().y);
    
    sf::Sprite hortsprit;
    hortsprit.setTexture(tex,true);
    hortsprit.setPosition(sf::Vector2f(10.0f, 10.0f));
    hortsprit.setScale(5.0,5.0);

    // WORKY!!!!!!!!!!!!!!!!!!!!

    // end base64 test =================================================================================

    sf::RenderWindow window(sf::VideoMode(800, 600), "Hello, GameTree!");

    //how do you make vsync actually work? see https://www.maketecheasier.com/get-rid-screen-tearing-linux/ - yup, like that, the intel
    //section worked on entrapta
    window.setVerticalSyncEnabled(true); // call it once, after creating the window

    // set up some stuff to draw
    sf::CircleShape shape(50.f);
    shape.setFillColor(sf::Color(100, 250, 50));
    shape.setOrigin(0,0);

    // run the program as long as the window is open
    while (window.isOpen())
    {
        // check all the window's events that were triggered since the last iteration of the loop
        sf::Event event;
        while (window.pollEvent(event))
        {
            // "close requested" event: we close the window
            if (event.type == sf::Event::Closed)
                window.close();
        }

        // clear the window with black color
        window.clear(sf::Color::Black);

        // draw everything here...
        window.draw(shape);

        // TEMP!
        window.draw(hortsprit);

        // end the current frame
        window.display();
    }

    return 0;
}
