# see if I can use something like add_custom_command to do the build steps for the demo assets
# https://cmake.org/cmake/help/v3.5/command/add_custom_command.html
# example https://gist.github.com/socantre/7ee63133a0a3a08f3990
# old but prolly important https://samthursfield.wordpress.com/2015/11/21/cmake-dependencies-between-targets-and-files-and-custom-commands/
# currently this does nothing and subdirectory is not added to main build yet
cmake_minimum_required(VERSION 3.14)

# although, meanwhile, add_custom_target *always* runs - well, we'll figure it out
# recall that getting the json we want from an aseprite file looks like this:
# aseprite -b -v --trim --inner-padding 1 --sheet outputs/Npc_Generic_019_sheet.png --sheet-columns 4 --filename-format 'c{title}*a{outertag}*d{innertag}*f{tagframe00}*p{layer}' --format json-hash --data outputs/Npc_Generic_019.json --split-layers Npc_Generic_019.aseprite

# SET ALL THIS UP WHEN I FEEL LIKE DEALING WITH CMAKE STUFF
# figure out how to have clean clear out the outputs, too?
# for now let's have a build_assets.sh
