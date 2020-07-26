#include <cstdio>
#include <fstream>
#include <SFML/Graphics.hpp>
#include "gametree.h"
#include "gtree_sfml.h"


using namespace gt;
using namespace gtree_sfml;


// piddly support routines ########################################################################################################

// colors for filling or outlining collision shapes
sf::Color get_color_for_purpose(GTArea_type t, sf::Uint8 alpha) {
    switch(t) {
        case GTAT_NoGo: return sf::Color(255,0,0,alpha);      //red for nogo
        case GTAT_Slow: return sf::Color(0,0,255,alpha);      //blue for slow
        case GTAT_Trigger: return sf::Color(0,255,0,alpha);   //green for trigger
        default: return sf::Color(128,128,128,alpha);         //grey for unk
    }
}

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
    float samurai_velocity = 2.0;
    
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
    // the "InteractionObject" layer should be handled differently... Should it be on the display list at all? Not yet
    std::shared_ptr<GTMapLayer> int_layer = the_map.get_layer_by_name("InteractionObject");
    // can you do this? I hate that you have to - nother slicey thing
    std::shared_ptr<GTObjectsMapLayer> interaction_layer = std::static_pointer_cast<GTObjectsMapLayer>(int_layer);
    if(interaction_layer == nullptr) {
        printf("*** WARNING: no interaction layer found, won't do collisions\n");
    }

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

    // INTERACTION SHAPES! REFACTOR THIS KIND OF THING INTO SCENE GRAPH! ==============================================================================
    //on top of everything, put the collision shapes!
    //Ugh, I might need my own class for this that's both drawable and transformable
    //That's what SFML shape was about - The problem is that my SFMLVertArray isn't a sf::Shape, so I can't use that
    //I need both Drawable and Transformable, so I can't use either one as the type for this
    std::vector<std::shared_ptr<SFMLShape>> int_shapes;
    // super gross, here is a parallel array of the gtshapes so we can know the origins
    // GTShape should have some kind of stub for drawing that plat-spec implementations fill in ...? 
    // I guess get_geometry is the nod at it for now
    std::vector<std::shared_ptr<GTShape>> int_gtshapes;
    // being able to show or hide all these at once is just the sort of thing a scene graph node could do.
    for(auto shap: interaction_layer->shapes) {
        //get geometry from the shape - bounding box, list of points, etc.
        std::shared_ptr<std::vector<GTPoint>> geo = shap->get_geometry();
        std::shared_ptr<SFMLShape> nu_sfmlshape = nullptr;
        
        if(shap->get_shape_type() == GTST_Rectangle) {
            auto nu_rect = std::shared_ptr<sf::RectangleShape>(
                new sf::RectangleShape(
                    // SHOULD THIS ALLOW FOR AN OFFSET OF SOME KIND? I guess only screen position needs to, qv.
                    sf::Vector2f((*geo)[1].x - (*geo)[0].x, (*geo)[1].y - (*geo)[0].y)
                )
            );
            nu_rect->setFillColor(get_color_for_purpose(shap->purpose, 64));       //low-opacity fill
            nu_rect->setOutlineColor(get_color_for_purpose(shap->purpose, 255));   //opaque outline
            nu_rect->setOutlineThickness(-1.0);    //thin outline, inside shape
            //this is screen position! nu_shape->setPosition(shap->position.x, shap->position.y);
            nu_sfmlshape = std::shared_ptr<SFMLRectangle>(new SFMLRectangle(nu_rect));
        } else if(shap->get_shape_type() == GTST_Ellipse) {
            //make a circle s.t. its scale is 1 in the major axis and
            //minor/major in the minor axis
            float major_axis = std::max((*geo)[1].x,(*geo)[1].y);
            auto nu_circ = std::shared_ptr<sf::CircleShape>(new sf::CircleShape(major_axis/2.0));
            if((*geo)[1].x > (*geo)[1].y) {
                // x larger, so set Y scale to y/x
                nu_circ->scale(1.0, float((*geo)[1].y) / major_axis);
            } else {
                // y larger, so set X scale to x/y
                nu_circ->scale(float((*geo)[1].x) / major_axis,1.0);
            }
            nu_circ->setFillColor(get_color_for_purpose(shap->purpose, 64));       //low-opacity fill
            nu_circ->setOutlineColor(get_color_for_purpose(shap->purpose, 255));   //opaque outline
            nu_circ->setOutlineThickness(-1.0);    //thin outline, inside shape
            //this is screen position! nu_shape->setPosition(shap->position.x, shap->position.y);
            nu_sfmlshape = std::shared_ptr<SFMLCircle>(new SFMLCircle(nu_circ));
        } else if(shap->get_shape_type() == GTST_Polygon) {
            //argh, how to do this? I guess just connected lines atm
            //I don't want to do the whole triangulating thing again
            //sf::PrimitiveType pty, size_t nPoints, sf::Texture *tx
            auto nu_poly = std::shared_ptr<SFMLVertArray>(new SFMLVertArray(sf::PrimitiveType::LineStrip,geo->size()+1,nullptr));
            //fill in all the vertices from geometry, with color determined by shape's purpose
            sf::Color col = get_color_for_purpose(shap->purpose,255);       //opaque outline
            for(int i = 0; i < geo->size(); i++) {
                //let's just put all the points in verbatim
                nu_poly->set_vertex(i,sf::Vertex(sf::Vector2f((*geo)[i].x, (*geo)[i].y),col));
            }
            // one last vertex to close up the polyline
            nu_poly->set_vertex(geo->size(),sf::Vertex(sf::Vector2f((*geo)[0].x, (*geo)[0].y),col));
            nu_sfmlshape = nu_poly;
        } else if(shap->get_shape_type() == GTST_Point) {
            printf("*** WARNING: Point not yet supported, skipping\n");
            nu_sfmlshape = nullptr;
        } else {
            printf("*** WARNING: unknown shape type, skipping\n");
            nu_sfmlshape = nullptr;
        }

        //add  to scene graph
        if(nu_sfmlshape != nullptr) {
            // figure out the sfml shape's position wrt screen
            nu_sfmlshape->setPosition(shap->position.x - viewport_world_ulx, shap->position.y - viewport_world_uly);
            int_shapes.push_back(nu_sfmlshape);           // do we even need this? Yes, for adjusting positions onscreen
            int_gtshapes.push_back(shap);                 // oh man this is gross, we need proper classes and scene graph
            scene_objects.push_back(nu_sfmlshape.get());
        }
    }

    // end INTERACTION SHAPES! REFACTOR THIS KIND OF THING INTO SCENE GRAPH! ==========================================================================


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
                        // OK THAT WORKS BUT DOESN'T DO ANYTHING YET!
                        // ******************* WRITE THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                        // ******************* WRITE THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                        // ******************* WRITE THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                        // ******************* WRITE THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                        // ******************* WRITE THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                        // ******************* WRITE THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                        // ******************* WRITE THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
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
                            // we know that adding both x and y makes her be inside, so if ONE doesn't, assume the other does
                            if(!shap->inside(samurai_world_x, last_worldy)) {
                                samurai_world_y = last_worldy;
                            } //what if we made these not exclusive? Used to be an else here 
                            if(!shap->inside(last_worldy,samurai_world_y)) {
                                samurai_world_x = last_worldx;
                            } 
                            // else {
                            //     samurai_world_x = last_worldx;
                            //     samurai_world_y = last_worldy;
                            // }
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

                // printf("adxy: %d, %d view_world: %d,%d New samwpos: %d,%d sam_screen %d,%d lyrpos %f,%f\n",
                //     actual_delta_x,actual_delta_y,viewport_world_ulx,viewport_world_uly,
                //     samurai_world_x,samurai_world_y,samurai_screen_x,samurai_screen_y,layerPosX,layerPosY);

                // tweak onscreen positions of interaction shapes
                // using embarrasingly gross parallel arrays of sfml shapes and GTShapes
                // that's asking for a mad refactor
                for(int i = 0; i < int_shapes.size(); i++) {
                    auto ishape = int_shapes[i];
                    auto gshape = int_gtshapes[i];

                    // FIGURE OUT IF GIVEN SHAPE IS ONSCREEN AND DRAW IT IF SO
                    // first just draw it
                    // DOES THIS NEED TO ACCOUNT FOR AN OFFSET OF SOME SORT? Doesn't seem to!
                    // might need to if there's a rectangle or ellipse whose ulx isn't 0?
                    ishape->setPosition(gshape->position.x - viewport_world_ulx, gshape->position.y - viewport_world_uly);
                }
            }

        } else {
            // idle! 
            samurai.set_action("Idle");             // write methods that do this w/index
        }

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
