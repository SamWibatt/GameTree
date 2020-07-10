//#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

int main()
{
    sf::RenderWindow window(sf::VideoMode(800, 600), "Hello, GameTree!");

    //how do you make vsync actually work? see https://www.maketecheasier.com/get-rid-screen-tearing-linux/
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

        // end the current frame
        window.display();
    }

    return 0;
}
