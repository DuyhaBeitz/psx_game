#pragma once
#include <string>
#include <vector>

class DialogSystem {
    void DrawBox(std::string content);
    void DrawChoicesBox(std::vector<std::string> choices);

public:
    DialogSystem() = default;
    void LoadScene(std::string scene_filename, std::string localization_filename);
    void Approach(std::string placement_name);
    void Draw();
    bool IsDialogActive();
};