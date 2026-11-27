#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS /* Visual Studio: bo canh bao fopen/sscanf */
#endif
#include <windows.h>
#include <stdlib.h>
#include <time.h>
#include "view.h"
#include "model.h"
#include "storage.h"
#include "sound.h"
#include "control.h"

int main(void) {
    SetConsoleTitleW(L"Cờ Caro");
    srand((unsigned)time(NULL));               /* AI chon ngau nhien giua cac nuoc ngang diem */
    fixConsole();                              /* co dinh kich thuoc cua so */
    resizeConsole(SCREEN_W, SCREEN_H);
    settingsLoad();                            /* doc am thanh + ten nguoi choi da luu */
    soundConfigure(settings.soundOn, settings.volume);
    resetSession();
    resetData();
    mainMenu();
    out();
    return 0;
}
