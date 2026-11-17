#include "Character.hpp"
#include "Heightmap.hpp"
#include "Vehicle.hpp"
#include "Components/LightComponent.hpp"
#include "Components/AnimatedModelComponent.hpp"
#include "Components/InstancedModelComponent.hpp"
#include "utils/FileHelper.hpp"

#include "NPC.hpp"
#include "Player.hpp"
#include "DrivableCar.hpp"
#include "Game.hpp"
#include "rlgl.h"

#include <LilEngine/LilEngine.hpp>
#include "lil3d/lil3d.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

constexpr int target_fps = 120;
constexpr float dt = 1.0f / float(target_fps);
RenderTexture2D render_texture;

// R3D_SurfaceShader* psxSnap = nullptr;
// R3D_ScreenShader* psxDither = nullptr;
// R3D_ScreenShader* psxPosterize = nullptr;

Music ambient;
bool debug = false;

void InitGame();
void SetupInputs();
void UpdateGame();
void DrawGame();
void UpdateDrawFrame();

int main() {
    InitGame();

	#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
	#else

    while (!WindowShouldClose()) {
        UpdateDrawFrame();
    }

    #endif
    
    UnloadMusicStream(ambient);
    Lil::World().Clear();
    Lil::Assets::Unload();
    CloseAudioDevice();
    CloseWindow();

    return 0;
}

void InitGame() {
    #if defined(PLATFORM_ANDROID) || defined(__ANDROID__)
    LIL3D_SetHint(LIL3D_HINT_SHADOW_MAP_SIZE, 128);
    LIL3D_SetHint(LIL3D_HINT_OMNI_SHADOW_MAP_SIZE, 64);
    LIL3D_SetHint(LIL3D_HINT_SHADOW_FILTER, 1);
    rlSetClipPlanes(0.1, 100.0);
    #endif    

    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    Lil::Render::InitWindow(0, 0, "game");
    
    float render_scale = 0.5;
    int RENDER_W = GetScreenWidth() * render_scale;
    int RENDER_H = GetScreenHeight() * render_scale;
    Lil::Render::SetResolution(RENDER_W, RENDER_H);
    render_texture = LoadRenderTexture(RENDER_W, RENDER_H);
    SetTraceLogLevel(LOG_ALL);
    DisableCursor();
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetTargetFPS(0);

    Lil::Engine().Get().Init();
    Game::Init();
    Lil::SceneManager().LoadScene("assets/scenes/scene1.json");

    Game::DialogSystem().LoadScene(
        "assets/dialogs/scene.json",
        "assets/dialogs/en.json"
    );

    Player* player = Game::CreatePlayer();
    player->SetPosition(Vector3{70, 19, 80});
    Game::SetCamera(player->GetCamera());

    // VehicleConfig vehicle_config{};
    // vehicle_config.wheel_radius = 0.37f;
    // vehicle_config.wheel_width = 0.2f;
    // vehicle_config.half_vehicle_length = 3.0f * 0.75;
    // vehicle_config.half_vehicle_width = 0.7f;
    // vehicle_config.half_vehicle_height = 0.03;

    // Car* car = Lil::World().CreateActor<Car>("police_car.glb", "police_car_wheel.glb", vehicle_config);
    // car->SetPosition(Vector3{-5.76, 6, -5.77});

    Lil::World().SetSimulationGoing(true);
    SetupShaders();

    ambient = LoadMusicStream("assets/mixkit-european-forest-ambience-1213.wav");
    PlayMusicStream(ambient);

    Lil::Audio().SetSFXVolume(0.1f);

    SetupInputs();
}

void SetupInputs() {
    Lil::Input::Vector2Action w("walk");
    Lil::Input::RegisterVector2Action(w);
    
    #if defined(PLATFORM_ANDROID) || defined(__ANDROID__)
    w.BindScreenJoystick(Vector2{0.15f, 0.7f}, 0.3f, 0.15f);
    #else
    w.BindKeys(KEY_W, KEY_S, KEY_A, KEY_D);
    #endif

    Lil::Input::Vector2Action l("look");
    Lil::Input::RegisterVector2Action(l, "look");
    
    #if defined(PLATFORM_ANDROID) || defined(__ANDROID__)
    l.BindScreenRectangleDelta(Rectangle{0.5f, 0.0f, 0.5f, 1.0f}, 0.005f).Negate(Lil::Input::Axes::Y);
    #else
    l.BindMouseDelta(0.005f);
    #endif

    Lil::Input::BoolAction j("jump");
    Lil::Input::RegisterBoolAction(j);
    
    #if defined(PLATFORM_ANDROID) || defined(__ANDROID__)
    j.BindScreenButton(Vector2{0.8, 0.5}, 0.1, "Jump", GetFontDefault());
    #else
    j.BindKey(KEY_SPACE);
    #endif
    

    Lil::Input::FloatAction d("car_drive");
    Lil::Input::RegisterFloatAction(d, "car");
    #if defined(PLATFORM_ANDROID) || defined(__ANDROID__)
    d.BindScreenAxis(Vector2{0.9f, 0.4f}, 0.6, 0.1, Lil::Input::ScreenAxis::Vertical, true).Deadzone(0.2, 1.0f);;
    #else
    d.BindKeyPair(KEY_S, KEY_W);
    #endif

    Lil::Input::FloatAction s("car_steer");
    Lil::Input::RegisterFloatAction(s, "car");

    #if defined(PLATFORM_ANDROID) || defined(__ANDROID__)
    s.BindScreenAxis(Vector2{0.2f, 0.7f}, 0.6, 0.1, Lil::Input::ScreenAxis::Horizontal, false).Deadzone(0.2, 1.0f);
    #else
    s.BindKeyPair(KEY_A, KEY_D);
    #endif    

    Lil::Input::BoolAction b("car_break");
    Lil::Input::RegisterBoolAction(b, "car");
    #if defined(PLATFORM_ANDROID) || defined(__ANDROID__)
    b.BindScreenButton(Vector2{0.8, 0.5}, 0.1, "break", GetFontDefault());
    #else
    b.BindKey(KEY_SPACE);
    #endif    

    Lil::Input::BoolAction i("interact");
    Lil::Input::RegisterBoolAction(i, "interact");
    #if defined(PLATFORM_ANDROID) || defined(__ANDROID__)
    i.BindScreenButton(Vector2{0.7, 0.2}, 0.1, "interact", GetFontDefault());
    #else
    i.BindKey(KEY_F);
    #endif

    Lil::Input::SetContextEnabled("Default", true);
    Lil::Input::SetContextEnabled("look", true);
    Lil::Input::SetContextEnabled("car", false);
    Lil::Input::SetContextEnabled("interact", false);
}

void UpdateGame() {
    SetMusicVolume(ambient, Lil::Audio().GetSFXVolume());
    UpdateMusicStream(ambient);
    
    if (IsKeyPressed(KEY_P)) {
        if (IsCursorHidden()) EnableCursor();
        else {
            DisableCursor();
            SetMousePosition(GetScreenWidth()/2, GetScreenHeight()/2);
        }
    }

    if (IsKeyPressed(KEY_V)) debug = !debug;

    #ifndef __EMSCRIPTEN__
    if (IsKeyPressed(KEY_F11)) ToggleBorderlessWindowed();
    #endif

    Lil::Engine::Get().Update(GetFrameTime(), dt);
}

static void DrawTextureProFitted(Texture2D texture, Rectangle target, Color tint) {
    Rectangle source = { 0.0f, 0.0f, (float)texture.width, -(float)texture.height };

    float scaleX = target.width / (float)texture.width;
    float scaleY = target.height / (float)texture.height;

    float scale = (scaleX < scaleY) ? scaleX : scaleY;

    float scaledWidth = texture.width * scale;
    float scaledHeight = texture.height * scale;

    Rectangle dest = {
        target.x + (target.width - scaledWidth) * 0.5f,
        target.y + (target.height - scaledHeight) * 0.5f,
        scaledWidth,
        scaledHeight
    };

    Vector2 origin = { 0.0f, 0.0f };
    DrawTexturePro(texture, source, dest, origin, 0.0f, tint);
}

void DrawGame() {
    Lil::Render::BeginView(Game::GetCamera(), {0, 0, float(render_texture.texture.width), float(render_texture.texture.height)}, &render_texture);
    Lil::Render::BeginScene();
        Lil::World().Draw();
    Lil::Render::EndScene();

    Lil::Render::BeginOverlay();
        if (debug) Lil::World().DebugDraw();
    Lil::Render::EndOverlay();

    BeginDrawing();
        
    ClearBackground(RAYBLACK);
    DrawTextureProFitted(render_texture.texture, {0, 0, float(GetScreenWidth()), float(GetScreenHeight())}, WHITE);
    Game::GetHUD().Draw();
    Game::DialogSystem().Draw();
    DrawFPS(100, 100);
    Lil::Input::Draw();

    EndDrawing();
}

void UpdateDrawFrame() {
    UpdateGame();
    DrawGame();
}
