#include "scanner.h"

#include <cmath>

// Default relationship groups on the side of the player (femga rdr3_discoveries, AI/RELATIONSHIP).
static const Hash kProtectedGroups[] = {
    joaat("PLAYER"), joaat("REL_GANG_DUTCHS"), joaat("REL_GANG_DUTCHS_HORSES"), joaat("REL_PLAYER_ALLY"),
    joaat("REL_COMPANION_GROUP"), joaat("REL_PLAYER_LIKE"),
};

// Lawmen and guards by default relationship group ...
static const Hash kLawGroups[] = {
    joaat("COP"), joaat("REL_COP"), joaat("REL_PLAYER_COP"), joaat("REL_PINKERTONS"), joaat("REL_GUAMA_LAW"),
    joaat("REL_BOUNTY_HUNTER"), joaat("REL_GUARD"), joaat("REL_GUARD_ASB"), joaat("REL_GUARD_BRONTE"),
    joaat("REL_GREYS_GUARD"), joaat("REL_BRAITH_GUARD"),
};

// ... and by model, for the ambient law peds that sit in civilian groups (femga peds_list).
static const Hash kLawModels[] = {
    joaat("s_m_m_ambientblwpolice_01"), joaat("s_m_m_ambientlawrural_01"), joaat("s_m_m_ambientsdpolice_01"),
    joaat("s_m_m_army_01"), joaat("s_m_y_army_01"), joaat("s_m_m_dispatchlawrural_01"),
    joaat("s_m_m_dispatchleaderpolice_01"), joaat("s_m_m_dispatchpolice_01"), joaat("s_m_m_marshallsrural_01"),
    joaat("s_m_m_pinlaw_01"), joaat("s_m_m_valdeputy_01"), joaat("s_m_m_tumdeputies_01"),
    joaat("u_m_m_rhdsheriff_01"), joaat("u_m_m_rhdbackupdeputy_01"), joaat("u_m_m_rhdbackupdeputy_02"),
    joaat("u_m_m_sdpolicechief_01"), joaat("u_m_m_strsherriff_01"), joaat("u_m_m_valsheriff_01"),
    joaat("u_m_o_asbsheriff_01"), joaat("u_m_o_blwpolicechief_01"), joaat("s_m_m_valbankguards_01"),
    joaat("s_m_m_unitrainguards_01"), joaat("s_m_m_racrailguards_01"), joaat("s_m_m_nbxriverboatguards_01"),
    joaat("s_m_m_cornwallguard_01"), joaat("s_m_m_hofguard_01"), joaat("s_m_m_micguard_01"),
    joaat("s_m_m_orpguard_01"), joaat("s_m_m_skpguard_01"), joaat("u_m_m_sdbankguard_01"),
    joaat("a_m_m_unicoachguards_01"),
};

bool isProtectedGroup(Hash group) {
    for (Hash g : kProtectedGroups)
        if (g == group) return true;
    return false;
}

bool isLawOrGuardGroup(Hash group, Hash model) {
    for (Hash g : kLawGroups)
        if (g == group) return true;
    for (Hash m : kLawModels)
        if (m == model) return true;
    return false;
}

float distanceBetween(const Vector3& a, const Vector3& b) {
    float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

ScanResult scanPeds(Ped player, const Settings& s, Mode mode, float radius, float exitRadius,
                    const std::unordered_set<Ped>& tracked) {
    ScanResult r;
    static int handles[1024];
    int count = worldGetAllPeds(handles, 1024);
    r.total = count;

    Vector3 playerPos = GET_ENTITY_COORDS(player);
    Ped playerMount = IS_PED_ON_MOUNT(player) ? GET_MOUNT(player) : 0;
    int playerGroup = GET_PLAYER_GROUP(PLAYER_ID());
    bool animalsAllowed = s.includeAnimals && (mode == Mode::Panic || mode == Mode::Off);

    for (int i = 0; i < count; ++i) {
        Ped ped = handles[i];
        if (ped == player || ped == playerMount || ped == 0) continue;
        if (!DOES_ENTITY_EXIST(ped) || IS_ENTITY_DEAD(ped) || IS_PED_DEAD_OR_DYING(ped)) continue;
        if (IS_PED_A_PLAYER(ped)) continue;

        Vector3 pos = GET_ENTITY_COORDS(ped);
        float dist = distanceBetween(pos, playerPos);
        bool wasTracked = tracked.count(ped) > 0;
        if (dist > radius && !(wasTracked && dist <= exitRadius)) continue;

        bool human = IS_PED_HUMAN(ped);
        if (!human && !animalsAllowed) { r.skippedAnimal++; continue; }
        if (IS_ENTITY_A_MISSION_ENTITY(ped)) { r.skippedMission++; continue; }
        if (IS_PED_IN_ANY_VEHICLE(ped)) continue;

        Hash group = GET_PED_RELATIONSHIP_GROUP_DEFAULT_HASH(ped);
        if (isProtectedGroup(group) || (playerGroup != 0 && IS_PED_GROUP_MEMBER(ped, playerGroup))) {
            r.skippedFriend++;
            continue;
        }
        if (s.excludeLawmen && human && isLawOrGuardGroup(group, GET_ENTITY_MODEL(ped))) { r.skippedLaw++; continue; }

        r.eligibleAny++;
        if (dist <= radius) r.inside.push_back(ped);
        else r.outside.push_back(ped);
    }
    return r;
}
