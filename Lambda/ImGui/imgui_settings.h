#include "imgui.h"

namespace c {

    // Blue accent
    inline ImVec4 accent = ImColor(89, 113, 162, 255);

    namespace background {
        // Window background
        inline ImVec4 bg     = ImColor(33, 33, 33, 255);
        inline ImVec2 size   = ImVec2(800, 460);
        // Strict 0 rounding for crisp corners
        inline float rounding = 0.f;
    }

    namespace child {
        // Slightly darker than window background
        inline ImVec4 bg          = ImColor(25, 25, 25, 255);
        // Low-contrast thin border
        inline ImVec4 border      = ImColor(89, 113, 162, 255);
        inline ImVec4 border_text = ImColor(200, 200, 200, 255);
        inline float rounding     = 0.f;
    }

    namespace tabs {
        inline ImVec4 bg_active = ImColor(255, 75, 75, 18);
        inline ImVec4 bg_hov    = ImColor(255, 255, 255, 6);
        inline ImVec4 bg        = ImColor(0, 0, 0, 0);
        inline ImVec4 i_bg_hov  = ImColor(255, 255, 255, 6);
        inline ImVec4 i_bg      = ImColor(0, 0, 0, 0);
        inline float rounding   = 6.f;
    }

    namespace checkbox {
        // Active checkmark uses the muted lavender-pink accent
        inline ImVec4 checkmark_active   = ImColor(213, 166, 189, 255);
        inline ImVec4 checkmark_inactive = ImColor(0, 0, 0, 0);
        inline ImVec4 i_bg_hov = ImColor(45, 45, 45, 255);
        inline ImVec4 i_bg     = ImColor(32, 32, 32, 255);
        inline float rounding  = 0.f;
    }

    namespace slider {
        // slider fill/grab will use the accent from style; keep circle neutral
        inline ImVec4 circle   = ImColor(255, 255, 255, 255);
        inline ImVec4 i_bg_hov = ImColor(45, 45, 45, 255);
        inline ImVec4 i_bg     = ImColor(32, 32, 32, 255);
        inline float rounding  = 0.f;
    }

    namespace input_text {
        inline ImVec4 i_bg_selected = ImColor(50, 50, 50, 255);
        inline ImVec4 i_bg_hov      = ImColor(45, 45, 45, 255);
        inline ImVec4 i_bg          = ImColor(32, 32, 32, 255);
        inline float rounding       = 4.f;
    }

    namespace keybind {
        inline ImVec4 i_bg_hov = ImColor(45, 45, 45, 255);
        inline ImVec4 i_bg     = ImColor(32, 32, 32, 255);
        inline float rounding  = 4.f;
    }

    namespace combo {
        inline ImVec4 i_bg_selected = ImColor(18, 18, 18, 255);
        inline ImVec4 i_bg_hov      = ImColor(45, 45, 45, 255);
        inline ImVec4 i_bg          = ImColor(28, 28, 28, 255);
        inline float rounding       = 0.f;
    }

    namespace selectable {
        inline ImVec4 i_bg_hov = ImColor(45, 45, 45, 255);
        inline ImVec4 i_bg     = ImColor(32, 32, 32, 255);
        inline float rounding  = 0.f;
    }

    namespace scroll {
        inline ImVec4 i_bg_hov = ImColor(255, 75, 75, 120);
        inline ImVec4 i_bg     = ImColor(255, 75, 75, 50);
        inline float rounding  = 30.f;
    }

    namespace picker {
        inline ImVec4 i_bg = ImColor(15, 15, 15, 255);
        inline float rounding = 0.f;
    }

    namespace button {
        inline ImVec4 i_bg_hov = ImColor(45, 45, 45, 255);
        inline ImVec4 i_bg     = ImColor(28, 28, 28, 255);
        inline float rounding  = 4.f;
    }

    namespace text {
        inline ImVec4 text_active = ImColor(240, 240, 240, 255);
        inline ImVec4 text_hov    = ImColor(200, 200, 200, 255);
        inline ImVec4 text        = ImColor(160, 160, 160, 255);
    }

}
