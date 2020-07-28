#include <cstdio>
#include <fstream>
#include <SFML/Graphics.hpp>
#include "gametree.h"
#include "gtree_sfml.h"


using namespace gt;
using namespace gtree_sfml;


// MAIN ###########################################################################################################################

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

    //timing! see https://www.sfml-dev.org/tutorials/2.5/system-time.php
    sf::Clock clock;

    // END HW SETUP ================================================================================================================

    // GLOBAL TIME OBJECT - can have multiple
    // these start out paused
    GTTime g_time;
    //g_time.unpause before any events get added
    //*** USE REAL UNPAUSE once it's writ
    g_time.paused = false;


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

    // map load & setup ----------------------------------------------------------------------------------------------------------

    // so HERE instead of that we need to load up a map in GameTree format and turn it into SFML-usable, yes?
    // Still got a piece left to do for that, though a lot of it is directly usable
    GTMap the_map;
    std::vector<std::shared_ptr<GTSFMapLayer>> slayers;         //figure out where to put this - sfml wrappers for the map layers, in order they were added to scene graph


    //read in the map we want to load as a json string and 
    std::string map_filename = "demo_assets/outputs/DemoMap.tmx.json";

    if(!the_map.load_from_json_file(map_filename)) {
        printf("Failed loading map %s\n",map_filename.c_str());
        printf("(are you running from GameTree dir?)\n");
        exit(1);
    }


    // Samurai sprite load & setup -----------------------------------------------------------------------------------------------
    // snag the Samurai lady sprite-bank
    std::shared_ptr<GTSpriteBank> samurai_gt_sbank = std::shared_ptr<GTSpriteBank>(new GTSpriteBank());
    std::string sbank_filename = "demo_assets/outputs/Samurai.json_a2gt_out.json";

    if(!samurai_gt_sbank->load_from_json_file(sbank_filename)) {
        printf("Failed loading sprite bank %s\n",sbank_filename.c_str());
        printf("(are you running from GameTree dir?)\n");
        exit(1);
    }

    //get sprite bank loaded before constructing an actor from it - does this help with Invisible Samurai bug? newp
    GTActor samurai(samurai_gt_sbank);

    //samurai.set_sprite_bank(samurai_sbank);

    //init samurai frame & such - temp, data-drive
    samurai.register_clock(g_time);

    samurai.current_character = samurai.sbank->info.character_to_index["Samurai"];          //MAKE A BETTER METHOD FOR THIS and error trap
    samurai.current_frame = 0;
    samurai.set_action("Idle");
    samurai.set_direction("R");
    
    samurai.active = true;

    // so far all of this sprite setup is pure gametree - now to make some SFML-specific objects
    GTSFActor samurai_gtsf_actor(&samurai);

    // MAP AND CHARACTER SETUP ----------------------------------------------------------------------------------------------------------------------
    // at some point will get character's initial position from a point in the InteractionObject layer like this one
    // <object id="72" name="character_start" type="start_point" x="198" y="409.5">
    //      <point/>
    // </object>

    //let's set layer width and height kludgily from map's first slayer
    float mapWidth = 0.0, mapHeight = 0.0;
    mapWidth = the_map.layers[0]->get_bounding_box().wid;
    mapHeight = the_map.layers[0]->get_bounding_box().ht;
    printf("Map width: %f height: %f\n",mapWidth,mapHeight);



    // how fast she walks - need to data drive this too
    float samurai_velocity = 1.0;       //was 2.0
    
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
    GTcoord_t samurai_screen_x = round((window.getSize().x / 2.0) / globalScaleX);
    GTcoord_t samurai_screen_y = round((window.getSize().y / 2.0) / globalScaleY);

    // let's represent everything in world coordinates as much as we can.
    // we have to start with screen coords if we want her centered - but do we?
    // there should be a function that, given her world coordinates, figures out the appropriate
    // screen coords.
    // perhaps a GTviewport class?
    // anyway, figure out what the window's presence is in world coordinates.
    GTcoord_t viewport_world_width = window.getSize().x / globalScaleX;
    GTcoord_t viewport_world_height = window.getSize().y / globalScaleY;
    GTcoord_t viewport_world_ulx = samurai_world_x - (viewport_world_width / 2);
    GTcoord_t viewport_world_uly = samurai_world_y - (viewport_world_height / 2);


    // then set up scroll margins so she can wander around a bit before the screen starts scrolling - assume symmetrical?
    // nah, have separate, like the max world y is
    GTcoord_t viewport_world_top_scroll_margin = 84;            // 4 tiles + rough character height
    GTcoord_t viewport_world_bottom_scroll_margin = 64;         // 4 tiles
    GTcoord_t viewport_world_left_scroll_margin = 64 + 8;       // 4 tiles + about half a character width
    GTcoord_t viewport_world_right_scroll_margin = 64 + 8;      // 4 tiles + about half a character width


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
    layerPosX = ((window.getSize().x / 2.0) / globalScaleX) - samurai_world_x;
    layerPosY = ((window.getSize().y / 2.0) / globalScaleY) - samurai_world_y;
    // that's good for an initial spot. Here reckon the max and min layer positions:
    // remember that max of 0 bc we move it "backwards" wrt how it appears to move onscreen
    float minLayerPosX = -(mapWidth-(window.getSize().x / globalScaleX));
    float maxLayerPosX = 0.0;
    float minLayerPosY = -(mapHeight-(window.getSize().y / globalScaleY));
    float maxLayerPosY = 0.0;
    // we'll check against those when moving herself around, but for now allow any position at start.

    printf("Viewport ul: %d, %d dims: %d x %d lyrpos %f, %f\n",viewport_world_ulx,viewport_world_uly,viewport_world_width,viewport_world_height,
        layerPosX,layerPosY);


    // add the map layers in order by names: Background, BGOverlay, Gettables, <sprites go here>, Front

    // ***** ALSO CONSIDER SPECIAL HANDLING OF GETTABLES TO TURN THE OBJECT LIST THERE INTO A BUNCH OF SPRITES SO THEY
    // ***** CAN BE MOVED / HIDDEN INDEPENDENTLY!
    std::shared_ptr<GTMapLayer> lyr;
    for(std::string nam : {"Background", "BGOverlay", "Gettables"}) {
        lyr = the_map.get_layer_by_name(nam);
        if(lyr != nullptr) {

            // ok HERE have to create the wrappers for the map layers
            auto slyr = std::shared_ptr<GTSFMapLayer>(new GTSFMapLayer(lyr.get()));

            slyr->setOrigin(0.0,0.0);              // WORK THIS OUT AT SOME POINT for now just draw from ul corner
            slyr->setPosition(layerPosX,layerPosY);            // same
            scene_objects.push_back(slyr.get());
            slayers.push_back(slyr);
        } else {
            printf("*** ERROR: couldn't find layer \"%s\"\n",nam.c_str());
        }
    }

    // sprites go here!
    scene_objects.push_back(&samurai_gtsf_actor);
    //scene_objects.push_back(samurai_gtsf_actor.spr.get());      //hm

    //temp, put a sprite with whole sheet texture there - looks good
    // Sprite s;
    // s.setTexture(samurai_gtsf_actor.sfsb->spritesheet);
    // scene_objects.push_back(&s);

    // printf("s texture: %lu tr: (%d,%d - w%d,h%d) org: %f,%f\n",s.getTexture(),
    //     s.getTextureRect().left, s.getTextureRect().top, s.getTextureRect().width, s.getTextureRect().height,
    //     s.getOrigin().x, s.getOrigin().y);


    // then add the front layer
    std::string nam = "Front";
    lyr = the_map.get_layer_by_name(nam);
    if(lyr != nullptr) {
        auto slyr = std::shared_ptr<GTSFMapLayer>(new GTSFMapLayer(lyr.get()));
        slyr->setOrigin(0.0,0.0);              // WORK THIS OUT AT SOME POINT for now just draw from ul corner
        slyr->setPosition(layerPosX,layerPosY);            // same
        scene_objects.push_back(slyr.get());
        slayers.push_back(slyr);
    } else {
        printf("*** ERROR: couldn't find layer \"%s\"\n",nam.c_str());
    }

    // the "InteractionObject" layer should be handled differently... Should it be on the display list at all? Not yet
    std::shared_ptr<GTMapLayer> interaction_layer = the_map.get_layer_by_name("InteractionObject");
    std::shared_ptr<GTSFMapLayer> interaction_slayer;
    // can you do this? I hate that you have to - nother slicey thing
    if(interaction_layer == nullptr) {
        printf("*** WARNING: no interaction layer found, won't do collisions\n");
    } else {
        auto slyr = std::shared_ptr<GTSFMapLayer>(new GTSFMapLayer(interaction_layer.get()));
        slyr->setOrigin(0.0,0.0);              // WORK THIS OUT AT SOME POINT for now just draw from ul corner
        slyr->setPosition(layerPosX,layerPosY);            // same
        scene_objects.push_back(slyr.get());
        slayers.push_back(slyr);
        interaction_slayer = slyr;
    }
    // what about scene graph? Let's not put them on there... do them in loop down below

    // for map scrolling
    //float scroll_velocity = 5.0;
    // end MAP AND CHARACTER SETUP ------------------------------------------------------------------------------------------------------------------

    // MAIN LOOP =============================================================================================

    //initial clear for trails version
    //window.clear(sf::Color::Black);


    // handle ongoing stuff like held down keys or stick 
    // NEED TO ACCOUNT FOR TIME ELAPSED BT FRAMES
    float deltaX, deltaY;
    deltaX = 0.0;
    deltaY = 0.0;

    // run the program as long as the window is open
    while (window.isOpen())
    {
        //timing stuff see https://www.sfml-dev.org/tutorials/2.5/system-time.php
        sf::Time elapsed = clock.restart();
        //RIDICULOUSLY VERBOSE DEBUG
        //this ranges from 1 to 19, with the large majority being 16
        //printf("frame elapsed millis = %d\n",elapsed.asMilliseconds());

        //so, we need to update the global event clock
        //updateGame(elapsed);
        g_time.advance_time(GTtimestamp_t(elapsed.asMilliseconds()));

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

        // so: if samurai is within the screen bounding box, let her just move onscreen
        // ELSE if the screen hasn't scrolled all the way in the direction it needs to go, scroll
        // ELSE let her move to her world min/max
        //... the better way would be to do all the reckoning in world coordinates and derive screen from that
        // only bother if at least one delta is nonzero bc that will affect which action she's using
        if(deltaX != 0.0 || deltaY != 0.0) {
            //printf("DeltaX %f Y %f\n",deltaX, deltaY);

            // all right, move her! Will correct if it doesn't work
            GTcoord_t last_worldx = samurai_world_x;
            GTcoord_t last_worldy = samurai_world_y;
            samurai_world_x += deltaX;
            samurai_world_y += deltaY;

            // HERE is where to put collisions, yes?
            // if she collided with something, how far do we move her? or do we just leave her where she was?
            // if it's a rectangle, easy to figure out where the intersection is
            // otherwise I'm not so sure
            // would be better to clip against the shape, say something is moving really fast like 100 pix per frame (and doesn't zoom right over the whole thing)
            // we'd want it to stop at the boundary
            // I guess it's a matter of finding the intersection of the shape and the vector from current position to the new position
            // WRITE THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            // first pass, just detect and light up shapes she's colliding with.
            if(interaction_layer != nullptr) {
                for(auto shap: interaction_layer->shapes) {
                    if(shap->inside(samurai_world_x, samurai_world_y)) {
                        //collide!
                        //printf("Collision with shape of type %d\n",shap->purpose);
                        // I added a shap->get_shape_type() method that returns GTST_Unknown, GTST_Rectangle, GTST_Ellipse,
                        // GTST_Polygon, or GTST_Point... but that doesn't tell me much.
                        // considering a get_geometry method that returns a vector of GTPoint; rects return ul/lr or ul/hw,
                        // ellipses same and it's treated like the ellipse that fits in that bbox (kinda gross but ok),
                        // polygons the list of points...
                        // perhaps the better way to handle it would be to ... extend those shape classes with SFML versions,
                        // that have ctors from the GT kind, 
                        // rebuild shape objects in the map layers as the SFML kind...???
                        // that's nice if we want to draw them, so might be worth doing
                        // Let's just do the get_geometry thing, all the platspec back ends will need them
                        // ok it is now written
                        // better still, just have a finer-grained collision function that can do that intersection of
                        // the motion vector with the shape's surface (intersection closest to the beginning spot!)
                        // YES LET US DO THAT
                        // OK found a graphics gems thing to do it BUT since our motion currently is like 2 pixels let's
                        // try just having her stop if she hits a nogo
                        // other problem with this is that it won't allow her to slide along the edge of the nogo like
                        // she does along the sides of the screen - but see how it looks
                        // cheap way around it - if moving her just in x makes her not be in there, keep the x
                        // if moving her in y makes her not be in there, keep the y
                        // this almost works but we're getting some stuck-in-walls
                        if(shap->purpose == GTAT_NoGo) {
                            // this seems to work for rectangles and ellipses...?
                            if(shap->get_shape_type() == GTST_Rectangle || shap->get_shape_type() == GTST_Ellipse) {
                                // we know that adding both x and y makes her be inside, so if ONE doesn't, assume the other does
                                if(!shap->inside(samurai_world_x, last_worldy)) {
                                    samurai_world_y = last_worldy;
                                } //what if we made these not exclusive? Used to be an else here - oh, that makes it so that
                                // you can punch through vertically bc now samurai_world_y is previous y. 
                                //Does that still happen if we restore the else? yup
                                else if(!shap->inside(last_worldy,samurai_world_y)) {
                                    samurai_world_x = last_worldx;
                                } 
                            }
                            else {
                                // for polygons, just stop them cold for now
                                // icky, but better than stuck in a wall
                                samurai_world_x = last_worldx;
                                samurai_world_y = last_worldy;
                            }
                        } else {
                            //other purposes: trigger, slow, etc.
                            //HAVE THIS BE DATA DRIVEN RATHER THAN HARDCODEY ENUMS
                            // ******************* WRITE THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                            // ******************* WRITE THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                            // ******************* WRITE THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                            // ******************* WRITE THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                            // ******************* WRITE THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                            // ******************* WRITE THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                            // ******************* WRITE THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

                        }
                        
                    }
                }
            }
            

            //clamp to world bounds
            if(samurai_world_x < min_samurai_world_x) samurai_world_x = min_samurai_world_x;
            if(samurai_world_y < min_samurai_world_y) samurai_world_y = min_samurai_world_y;
            if(samurai_world_x > max_samurai_world_x) samurai_world_x = max_samurai_world_x;
            if(samurai_world_y > max_samurai_world_y) samurai_world_y = max_samurai_world_y;

            GTcoord_t actual_delta_x = samurai_world_x - last_worldx;
            GTcoord_t actual_delta_y = samurai_world_y - last_worldy;

            // Determine action/ direction according to whether she *really* moved
            if(actual_delta_x != 0 || actual_delta_y != 0) {
                samurai.set_action("Walk");
                //so - if moving rightward and x delta >= y delta, right
                if(actual_delta_x > 0 && actual_delta_x >= abs(actual_delta_y)) {
                    samurai.set_direction("R");
                } else if(actual_delta_x < 0 && abs(actual_delta_x) >= abs(actual_delta_y)) {
                    samurai.set_direction("L");
                } else if(actual_delta_y >= 0) {      //favor down to up so she's facing us
                    samurai.set_direction("D");
                } else {
                    samurai.set_direction("U");
                }

                // if her new position is within the (world coord) scroll box, let her move and all's well
                // so - if she's NOT, we need to do something about scroll position, IF WE CAN!
                // memorize last viewport world ulx & y so we don't change anything when we don't need to
                // I think there's some rockiness bc of that
                if(samurai_world_x < viewport_world_ulx + viewport_world_left_scroll_margin) {
                    //she's off the left edge of the scroll box
                    // so: if the view can move left, have it do so. 
                    viewport_world_ulx -= (viewport_world_ulx + viewport_world_left_scroll_margin) - samurai_world_x;
                    if(viewport_world_ulx < 0) viewport_world_ulx = 0;
                } else if(samurai_world_x > viewport_world_ulx + viewport_world_width - viewport_world_right_scroll_margin) {
                    //she's off the right edge of the scroll box
                    // so: if the view can move right, have it do so. 
                    viewport_world_ulx += samurai_world_x - (viewport_world_ulx + viewport_world_width - viewport_world_right_scroll_margin);
                    if(viewport_world_ulx > mapWidth - viewport_world_width) viewport_world_ulx = mapWidth - viewport_world_width;
                }

                if(samurai_world_y < viewport_world_uly + viewport_world_top_scroll_margin) {
                    //she's off the top edge of the scroll box
                    // so: if the view can move down, have it do so. 
                    viewport_world_uly -= (viewport_world_uly + viewport_world_top_scroll_margin) - samurai_world_y;
                    if(viewport_world_uly < 0) viewport_world_uly = 0;
                } else if(samurai_world_y > viewport_world_uly + viewport_world_height - viewport_world_bottom_scroll_margin) {
                    //she's off the bottom edge of the scroll box
                    // so: if the view can move down, have it do so. 
                    viewport_world_uly += samurai_world_y - (viewport_world_uly + viewport_world_height - viewport_world_bottom_scroll_margin);
                    if(viewport_world_uly > mapHeight - viewport_world_height) viewport_world_uly = mapHeight - viewport_world_height;
                }

                // OK so derive the layers' position from the viewport position
                layerPosX = -viewport_world_ulx;
                layerPosY = -viewport_world_uly;

                // then derive samurai onscreen position from her world position relative to the viewport, yes?
                samurai_screen_x = (samurai_world_x - viewport_world_ulx);
                samurai_screen_y = (samurai_world_y - viewport_world_uly);
            } else {
                //since she didn't actually move, make her idle
                samurai.set_action("Idle");             // write methods that do this w/index
            }

        } else {
            // idle! 
            samurai.set_action("Idle");             // write methods that do this w/index
        }

        // position layers!
        for(auto lyr: slayers) {
            lyr->setPosition(layerPosX,layerPosY);
        }

        //position character - platspec bc it's rendering-related
        samurai_gtsf_actor.setPosition(samurai_screen_x,samurai_screen_y);

        // clear the window with black color
        // let's not do this anymore; assume tile map will do it? 
        // only if scroll is constrained s.t. nothing outside the map shows
        // leave it for debug purps
        window.clear(sf::Color::Black);

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
