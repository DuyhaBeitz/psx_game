#pragma once

#include "HumanCharacter.hpp"
#include "Interactable.hpp"
#include "Reflection.hpp"
#include "Serialization.hpp"

class AnimatedModelComponent;

class NPC : public HumanCharacter, public Interactable {
public:
    LIL_REFLECTABLE()
    LIL_SERIALIZABLE()

    std::string m_placement_name;

    NPC() = default;
    virtual ~NPC() = default;

    virtual void SetupComponents() override;
    virtual void LayoutUpdate() override;
    virtual void SimulationUpdate(float delta_time) override;

    virtual void OnMouseHover(RayCollision res);
    virtual void OnMouseUnhover();
};
LIL_REFLECT(NPC, (bases<HumanCharacter>),
    field(m_placement_name)
)
LIL_SER_BEGIN(NPC)
LIL_SER_BASE(HumanCharacter)
LIL_SER_FIELD(m_placement_name)
LIL_SER_END()