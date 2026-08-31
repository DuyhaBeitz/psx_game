#include "Character.hpp"
#include "Heightmap.hpp"
#include "Vehicle.hpp"
#include "Components/LightComponent.hpp"
#include "Components/AnimatedModelComponent.hpp"
#include "Components/InstancedModelComponent.hpp"

#include "Player.hpp"
#include "Car.hpp"

#include <LilEngine/LilEngine.hpp>

int main() {
    InitWindow(0, 0, "game");
    R3D_Init(640, 480);
    InitAudioDevice();
    SetTraceLogLevel(LOG_ALL);
    DisableCursor();
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    Lil::Engine().Get().Init();

    Lil::SceneManager().LoadScene("assets/scenes/scene1.json");
    Lil::Environment().Update();

    Player* player = Lil::World().CreateActor<Player>();
    player->SetPosition(Vector3{0, 10, 0});

    VehicleConfig vehicle_config{};
    vehicle_config.wheel_radius = 0.37f;
    vehicle_config.wheel_width = 0.2f;
    vehicle_config.half_vehicle_length = 3.0f * 0.75;
    vehicle_config.half_vehicle_width = 0.7f;
    vehicle_config.half_vehicle_height = 0.03;

    Car* car = Lil::World().CreateActor<Car>("police_car.glb", "police_car_wheel.glb", vehicle_config);
    car->SetPosition(Vector3{10, 10, 10});

    bool debug = false;
    Lil::World().SetSimulationGoing(true);

    R3D_SurfaceShader* psxSnap = R3D_LoadSurfaceShader("assets/shaders/psx_snap.glsl");
    Vector2 snapRes = {320.0f/2, 240.0f/2};
    R3D_SetSurfaceShaderUniform(psxSnap, "u_snap_resolution", &snapRes);

    for (auto& [key, model] : Lil::Resources().Models()) {
        for (int i = 0; i < model.materialCount; i++) {
            model.materials[i].shader = psxSnap;
        }
    }

    R3D_ScreenShader* psxDither = R3D_LoadScreenShader("assets/shaders/psx_dither.glsl");
    Vector2 targetRes = { 640.0f, 480.0f };
    R3D_SetScreenShaderUniform(psxDither, "u_target_resolution", &targetRes);

    R3D_ScreenShader* psxPosterize = R3D_LoadScreenShader("assets/shaders/psx_psxPosterize.glsl");
    float levels = 32.0f;
    float gamma = 2.2f;
    R3D_SetScreenShaderUniform(psxPosterize, "u_color_levels", &levels);
    R3D_SetScreenShaderUniform(psxPosterize, "u_gamma", &gamma);

    R3D_ScreenShader* final_chain[] = { psxPosterize, psxDither };
    R3D_SetScreenShaderChain(R3D_SCREEN_SHADER_STAGE_FINAL, final_chain, 2);

    Music ambient = LoadMusicStream("assets/mixkit-european-forest-ambience-1213.wav");
    PlayMusicStream(ambient);

    while (!WindowShouldClose()) {
        UpdateMusicStream(ambient);
        if (IsKeyPressed(KEY_V)) debug = !debug;
        if (IsKeyPressed(KEY_F11)) {
            SetWindowSize(GetMonitorWidth(GetCurrentMonitor()), GetMonitorHeight(GetCurrentMonitor()));
            ToggleFullscreen();
        }
        //camera.position = player->GetPosition();
        Lil::Engine::Get().Update();
        

        BeginDrawing();
            ClearBackground(RAYBLACK);
            
            R3D_Begin(player->GetCamera());
                Lil::World().Draw();
            R3D_End();

            BeginMode3D(player->GetCamera());
                if (debug) Lil::World().DebugDraw();
            EndMode3D();

            DrawFPS(100, 100);
        EndDrawing();
    }
    
    R3D_UnloadScreenShader(psxPosterize);
    R3D_UnloadScreenShader(psxDither);
    R3D_UnloadSurfaceShader(psxSnap);
    Lil::Resources().Unload();
    CloseAudioDevice();
    CloseWindow();

    return 0;
}