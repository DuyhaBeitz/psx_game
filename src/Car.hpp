#pragma once

#include "Vehicle.hpp"
#include "ReflAttributes.hpp"

class Car : public Vehicle {
public:
    LIL_REFLECTABLE()
    LIL_SERIALIZABLE()
    
    Car() = default;
    Car(std::string base_model_key, std::string wheel_model_key, const VehicleConfig& vehicle_config);
    virtual ~Car() = default;

    std::string m_base_model_key;
    std::string m_wheel_model_key;

    virtual void SetupComponents() override;
    virtual void Draw() override;
};
LIL_REFLECT(Car, bases<Vehicle>,
    field(m_base_model_key, ModelKeyAttribute{}),
    field(m_wheel_model_key, ModelKeyAttribute{})
)
LIL_SER_BEGIN(Car)
LIL_SER_BASE(Vehicle)
LIL_SER_FIELD(m_base_model_key)
LIL_SER_FIELD(m_wheel_model_key)
LIL_SER_END()