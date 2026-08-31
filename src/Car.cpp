#include "Components/LightComponent.hpp"

#include "Car.hpp"
#include "LilEngine.hpp"

Car::Car(std::string base_model_key, std::string wheel_model_key, const VehicleConfig& vehicle_config) 
: Vehicle(vehicle_config), m_base_model_key(base_model_key), m_wheel_model_key(wheel_model_key)
{
}

void Car::SetupComponents() {
    LightComponent* light1 = Lil::World().CreateComponent<LightComponent>();
    AttachComponent(light1);
    light1->MarkRequired();
    light1->m_type = int(R3D_LIGHT_SPOT);
    light1->Local().translation = Vector3{m_vehicle_config.half_vehicle_width, m_vehicle_config.half_vehicle_height, m_vehicle_config.half_vehicle_length + 0.4f};
    light1->Local().rotation = QuaternionFromAxisAngle(Vector3{1, 0, 0}, -M_PI/2.0f);
    light1->m_lumen = 15.0f;

    LightComponent* light2 = Lil::World().CreateComponent<LightComponent>();
    AttachComponent(light2);
    light2->MarkRequired();
    light2->m_type = int(R3D_LIGHT_SPOT);
    light2->Local().translation = Vector3{-m_vehicle_config.half_vehicle_width, m_vehicle_config.half_vehicle_height, m_vehicle_config.half_vehicle_length + 0.4f};
    light2->Local().rotation = QuaternionFromAxisAngle(Vector3{1, 0, 0}, -M_PI/2.0f);
    light2->m_lumen = 15.0f;
}

void Car::Draw()
{
    Vehicle::Draw();

    for (Transform wheel_transform : GetWheelTransforms()) {
        if (R3D_Model* m = Lil::Resources().GetModel(m_wheel_model_key)) R3D_DrawModelEx(*m, wheel_transform.translation, wheel_transform.rotation, wheel_transform.scale);
    }
    if (R3D_Model* m = Lil::Resources().GetModel(m_base_model_key)) R3D_DrawModelEx(*m, GetPosition(), GetRotation(), GetScale());
}
