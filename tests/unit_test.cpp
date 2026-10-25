// Offline checks for the parts that do not need the game: joaat hashes against
// values from the RDR3 NativeDB / femga lists, key parsing, ini parsing, the
// speech struct layout.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

#include "config.h"

static int g_failed = 0;

#define CHECK(cond)                                                                   \
    do {                                                                              \
        if (!(cond)) {                                                                \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                   \
            g_failed++;                                                               \
        }                                                                             \
    } while (0)

// Same layout as speech.cpp; natives.h cannot be included without the SDK.
struct ScriptedSpeechParams {
    const char* speechName;
    const char* voiceName;
    alignas(8) int variation;
    alignas(8) uint32_t speechParamHash;
    alignas(8) int listenerPed;
    alignas(8) int syncOverNetwork;
    alignas(8) int v7;
    alignas(8) int v8;
};
static_assert(sizeof(ScriptedSpeechParams) == 0x40, "ScriptedSpeechParams must be 0x40 bytes");
static_assert(offsetof(ScriptedSpeechParams, speechParamHash) == 24, "speech param hash at offset 24");
static_assert(offsetof(ScriptedSpeechParams, v8) == 56, "v8 at offset 56");

int main() {
    // joaat (values cross-checked with a Python implementation and the femga weapon list)
    CHECK(joaat("REL_COP") == 0x2A318608u);
    CHECK(joaat("PLAYER") == 0x6F0783F5u);
    CHECK(joaat("rel_civmale") == 0x8A33CDCFu);
    CHECK(joaat("WEAPON_MELEE_KNIFE") == 0xDB21AC8Cu);
    CHECK(joaat("WEAPON_REVOLVER_CATTLEMAN") == 0x169F59F7u);
    CHECK(joaat("ADD_REASON_DEFAULT") == 0x2CD419DCu);
    CHECK(joaat("s_m_m_valdeputy_01") == 0xF4B4127Au);
    CHECK(joaat("u_m_m_valsheriff_01") == 0x93B09465u);
    CHECK(joaat(std::string("PLAYER")) == 0x6F0783F5u);

    // speech params (femga audio_banks README table)
    CHECK(parseSpeechParams("force") == 0xF85DD899u);
    CHECK(parseSpeechParams("force_shouted_critical") == 0xFBDA0604u);
    CHECK(parseSpeechParams("beat_shouted_clear") == 0x67F3AB43u);
    CHECK(parseSpeechParams("SPEECH_PARAMS_STANDARD") == 0x3CA9FB81u);
    CHECK(parseSpeechParams("lms") == 0xE7176FB2u);
    CHECK(parseSpeechParams("0x12345678") == 0x12345678u);
    CHECK(parseSpeechParams("") == 0xF85DD899u);

    // keys
    CHECK(parseKey("F6") == 0x75);
    CHECK(parseKey("f1") == 0x70);
    CHECK(parseKey("F24") == 0x87);
    CHECK(parseKey("Numpad8") == 0x68);
    CHECK(parseKey("numpad0") == 0x60);
    CHECK(parseKey("a") == 'A');
    CHECK(parseKey("5") == '5');
    CHECK(parseKey("0x23") == 0x23);
    CHECK(parseKey("none") == 0);
    CHECK(parseKey("garbage") == 0);
    CHECK(keyName(0x75) == "F6");
    CHECK(keyName(0x68) == "Numpad8");
    CHECK(keyName('A') == "A");

    // defaults
    {
        Config c = loadConfig("Z:\\does\\not\\exist");
        CHECK(c.enabled);
        CHECK(c.keyMenu == 0x75);
        CHECK(c.settings.radius == 60);
        CHECK(c.settings.excludeLawmen);
        CHECK(!c.settings.includeAnimals);
        CHECK(c.settings.drunkLevel == 2);
        CHECK(c.insultLines.size() == 11);
        CHECK(c.panicLines.size() == 8);
        CHECK(c.weapons.size() == 2);
        CHECK(c.insultSpeechParams == 0xF85DD899u);
        CHECK(c.panicSpeechParams == 0xFBDA0604u);
        CHECK(c.textBackendBg);
    }

    // ini parsing
    const char* dir = getenv("TEMP");
    std::string tmp = dir ? dir : ".";
    {
        std::ofstream f(tmp + "\\ShroudMenu.ini");
        f << "; comment\n[General]\nRadius=90 ; metres\nShowOverlay=0\nLogLevel=2\n"
          << "[Keys]\nMenu=F7\nUp=Up\n"
          << "[Filter]\nExcludeLawmen=0\nIncludeAnimals=yes\n"
          << "[Drunk]\nLevel=0\nLines=DRUNK, GENERIC_CURSE_MED\n"
          << "[Insult]\nSpeechParams=0xE7176FB2\nDistanceM=8.5\n"
          << "[Aggressive]\nGiveWeapons=1\nWeapons=WEAPON_MELEE_KNIFE\nAccuracy=250\n"
          << "[UI]\nTextBackend=legacy\n";
    }
    {
        Config c = loadConfig(tmp);
        CHECK(c.settings.radius == 90);
        CHECK(!c.settings.showOverlay);
        CHECK(c.logLevel == 2);
        CHECK(c.keyMenu == 0x76);
        CHECK(c.keyUp == 0x26);
        CHECK(c.keyDown == 0x62);
        CHECK(!c.settings.excludeLawmen);
        CHECK(c.settings.includeAnimals);
        CHECK(c.settings.drunkLevel == 0);
        CHECK(c.drunkLines.size() == 2 && c.drunkLines[1] == "GENERIC_CURSE_MED");
        CHECK(c.insultSpeechParams == 0xE7176FB2u);
        CHECK(c.insultDistance > 8.4f && c.insultDistance < 8.6f);
        CHECK(c.settings.giveWeapons);
        CHECK(c.weapons.size() == 1);
        CHECK(c.accuracy == 100);
        CHECK(!c.textBackendBg);
    }
    remove((tmp + "\\ShroudMenu.ini").c_str());

    // state round trip
    {
        Settings s;
        s.radius = 120;
        s.includeAnimals = true;
        s.drunkLevel = 1;
        s.giveWeapons = true;
        s.showOverlay = false;
        saveSettings(tmp, s);
        Settings loaded = loadSettings(tmp, Settings{});
        CHECK(loaded.radius == 120);
        CHECK(loaded.includeAnimals);
        CHECK(loaded.drunkLevel == 1);
        CHECK(loaded.giveWeapons);
        CHECK(!loaded.showOverlay);
        CHECK(loaded.excludeLawmen);
        remove((tmp + "\\ShroudMenu.state").c_str());
        Settings none = loadSettings(tmp, s);
        CHECK(none.radius == 120);
    }

    CHECK(std::string(modeName(Mode::Drunk)) == "Drunk");
    CHECK(std::string(modeName(Mode::Robbery)) == "Robbery");
    CHECK(std::string(modeName(Mode::Lasso)) == "Lasso");
    {
        Config c = loadConfig("Z:\\does\\not\\exist");
        CHECK(c.robRobberLines.size() == 3 && c.robVictimLines.size() == 5);
        CHECK(c.lassoLassoerLines.size() == 3 && !c.lassoHogtie && !c.lassoTargetFlees);
        CHECK(c.lassoThrowDistance > 4.9f && c.lassoThrowDistance < 5.1f);
    }
    CHECK(splitList(" a, b ,,c ").size() == 3);

    if (g_failed) {
        printf("%d check(s) failed\n", g_failed);
        return 1;
    }
    printf("all checks passed\n");
    return 0;
}
