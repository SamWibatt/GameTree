#include "tiled2sfml.h"

using namespace tiledreader;

// NEW CONVERTER preceded by piddly support routines ===================================================================================================

std::string Tiled2SFML::get_name_for_bmp(tile_index_t gid) {
    char gidnum[9];
  sprintf(gidnum,"%04d",gid);
  return "g" + std::string(gidnum);    
}

// uses lots of crunch globals, ick - big chunk of its old main()
bool Tiled2SFML::do_crunch(std::string name,std::string outputDir) {
  //time for some raw crunch code
  //Sort the bitmaps by area
  //printf("- sorting bitmap list of %lu elements\n",bitmaps.size());
  sort(bitmaps.begin(), bitmaps.end(), [](const Bitmap* a, const Bitmap* b) {
      return (a->width * a->height) < (b->width * b->height);
  });
  
  //Pack the bitmaps
  //clear packers here in case we've done other layers

  packers.clear();
  //printf("- Packing bitmaps\n");
  while (!bitmaps.empty())
  {
      if (optVerbose)
          cout << "packing " << bitmaps.size() << " images..." << endl;
      auto packer = new Packer(optSize, optSize, optPadding);
      packer->Pack(bitmaps, optVerbose, optUnique, optRotate);
      packers.push_back(packer);
      if (optVerbose)
          cout << "finished packing: " << name << to_string(packers.size() - 1) << " (" << packer->width << " x " << packer->height << ')' << endl;
  
      if (packer->bitmaps.empty())
      {
          cerr << "packing failed, could not fit bitmap: " << (bitmaps.back())->name << endl;
          return false;
      }
  }
  
  //Save the atlas image
  //printf("- Saving atlas images\n");
  for (size_t i = 0; i < packers.size(); ++i)
  {
      if (optVerbose)
          cout << "writing png: " << outputDir << name << to_string(i) << ".png" << endl;
      packers[i]->SavePng(outputDir + name + to_string(i) + ".png");
  }

  //Save the atlas xml
  if (optXml)
  {
      //printf("- Saving atlas xml\n");
      if (optVerbose)
          cout << "writing xml: " << outputDir << name << ".xml" << endl;
      
      ofstream xml(outputDir + name + ".xml");
      xml << "<atlas>" << endl;
      for (size_t i = 0; i < packers.size(); ++i)
          packers[i]->SaveXml(name + to_string(i), xml, optTrim, optRotate);
      xml << "</atlas>";
  }
  
  //Save the atlas json
  if (optJson)
  {
      //printf("- Saving atlas json\n");

      if (optVerbose)
          cout << "writing json: " << outputDir << name << ".json" << endl;
      
      ofstream json(outputDir + name + ".json");
      json << '{' << endl;
      json << "\t\"textures\":[" << endl;
      for (size_t i = 0; i < packers.size(); ++i)
      {
          json << "\t\t{" << endl;
          packers[i]->SaveJson(name + to_string(i), json, optTrim, optRotate);
          json << "\t\t}";
          if (i + 1 < packers.size())
              json << ',';
          json << endl;
      }
      json << "\t]" << endl;
      json << '}';
  }
  return true;
}

//Emits a pair of Texture and atlas, where atlas is an unordered map from original gid -> atlas record above.
std::pair<sf::Texture,std::unordered_map<tile_index_t, atlas_record>> Tiled2SFML::MakeLayerTextureAndAtlas(std::shared_ptr<TiledMap> sptm, int layer_num) {
  sf::Texture layertex;
  std::unordered_map<tile_index_t,atlas_record> new_atlas;      //atlas of gid->atlas record for result
  std::unordered_map<tile_index_t,atlas_record> orig_atlas;     //gid -> atlas record for original
  std::shared_ptr<TiledMapLayer> sptml = sptm->layers[layer_num];

  // Steps appropriate to all layer types (so far):
  //Step 1: figure out which imagery we need. May not be all of it, esp if using 3rd party assets
  //        which gids participate in this layer? Easy enough, just build a set. std::set is sorted and iterable yay
  std::set<tile_index_t> unique_gids;
  //let's assume that layers encoded with ways other than csv have the same ultimate mapcells entry
  for(tile_index_t mc : sptml->mapcells) {
    //mask off rotation bits - they will still be there in the layer mapcells when we build the vertex array
    unique_gids.insert(mc & 0x00FFFFFF);
  }
  //debug
  //printf("Got these unique gids: "); for(tile_index_t gid : unique_gids) printf("%u ", gid); printf("\n");

  //Step 2. where does their imagery originate?
  //        need to map a gid to a region on a source image.
  //        read the images as crunch Bitmaps.
  // also need to build the set of original tileset images
  // ... only those we use, though.
  //indexed by pair(tileset number within map, image number within tileset)
  std::map<std::pair<int, int>,std::shared_ptr<Bitmap>> tileset_bitmaps;

  //printf("- Building bitmap list\n");

  for(tile_index_t gid:unique_gids) {
    if(gid != 0) {    //0 is special case blank tile, just skipped and no quad emitted, doesn't need imagery

      //so: we need to know which tileset it belongs to, which image within that, coords within the image.
      //sounds like a job for member functions of Tileset!
      bool foundgid = false;
      int imgnum = -1, ulx = -1, uly = -1, wid = -1, ht = -1;
      for(auto tsnum = 0; tsnum < sptm -> tilesets.size(); tsnum++) {
        if(sptm->tilesets[tsnum]->contains_gid(gid)) {
          // so tsnum is the tileset number within map that it belongs to
          foundgid = true;
          tie(imgnum, ulx, uly, wid, ht) = sptm->tilesets[tsnum]->image_num_and_coords_for_gid(gid);
          //printf("YAY gid %u is in tileset %u - image %d, ulx %d, uly %d, w %d, h %d\n",gid, tsnum,
          //        imgnum, ulx, uly, wid, ht);

          // so we need a registry of crunch Bitmaps that contain the entire tileset images from which these come, yes?
          // s.t. each is only loaded once.
          // so, map from tileset number/image number to Bitmap, yes? yes.
          std::pair<int,int> tsimg = std::make_pair(tsnum,imgnum);
          if(tileset_bitmaps.find(tsimg) == tileset_bitmaps.end()) {
            //hasn't been loaded yet, load that suckoir
            //hardcoding premultiply and trim to false
            string bmpname = "tileset_" + std::to_string(tsnum) + "_img_" + std::to_string(imgnum);
            //printf("Need to load bitmap %s from %s\n",bmpname.c_str(),sptm->tilesets[tsnum]->images[imgnum].image_source.c_str());
            tileset_bitmaps[tsimg] = 
              std::shared_ptr<Bitmap>(new Bitmap(sptm->tilesets[tsnum]->images[imgnum].image_source, bmpname, false, false));
            //Key-color transparentify it if it needs it!
            if(!sptm->tilesets[tsnum]->images[imgnum].image_trans.empty()) {
              tileset_bitmaps[tsimg]->Transparentify(sptm->tilesets[tsnum]->images[imgnum].image_trans);
            }
          }

          orig_atlas[gid] = atlas_record(gid, tsnum, imgnum, ulx, uly, wid, ht);

          break;
        }
      }
      if(!foundgid) {
        printf("ERROR! didn't find gid %u in any tileset! Dying!\n",gid);
        exit(1);
      }

    }
  }    




  //Step 3. Assemble those regions as "Bitmap" objects for crunch to arrange.
  bitmaps.clear();        //crunch's global 

  //set up crunch options
  init_crunch_options();
  //optVerbose = true;      //danger verbose lol
  //optXml = true;          //let's emit some xml
  //optJson = true;         //and INHERENTLY SUPERIOR LOL json

  //so what we need to put in there is one for every gid in unique_gids, yes?
  for(tile_index_t gid:unique_gids) {
    if(gid != 0) {      //skip the blank tile
      atlas_record oar = orig_atlas[gid];
      std::shared_ptr<Bitmap> bmp = tileset_bitmaps[std::make_pair(oar.tileset_index,oar.image_index)];

      //create bitmap from scratch
      Bitmap *nubmp = new Bitmap(oar.wid, oar.ht);
      nubmp->name = get_name_for_bmp(gid);    
      nubmp->CopyPixelRegion(bmp.get(), oar.ulx, oar.uly, oar.wid, oar.ht);
      bitmaps.push_back(nubmp);
      //printf("nubmp -> name = %s\n",bitmaps.back()->name.c_str());
    }
  }

  //Step 4. invoke crunch to produce a single bitmap we'll use as a texture for this layer.
  //        create texture from crunch's output bitmap
  //        say we die if there is more than one packer for now
  std::string name = sptml->layer_name + "_crunchout";
  std::string outputDir = "/home/sean/tmp/";    //get real one
  if(!do_crunch(name,outputDir)) {
    printf("ERROR crunch failed\n");
    //return std::make_pair(nullptr,nullptr);
    //WHAT IS OUR RETURN ERROR VALUE?
  }
  if(packers.size() > 1) {
    //hey we don't support this yet!
    printf("ERROR: ended up with %zu packers, hoped for 1 - deal with!\n",packers.size());
  }

  //Step 5. create atlas from crunch's atlas: map gid to region in new layer texture
  //how do we know that? I guess by looking up gid names somehow?
  //oh yes, packer's int FindPointsAndBitmapsIndexByName(string &name);      //sean adds - what does the return value mean? index into bitmaps[]
  //new_atlas[0] = atlas_record();
  for(tile_index_t gid: unique_gids) {
    if(gid != 0) {
      atlas_record atrec;
      atrec.gid = gid;
      atrec.image_index = 0;        //each layer currently only has one texture, so this is 0
      atrec.tileset_index = 0;      //meaningless in output, make 0
      string gidname = get_name_for_bmp(gid); 
      //index into bitmaps/points within packer
      int pkrbmpind = packers[0]->FindPointsAndBitmapsIndexByName(gidname);
      atrec.ulx = packers[0]->points[pkrbmpind].x;
      atrec.uly = packers[0]->points[pkrbmpind].y;
      atrec.wid = packers[0]->bitmaps[pkrbmpind]->width;
      atrec.ht = packers[0]->bitmaps[pkrbmpind]->height;
      new_atlas[gid] = atrec;
    }
  }

  //Step 6: make a texture out of the layer Bitmap
  //not happy re loading it from disk, but can fix later if care enough
  std::string layerpngname = outputDir + name + "0.png";    //hardcodey 0 bc packer number appended
  //printf("- Creating texture from %s\n",layerpngname.c_str());
  layertex.loadFromFile(layerpngname);


  //steps that differ by layer type (so far there isn't anything for this step!)
  /*
  if(sptml->type == TL_TiledLayer) {
    //...I don't think there are any!
  } else if(sptml->type == TL_ObjectLayer) {
    //process ogject layer!
    //printf("OH HEY NEED TO BUILD TEXTURE FOR OBJECT LAYER %s\n",sptml->layer_name.c_str());
  } else {
    printf("Unknown layer type! %d\n",sptml->type);
    exit(1);
  }
  */

  return std::make_pair(layertex,new_atlas);
}


//Given the texture and atlas, emits a single vertex array that represents the level.
sf::VertexArray Tiled2SFML::MakeLayerVertexArray(std::shared_ptr<TiledMap> sptm, int layer_num, sf::Texture layertex,
                                                  std::unordered_map<tile_index_t,atlas_record> atlas) {

  //printf("- Building vertex arrays\n");
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
