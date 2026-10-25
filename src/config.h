#pragma once
// Settings: ShroudMenu.ini (read at start) and ShroudMenu.state (menu changes,
// written by the mod). No SDK dependency, so the unit test can build this alone.
#include <cstdint>
#include <string>
#include <vector>

using Hash = uint32_t;

// Rockstar "one at a time" hash (joaat), same as GET_HASH_KEY: lower-cases the input.
constexpr Hash joaat(const char* s) {
    uint32_t h = 0;
    for (; *s; ++s) {
        uint32_t c = static_cast<unsigned char>(*s);
        if (c >= 'A' && c <= 'Z') c += 32;
        h += c;
        h += h << 10;
        h ^= h >> 6;
    }
    h += h << 3;
    h ^= h >> 11;
    h += h << 15;
    return h;
}
Hash joaat(const std::string& s);

// "F6", "Numpad8", "Up", "Enter", "Backspace", "A", "5", "0x75" ... 0 = none/unknown.
int parseKey(const std::string& text);
std::string keyName(int vk);
// "force", "force_shouted_critical", ... -> joaat("SPEECH_PARAMS_<NAME>"); "0x..." passes through; "lms" = 0xE7176FB2
Hash parseSpeechParams(const std::string& text);
std::vector<std::string> splitList(const std::string& text);
std::string trim(const std::string& s);
std::string lower(const std::string& s);

enum class Mode { Off = 0, Drunk, Insult, Aggressive, Panic, Count };
const char* modeName(Mode m);

// Values the user can change in the menu. Persisted in ShroudMenu.state.
struct Settings {
    int radius = 60;              // metres
    bool includeAnimals = false;  // Panic and glass jaw only
    bool excludeLawmen = true;
    int drunkLevel = 2;           // 0 slightly, 1 moderately, 2 very
    bool drunkStumble = true;
    bool giveWeapons = false;
    bool disableLaw = false;
    bool bumpTrigger = false;
    bool noRetaliation = false;
    bool showOverlay = true;
};

struct Config {
    bool enabled = true;
    int scanIntervalMs = 500;
    int logLevel = 1;
    Settings settings;

    // keys (virtual key codes)
    int keyMenu = 0x75;   // F6
    int keyUp = 0x68;     // Numpad 8
    int keyDown = 0x62;   // Numpad 2
    int keyLeft = 0x64;   // Numpad 4
    int keyRight = 0x66;  // Numpad 6
    int keySelect = 0x65; // Numpad 5
    int keyBack = 0x60;   // Numpad 0

    // drunk
    float drunkLevels[3] = {0.25f, 0.5f, 1.0f};
    int drunkStumbleMinSec = 10, drunkStumbleMaxSec = 25;
    int drunkStumbleRagdollType = 0;
    int drunkStumbleMinMs = 700, drunkStumbleMaxMs = 1400;
    bool drunkSpeech = true;
    int drunkSpeechMinSec = 10, drunkSpeechMaxSec = 30;
    Hash drunkSpeechParams = 0;
    bool drunkMotivation = false;
    std::vector<std::string> drunkLines;

    // insult
    float insultDistance = 12.0f;
    int insultMinSec = 6, insultMaxSec = 15;
    bool insultFaceTarget = true;
    bool insultNeedLos = true;
    Hash insultSpeechParams = 0;
    std::vector<std::string> insultLines;

    // aggressive
    std::vector<std::string> weapons;
    int accuracy = 15;
    int combatRetaskSec = 4;

    // glass jaw
    int ragdollMinMs = 1500, ragdollMaxMs = 2500;
    int ragdollType = 0;
    float pushForce = 6.0f;
    bool healAfterHit = true;

    // panic
    bool panicSpeech = true;
    int panicMinSec = 5, panicMaxSec = 12;
    Hash panicSpeechParams = 0;
    float panicSpeed = 3.0f;
    int panicFleeType = 0;
    std::vector<std::string> panicLines;

    // ui
    bool textBackendBg = true;
    float menuX = 0.02f, menuY = 0.12f, menuWidth = 0.27f, textScale = 0.38f;
};

Config loadConfig(const std::string& dir);
Settings loadSettings(const std::string& dir, const Settings& defaults);
void saveSettings(const std::string& dir, const Settings& s);
