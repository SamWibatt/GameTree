//GameTree is a game-library-independent minimalist hierarchy of C++ classes for writing games.
//so far, 2D only.
//intended for use anywhere C++11 compiles, such as Linux / Mac / Windows computers,
//sufficiently powerful microcontrollers, etc.
//requires a 64 bit source of milliseconds?

#ifndef GAMETREE_H_INCLUDED
#define GAMETREE_H_INCLUDED

#include <cstdio>
#include <cstdint>
#include <memory>
#include <vector>
#include <map>
#include <functional>
//set up include dirs to have json/single_include
#include "nlohmann/json.hpp"
#include <fstream>

using json = nlohmann::json;

// just include a bunch of other includes
#include "gt_base.h"
#include "gt_sprite.h"
#include "gt_entity.h"
#include "gt_map.h"
#include "gt_view.h"

#endif