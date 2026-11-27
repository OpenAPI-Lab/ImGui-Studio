#pragma once

#include "ui.hpp"

inline void RenderTitleBarHelp(const Barra_de_ttuloState& state, bool& helpOpen) {
    if (state.chrome_help) helpOpen = true;
    if (!helpOpen) return;

    if (ImGui::Begin("Ayuda", &helpOpen)) {
        ImGui::TextUnformatted("Ayuda del panel de actividad.");
    }
    ImGui::End();
}
