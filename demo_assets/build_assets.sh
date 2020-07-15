#!/usr/bin/env bash

# TEMPORARY ASSET BUILD FILE until I figure out how to get cmake to do its thing OR for people who don't want to use cmake to see how this works
# RUN THIS FROM DEMO_ASSETS DIRECTORY

echo Building characters...
# maybe put this in a loop and parameterize out the "Npc_Generic_019"
# no longer using this one
#aseprite -b -v --trim --inner-padding 1 --sheet outputs/Npc_Generic_019_sheet.png --sheet-columns 4 --filename-format 'c{title}*a{outertag}*d{innertag}*f{tagframe00}*p{layer}' --format json-hash --data outputs/Npc_Generic_019.json --split-layers Npc_Generic_019.aseprite

echo Building map...
../build/tiledreader/tiled2gt -i Tiled/DemoMap.tmx -o outputs/