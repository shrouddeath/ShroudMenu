#pragma once
#include <unordered_set>
#include <vector>

#include "config.h"
#include "natives.h"

struct ScanResult {
    std::vector<Ped> inside;    // eligible and inside the radius
    std::vector<Ped> outside;   // eligible, already tracked, between radius and exit radius
    int total = 0;
    int eligibleAny = 0;
    int skippedMission = 0;
    int skippedLaw = 0;
    int skippedFriend = 0;
    int skippedAnimal = 0;
};

// Which peds a mode may touch: never the player, the player group or gang,
// mission entities (story protection), peds in vehicles, and optionally lawmen/guards.
bool isLawOrGuardGroup(Hash defaultGroup, Hash model);
bool isProtectedGroup(Hash defaultGroup);

ScanResult scanPeds(Ped player, const Settings& s, Mode mode, float radius, float exitRadius,
                    const std::unordered_set<Ped>& tracked);

float distanceBetween(const Vector3& a, const Vector3& b);
