#pragma once

#include "Vehicle.hpp"
#include "ReflAttributes.hpp"
#include "ComponentRef.hpp"

class ModelComponent;

class Car : public Vehicle {
public:
    LIL_REFLECTABLE()
    
    Car() = default;
    virtual ~Car() = default;

    std::string m_base_model_key = "None";
    std::string m_wheel_model_key = "None";

    ComponentRef<ModelComponent> m_base_model;
    ComponentRef<ModelComponent> m_wheel_models[4];

    virtual void SetupComponents() override;
    void LayoutUpdate() override;
    virtual void Draw() override;

    template <class Archive>
    void save( Archive & ar ) const {
        LIL_SAVE_BASE(Vehicle);
        LIL_SER_FIELD(m_base_model_key)
        LIL_SER_FIELD(m_wheel_model_key)
        LIL_SER_FIELD(m_base_model)
        for (int i = 0; i < 4; i++) {
            LIL_SER_FIELD(m_wheel_models[i])
        }
    }
       
    template <class Archive>
    void load( Archive & ar ) {
        LIL_LOAD_BASE(Vehicle)
        LIL_SER_FIELD(m_base_model_key)
        LIL_SER_FIELD(m_wheel_model_key)
        LIL_SER_FIELD(m_base_model)
        for (int i = 0; i < 4; i++) {
            LIL_SER_FIELD(m_wheel_models[i])
        }
    }
};
LIL_REFLECT(Car, bases<Vehicle>,
    field(m_base_model_key, ModelKeyAttribute{}),
    field(m_wheel_model_key, ModelKeyAttribute{})
)
LIL_SER_REGISTER_POLYMORPHIC(Car)
LIL_DISAMBIGUATE_LOAD_SAVE(Car)