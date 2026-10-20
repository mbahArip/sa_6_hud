/*
    GTA SA Plugin SDK — Robust INI Configuration Reader & Writer
    Demonstrates: Reading/writing INI files with nested section hierarchies ([group.submodule]),
                  handling types (bool, int, float, string),
                  why Win32 API is preferred over plugin::config_file (which strips '[').
    Compatibility: Windows 95 through Windows 11 (kernel32.dll built-in).
*/
#include <plugin.h>
#include <windows.h>
#include <string>

using namespace plugin;

/*
    RECOMMENDED INI STRUCTURE:
    Use dot notation in section headers to emulate nesting without parser overhead:

    ; mod_config.ini
    [minimap.radar]
    use_radar = 1
    use_tilt = 1
    zoom_level = 1.25

    [minimap.place]
    font = 3
    align = 0
    text_color = 255, 255, 255, 200

    [minimap.gps]
    show_on_menu = 0
*/

class IniConfig {
private:
    char m_filePath[MAX_PATH];

public:
    IniConfig(const char* relativeFileName) {
        // NOTE: Win32 INI functions require an ABSOLUTE path.
        // A relative path without leading ".\" will look in C:\Windows\ instead!
        sprintf_s(m_filePath, "%s", PLUGIN_PATH(relativeFileName));
    }

    // --- READ HELPERS ---

    int ReadInt(const char* section, const char* key, int defaultVal) const {
        return GetPrivateProfileIntA(section, key, defaultVal, m_filePath);
    }

    bool ReadBool(const char* section, const char* key, bool defaultVal) const {
        return GetPrivateProfileIntA(section, key, defaultVal ? 1 : 0, m_filePath) != 0;
    }

    float ReadFloat(const char* section, const char* key, float defaultVal) const {
        char buf[64];
        char defaultBuf[64];
        sprintf_s(defaultBuf, "%f", defaultVal);

        GetPrivateProfileStringA(section, key, defaultBuf, buf, sizeof(buf), m_filePath);
        return static_cast<float>(atof(buf));
    }

    std::string ReadString(const char* section, const char* key, const char* defaultVal) const {
        char buf[512];
        GetPrivateProfileStringA(section, key, defaultVal, buf, sizeof(buf), m_filePath);
        return std::string(buf);
    }

    // --- WRITE HELPERS ---

    void WriteInt(const char* section, const char* key, int value) const {
        char buf[32];
        sprintf_s(buf, "%d", value);
        WritePrivateProfileStringA(section, key, buf, m_filePath);
    }

    void WriteBool(const char* section, const char* key, bool value) const {
        WritePrivateProfileStringA(section, key, value ? "1" : "0", m_filePath);
    }

    void WriteFloat(const char* section, const char* key, float value) const {
        char buf[32];
        sprintf_s(buf, "%.3f", value);
        WritePrivateProfileStringA(section, key, buf, m_filePath);
    }

    void WriteString(const char* section, const char* key, const char* value) const {
        WritePrivateProfileStringA(section, key, value, m_filePath);
    }
};

// --- USAGE DEMO ---

struct ModSettings {
    struct {
        bool  useRadar   = true;
        bool  useTilt    = true;
        float zoomLevel  = 1.25f;
    } radar;

    struct {
        int font  = 3;
        int align = 0;
    } place;

    struct {
        bool showOnMenu = false;
    } gps;

    void Load() {
        IniConfig ini("mod_config.ini");

        // [minimap.radar]
        radar.useRadar  = ini.ReadBool("minimap.radar", "use_radar", true);
        radar.useTilt   = ini.ReadBool("minimap.radar", "use_tilt", true);
        radar.zoomLevel = ini.ReadFloat("minimap.radar", "zoom_level", 1.25f);

        // [minimap.place]
        place.font  = ini.ReadInt("minimap.place", "font", 3);
        place.align = ini.ReadInt("minimap.place", "align", 0);

        // [minimap.gps]
        gps.showOnMenu = ini.ReadBool("minimap.gps", "show_on_menu", false);
    }

    void Save() {
        IniConfig ini("mod_config.ini");

        ini.WriteBool("minimap.radar", "use_radar", radar.useRadar);
        ini.WriteBool("minimap.radar", "use_tilt", radar.useTilt);
        ini.WriteFloat("minimap.radar", "zoom_level", radar.zoomLevel);

        ini.WriteInt("minimap.place", "font", place.font);
        ini.WriteInt("minimap.place", "align", place.align);

        ini.WriteBool("minimap.gps", "show_on_menu", gps.showOnMenu);
    }
} gSettings;

class IniConfigDemo {
public:
    IniConfigDemo() {
        Events::initGameEvent += [] {
            gSettings.Load();
        };
    }
} iniDemo;
