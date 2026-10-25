#include "keys.h"

#include <vector>

namespace keys {

struct KeyState {
    int vk = 0;
    bool down = false;
    bool pressedNow = false;
    bool repeatNow = false;
    DWORD downSince = 0;
    DWORD lastRepeat = 0;
};

static std::vector<KeyState> g_keys;
static const DWORD kRepeatDelay = 400;
static const DWORD kRepeatRate = 110;

static KeyState* find(int vk) {
    for (auto& k : g_keys)
        if (k.vk == vk) return &k;
    return nullptr;
}

void watch(int vk) {
    if (vk <= 0 || vk > 255 || find(vk)) return;
    KeyState s;
    s.vk = vk;
    g_keys.push_back(s);
}

void update() {
    DWORD now = GetTickCount();
    for (auto& k : g_keys) {
        bool isDown = (GetAsyncKeyState(k.vk) & 0x8000) != 0;
        k.pressedNow = isDown && !k.down;
        k.repeatNow = k.pressedNow;
        if (k.pressedNow) {
            k.downSince = now;
            k.lastRepeat = now;
        } else if (isDown && now - k.downSince >= kRepeatDelay && now - k.lastRepeat >= kRepeatRate) {
            k.repeatNow = true;
            k.lastRepeat = now;
        }
        k.down = isDown;
    }
}

bool down(int vk) { KeyState* k = find(vk); return k && k->down; }
bool pressed(int vk) { KeyState* k = find(vk); return k && k->pressedNow; }
bool repeat(int vk) { KeyState* k = find(vk); return k && k->repeatNow; }

} // namespace keys
