//sean adds - crunch main module functions brought out for use as a library
#ifndef CRUNCH_MAIN_MODULE_H_INCLUDED
#define CRUNCH_MAIN_MODULE_H_INCLUDED

#include <iostream>
#include <fstream>
#include <streambuf>
#include <sstream>
#include <string>
#include <vector>
#include <stdlib.h>
#include <algorithm>
#include "tinydir.h"
#include "bitmap.hpp"
#include "packer.hpp"
#include "binary.hpp"
#include "hash.hpp"
#include "str.hpp"


using namespace std;

extern int optSize;
extern int optPadding;
extern bool optXml;
extern bool optBinary;
extern bool optJson;
extern bool optPremultiply;
extern bool optTrim;
extern bool optVerbose;
extern bool optForce;
extern bool optUnique;
extern bool optRotate;
extern vector<Bitmap*> bitmaps;
extern vector<Packer*> packers;

void SplitFileName(const string& path, string* dir, string* name, string* ext);
string GetFileName(const string& path);
void LoadBitmap(const string& prefix, const string& path);
void LoadBitmaps(const string& root, const string& prefix);
void RemoveFile(string file);
int GetPackSize(const string& str);
int GetPadding(const string& str);
void init_crunch_options();     //sean adds


#endif 