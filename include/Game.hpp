#pragma once

#include <raylib.h>
#include "HUD.hpp"

namespace Game {
    void SetCamera(Camera& camera);
    Camera& GetCamera();

    HUD& GetHUD();
}