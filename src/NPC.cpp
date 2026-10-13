#include "NPC.hpp"
#include "Game.hpp"
#include "LilEngine.hpp"
#include "Components/ModelComponent.hpp"

void NPC::SetupComponents() {
    HumanCharacter::SetupComponents();
    auto* model = Lil::World().CreateComponent<ModelComponent>();
    AttachComponent(model);
    model->MarkRequired();

    model->SetModel("cop.glb");
    model->Local().translation = Vector3{0.0f, -1.0f, 0.0f};
    model->m_visible = false;
    model->MarkRequired();
}

void NPC::LayoutUpdate() {
    HumanCharacter::LayoutUpdate();
}

void NPC::SimulationUpdate(float delta_time) {
    HumanCharacter::SimulationUpdate(delta_time);
    
    CheckInteractable(this, this);

    if (IsHovered()) {
        Game::GetHUD().AddCenterHint("F - talk");
        if (IsKeyPressed(KEY_F)) Game::DialogSystem().Approach(m_placement_name);
    }
    else {
        Game::GetHUD().RemoveCenterHint("F - talk");
    }
}

bool NPC::HoverTreshold(RayCollision res) { return (res.distance < 3.0f); }
void NPC::OnMouseHover(RayCollision res) {}
void NPC::OnMouseUnhover() {}
