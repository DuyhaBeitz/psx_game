#include "HUD.hpp"
#include <raylib.h>

void HUD::SetCenterHint(std::string center_hint) {
    m_center_hints.emplace(center_hint);
}

void HUD::Draw() {
    DrawCircle(GetScreenWidth()/2, GetScreenHeight()/2, 3, WHITE);

    int i = 0;
    for (auto& hint : m_center_hints) {
        const char* text = hint.c_str();
        int font_size = 16;
        float spacing = 1.0;
        Vector2 size = MeasureTextEx(GetFontDefault(), text, font_size, spacing);
        DrawTextEx(GetFontDefault(), text, Vector2{GetScreenWidth()/2 - size.x/2, GetScreenHeight()/2 - size.y/2*(i+1)}, font_size, spacing, WHITE);
        i++;
    }
}