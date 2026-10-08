#pragma once
#include "../../SDK/Interfaces.h"
#include "../../SDK/Render.h"
#include "../../SDK/Config.h"

class CESPPreview {
public:
    void Render(const Vector2& pos, const Vector2& size);
    
private:
    void DrawPreviewBox(const Vector2& box_min, const Vector2& box_max, float alpha);
    void DrawPreviewHealth(const Vector2& box_min, const Vector2& box_max, int health, float alpha);
    void DrawPreviewName(const Vector2& box_min, const Vector2& box_max, const char* name, float alpha);
    void DrawPreviewWeapon(const Vector2& box_min, const Vector2& box_max, const char* weapon, float alpha);
    void DrawPreviewFlags(const Vector2& box_min, const Vector2& box_max, float alpha);
    
    Vector2 GetBoundingBox(const Vector2& center, float scale);
};

extern CESPPreview* ESPPreview;
