#pragma once

#include "HumanCharacter.hpp"
#include "Input.hpp"

class AnimatedModelComponent;

enum class PlayerPerspective {
    FIRST_PERSON,
    THIRD_PERSON
};

class Player : public HumanCharacter {
private:
    PlayerPerspective m_perspective = PlayerPerspective::FIRST_PERSON;

    Lil::Input::Vector2Action m_walk_action = "walk";
    Lil::Input::Vector2Action m_look_action = "look";
    Lil::Input::BoolAction    m_jump_action = "jump";

    void CameraUpdate();

public:
    Player() : HumanCharacter() {};
    virtual ~Player() = default;

    virtual void SetupComponents() override;
    virtual void LayoutUpdate() override;
    virtual void SimulationUpdate(float delta_time) override;

    float m_camera_yaw = 0.0f;
    float m_camera_pitch = 0.0f;

    Vector3 GetLookingVector();

    Camera3D m_camera = {
        .position   = { 0.0f, 0.0f, 0.0f },
        .target     = { 0.0f, 0.0f, 0.0f },
        .up         = { 0.0f, 1.0f, 0.0f },
        .fovy       = 60.0f,
        .projection = CAMERA_PERSPECTIVE
    };

    Camera3D& GetCamera() { return m_camera; }
};