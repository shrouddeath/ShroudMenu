#pragma once
#include <windows.h>

// Keyboard polling (GetAsyncKeyState). update() once per frame, then ask
// pressed() (edge: fires once per press) or repeat() (edge plus auto-repeat
// while held, for menu navigation).
namespace keys {

void watch(int vk);     // register a key to be polled
void update();
bool down(int vk);
bool pressed(int vk);
bool repeat(int vk);

} // namespace keys
