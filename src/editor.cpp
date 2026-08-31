#include "Character.hpp"
#include "Heightmap.hpp"
#include "Vehicle.hpp"
#include "Components/LightComponent.hpp"
#include "Components/AnimatedModelComponent.hpp"
#include "Components/InstancedModelComponent.hpp"

#include "Player.hpp"
#include "Car.hpp"

#include <LilEngine/LilEngine.hpp>
#include <LilEditor/LilEditor.hpp>

int main() {
    InitWindow(0, 0, "editor");
    R3D_Init(640, 480);
    InitAudioDevice();
    SetTraceLogLevel(LOG_ALL);
    DisableCursor();
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    Lil::Engine().Get().Init();

    Lil::Editor::Get().Init();

    while (!WindowShouldClose()) {
        Lil::Editor::Get().Update();
        BeginDrawing();
        Lil::Editor::Get().Draw();
        EndDrawing();
    }

    Lil::Resources().Unload();
    CloseAudioDevice();
    CloseWindow();

    return 0;
}