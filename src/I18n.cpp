#include <windows.h>
#include <cstdio>
#include <cstring>

#include <CMenuManager.h>

#include "Config.h"
#include "I18n.h"

namespace hud6 {

namespace {

// Key names as they appear in the INI. Indexed by eI18nKey, so the two must stay
// in the same order.
const char* const kKeyNames[I18N_COUNT] = {
    "GPS_TURN_LEFT",
    "GPS_TURN_RIGHT",
    "GPS_BEAR_LEFT",
    "GPS_BEAR_RIGHT",
    "GPS_UTURN_LEFT",
    "GPS_UTURN_RIGHT",
    "GPS_CONTINUE",
    "GPS_IN_KM",
    "GPS_IN_M",
    "GPS_IN_MI",
    "GPS_IN_FT",
    "GPS_NOW_M",
    "GPS_NOW_FT",
    "VEHICLE_FALLBACK",
    "MENU_TAB_RADAR",
    "MENU_TAB_GPS",
    "MENU_TAB_CARDS",
    "MENU_TAB_SEARCH",
    "MENU_TAB_HEAT",
    "MENU_TAB_SIRENS",
    "MENU_TAB_ARCS",
    "MENU_TAB_LAYOUT",
};

// English defaults, also the fallback when a key is missing from the active
// language file. Keep in sync with assets/i18n/en.ini.
const char* const kEnglishDefaults[I18N_COUNT] = {
    "Turn Left",
    "Turn Right",
    "Bear Left",
    "Bear Right",
    "Make U-Turn Left",
    "Make U-Turn Right",
    "Continue",
    "In %.1fkm",
    "In %.0fm",
    "In %.2fmi",
    "In %.0fft",
    "0m",
    "0ft",
    "Vehicle",
    "Radar",
    "GPS",
    "Cards",
    "Search",
    "Heat",
    "Sirens",
    "Arcs/Snitch",
    "Layout",
};

char s_strings[I18N_COUNT][96] = {};
bool s_loaded = false;
int  s_loadedLanguage = -1;

// Maps the game's language index to the file name. The game only ships five
// languages, so anything outside that range uses English.
const char* LanguageFileName(int languageIndex) {
    switch (languageIndex) {
        case 1:  return "fr";
        case 2:  return "de";
        case 3:  return "it";
        case 4:  return "es";
        default: return "en";
    }
}

void GetI18nPath(char* outPath, size_t outSize, const char* language) {
    char dir[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, dir, MAX_PATH);
    char* slash = strrchr(dir, '\\');
    if (slash) *slash = '\0';
    snprintf(outPath, outSize, "%s\\6hud\\assets\\i18n\\%s.ini", dir, language);
}

// Reads one key from the language file. Returns false when the key is absent so
// the caller can fall back to English.
bool ReadLocalised(const char* path, const char* key, char* out, size_t outSize) {
    DWORD read = GetPrivateProfileStringA("i18n", key, "", out, (DWORD)outSize, path);
    return read > 0 && out[0] != '\0';
}

} // namespace

void LoadI18n() {
    // m_nPrefsLanguage is fixed before the game starts, so this resolves once.
    int language = (int)(unsigned char)FrontEndMenuManager.m_nPrefsLanguage;
    if (s_loaded && language == s_loadedLanguage) return;

    const char* languageName = LanguageFileName(language);
    char path[MAX_PATH] = {};
    GetI18nPath(path, sizeof(path), languageName);
    bool fileExists = GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;

    for (int i = 0; i < I18N_COUNT; ++i) {
        char value[96] = {};
        if (fileExists && ReadLocalised(path, kKeyNames[i], value, sizeof(value))) {
            strncpy_s(s_strings[i], sizeof(s_strings[i]), value, _TRUNCATE);
        } else {
            strncpy_s(s_strings[i], sizeof(s_strings[i]), kEnglishDefaults[i], _TRUNCATE);
        }
    }

    s_loaded = true;
    s_loadedLanguage = language;
    LogWrite("i18n: loaded language '%s'%s", languageName, fileExists ? "" : " (file missing, using English)");
}

const char* I18n(eI18nKey key) {
    if (key < 0 || key >= I18N_COUNT) return "";
    if (!s_loaded) LoadI18n();
    return s_strings[key];
}

} // namespace hud6
