#pragma once
#include <functional>
#include <string>
#include <vector>

namespace ui {

struct Rgba { int r, g, b, a; };

void init(bool bgBackend, float textScale);
void text(const std::string& s, float x, float y, Rgba color, float scale = 0.0f);
void rect(float left, float top, float width, float height, Rgba color);   // top-left anchored

enum class ItemKind { Toggle, Choice, Number, Action, Submenu, Label };

struct Menu;

struct Item {
    ItemKind kind = ItemKind::Label;
    std::string label;
    std::function<std::string()> value;      // right-hand text (Toggle/Choice/Number/Label)
    std::function<void(int)> change;         // Choice/Number: +1 / -1; Toggle: any
    std::function<void()> select;            // Action (and Toggle if change is empty)
    Menu* submenu = nullptr;
};

struct Menu {
    std::string title;
    std::vector<Item> items;
    int cursor = 0;
};

struct Layout {
    float x = 0.02f, y = 0.12f, width = 0.27f, lineHeight = 0.034f;
};

class MenuController {
public:
    void setRoot(Menu* root) { m_root = root; }
    void setLayout(const Layout& l) { m_layout = l; }
    bool isOpen() const { return m_open; }
    void open();
    void close();
    void toggle() { m_open ? close() : open(); }
    // Call every frame while open.
    void input(int up, int down, int left, int right, int select, int back);
    void draw();

private:
    Menu* current() { return m_stack.empty() ? m_root : m_stack.back(); }
    void beep();

    Menu* m_root = nullptr;
    std::vector<Menu*> m_stack;
    Layout m_layout;
    bool m_open = false;
};

} // namespace ui
