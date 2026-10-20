#pragma once

#include "Car.hpp"
#include "Interactable.hpp"

class DrivableCar : public Car, public Interactable {
protected:
    bool m_controlled = false;
    void CameraUpdate();
    float m_skid_smoothed;

public:
    LIL_SERIALIZABLE()
    LIL_REFLECTABLE()

    DrivableCar() = default;
    ~DrivableCar() = default;

    virtual void SimulationUpdate(float delta_time) override;

    virtual bool HoverTreshold(RayCollision res);
    virtual void OnMouseHover(RayCollision res);
    virtual void OnMouseUnhover();

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
