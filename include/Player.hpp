#pragma once

#include "Character.hpp"
#include "Stepper.hpp"

class AnimatedModelComponent;

class Player : public Character {
public:
    Player();
    virtual ~Player() = default;

    virtual void SetupComponents() override;
    virtual void LayoutUpdate() override;
    virtual void SimulationUpdate(float delta_time) override;

    float m_camera_yaw = 0.0f;
    float m_camera_pitch = 0.0f;
    AnimatedModelComponent* m_animated_model;
    Stepper m_stepper;

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