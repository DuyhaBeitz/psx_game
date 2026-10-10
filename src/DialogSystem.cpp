#include "DialogSystem.hpp"
#include "dialog.hpp"
#include <iostream>
#include <raylib.h>

dialog::Runtime dlg; 

void DialogSystem::DrawBox(std::string content) {
    int w = GetScreenWidth();
    int h = GetScreenHeight();
    DrawRectangle(0, h/2, w, h/2, Fade(BLACK, 0.3));

    DrawText(content.c_str(), w/2+30, h/2+30, 32, WHITE);
}

void DialogSystem::DrawChoicesBox(std::vector<std::string> choices) {
    int w = GetScreenWidth();
    int h = GetScreenHeight();
    DrawRectangle(0, h/2, w, h/2, Fade(BLACK, 0.3));

    for (size_t i = 0; i < choices.size(); i++) {
        DrawText(choices[i].c_str(), w/2+30, h/2+30+32*i, 32, WHITE);
    }
}

void DialogSystem::LoadScene(std::string scene_filename, std::string localization_filename) {
    std::cout << "Loading dialog scene" << std::endl;
    dlg = dialog::load_files(scene_filename, {localization_filename}, "en");
    if (!dlg.ok()) {
        for (auto& e : dlg.errors()) std::cerr << "Loading error\nnode path: " << e.node_path << "\nfield: " << e.field << "\nmessage: " << e.message  << std::endl;
    }
}

void DialogSystem::Approach(std::string placement_name) {
    dlg.approach(placement_name);
}

void DialogSystem::Draw() {
    const auto& f = dlg.frame();
    switch (f.kind) {
    case dialog::FrameKind::Line:
        {
        std::string text = TextFormat("%s: %s\n", f.speaker_name.c_str(), f.text.c_str());
        DrawBox(text);
        if (IsKeyPressed(KEY_ENTER)) dlg.advance();
        }
        break;
    case dialog::FrameKind::Choice:
        DrawChoicesBox(f.choices);
        dlg.choose(0);
        break;
    case dialog::FrameKind::Ended:
        return;
    case dialog::FrameKind::Error:
        std::cerr << f.error << std::endl;
        return;
    }
}
