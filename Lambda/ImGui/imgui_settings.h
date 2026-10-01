#include "imgui.h"

namespace c {

    inline ImVec4 accent = ImColor(255, 75, 75, 255);

    namespace background {
        inline ImVec4 bg     = ImColor(15, 15, 15, 255);
        inline ImVec2 size   = ImVec2(950, 700);
        inline float rounding = 8.f;
    }

    namespace child {
        inline ImVec4 bg          = ImColor(22, 22, 22, 255);
        inline ImVec4 border      = ImColor(255, 75, 75, 60);
        inline ImVec4 border_text = ImColor(200, 200, 200, 255);
        inline float rounding     = 6.f;
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
        inline ImVec4 checkmark_active   = ImColor(15, 15, 15, 255);
        inline ImVec4 checkmark_inactive = ImColor(0, 0, 0, 0);
        inline ImVec4 i_bg_hov = ImColor(45, 45, 45, 255);
        inline ImVec4 i_bg     = ImColor(32, 32, 32, 255);
        inline float rounding  = 3.f;
    }

    namespace slider {
        inline ImVec4 circle   = ImColor(255, 255, 255, 255);
        inline ImVec4 i_bg_hov = ImColor(45, 45, 45, 255);
        inline ImVec4 i_bg     = ImColor(32, 32, 32, 255);
        inline float rounding  = 30.f;
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
        inline ImVec4 i_bg          = ImColor(32, 32, 32, 255);
        inline float rounding       = 4.f;
    }

    namespace selectable {
        inline ImVec4 i_bg_hov = ImColor(45, 45, 45, 255);
        inline ImVec4 i_bg     = ImColor(32, 32, 32, 255);
        inline float rounding  = 4.f;
    }

    namespace scroll {
        inline ImVec4 i_bg_hov = ImColor(255, 75, 75, 120);
        inline ImVec4 i_bg     = ImColor(255, 75, 75, 50);
        inline float rounding  = 30.f;
    }

    namespace picker {
        inline ImVec4 i_bg = ImColor(15, 15, 15, 255);
        inline float rounding = 4.f;
    }

    namespace button {
        inline ImVec4 i_bg_hov = ImColor(45, 45, 45, 255);
        inline ImVec4 i_bg     = ImColor(28, 28, 28, 255);
        inline float rounding  = 4.f;
    }

    namespace text {
        inline ImVec4 text_active = ImColor(230, 230, 230, 255);
        inline ImVec4 text_hov    = ImColor(180, 180, 180, 255);
        inline ImVec4 text        = ImColor(110, 110, 110, 255);
    }

}
