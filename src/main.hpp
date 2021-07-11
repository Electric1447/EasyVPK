#pragma once

#include <psp2/ctrl.h>
#include <psp2/kernel/clib.h>
#include <psp2/sysmodule.h>
#include <vita2d.h>

#include "utils/json.hpp"


#define PARENT_URL "http://rinnegatamante.it/vitadb/"
#define HOMEBREW_URL PARENT_URL "list_hbs_json.php"

#define BLACK   RGBA8(  0,   0,   0, 255)
#define WHITE   RGBA8(255, 255, 255, 255)
#define LGREY   RGBA8(191, 191, 191, 255)
#define LGREY2  RGBA8(215, 215, 215, 255)
#define RED     RGBA8(255,   0,   0, 255)
#define GREEN   RGBA8(  0, 255,   0, 255)
#define BLUE    RGBA8(  0,   0, 255, 255)
#define YELLOW  RGBA8(255, 255,   0, 255)
#define PURPLE  RGBA8(109,   0, 252, 255)


using namespace std;
using json = nlohmann::json;


extern unsigned int basicfont_size;
extern unsigned char basicfont[];

struct SharedData {
	int scene = 0;
	int cursorY = 0;
	bool blockCross 	= false;
	bool blockSquare 	= false;
	bool blockCircle 	= false;
	bool blockTriangle 	= false;
	bool blockStart 	= false;
	bool blockSelect 	= false;
	bool blockLTrigger 	= false;
	bool blockRTrigger 	= false;
	bool initDetail  = true;
	int dl_type_sd = 0; // VPK
	string vpkDownloadPath = "ux0:VPK/";
	vector<vita2d_texture *> screenshots;
	json vpks;
	json original;
	vita2d_font *font = vita2d_load_font_mem(basicfont, basicfont_size);
};