#pragma once
#include <string>
#include <vector>

#include "natives.h"

namespace speech {

// Plays one ambient speech line with the own voice of the ped. Returns the result of the native.
bool say(Ped ped, const char* line, Hash speechParams, Ped listener);

// Picks random lines from the list (up to 4 tries) and plays the first one the
// voice of the ped has (DOES_CONTEXT_EXIST_FOR_THIS_PED). Falls back to the first pick.
bool sayOneOf(Ped ped, const std::vector<std::string>& lines, Hash speechParams, Ped listener,
              std::string* used = nullptr);

// Global throttle so a crowd does not shout in chorus: true if a new line may start now.
bool globalSlot(unsigned long now, unsigned long minGapMs);

} // namespace speech
