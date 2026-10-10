#include "NPC.hpp"
#include "Game.hpp"
#include "LilEngine.hpp"
#include "Components/ModelComponent.hpp"

void NPC::SetupComponents() {
    HumanCharacter::SetupComponents();

}

void NPC::LayoutUpdate() {
    HumanCharacter::LayoutUpdate();

    auto* model = Lil::World().CreateComponent<ModelComponent>();
    AttachComponent(model);
    model->MarkRequired();

    model->SetModel("cop.glb");
    model->Local().translation = Vector3{0.0f, -1.0f, 0.0f};
    model->m_visible = false;
}

void NPC::SimulationUpdate(float delta_time) {
    HumanCharacter::SimulationUpdate(delta_time);
    CheckInteractable(this, this);
}

void NPC::OnMouseHover(RayCollision res) {
    std::cout << "Approached" << std::endl;
    Game::DialogSystem().Approach(m_placement_name);
}

void NPC::OnMouseUnhover() {

}
