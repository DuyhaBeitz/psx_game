#pragma once

#include <raylib.h>
#include "DialogSystem.hpp"
#include "HUD.hpp"

namespace Game {
    void SetCamera(Camera& camera);
    Camera& GetCamera();

    HUD& GetHUD();

    DialogSystem& DialogSystem();
}