#pragma once

// Localisation table. Strings live in assets/i18n/<lang>.ini, one flat
// key=value per line. The language comes from the game's own setting
// (FrontEndMenuManager.m_nPrefsLanguage), so the user configures it once in the
// GTA menu and 6HUD follows.
//
// Missing keys fall back to English, then to the key name itself, so a partial
// translation degrades instead of printing blanks.

namespace hud6 {

enum eI18nKey {
    I18N_GPS_TURN_LEFT = 0,
    I18N_GPS_TURN_RIGHT,
    I18N_GPS_BEAR_LEFT,
    I18N_GPS_BEAR_RIGHT,
    I18N_GPS_UTURN_LEFT,
    I18N_GPS_UTURN_RIGHT,
    I18N_GPS_CONTINUE,
    I18N_GPS_IN_KM,
    I18N_GPS_IN_M,
    I18N_GPS_IN_MI,
    I18N_GPS_IN_FT,
    I18N_GPS_NOW_M,
    I18N_GPS_NOW_FT,
    I18N_VEHICLE_FALLBACK,
    I18N_MENU_TAB_RADAR,
    I18N_MENU_TAB_GPS,
    I18N_MENU_TAB_CARDS,
    I18N_MENU_TAB_SEARCH,
    I18N_MENU_TAB_HEAT,
    I18N_MENU_TAB_SIRENS,
    I18N_MENU_TAB_ARCS,
    I18N_MENU_TAB_LAYOUT,
    I18N_COUNT
};

// Returns the localised string for `key`. Never null, never empty.
const char* I18n(eI18nKey key);

// Resolves the language from the game setting and loads its file. Safe to call
// more than once; reloads only when the resolved language actually changes.
void LoadI18n();

} // namespace hud6
