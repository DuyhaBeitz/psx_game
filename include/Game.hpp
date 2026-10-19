#pragma once

#include <raylib.h>
#include "DialogSystem.hpp"
#include "HUD.hpp"

class Player;

namespace Game {
    void SetCamera(Camera& camera);
    Camera& GetCamera();

    HUD& GetHUD();

    DialogSystem& DialogSystem();

    Player* CreatePlayer();
    void DestroyPlayer();
    void SetPlayer(Player* pl);
    Player* GetPlayer();
}