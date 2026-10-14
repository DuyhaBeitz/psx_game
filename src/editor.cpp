#include "Character.hpp"
#include "Heightmap.hpp"
#include "Vehicle.hpp"
#include "Components/LightComponent.hpp"
#include "Components/AnimatedModelComponent.hpp"
#include "Components/InstancedModelComponent.hpp"

#include "NPC.hpp"
#include "Player.hpp"
#include "Car.hpp"

#include <LilEngine/LilEngine.hpp>
#include <LilEditor/LilEditor.hpp>

int main() {
    InitWindow(1920, 1080, "editor");
    R3D_Init(GetScreenWidth(), GetScreenHeight());
    InitAudioDevice();
    SetTraceLogLevel(LOG_ALL);
    DisableCursor();
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetTargetFPS(60.0f);

    Lil::Engine().Get().Init();

    Lil::Editor::Get().Init();

    while (!WindowShouldClose()) {
        Lil::Editor::Get().Update(1.0f/60.f);
        BeginDrawing();
        Lil::Editor::Get().Draw();
        EndDrawing();
    }

    Lil::Editor::Get().Close();
    Lil::Resources().Unload();
    CloseAudioDevice();
    CloseWindow();

    return 0;
}