#include "tiled2sfml.h"

using namespace tiledreader;

// NEW CONVERTER preceded by piddly support routines ===================================================================================================

//Given the texture and atlas, emits a single vertex array that represents the level.
sf::VertexArray Tiled2SFML::MakeLayerVertexArray(std::shared_ptr<TiledMap> sptm, int layer_num, sf::Texture layertex,
                                                  std::unordered_map<tile_index_t,atlas_record> atlas) {

  //printf("- Building vertex arrays\n");

  //**************************************************************************************************************************************
  //**************************************************************************************************************************************
  //**************************************************************************************************************************************
  // HEY LET'S USE TRIANGLE STRIPS INSTEAD OF QUADS
  // no worse for object layers
  // lots fewer verts for contiguous tiled bits of tiles the same size
  // no-tile index 0 means start a new strip
  // make strips no longer than screen-tile-width tiles wide, then have no worse than (2hw + 2w) tiles to clip/draw?
  // or do it by height, whichever seems better
  // see https://github.com/SamWibatt/GameTree/issues/5
  //**************************************************************************************************************************************
  //**************************************************************************************************************************************
  //**************************************************************************************************************************************

  sf::VertexArray layerverts;
  std::shared_ptr<TiledMapLayer> sptml = sptm->layers[layer_num];
  layerverts.setPrimitiveType(sf::Quads);
  tile_index_t tileNumber;
  tile_index_t gid;

  if(sptml->type == TL_TiledLayer) {

    //figure this out
    int tileSizeX = sptm->layers[layer_num]->tile_height;
    int tileSizeY = sptm->layers[layer_num]->tile_width;

    // populate the vertex array, with one quad per tile
    for (unsigned int x = 0; x < sptml->layer_w; ++x) {
      for (unsigned int y = 0; y < sptml->layer_h; ++y) {
        //// get a pointer to the current tile's quad
        //sf::Vertex* quad = &layerverts[(x + y * sptm->layers[layer_num]->layer_w) * 4];
        //new way, only build the quad if gid is nonzero
        // get the current tile number
        tileNumber = sptml->mapcells[x + y * sptml->layer_w];
        gid = tileNumber & 0x00FFFFFF;      //shear off rot flags      

        if(gid != 0) {      //ignore blank tile
          sf::Vertex *quad = new sf::Vertex[4];

          // define its 4 corners 
          //quad coords are still right for me, at least the upper left corner
          if(atlas[gid].wid == tileSizeX && atlas[gid].ht == tileSizeY) {
            //ordinary sized tile, draw as normal
            quad[0].position = sf::Vector2f(x * tileSizeX, y * tileSizeY);
            quad[1].position = sf::Vector2f((x + 1) * tileSizeX, y * tileSizeY);
            quad[2].position = sf::Vector2f((x + 1) * tileSizeX, (y + 1) * tileSizeY);
            quad[3].position = sf::Vector2f(x * tileSizeX, (y + 1) * tileSizeY);
          } else {
            //NON-STANDARD TILES like artist palette in example singlescreen
            //draw them such that their lower left is the quad's lower left but extent is full size of image
            //so quad[3], lower left, is the same as before
            quad[3].position = sf::Vector2f(x * tileSizeX, (y + 1) * tileSizeY);
            quad[0].position = sf::Vector2f(x * tileSizeX, ((y + 1) * tileSizeY) - atlas[gid].ht); //upper left
            quad[1].position = sf::Vector2f((x * tileSizeX) + atlas[gid].wid, ((y + 1) * tileSizeY) - atlas[gid].ht); //upper right
            quad[2].position = sf::Vector2f((x * tileSizeX) + atlas[gid].wid, (y + 1) * tileSizeY); //lower right
          }

          //set texture coordinates
          //try clockwise to avoid weird slanty thing - works!
          quad[0].texCoords = sf::Vector2f(atlas[gid].ulx, atlas[gid].uly);
          quad[1].texCoords = sf::Vector2f(atlas[gid].ulx + atlas[gid].wid, atlas[gid].uly);
          quad[2].texCoords = sf::Vector2f(atlas[gid].ulx + atlas[gid].wid, 
                atlas[gid].uly + atlas[gid].ht);
          quad[3].texCoords = sf::Vector2f(atlas[gid].ulx, atlas[gid].uly + atlas[gid].ht);

          //append quad to layerverts
          for(auto j=0;j<4;j++) layerverts.append(quad[j]);
        }
      }
    }
  } else if(sptml->type == TL_ObjectLayer) {
    //just create a quad from each of sptml->tile_objects
    for(auto objy : sptml->tile_objects) {
      tileNumber = objy->gid;
      gid = tileNumber &0x00FFFFFF;       //mask off rotations - does this happen in these layers?
      if(gid != 0) {      //ignore blank tiles - shouldn't come up here, but who knows
        sf::Vertex *quad = new sf::Vertex[4];

        //set position coordinates - clockwise
        //ok, these seem to go by lower left, not upper left - by default, anyway
        //so call it origin x/y and for now assume it's lower left
        quad[0].position = sf::Vector2f(objy->orx, objy->ory - objy->ht);
        quad[1].position = sf::Vector2f(objy->orx + objy->wid, objy->ory - objy->ht);
        quad[2].position = sf::Vector2f(objy->orx + objy->wid, objy->ory);
        quad[3].position = sf::Vector2f(objy->orx, objy->ory);

        //set texture coordinates
        quad[0].texCoords = sf::Vector2f(atlas[gid].ulx, atlas[gid].uly);
        quad[1].texCoords = sf::Vector2f(atlas[gid].ulx + atlas[gid].wid, atlas[gid].uly);
        quad[2].texCoords = sf::Vector2f(atlas[gid].ulx + atlas[gid].wid, 
              atlas[gid].uly + atlas[gid].ht);
        quad[3].texCoords = sf::Vector2f(atlas[gid].ulx, atlas[gid].uly + atlas[gid].ht);

        //append quad's vertices to layerverts
        for(auto j=0;j<4;j++) layerverts.append(quad[j]);
      }

    }
  } else {
    printf("Unknown layer type %d\n",sptml->type);
    exit(1);
  }
  //printf("- # layer vertices: %lu\n",layerverts.getVertexCount());
  return layerverts;
}



std::shared_ptr<SFMLMap> Tiled2SFML::ConvertTiledMapToSFMLMap(std::shared_ptr<TiledMap> ptm, std::string outputDir) {
  std::shared_ptr<SFMLMap> mappy = std::shared_ptr<SFMLMap>(new SFMLMap);

  //so the old version just did stuff - now let's focus on what we want to get out.
  //from the SFMLMap definition:
  //let's start with one texture per layer
  //std::vector<sf::Texture> layer_tiletexs;
  //let's just start with one vertex array per layer. 
  //std::vector<sf::VertexArray> layer_vert_arrays;

  //so: we want to loop by layers, adding a tile texture and vertex array per layer to the SMFLMap
  for(auto layer_num = 0; layer_num < ptm->layers.size(); layer_num++) {
    std::shared_ptr<TiledMapLayer> sptml = ptm ->layers[layer_num];
    printf("- Processing layer: %s\n",sptml->layer_name.c_str());

    //Create layer texture and atlas
    sf::Texture layer_texture;
    std::unordered_map<tile_index_t,atlas_record> atlas;
    tie(layer_texture, atlas) = MakeLayerTextureAndAtlas(ptm,layer_num);

    //create vertex array from those
    sf::VertexArray layer_vertex_array = MakeLayerVertexArray(ptm,layer_num,layer_texture,atlas);

    mappy->layer_vert_arrays.push_back(layer_vertex_array);
    mappy->layer_tiletexs.push_back(layer_texture);
  }
  return mappy;
}
