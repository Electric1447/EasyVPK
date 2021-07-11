#pragma once

#include <psp2/ime_dialog.h>

#include "json.hpp"


#define IME_DIALOG_RESULT_NONE 0
#define IME_DIALOG_RESULT_RUNNING 1
#define IME_DIALOG_RESULT_FINISHED 2
#define IME_DIALOG_RESULT_CANCELED 3


using namespace std;
using json = nlohmann::json;


int initImeDialog(char *title, char *initial_text, int max_text_length);
uint16_t *getImeDialogInputTextUTF16();
uint8_t *getImeDialogInputTextUTF8();
int isImeDialogRunning();
int updateImeDialog();

json filterJson(string filter, json original);
json moveToTopJson(string titleid, json original);

string toLowercase(string strToConvert);