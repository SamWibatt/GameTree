#include <cstdio>
#include <fstream>
#include <SFML/Graphics.hpp>
#include "gametree.h"
#include "gtree_sfml.h"


using namespace gt;
using namespace gtree_sfml;


int main(int argc, char *argv[])
{

    // HW SETUP ====================================================================================================================

    // this will get replaced with some utility sfml plat-spec that sees which modes are available and finds the most 
    // suitable one (and makes sure it can work - recall how enabling vsync on entrapta broke all the 4:3 modes)
    // it will also choose scale factor
    // look into https://www.sfml-dev.org/tutorials/2.5/graphics-transform.php#object-hierarchies-scene-graph
    // for how to have a global scale factor; that's for a bit later
    //should be 
    sf::VideoMode screenMode = sf::VideoMode(720, 400);
    //sf::VideoMode screenMode = sf::VideoMode(600, 600);

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

    //Check for joystick!
    printf("Checking for joysticks...\n");
    int firststick = -1;
    for(int j=0;j<8;j++) 
        if (sf::Joystick::isConnected(j))
        {
            // joystick number 0 is connected
            printf("Stick %d is connected! button count is %u - axes ",j,sf::Joystick::getButtonCount(0));
            if(sf::Joystick::hasAxis(j,sf::Joystick::Axis::X)) printf("X ");
            if(sf::Joystick::hasAxis(j,sf::Joystick::Axis::Y)) printf("Y ");
            if(sf::Joystick::hasAxis(j,sf::Joystick::Axis::Z)) printf("Z ");
            if(sf::Joystick::hasAxis(j,sf::Joystick::Axis::R)) printf("R ");
            if(sf::Joystick::hasAxis(j,sf::Joystick::Axis::U)) printf("U ");
            if(sf::Joystick::hasAxis(j,sf::Joystick::Axis::V)) printf("V ");
            if(sf::Joystick::hasAxis(j,sf::Joystick::Axis::PovX)) printf("PovX ");
            if(sf::Joystick::hasAxis(j,sf::Joystick::Axis::PovY)) printf("PovY ");
            printf("\n");
            firststick = j;
            break;
        }

    // this will be a matter of calibration at some point
    float joystick_deadspot = 15.0; //stick goes 0..100 in each axis; if abs(move) < this, ignore
                                    //to avoid little jitter


    // END HW SETUP ================================================================================================================

    // KLUDGY WAY TO DO A SCENE GRAPH
    std::vector<sf::Drawable *> scene_objects;

    // // set up some stuff to draw
    // sf::CircleShape circy(50.f);
    // circy.setFillColor(sf::Color(100, 250, 50));
    // circy.setOrigin(0,0);
    // scene_objects.push_back(&circy);

    // // let's find out if you can transform VertexArrays
    // xfVertArray tri(sf::PrimitiveType::Triangles,3, nullptr);
    // sf::Vertex vert = sf::Vertex(sf::Vector2f(-50.0,-50.0),sf::Color(255,0,0));
    // tri.append(vert);
    // vert = sf::Vertex(sf::Vector2f(50.0,-50.0),sf::Color(0,255,0));
    // tri.append(vert);
    // vert = sf::Vertex(sf::Vector2f(0.0,50.0),sf::Color(0,0,255));
    // tri.append(vert);
    // tri.setPosition(150.0,150.0);
    // tri.setOrigin(0.0,0.0);
    // float trot = 30.0;
    // tri.setRotation(trot);
    // scene_objects.push_back(&tri);


    // so HERE instead of that we need to load up a map in GameTree format and turn it into SFML-usable, yes?
    // Still got a piece left to do for that, though a lot of it is directly usable
    SFMLMap the_map;

    //read in the map we want to load as a json string and 
    std::string map_filename = "demo_assets/outputs/DemoMap.tmx.json";

    if(!the_map.load_from_json_file(map_filename)) {
        printf("Failed loading map %s\n",map_filename.c_str());
        printf("(are you running from GameTree dir?)\n");
        exit(1);
    }

    // snag the Samurai lady sprite-bank
    std::shared_ptr<SFMLSpriteBank> samurai_sbank = std::shared_ptr<SFMLSpriteBank>(new SFMLSpriteBank());
    SFMLActor samurai;
    std::string sbank_filename = "demo_assets/outputs/Samurai.json_a2gt_out.json";

    if(!samurai_sbank->load_from_json_file(sbank_filename)) {
        printf("Failed loading sprite bank %s\n",sbank_filename.c_str());
        printf("(are you running from GameTree dir?)\n");
        exit(1);
    }

    samurai.set_sprite_bank(samurai_sbank);


    // MAP AND CHARACTER SETUP ----------------------------------------------------------------------------------------------------------------------
    // at some point will get character's initial position from a point in the InteractionObject layer like this one
    // <object id="72" name="character_start" type="start_point" x="198" y="409.5">
    //      <point/>
    // </object>

    //let's set layer width and height kludgily from map's first slayer
    float mapWidth = 0.0, mapHeight = 0.0;
    mapWidth = the_map.slayers[0]->get_bounding_box().width;
    mapHeight = the_map.slayers[0]->get_bounding_box().width;
    printf("Map width: %f height: %f\n",mapWidth,mapHeight);


    //init samurai frame & such - temp, data-drive
    samurai.current_character = 0;
    samurai.current_action = 0;
    samurai.current_direction = 2;
    samurai.current_frame = 0;

    // how fast she walks - need to data drive this too
    float samurai_velocity = 3.0;
    
    //initial just to see where this puts her - constant position onscreen, not relative to map.
    // will have to think about how to handle map coords vs. screen
    //samurai.setPosition(100.0,100.0);

    // world-relative - hand-entered values from "start_point" point in map data
    GTcoord_t samurai_world_x = 198;
    GTcoord_t samurai_world_y = 410;

    // minimum / maximum positions; could do better but let's just make it half a tile away from 
    // edges in x, 1 pixel up from the bottom in y, 20 pix down from top
    GTcoord_t min_samurai_world_x = 8;
    GTcoord_t max_samurai_world_x = mapWidth - 8;
    GTcoord_t min_samurai_world_y = 20;
    GTcoord_t max_samurai_world_y = mapHeight - 1;

    // center her onscreen; I don't think I need to scale pixels - oh, nope, do
    // also figure out how to make relative to a viewport
    float samurai_screen_x = (window.getSize().x / 2.0) / globalScaleX;
    float samurai_screen_y = (window.getSize().y / 2.0) / globalScaleY;


    //ok! now let's do a quick thing to add the map's layers to our scene objects and see what we get
    // then I'll just make layers drawable/transformable and that's what it takes
    // - that works
    // - Layers should all have their own separate transforms so you can do multiple parallax and stuff
    //   - similarly I don't think the map itself should be a drawable... or should it?
    // - should the map have a transform?
    // - that could be global_transform
    float layerPosX = 0.0, layerPosY = 0.0;
    //work out how to derive these from samurai's world position
    //recall that if we want the map to appear to move left, we move its position right
    // to center the character's hot spot, hm.
    // so - if character's world x and y were at 0,0, map would be positioned at
    // -chara_screen_x, -chara_screen_y, yes? let us try that - and no, that's not it
    // let's position the map's upper left corner (world 0,0) at middle of screen
    // ok, this does that:
    //layerPosX = ((window.getSize().x / 2.0) / globalScaleX);
    //layerPosY = ((window.getSize().y / 2.0) / globalScaleY);
    // how then to adjust for chara pos? This puts her at 10,20 in the map
    //layerPosX = ((window.getSize().x / 2.0) / globalScaleX) - 10;
    //layerPosY = ((window.getSize().y / 2.0) / globalScaleY) - 20;
    // THIS DOES IT!
    layerPosX = ((window.getSize().x / 2.0) / globalScaleX) - samurai_world_x;
    layerPosY = ((window.getSize().y / 2.0) / globalScaleY) - samurai_world_y;
    // that's good for an initial spot. Here reckon the max and min layer positions:
    // if(layerPosX < -(mapWidth-(window.getSize().x / globalScaleX))) layerPosX = -(mapWidth-(window.getSize().x / globalScaleX));
    // if(layerPosX > 0) layerPosX = 0;
    // if(layerPosY < -(mapHeight-(window.getSize().y / globalScaleY))) layerPosY = -(mapWidth-(window.getSize().y / globalScaleY));
    // if(layerPosY > 0) layerPosY = 0;
    // remember that max of 0 bc we move it "backwards" wrt how it appears to move onscreen
    float minLayerPosX = -(mapWidth-(window.getSize().x / globalScaleX));
    float maxLayerPosX = 0.0;
    float minLayerPosY = -(mapHeight-(window.getSize().y / globalScaleY));
    float maxLayerPosY = 0.0;
    // we'll check against those when moving herself around, but for now allow any position at start.


    // add the map layers in order by names: Background, BGOverlay, Gettables, <sprites go here>, Front
    // the "InteractionObject" layer should be handled differently... Should it be on the display list at all? Not yet
    // ***** ALSO CONSIDER SPECIAL HANDLING OF GETTABLES TO TURN THE OBJECT LIST THERE INTO A BUNCH OF SPRITES SO THEY
    // ***** CAN BE MOVED / HIDDEN INDEPENDENTLY!
    std::shared_ptr<SFMLMapLayer> lyr;
    for(std::string nam : {"Background", "BGOverlay", "Gettables"}) {
        lyr = the_map.get_sfml_layer_by_name(nam);
        if(lyr != nullptr) {
            lyr->setOrigin(0.0,0.0);              // WORK THIS OUT AT SOME POINT for now just draw from ul corner
            lyr->setPosition(layerPosX,layerPosY);            // same
            scene_objects.push_back(lyr.get());
        } else {
            printf("*** ERROR: couldn't find layer \"%s\"\n",nam.c_str());
        }
    }

    // sprites go here!
    scene_objects.push_back(&samurai);

    // then add the front layer
    std::string nam = "Front";
    lyr = the_map.get_sfml_layer_by_name(nam);
    if(lyr != nullptr) {
        lyr->setOrigin(0.0,0.0);              // WORK THIS OUT AT SOME POINT for now just draw from ul corner
        lyr->setPosition(layerPosX,layerPosY);            // same
        scene_objects.push_back(lyr.get());
    } else {
        printf("*** ERROR: couldn't find layer \"%s\"\n",nam.c_str());
    }



    // for map scrolling
    //float scroll_velocity = 5.0;
    // end MAP AND CHARACTER SETUP ------------------------------------------------------------------------------------------------------------------

    // MAIN LOOP =============================================================================================

    //initial clear for trails version
    window.clear(sf::Color::Black);


    // handle ongoing stuff like held down keys or stick 
    // NEED TO ACCOUNT FOR TIME ELAPSED BT FRAMES
    float deltaX, deltaY;
    deltaX = 0.0;
    deltaY = 0.0;

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

        deltaX = 0.0;
        deltaY = 0.0;

        if(firststick != -1) {
            float stick_x = sf::Joystick::getAxisPosition(firststick,sf::Joystick::Axis::X);
            float stick_y = sf::Joystick::getAxisPosition(firststick,sf::Joystick::Axis::Y);
            
            //move according to scaled joystick value
            //have a dead spot in the middle of the joystick so princess doesn't wander when stick
            //is released
            if(std::abs(stick_x) > joystick_deadspot) deltaX += ((stick_x / 100.0) * samurai_velocity);
            if(std::abs(stick_y) > joystick_deadspot) deltaY += ((stick_y / 100.0) * samurai_velocity);
        } 

        //if there was a stick handlement, skip keys? Or just do both
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) deltaX -= samurai_velocity;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) deltaX += samurai_velocity;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) deltaY -= samurai_velocity;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) deltaY += samurai_velocity;

        // if(deltaX != 0.0 || deltaY != 0.0) {
        //     printf("DeltaX %f Y %f\n",deltaX, deltaY);
        // }

        // OLD: just move the map around
        // // set layer positions according to map extents - currently , position/origin are both upper left corner
        // // turns out the deltas are how we're moving the VIEW, so layer position goes the other way
        // layerPosX -= deltaX;
        // layerPosY -= deltaY;

        // // bounding box is also backwards
        // if(layerPosX < -(mapWidth-(window.getSize().x / globalScaleX))) layerPosX = -(mapWidth-(window.getSize().x / globalScaleX));
        // if(layerPosX > 0) layerPosX = 0;
        // if(layerPosY < -(mapHeight-(window.getSize().y / globalScaleY))) layerPosY = -(mapWidth-(window.getSize().y / globalScaleY));
        // if(layerPosY > 0) layerPosY = 0;

        for(auto lyr: the_map.slayers) {
            lyr->setPosition(layerPosX,layerPosY);
        }

        //position character
        samurai.setPosition(samurai_screen_x,samurai_screen_y);




        // clear the window with black color
        // let's not do this anymore; assume tile map will do it? 
        // only if scroll is constrained s.t. nothing outside the map shows
        // leave it for debug purps
        window.clear(sf::Color::Black);

        // trot += 1.0;
        // tri.setRotation(trot);

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
