#include "ui.h"

#include "keys.h"
#include "natives.h"

namespace ui {

static bool g_bg = true;
static float g_scale = 0.38f;

void init(bool bgBackend, float textScale) {
    g_bg = bgBackend;
    g_scale = textScale;
}

void text(const std::string& s, float x, float y, Rgba c, float scale) {
    if (s.empty()) return;
    const char* var = VAR_STRING_LITERAL(s.c_str());
    if (g_bg) {
        float sc = scale > 0.0f ? scale : g_scale;
        _BG_SET_TEXT_SCALE(sc, sc);
        _BG_SET_TEXT_COLOR(c.r, c.g, c.b, c.a);
        _BG_DISPLAY_TEXT(var, x, y);
    } else {
        _SET_TEXT_COLOR_LEGACY(c.r, c.g, c.b, c.a);
        _DISPLAY_TEXT_LEGACY(var, x, y);
    }
}

void rect(float left, float top, float width, float height, Rgba c) {
    DRAW_RECT(left + width * 0.5f, top + height * 0.5f, width, height, c.r, c.g, c.b, c.a);
}

void MenuController::open() {
    m_open = true;
    m_stack.clear();
}

void MenuController::close() {
    m_open = false;
    m_stack.clear();
}

void MenuController::beep() { PLAY_SOUND_FRONTEND("NAV_RIGHT", "HUD_SHOP_SOUNDSET"); }

void MenuController::input(int up, int down, int left, int right, int select, int back) {
    Menu* m = current();
    if (!m || m->items.empty()) return;
    const int n = static_cast<int>(m->items.size());

    // Arrow keys / Enter / Backspace always work as a second layout while the menu is open.
    bool goUp = keys::repeat(up) || keys::repeat(0x26);
    bool goDown = keys::repeat(down) || keys::repeat(0x28);
    bool goLeft = keys::repeat(left) || keys::repeat(0x25);
    bool goRight = keys::repeat(right) || keys::repeat(0x27);
    bool doSelect = keys::pressed(select) || keys::pressed(0x0D);
    bool doBack = keys::pressed(back) || keys::pressed(0x08);

    if (goUp) {
        m->cursor = (m->cursor + n - 1) % n;
        beep();
    } else if (goDown) {
        m->cursor = (m->cursor + 1) % n;
        beep();
    }
    Item& it = m->items[m->cursor];
    if (goLeft || goRight) {
        int dir = goRight ? 1 : -1;
        if ((it.kind == ItemKind::Choice || it.kind == ItemKind::Number || it.kind == ItemKind::Toggle) && it.change) {
            it.change(dir);
            beep();
        }
    }
    if (doSelect) {
        switch (it.kind) {
        case ItemKind::Submenu:
            if (it.submenu) { m_stack.push_back(it.submenu); beep(); }
            break;
        case ItemKind::Toggle:
            if (it.change) it.change(1);
            else if (it.select) it.select();
            beep();
            break;
        case ItemKind::Choice:
        case ItemKind::Number:
            if (it.change) { it.change(1); beep(); }
            break;
        case ItemKind::Action:
            if (it.select) { it.select(); beep(); }
            break;
        case ItemKind::Label:
            break;
        }
    }
    if (doBack) {
        if (!m_stack.empty()) m_stack.pop_back();
        else close();
        beep();
    }
}

void MenuController::draw() {
    Menu* m = current();
    if (!m) return;
    const Layout& L = m_layout;
    const Rgba titleBg{20, 20, 20, 230}, titleFg{235, 200, 120, 255};
    const Rgba itemBg{10, 10, 10, 170}, itemFg{230, 230, 230, 230};
    const Rgba activeBg{200, 180, 120, 220}, activeFg{10, 10, 10, 255};
    const Rgba footFg{200, 200, 200, 200};

    float y = L.y;
    rect(L.x, y, L.width, L.lineHeight * 1.25f, titleBg);
    text(m->title, L.x + 0.006f, y + L.lineHeight * 0.18f, titleFg);
    y += L.lineHeight * 1.25f;

    for (size_t i = 0; i < m->items.size(); ++i) {
        const Item& it = m->items[i];
        bool active = static_cast<int>(i) == m->cursor;
        rect(L.x, y, L.width, L.lineHeight, active ? activeBg : itemBg);
        Rgba fg = active ? activeFg : itemFg;
        text(it.label, L.x + 0.006f, y + L.lineHeight * 0.12f, fg);
        std::string v;
        switch (it.kind) {
        case ItemKind::Toggle: v = it.value ? it.value() : ""; break;
        case ItemKind::Choice:
        case ItemKind::Number: v = "< " + (it.value ? it.value() : std::string()) + " >"; break;
        case ItemKind::Label: v = it.value ? it.value() : ""; break;
        case ItemKind::Submenu: v = ">"; break;
        case ItemKind::Action: v = ""; break;
        }
        if (!v.empty()) text(v, L.x + L.width * 0.56f, y + L.lineHeight * 0.12f, fg);
        y += L.lineHeight;
    }
    rect(L.x, y, L.width, L.lineHeight * 0.9f, titleBg);
    text("8/2 move  4/6 change  5 select  0 back", L.x + 0.006f, y + L.lineHeight * 0.1f, footFg, g_scale * 0.8f);
}

} // namespace ui
