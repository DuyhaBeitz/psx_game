#pragma once

#include "Car.hpp"
#include "Interactable.hpp"
#include "Input.hpp"

class DrivableCar : public Car, public Interactable {
protected:
    bool m_controlled = false;
    void CameraUpdate();
    float m_skid_smoothed;

    Lil::Input::Vector2Action m_look_action = "look";
    Lil::Input::FloatAction m_drive_action = "car_drive";
    Lil::Input::FloatAction m_steer_action = "car_steer";
    Lil::Input::BoolAction  m_break_action = "car_break";
    Lil::Input::BoolAction  m_interact_action = "interact";

public:
    LIL_SERIALIZABLE()
    LIL_REFLECTABLE()

    DrivableCar() = default;
    ~DrivableCar() = default;

    virtual void SimulationUpdate(float delta_time) override;

    virtual bool HoverTreshold(RayCollision res) override;
    virtual void OnMouseHover(RayCollision res) override;
    virtual void OnMouseUnhover() override;

    float m_camera_yaw = 0.0f;
    float m_camera_pitch = 0.0f;
    float m_steer = 0.0f;

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
LIL_REFLECT(DrivableCar, bases<Car>)
LIL_SER_BEGIN(DrivableCar)
LIL_SER_BASE(Car)
LIL_SER_END()
