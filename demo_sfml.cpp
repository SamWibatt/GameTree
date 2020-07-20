#include <cstdio>
#include <SFML/Graphics.hpp>
#include "gametree.h"


using namespace gt;

// let's see how simple we can make this
class xfVertArray : public sf::Drawable {
  public:
    sf::VertexArray va;
    sf::Transformable tr;

  public:
    xfVertArray(sf::PrimitiveType pty, size_t nPoints) {
      va = sf::VertexArray(pty, nPoints);
      tr = sf::Transformable();
    }

    void append(sf::Vertex v) {
      va.append(v);
    }

    void setRotation(float rot) {
      tr.setRotation(rot);
    }

    void setPosition(const sf::Vector2f vec) {
      tr.setPosition(vec);
    }

    void setPosition(float x, float y) {
      tr.setPosition(x,y);
    }

    void setOrigin(const sf::Vector2f vec) {
      tr.setOrigin(vec);
    }

    void setOrigin(float x, float y) {
      tr.setOrigin(x,y);
    }

    void setScale(const sf::Vector2f vec) {
      tr.setScale(vec);
    }

    void setScale(float x, float y) {
      tr.setScale(x,y);
    }

    void draw(sf::RenderTarget &target, sf::RenderStates states) const {
      sf::RenderStates rs = states;
      rs.transform = states.transform * tr.getTransform();
      target.draw(va, rs);
    }
};

int main(int argc, char *argv[])
{

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
    window.setVerticalSyncEnabled(true); // call it once, after creating the window

    // KLUDGY WAY TO DO A SCENE GRAPH
    std::vector<std::shared_ptr<sf::Drawable>> scene_objects;

    // set up some stuff to draw
    // std::shared_ptr<sf::CircleShape> circy = std::shared_ptr<sf::CircleShape>(new sf::CircleShape(50.f));
    // circy->setFillColor(sf::Color(100, 250, 50));
    // circy->setOrigin(0,0);
    // scene_objects.push_back(circy);

    // let's find out if you can transform VertexArrays
    std::shared_ptr<xfVertArray> tri = 
      std::shared_ptr<xfVertArray>(new xfVertArray(sf::PrimitiveType::Triangles,3));
    sf::Vertex vert = sf::Vertex(sf::Vector2f(-50.0,-50.0),sf::Color(255,0,0));
    tri->append(vert);
    vert = sf::Vertex(sf::Vector2f(50.0,-50.0),sf::Color(0,255,0));
    tri->append(vert);
    vert = sf::Vertex(sf::Vector2f(0.0,50.0),sf::Color(0,0,255));
    tri->append(vert);
    tri->setPosition(150.0,150.0);
    tri->setOrigin(0.0,0.0);
    float trot = 30.0;
    tri->setRotation(trot);
    scene_objects.push_back(tri);


    // so HERE instead of that we need to load up a map in GameTree format and turn it into SFML-usable, yes?
    // Still got a piece left to do for that, though a lot of it is directly usable
    GTMap the_map;

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

        // clear the window with black color
        window.clear(sf::Color::Black);

        trot += 1.0;
        tri->setRotation(trot);

        // draw everything here...
        // again, simple kludgy way to do a scene graph
        for(auto sobj : scene_objects) {
            // apply global transform to everything
            window.draw(*sobj,globalTransform);
        }

        // end the current frame
        window.display();
    }

    return 0;
}
