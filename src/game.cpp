#include "Character.hpp"
#include "Heightmap.hpp"
#include "Vehicle.hpp"
#include "Components/LightComponent.hpp"
#include "Components/AnimatedModelComponent.hpp"
#include "Components/InstancedModelComponent.hpp"
#include "utils/FileHelper.hpp"

#include "Assets.hpp"
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

    ambient = LoadMusicStream("assets/mixkit-european-forest-ambience-1213.wav");
    PlayMusicStream(ambient);

    Lil::Audio().SetSFXVolume(0.1f);

    SetupInputs();
}

void SetupInputs() {
    Lil::Input::Vector2Action walk("walk");
    Lil::Input::RegisterVector2Action(walk);
    Lil::Input::Vector2Action look("look");
    Lil::Input::BoolAction sprint("sprint");
    Lil::Input::RegisterBoolAction(sprint);
    Lil::Input::RegisterVector2Action(look, "look");
    Lil::Input::BoolAction jump("jump");
    Lil::Input::RegisterBoolAction(jump);
  
    Lil::Input::BoolAction switch_camera("switch_camera");
    Lil::Input::RegisterBoolAction(switch_camera);

    Lil::Input::FloatAction drive("car_drive");
    Lil::Input::RegisterFloatAction(drive, "car");
    Lil::Input::FloatAction steer("car_steer");
    Lil::Input::RegisterFloatAction(steer, "car");
    Lil::Input::BoolAction car_break("car_break");
    Lil::Input::RegisterBoolAction(car_break, "car");

    Lil::Input::BoolAction interact("interact");
    Lil::Input::RegisterBoolAction(interact, "interact");

    Lil::Input::BoolAction dialog_right("dialog_right");
    Lil::Input::RegisterBoolAction(dialog_right, "dialog");
    Lil::Input::BoolAction dialog_up("dialog_up");
    Lil::Input::RegisterBoolAction(dialog_up, "dialog");
    Lil::Input::BoolAction dialog_down("dialog_down");
    Lil::Input::RegisterBoolAction(dialog_down, "dialog");

    // Binding
    #if defined(PLATFORM_ANDROID) || defined(__ANDROID__)
    // Relative positions for screen inputs
    float knob_radius = 0.12f;
    float button_radius = knob_radius;
    float joystick_radius = 0.25f;
    float axis_length = 0.5f;


    // main placings
    float main_l = 0.12f;
    float main_r = 1.0f - main_l;
    float main_h = 0.75f;
    Vector2 left_main  = Vector2{main_l, main_h};
    Vector2 right_main = Vector2{main_r, main_h};


    // secondary placements
    float sec_l = 0.12f;
    float sec_r = 1.0f - sec_l;
    float sec_h = 0.5f;
    Vector2 left_sec  = Vector2{sec_l, sec_h};
    Vector2 right_sec = Vector2{sec_r, sec_h};


    // third placements
    float third_l = 0.12;
    float third_r = 1.0f - third_l;
    float third_h = 0.25f;
    Vector2 left_third  = Vector2{third_l, third_h};
    Vector2 right_third = Vector2{third_r, third_h};

    Font* buttons_font = Lil::Assets::GetFontData("Font Awesome 7 Free-Solid-900.otf");
    if (!buttons_font) {
        TraceLog(LOG_ERROR, "buttons_font is null");
    }

    // move
    walk.BindTouchJoystick(left_main, joystick_radius, knob_radius);
    sprint.BindTouchButton(right_sec, button_radius, ICON_FA_PERSON_RUNNING, *buttons_font);
    look.BindTouchAreaDelta(Rectangle{0.5f, 0.0f, 0.5f, 1.0f}, 0.005f).Negate(Lil::Input::Axes::Y);
    jump.BindTouchButton(right_main, button_radius, ICON_FA_ARROW_UP, *buttons_font);

    // camera
    switch_camera.BindTouchButton(right_third, button_radius, ICON_FA_CAMERA, *buttons_font);

    // car
    Vector2 drive_touch_pos = Vector2{0.85f, 0.85f};
    drive.BindTouchButtonPair(drive_touch_pos+Vector2{-button_radius*0.5f, 0.0f}, ICON_FA_ARROW_DOWN, drive_touch_pos+Vector2{button_radius*0.5f, 0.0f}, ICON_FA_ARROW_UP, button_radius, *buttons_font);
    steer.BindTouchAxis(left_main, axis_length, knob_radius, Lil::Input::ScreenAxis::Horizontal, false).Deadzone(0.2, 1.0f);
    car_break.BindTouchButton(right_sec, button_radius, ICON_FA_STOP, *buttons_font);
    
    interact.BindTouchButton(left_third, button_radius, ICON_FA_HAND, *buttons_font);

    // dialog
    dialog_right.BindTouchButton(right_sec, button_radius, ">", *buttons_font);
    dialog_up.BindTouchButton(right_sec, button_radius, ">", *buttons_font);

    #else
    // move
    walk.BindKeys(KEY_W, KEY_S, KEY_A, KEY_D);
    sprint.BindKey(KEY_LEFT_SHIFT);
    look.BindMouseDelta(0.005f);
    jump.BindKey(KEY_SPACE);
    
    // camera
    switch_camera.BindKey(KEY_C);

    // car
    drive.BindKeyPair(KEY_S, KEY_W);
    steer.BindKeyPair(KEY_A, KEY_D);
    car_break.BindKey(KEY_SPACE);
    
    interact.BindKey(KEY_F);

    // dialog
    dialog_right.BindKey(KEY_ENTER);
    #endif

    Lil::Input::SetContextEnabled("Default", true);
    Lil::Input::SetContextEnabled("look", true);
    Lil::Input::SetContextEnabled("car", false);
    Lil::Input::SetContextEnabled("interact", false);
    Lil::Input::SetContextEnabled("dialog", false);
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
