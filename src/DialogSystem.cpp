
#include "DialogSystem.hpp"
#include "dialog.hpp"
#include <raylib.h>
#include <algorithm>
#include <iostream>
#include "AdvancedText.hpp"

dialog::Runtime dlg;
int selectedChoice = 0;
AnimatedText anim_text{};

void DialogSystem::DrawBox(std::string name, std::string content) {
    int w = GetScreenWidth();
    int h = GetScreenHeight();

    int panelY = h * 2 / 3;

    // Transparent dark overlay on the lower third
    DrawRectangle(0, panelY, w, h - panelY, Fade(BLACK, 0.65f));

    // Center dialogue text horizontally
    int fontSize = 64;
    int spacing = 54;
    int textWidth = MeasureText(content.c_str(), fontSize);
    int start = w * 0.1;

    DrawText(
        name.c_str(),
        start,
        panelY + 10,
        fontSize,
        GRAY
    );

    DrawText(
        content.c_str(),
        start+30,
        panelY + 120,
        fontSize,
        WHITE
    );
}

void DialogSystem::DrawChoicesBox(std::vector<std::string> choices) {
    int w = GetScreenWidth();
    int h = GetScreenHeight();

    int panelY = h * 2 / 3;

    DrawRectangle(0, panelY, w, h - panelY, Fade(BLACK, 0.65f));

    int fontSize = 64;
    int spacing = 54;
    int totalHeight = (int)choices.size() * spacing;
    int startY = panelY + ((h - panelY) - totalHeight) / 2;

    for (int i = 0; i < (int)choices.size(); i++)  {
        std::string text = (i == selectedChoice ? ">  " : "   ") + choices[i];

        int textWidth = MeasureText(text.c_str(), fontSize);

        DrawText(
            text.c_str(),
            (w - textWidth) / 2,
            startY + i * spacing,
            fontSize,
            i == selectedChoice ? WHITE : GRAY
        );
    }
}


void DialogSystem::LoadScene(std::string scene_filename, std::string localization_filename) {
    dlg = dialog::load_files(scene_filename, {localization_filename}, "en");

    selectedChoice = 0;

    if (!dlg.ok()) for (auto& e : dlg.errors()) std::cerr << e.message << std::endl;
}

void DialogSystem::Approach(std::string placement_name) {
    selectedChoice = 0;
    dlg.approach(placement_name);
}

void DialogSystem::Draw() {
    const auto& f = dlg.frame();

    switch (f.kind) {
        case dialog::FrameKind::Line:
        {
            if (anim_text.text != f.text) {
                anim_text.frame_counter = 0;
                anim_text.text = f.text;
                anim_text.on_new_symbol = [](int index, char c){
                    LilSound* sound = Lil::Resources().GetSound("UIClick_BLEEOOP_Baby_Click.ogg");
                    if (sound) sound->Play();
                };
            }
            anim_text.Update();
            DrawBox(f.speaker_name, anim_text.GetCurrentText());

            if (IsKeyPressed(KEY_ENTER)) {
                if (anim_text.IsDone()) dlg.advance();
                else anim_text.Complete();
            }

            break;
        }

        case dialog::FrameKind::Choice:
        {
            int count = (int)f.choices.size();
            if (count == 0) break;

            if (IsKeyPressed(KEY_UP))
                selectedChoice--;

            if (IsKeyPressed(KEY_DOWN))
                selectedChoice++;

            selectedChoice = (selectedChoice + count) % count;

            DrawChoicesBox(f.choices);

            if (IsKeyPressed(KEY_ENTER))
            {
                dlg.choose(selectedChoice);
                selectedChoice = 0;
            }

            break;
        }

        case dialog::FrameKind::Ended:
            break;

        case dialog::FrameKind::Error:
            std::cerr << f.error << std::endl;
            break;
    }
}

bool DialogSystem::IsDialogActive() {
    const auto kind = dlg.frame().kind;
    return kind == dialog::FrameKind::Line
        || kind == dialog::FrameKind::Choice;
}
