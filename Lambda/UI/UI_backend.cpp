#include "UI.h"

#include "../ImGui/imgui.h"
#include "../ImGui/backends/imgui_impl_win32.h"
#include "../ImGui/backends/imgui_impl_dx9.h"
#include "../ImGui/imgui_internal.h"
#include <d3d9.h>
#include <tchar.h>

#include "../ImGui/imgui_settings.h"

#include "logo.h"
#include "mulish_font.h"
#include "icons.h"

#include "../SDK/Interfaces.h"
#include "../SDK/Misc/xorstr.h"
#include "../SDK/Globals.h"
#include "../Utils/Console.h"

#include "../Features/RageBot/Ragebot.h"
#include "../Features/Visuals/Elements.h"




CMenu* Menu = new CMenu;

namespace font {
    ImFont* general = nullptr;
    ImFont* tab = nullptr;
}

static ImGuiIO* im_io;

void CMenu::Setup() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    im_io = &ImGui::GetIO();

    ImFontConfig cfg;

    font::general = im_io->Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\verdana.ttf", 14.f, &cfg, im_io->Fonts->GetGlyphRangesCyrillic());
    font::tab = im_io->Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\verdana.ttf", 12.f, &cfg, im_io->Fonts->GetGlyphRangesCyrillic());
    
    if (!font::general) font::general = im_io->Fonts->AddFontFromMemoryTTF(mulish, sizeof(mulish), 14.f, &cfg, im_io->Fonts->GetGlyphRangesCyrillic());
    if (!font::tab) font::tab = im_io->Fonts->AddFontFromMemoryTTF(mulish, sizeof(mulish), 12.f, &cfg, im_io->Fonts->GetGlyphRangesCyrillic());

    im_io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    im_io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();

    // Enforce strict 2020 style rules (no curves) and palette adjustments
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.ChildRounding = 0.0f;
    style.FrameRounding = 0.0f;
    style.PopupRounding = 0.0f;
    style.GrabRounding = 0.0f;

    // Slim, razor-thin sliders
    style.FramePadding.y = 3.0f;
    style.GrabMinSize = 6.0f;

    // Borders (thin)
    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;

    // Tight horizontal spacing for top tabs
    style.ItemSpacing.x = 2.0f;

    // Apply palette from imgui_settings (deep charcoal backgrounds, muted accent, etc.)
    style.Colors[ImGuiCol_WindowBg] = c::background::bg;
    style.Colors[ImGuiCol_ChildBg] = c::child::bg;
    style.Colors[ImGuiCol_Border] = c::child::border;
    style.Colors[ImGuiCol_CheckMark] = c::checkbox::checkmark_active;
    style.Colors[ImGuiCol_SliderGrab] = c::accent;
    style.Colors[ImGuiCol_SliderGrabActive] = c::accent;

    D3DDEVICE_CREATION_PARAMETERS creationParameters = { };
    if (FAILED(DirectXDevice->GetCreationParameters(&creationParameters)))
        return;

    HWND hWindow = creationParameters.hFocusWindow;
    if (hWindow == nullptr)
        return;

    ImGui_ImplWin32_Init(hWindow);
    ImGui_ImplDX9_Init(DirectXDevice);

    pic::logo = Render->LoadImageFromMemory(ui_logo, sizeof(ui_logo), Vector2(128, 128));
    pic::tab::aimbot = Render->LoadImageFromMemory(aimbot, sizeof(aimbot), Vector2(20.f, 20.f));
    pic::tab::antiaim = Render->LoadImageFromMemory(anti_aimbot, sizeof(anti_aimbot), Vector2(20.f, 20.f));
    pic::tab::visuals = Render->LoadImageFromMemory(visuals, sizeof(visuals), Vector2(20.f, 20.f));
    pic::tab::misc = Render->LoadImageFromMemory(misc, sizeof(misc), Vector2(20.f, 20.f));
    pic::tab::players = Render->LoadImageFromMemory(players, sizeof(players), Vector2(20.f, 20.f));
    pic::tab::skins = Render->LoadImageFromMemory(skins, sizeof(skins), Vector2(20.f, 20.f));
    pic::tab::configs = Render->LoadImageFromMemory(configs, sizeof(configs), Vector2(20.f, 20.f));
    pic::tab::scripts = Render->LoadImageFromMemory(scripts, sizeof(scripts), Vector2(20.f, 20.f));

    m_WindowSize  = c::background::size;
    m_ItemSpacing = ImVec2(8, 8);

    SetupUI();

    m_bIsInitialized = true;
}

void CMenu::Release() {
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::Shutdown();
}

void CMenu::Draw() {
    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    

    static bool insert_pressed = false;
    const bool is_active = ctx.active_app || (GetForegroundWindow() == FindWindowA("Valve001", nullptr));

    if (is_active && (GetAsyncKeyState(VK_INSERT) & 0x8000 || GetAsyncKeyState(VK_DELETE) & 0x8000)) {
        if (!insert_pressed) {
            m_bMenuOpened = !m_bMenuOpened;
            insert_pressed = true;
            if (m_bMenuOpened)
                Ragebot->UpdateUI();
            else
                ImGui::GetIO().MouseDown[0] = false;
        }
    }
    else {
        insert_pressed = false;
    }

    if (m_bMenuOpened) {
        ImGui::SetNextWindowSize(m_WindowSize);
        ImGui::Begin("##lambda_menu", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);
        {
            auto s = ImVec2(ImGui::GetWindowSize().x - ImGui::GetStyle().WindowPadding.x * 2, ImGui::GetWindowSize().y - ImGui::GetStyle().WindowPadding.y * 2); 
            auto p = ImVec2(ImGui::GetWindowPos().x + ImGui::GetStyle().WindowPadding.x, ImGui::GetWindowPos().y + ImGui::GetStyle().WindowPadding.y); 
            auto draw = ImGui::GetWindowDrawList();
            
            draw->AddRectFilled(p, ImVec2(p.x + s.x, p.y + s.y), ImColor(33, 33, 33)); //tabs bg
            draw->AddRectFilled(ImVec2(p.x, p.y + 20), ImVec2(p.x + s.x, p.y + s.y - 20), ImColor(25, 25, 25)); // content bg

            draw->AddLine(ImVec2(p.x, p.y + 20), ImVec2(p.x + s.x, p.y + 20), ImColor(89, 113, 162)); // tab seperator
            draw->AddLine(ImVec2(p.x, p.y + s.y - 20), ImVec2(p.x + s.x, p.y + s.y - 20), ImColor(89, 113, 162)); // bottom seperator
            
            draw->AddRect(p, ImVec2(p.x + s.x, p.y + s.y), ImColor(0, 0, 0)); // black outline
            
            const char* title = "KoolAidz";
            ImVec2 title_size = ImGui::CalcTextSize(title);
            draw->AddText(ImVec2(p.x + s.x - title_size.x - 10, p.y + (20.f - title_size.y) * 0.5f), ImColor(200, 200, 200), title);
            
            static int tabs = 0;
            ImGui::PushFont(font::general);
            ImGui::SetCursorPosX(20);
            ImGui::SetCursorPosY(6);
            ImGui::BeginGroup();
            for (int i = 0; i < (int)m_Tabs.size(); i++) {
                if (ImGui::tab(m_Tabs[i]->name.c_str(), i == tabs)) tabs = i; 
                ImGui::SameLine();
            }
            ImGui::EndGroup();
            ImGui::PopFont();
            
            ImGui::SetCursorPosY(20); // offset for content below tab separator

            for (auto group : m_Tabs[tabs]->groupboxes)
                group->Render();
        }
        ImGui::End();
    }

    Elements->Draw();
    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
}

LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool CMenu::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);

    switch (msg)
    {
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    case WM_MOUSEMOVE:
    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
        return true;
    default:
        return ImGui::GetIO().WantTextInput;
    }
}

void CMenu::RecalculateGroupboxes() {
    const float sp  = 16.f;
    const float tab_h   = 20.f; // height of top tab bar
    const float bot_h   = 20.f; // height of bottom status bar
    const float padding = 4.f;  // extra bottom breathing room

    const float content_w = m_WindowSize.x - sp * 2.f;
    const float content_h = m_WindowSize.y - tab_h - bot_h - sp - padding; // available vertical space
    const float groupbox_width = (content_w - sp) / 2.f;
    const ImVec2 base_position(sp, tab_h + sp * 0.5f);

    for (auto tab : m_Tabs) {
        std::vector<CMenuGroupbox*>& groupboxes = tab->groupboxes;
        const float gb_width = groupboxes.size() > 1 ? groupbox_width : content_w;

        float total_relative[2] = { 0.f, 0.f };
        int   n_groupboxes[2]   = { 0, 0 };
        for (auto gb : groupboxes) {
            total_relative[gb->column] += gb->relative_size;
            n_groupboxes[gb->column]++;
        }

        float available_space[2] = {
            content_h - sp * (float)(n_groupboxes[0] - 1),
            content_h - sp * (float)(n_groupboxes[1] - 1)
        };
        float current_position[2] = { 0.f, 0.f };

        for (auto gb : groupboxes) {
            gb->position.y = base_position.y + current_position[gb->column];
            gb->position.x = base_position.x + (gb_width + sp) * gb->column + (gb->column == 1 ? 8.f : 0.f);
            gb->size.x     = gb_width;
            gb->size.y     = available_space[gb->column] * (gb->relative_size / total_relative[gb->column]);

            current_position[gb->column] += gb->size.y + sp;
        }
    }
}

CMenuTab* CMenu::AddTab(const std::string& tab, DXImage icon) {
    CMenuTab* result = new CMenuTab;

    result->icon = icon.texture;
    result->icon_size = ImVec2(icon.width, icon.height);
    result->name = tab;

    m_Tabs.push_back(result);

    return result;
}

CMenuGroupbox* CMenu::AddGroupBox(const std::string& tab, const std::string& groupbox, float realtive_size, int column) {
    CMenuTab* _tab = Menu->FindTab(tab);

    if (!_tab)
        return nullptr;

    CMenuGroupbox* gb = new CMenuGroupbox;

    if (column == -1) {
        int n_columns[2]{ 0, 0 };

        for (auto gb : _tab->groupboxes) {
            n_columns[gb->column]++;
        }

        gb->column = (n_columns[0] <= n_columns[1]) ? 0 : 1;
    }
    else {
        gb->column = column;
    }

    gb->name = groupbox;
    gb->parent = _tab;

    _tab->groupboxes.push_back(gb);

    RecalculateGroupboxes();

    return gb;
}

void CMenuGroupbox::Render() {
    ImGui::SetCursorPos(position);

    ImGui::BeginGroup();
    ImGui::MenuChild(name.c_str(), size, false);

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.f);

    for (int i = 0; i < widgets.size(); i++) {
        auto el = widgets[i];
        if (!el || !el->visible || el->GetType() == WidgetType::ColorPicker || el->GetType() == WidgetType::KeyBind)
            continue;

        el->Render();
    }

    ImGui::EndChild();
    ImGui::EndGroup();
}

bool CKeyBind::get() {
    if (key == 0)
        return false;

    if (mode == 2)
        return true;

    if (!ctx.KeysBlocked() && GetAsyncKeyState(key) & 0x8000) {
        if (!pressed_once) {
            pressed_once = true;
            toggled = !toggled;
        }
    }
    else {
        pressed_once = false;
    }

    if (mode == 1) {
        return toggled;
    }
    else {
        return !ctx.KeysBlocked() && (GetAsyncKeyState(key) & 0x8000);
    }
};

void CCheckBox::Render() {
    if (ImGui::Checkbox(name.c_str(), &value)) {
        for (auto& cb : callbacks)
            cb();
        for (auto lcb : lua_callbacks) {
            auto res = lcb.func();
            if (!res.valid()) {
                sol::error er = res;
                Console->Error(er.what());
            }
        }
    }

    if (additional) {
        ImGui::SetCursorPos(ImGui::GetCursorPos() - ImVec2(0, (ImGui::GetStyle().ItemSpacing.y * 2) + 4));
        additional->Render();
    }
}

void CSliderInt::Render() {
    ImGui::SetNextItemWidth(-1);

    if (ImGui::SliderInt(name.c_str(), &value, min, max, format.c_str(), flags)) {
        for (auto& cb : callbacks)
            cb();
        for (auto lcb : lua_callbacks) {
            auto res = lcb.func();
            if (!res.valid()) {
                sol::error er = res;
                Console->Error(er.what());
            }
        }
    }
}

void CSliderFloat::Render() {
    ImGui::SetNextItemWidth(-1);

    if (ImGui::SliderFloat(name.c_str(), &value, min, max, format.c_str(), flags)) {
        for (auto& cb : callbacks)
            cb();
        for (auto lcb : lua_callbacks) {
            auto res = lcb.func();
            if (!res.valid()) {
                sol::error er = res;
                Console->Error(er.what());
            }
        }
    }
}

void CKeyBind::Render() {
    if (ImGui::Keybind(name.c_str(), &key, &mode, false)) {
        if (key == VK_ESCAPE)
            key = 0;

        for (auto& cb : callbacks)
            cb();
        for (auto lcb : lua_callbacks) {
            auto res = lcb.func();
            if (!res.valid()) {
                sol::error er = res;
                Console->Error(er.what());
            }
        }
    }
}

void CLabel::Render() {
    ImGui::Text(name.c_str());

    if (additional) {
        ImGui::SetCursorPos(ImGui::GetCursorPos() - ImVec2(0, (ImGui::GetStyle().ItemSpacing.y * 2) + 4));
        additional->Render();
    }
}

void CColorPicker::Render() {
    if (ImGui::ColorEdit4(name.c_str(), value, ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar | (has_alpha ? 0 : ImGuiColorEditFlags_NoAlpha))) {
        for (auto& cb : callbacks)
            cb();
        for (auto lcb : lua_callbacks) {
            auto res = lcb.func();
            if (!res.valid()) {
                sol::error er = res;
                Console->Error(er.what());
            }
        }
    }
}

void CComboBox::Render() {
    ImGui::SetNextItemWidth(-1);

    if (ImGui::Combo(name.c_str(), &value, elements.data(), static_cast<int>(elements.size()), 5, ImGui::GetContentRegionMax().x - ImGui::GetStyle().WindowPadding.x)) {
        for (auto& cb : callbacks)
            cb();
        for (auto lcb : lua_callbacks) {
            auto res = lcb.func();
            if (!res.valid()) {
                sol::error er = res;
                Console->Error(er.what());
            }
        }
    }
}

void CMultiCombo::Render() {
    ImGui::SetNextItemWidth(-1);

    if (ImGui::MultiCombo(name.c_str(), value, elements.data(), elements.size(), ImGui::GetContentRegionMax().x - ImGui::GetStyle().WindowPadding.x)) {
        for (auto& cb : callbacks)
            cb();
        for (auto lcb : lua_callbacks) {
            auto res = lcb.func();
            if (!res.valid()) {
                sol::error er = res;
                Console->Error(er.what());
            }
        }
    }
}

void CButton::Render() {
    if (ImGui::Button(name.c_str())) {
        for (auto& cb : callbacks)
            cb();
        for (auto lcb : lua_callbacks) {
            auto res = lcb.func();
            if (!res.valid()) {
                sol::error er = res;
                Console->Error(er.what());
            }
        }
    }
}

void CInputBox::Render() {
    if (ImGui::TextField(name.c_str(), nullptr, buf, 64, flags)) {
        for (auto& cb : callbacks)
            cb();
        for (auto lcb : lua_callbacks) {
            auto res = lcb.func();
            if (!res.valid()) {
                sol::error er = res;
                Console->Error(er.what());
            }
        }
    }
}

CMenuTab* CMenu::FindTab(const std::string& name) {
    for (auto tab : m_Tabs)
        if (tab->name == name)
            return tab;

    return nullptr;
}

CMenuGroupbox* CMenu::FindGroupbox(const std::string& tab, const std::string& groupbox) {
    CMenuTab* _tab = FindTab(tab);

    if (!_tab)
        return nullptr;

    for (auto gb : _tab->groupboxes) {
        if (gb->name == groupbox)
            return gb;
    }

    return nullptr;
}

IBaseWidget* CMenu::FindItem(const std::string& tab, const std::string& groupbox, const std::string& name, WidgetType type) {
    CMenuGroupbox* gb = FindGroupbox(tab, groupbox);

    if (!gb)
        return nullptr;

    for (auto item : gb->widgets)
        if (item->name == name && ((type == WidgetType::Any && item->GetType() != WidgetType::Label) || type == item->GetType()))
            return item;

    return nullptr;
}

std::vector<IBaseWidget*> CMenu::GetKeyBinds() {
    return m_KeyBinds;
}

void CMenu::RemoveItem(IBaseWidget* widget) {
    CMenuGroupbox* gb = widget->parent;

    if (widget->GetType() == WidgetType::KeyBind) {
        for (auto it = m_KeyBinds.begin(); it != m_KeyBinds.end();) {
            if (*it == widget) {
                it = m_KeyBinds.erase(it);
                continue;
            }

            it++;
        }
    }

    for (auto it = gb->widgets.begin(); it != gb->widgets.end(); it++) {
        if (*it == widget) {
            if (widget->GetType() == WidgetType::MultiCombo || widget->GetType() == WidgetType::Combo) {
                CComboBox* box = reinterpret_cast<CComboBox*>(widget);

                for (auto st : box->elements)
                    delete[] st;
            }

            gb->widgets.erase(it);

            delete widget;
            return;
        }
    }
}

void CMenu::RemoveGroupBox(CMenuGroupbox* gb) {
    for (auto it = gb->parent->groupboxes.begin(); it != gb->parent->groupboxes.end(); it++) {
        if (*it == gb) {
            gb->parent->groupboxes.erase(it);

            delete gb;
            return;
        }
    }
}

void CMenu::RemoveTab(CMenuTab* tab) {
    for (auto it = m_Tabs.begin(); it != m_Tabs.end(); it++) {
        if (*it == tab) {
            m_Tabs.erase(it);

            delete tab;
            return;
        }
    }
}

CCheckBox* CMenuGroupbox::AddCheckBox(const std::string& name, bool init) {
    CCheckBox* item = new CCheckBox;

    item->name = name;
    item->parent = this;
    item->value = init;

    widgets.push_back(item);

    return item;
}

CSliderInt* CMenuGroupbox::AddSliderInt(const std::string& name, int min, int max, int init, const std::string& format, ImGuiSliderFlags flags) {
    CSliderInt* item = new CSliderInt;

    item->name = name;
    item->parent = this;
    item->min = min;
    item->max = max;
    item->value = init;
    item->format = format;
    item->flags = flags;

    widgets.push_back(item);

    return item;
}

CSliderFloat* CMenuGroupbox::AddSliderFloat(const std::string& name, float min, float max, float init, const std::string& format, ImGuiSliderFlags flags) {
    CSliderFloat* item = new CSliderFloat;

    item->name = name;
    item->parent = this;
    item->min = min;
    item->max = max;
    item->value = init;
    item->format = format;
    item->flags = flags;

    widgets.push_back(item);

    return item;
}

CKeyBind* CMenuGroupbox::AddKeyBind(const std::string& name) {
    IBaseWidget* parent_item = Menu->FindItem(parent->name, this->name, name);

    if (!parent_item)
        parent_item = AddLabel(name);

    CKeyBind* item = new CKeyBind;

    item->name = name;
    item->parent = this;
    item->parent_item = parent_item;
    parent_item->additional = item;

    widgets.push_back(item);
    Menu->m_KeyBinds.push_back(item);

    return item;
}

CLabel* CMenuGroupbox::AddLabel(const std::string& name) {
    CLabel* item = new CLabel;

    item->name = name;
    item->parent = this;

    widgets.push_back(item);

    return item;
}

CColorPicker* CMenuGroupbox::AddColorPicker(const std::string& name, Color init, bool has_alpha) {
    IBaseWidget* parent_item = Menu->FindItem(parent->name, this->name, name);

    if (!parent_item)
        parent_item = AddLabel(name);

    CColorPicker* item = new CColorPicker;

    item->name = name;
    item->parent = this;
    item->parent_item = parent_item;
    parent_item->additional = item;
    item->value[0] = init.r / 255.f;
    item->value[1] = init.g / 255.f;
    item->value[2] = init.b / 255.f;
    item->value[3] = init.a / 255.f;
    item->has_alpha = has_alpha;

    widgets.push_back(item);

    return item;
}

CComboBox* CMenuGroupbox::AddComboBox(const std::string& name, std::vector<std::string> items) {
    CComboBox* item = new CComboBox;

    item->name = name;
    item->parent = this;

    std::vector<const char*> elems;
    for (auto st : items) {
        char* buf = new char[st.size() + 1];
        memcpy(buf, st.c_str(), st.size());
        buf[st.size()] = 0;
        elems.push_back(buf);
    }
    item->elements = elems;

    widgets.push_back(item);

    return item;
}

CMultiCombo* CMenuGroupbox::AddMultiCombo(const std::string& name, std::vector<std::string> items) {
    CMultiCombo* item = new CMultiCombo;

    item->name = name;
    item->parent = this;

    std::vector<const char*> elems;
    for (auto st : items) {
        char* buf = new char[st.size() + 1];
        memcpy(buf, st.c_str(), st.size());
        buf[st.size()] = 0;
        elems.push_back(buf);
    }
    item->elements = elems;

    widgets.push_back(item);

    return item;
}

CButton* CMenuGroupbox::AddButton(const std::string& name) {
    CButton* item = new CButton;

    item->name = name;
    item->parent = this;

    widgets.push_back(item);

    return item;
}

CInputBox* CMenuGroupbox::AddInput(const std::string& name, const std::string& init, ImGuiInputTextFlags flags) {
    CInputBox* item = new CInputBox;

    item->name = name;
    item->parent = this;
    item->flags = flags;
    
    memset(item->buf, 0, 64);
    std::memcpy(item->buf, init.c_str(), min(init.size(), 64));

    widgets.push_back(item);

    return item;
}