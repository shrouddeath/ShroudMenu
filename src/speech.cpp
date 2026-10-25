#include "speech.h"

#include <cstdlib>

#include "log.h"

namespace speech {

// Layout from the RDR3 NativeDB and LMS (rdr2mods.com), 0x40 bytes.
struct ScriptedSpeechParams {
    const char* speechName;
    const char* voiceName;
    alignas(8) int variation;
    alignas(8) Hash speechParamHash;
    alignas(8) Ped listenerPed;
    alignas(8) int syncOverNetwork;
    alignas(8) int v7;
    alignas(8) int v8;
};
static_assert(sizeof(ScriptedSpeechParams) == 0x40, "ScriptedSpeechParams must be 0x40 bytes");

static unsigned long g_lastGlobal = 0;

bool say(Ped ped, const char* line, Hash speechParams, Ped listener) {
    ScriptedSpeechParams p{};
    p.speechName = line;
    p.voiceName = nullptr;          // own voice of the ped
    p.variation = 0;                // 0 = random line of that context
    p.speechParamHash = speechParams;
    p.listenerPed = listener;
    p.syncOverNetwork = 1;
    p.v7 = 1;
    p.v8 = 1;
    bool ok = PLAY_PED_AMBIENT_SPEECH_NATIVE(ped, &p);
    logfile::debug("speech %s on ped %d params 0x%08X -> %d", line, ped, speechParams, ok ? 1 : 0);
    return ok;
}

bool sayOneOf(Ped ped, const std::vector<std::string>& lines, Hash speechParams, Ped listener, std::string* used) {
    if (lines.empty()) return false;
    const std::string* fallback = nullptr;
    for (int attempt = 0; attempt < 4; ++attempt) {
        const std::string& line = lines[static_cast<size_t>(rand()) % lines.size()];
        if (!fallback) fallback = &line;
        if (DOES_CONTEXT_EXIST_FOR_THIS_PED(ped, line.c_str())) {
            if (used) *used = line;
            return say(ped, line.c_str(), speechParams, listener);
        }
    }
    if (used) *used = *fallback;
    return say(ped, fallback->c_str(), speechParams, listener);
}

bool globalSlot(unsigned long now, unsigned long minGapMs) {
    if (now - g_lastGlobal < minGapMs) return false;
    g_lastGlobal = now;
    return true;
}

} // namespace speech
