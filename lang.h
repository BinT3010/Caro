#pragma once
/* ===================== LANG: chu hien thi theo ngon ngu =====================
 * Moi chu tren man hinh lay qua tr(S_...) thay vi viet thang trong code.
 * Them 1 cau moi: them 1 ma vao enum StrId duoi day, roi them ban dich cho
 * CA HAI ngon ngu trong lang.c (test_logic kiem tra khong thieu cau nao va
 * cac %d / %s cua 2 ban dich phai giong nhau).
 * Khong phu thuoc Windows (test duoc doc lap).
 */

enum { LANG_VI = 0, LANG_EN = 1, LANG_COUNT };

typedef enum {
    /* ---- menu ---- */
    S_MENU_NEW, S_MENU_LOAD, S_MENU_HISTORY, S_MENU_SETTINGS, S_MENU_EXIT,
    S_TITLE_NEW_GAME, S_MODE_2P, S_MODE_AI_EASY, S_MODE_AI_MEDIUM, S_MODE_AI_HARD, S_BACK,
    S_TITLE_TIME, S_TIME_5, S_TIME_10, S_TIME_15,
    S_HELP_MENU,
    /* ---- may (AI) ---- */
    S_AI_EASY, S_AI_MEDIUM, S_AI_HARD,
    S_AIMODE_EASY, S_AIMODE_MEDIUM, S_AIMODE_HARD, S_AIMODE_2P,
    /* ---- bang thong so (HUD) ---- */
    S_HUD_TIME_LEFT,        /* %02d %02d %s */
    S_HUD_TOTAL_TIME,       /* %02d %02d */
    S_HUD_LIMIT,            /* %d */
    S_HUD_TURN,             /* %c %s */
    S_HUD_MOVES,            /* %d */
    S_HUD_WINLOSS,          /* %d %d */
    S_HUD_DRAWS,            /* %d %d */
    S_HUD_KEYS_MOVE, S_HUD_KEYS_ENTER, S_HUD_KEYS_SAVE, S_HUD_KEYS_ESC,
    S_HUD_UNDO,             /* %d */
    /* ---- ket thuc van ---- */
    S_AI_WON,               /* %s */
    S_YOU_BEAT_AI,          /* %s %s */
    S_X_WON,                /* %s */
    S_O_WON,                /* %s */
    S_DRAW,
    S_TIMEOUT_PREFIX,
    S_END_REPLAY, S_END_MENU,
    /* ---- cai dat ---- */
    S_TITLE_SETTINGS,
    S_SET_SOUND,            /* %s */
    S_ON, S_OFF,
    S_SET_VOLUME,           /* %s %3d */
    S_SET_NAME1, S_SET_NAME2,   /* %s */
    S_SET_LANGUAGE,         /* %s */
    S_LANG_NAME,            /* ten ngon ngu nay: "Tiếng Việt" / "English" */
    S_SET_TEST_SOUND,
    S_HELP_SETTINGS1, S_HELP_SETTINGS2,
    S_PROMPT_NEW_NAME,
    /* ---- lich su ---- */
    S_TITLE_HISTORY,
    S_HIST_SUMMARY,         /* %d %d %d %d */
    S_HIST_COL_WHEN, S_HIST_COL_X, S_HIST_COL_O, S_HIST_COL_RESULT, S_HIST_COL_MOVES, S_HIST_COL_TIME,
    S_HIST_X_WIN, S_HIST_O_WIN, S_HIST_DRAW,
    S_HIST_EMPTY,
    S_HELP_HISTORY1, S_HELP_HISTORY2,
    S_CONFIRM_CLEAR_HISTORY,
    /* ---- luu / tai ---- */
    S_SAVE_LIST_TITLE, S_SAVE_LIST_EMPTY,
    S_SAVE_LIST_MORE,       /* %d */
    S_PROMPT_SAVE, S_PROMPT_LOAD,
    S_BAD_FILE_NAME_LONG, S_BAD_FILE_NAME,
    S_CONFIRM_OVERWRITE,
    S_SAVED,                /* %s */
    S_SAVE_FAILED,
    S_LOADED_FROM,          /* %s */
    S_LOAD_IO_ERROR, S_LOAD_BAD_FILE,
    S_LOADED_CONTINUE,
    S_TITLE_LOAD,
    /* ---- trong van ---- */
    S_ASK_LEAVE,
    S_AI_THINKING,
    S_UNDO_ONLY_AI, S_UNDO_NONE, S_UNDO_DONE,
    S_CELL_TAKEN,
    /* ---- dat ten ---- */
    S_TITLE_NAMES,
    S_NAMES_HELP,           /* %d */
    S_NAME_PROMPT,          /* %c %s */
    S_NAME_DONE,            /* %c */
    S_NAME_OPPONENT,
    S_NAME_DUPLICATE,
    S_DEFAULT_PLAYER,       /* ten khi bo trong hoan toan */
    S_DEFAULT_PLAYER1, S_DEFAULT_PLAYER2,
    S_COUNT
} StrId;

void langSet(int lang);         /* LANG_VI / LANG_EN; gia tri khac -> LANG_VI */
int  langGet(void);
const char *tr(StrId id);                      /* cau theo ngon ngu dang chon */
const char *trIn(int lang, StrId id);          /* cau theo ngon ngu bat ky (de test, doi ten mac dinh) */
const char *langCode(int lang);                /* "vi" / "en" (ghi vao file cai dat) */
int  langFromCode(const char *code);           /* "vi"/"en" -> LANG_...; khong ro -> LANG_VI */
