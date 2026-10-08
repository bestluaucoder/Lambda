#pragma once
#include "ESPPreview.h"
#include "../../ImGui/imgui.h"
#include "../../ImGui/imgui_internal.h"

CESPPreview* ESPPreview = new CESPPreview;

void CESPPreview::Render(const Vector2& pos, const Vector2& size) {
    ImDrawList* draw = ImGui::GetWindowDrawList();
    
    Vector2 center = pos + size * 0.5f;
    center.y += size.y * 0.15f;
    
    float scale = size.y * 0.4f;
    Vector2 box_min(center.x - scale * 0.25f, center.y - scale);
    Vector2 box_max(center.x + scale * 0.25f, center.y + scale * 0.8f);
    
    float alpha = 1.0f;
    int health = 85;
    
    DrawPreviewBox(box_min, box_max, alpha);
    DrawPreviewHealth(box_min, box_max, health, alpha);
    DrawPreviewName(box_min, box_max, "Preview", alpha);
    DrawPreviewWeapon(box_min, box_max, "AK-47", alpha);
    DrawPreviewFlags(box_min, box_max, alpha);
}

void CESPPreview::DrawPreviewBox(const Vector2& box_min, const Vector2& box_max, float alpha) {
    if (!config.visuals.esp.bounding_box->get())
        return;
        
    Color clr = config.visuals.esp.box_color->get();
    clr.a = static_cast<int>(clr.a * alpha);
    
    Render->Box(box_min, box_max, clr);
    Render->Box(box_min - Vector2(1, 1), box_max + Vector2(1, 1), Color(0, 0, 0, static_cast<int>(150 * alpha)));
    Render->Box(box_min + Vector2(1, 1), box_max - Vector2(1, 1), Color(0, 0, 0, static_cast<int>(150 * alpha)));
}

void CESPPreview::DrawPreviewHealth(const Vector2& box_min, const Vector2& box_max, int health, float alpha) {
    if (!config.visuals.esp.health_bar->get())
        return;
        
    Color clr = Color(0, 255, 0);
    
    clr.r = std::clamp(health < 50 ? 250 : (100 - health) / 50.f * 250.f, 120.f, 230.f);
    clr.g = std::clamp(health > 50 ? 250 : health / 50.f * 250.f, 120.f, 230.f);
    clr.b = 120;
    
    if (config.visuals.esp.custom_health->get())
        clr = config.visuals.esp.custom_health_color->get();
        
    clr.a = static_cast<int>(clr.a * alpha);
    
    float h = box_max.y - box_min.y;
    float health_fraction = std::clamp(1 - health / 100.f, 0.f, 1.f);
    Vector2 health_box_start = box_min - Vector2(7, 1);
    Vector2 health_box_end(box_min.x - 3, box_max.y + 1);
    
    Render->BoxFilled(health_box_start, health_box_end, Color(8, static_cast<int>(220 * alpha)));
    Render->BoxFilled(health_box_start + Vector2(1, 1 + h * health_fraction), health_box_end - Vector2(1, 1), clr);
    
    Render->Text(std::to_string(health), box_min - Vector2(6.f, -health_fraction * h + 5), Color(240, 240, 240, static_cast<int>(255 * alpha)), SmallFont, TEXT_CENTERED | TEXT_OUTLINED);
}

void CESPPreview::DrawPreviewName(const Vector2& box_min, const Vector2& box_max, const char* name, float alpha) {
    if (!config.visuals.esp.name->get())
        return;
        
    Color clr = config.visuals.esp.name_color->get();
    clr.a = static_cast<int>(clr.a * alpha);
    
    Vector2 name_pos((box_min.x + box_max.x) * 0.5f, box_min.y - 13);
    Render->Text(name, name_pos, clr, Verdana, TEXT_CENTERED | TEXT_DROPSHADOW);
}

void CESPPreview::DrawPreviewWeapon(const Vector2& box_min, const Vector2& box_max, const char* weapon, float alpha) {
    if (!config.visuals.esp.weapon->get())
        return;
        
    Color clr = config.visuals.esp.weapon_color->get();
    clr.a = static_cast<int>(clr.a * alpha);
    
    Vector2 weapon_pos((box_min.x + box_max.x) * 0.5f, box_max.y + 3);
    Render->Text(weapon, weapon_pos, clr, SmallFont, TEXT_CENTERED | TEXT_DROPSHADOW);
}

void CESPPreview::DrawPreviewFlags(const Vector2& box_min, const Vector2& box_max, float alpha) {
    if (!config.visuals.esp.flags->get(0) && !config.visuals.esp.flags->get(1))
        return;
        
    std::vector<std::string> flags;
    
    if (config.visuals.esp.flags->get(0))
        flags.push_back("HK");
    if (config.visuals.esp.flags->get(1))
        flags.push_back("ZOOM");
    if (config.visuals.esp.flags->get(4))
        flags.push_back("BOMB");
        
    float y_offset = 0;
    for (const auto& flag : flags) {
        Color flag_color(215, 215, 215, static_cast<int>(255 * alpha));
        Vector2 flag_pos(box_max.x + 3, box_min.y + y_offset);
        Render->Text(flag, flag_pos, flag_color, SmallFont, TEXT_DROPSHADOW);
        y_offset += 11;
    }
}
