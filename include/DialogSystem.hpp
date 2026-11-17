#pragma once
#include <string>
#include <vector>
#include "Input.hpp"

class DialogSystem {
private:
    void DrawBox(std::string name, std::string content);
    void DrawChoicesBox(std::vector<std::string> choices);

    Lil::Input::BoolAction dialog_right = "dialog_right";

public:
    void Init();

    DialogSystem() = default;
    void LoadScene(std::string scene_filename, std::string localization_filename);
    void Approach(std::string placement_name);
    void Draw();
    bool IsDialogActive();
};