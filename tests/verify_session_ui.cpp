#include "editor/studio_ui.hpp"
#include "imgui.h"
#include "imgui_internal.h"

#include <cassert>
#include <iostream>

int main() {
  using namespace ImGuiStudio;
  const auto root = std::filesystem::temp_directory_path() / nextId("studio_ui_test");
  std::filesystem::create_directory(root);
  const auto recent = root / std::filesystem::u8path("Proyecto reciente ñ.json");
  const auto saved = createDefaultProject("Proyecto reciente");
  assert(!saved.nodes.front().props.showCollapseArrow);
  assert(writeTextFileAtomically(projectToJson(saved).dump(), recent));
  assert(writeTextFileAtomically(Json::array({recent.u8string()}).dump(), root / "recent_projects.json"));
  const auto localeDir = root / "locales";
  std::filesystem::create_directory(localeDir);
  const auto veryLong = std::string(400, 'W');
  const Json custom{{"name", "English (large labels)"}, {"strings", {
    {"WelcomeNew", veryLong}, {"WelcomeOpen", "Open 100% compatible projects"},
    {"File", veryLong}, {"Language", "Invalid###id"}, {"PreviewReset", false}}}};
  assert(writeTextFileAtomically(custom.dump(), localeDir / "en-wide.json"));
  assert(writeTextFileAtomically("{broken", localeDir / "broken.json"));
  {
    StudioLocale locale;
    locale.load(root);
    assert(locale.size() == 7 && locale.id(locale.selected()) == "es");
    assert(locale.select(6));
    StudioLocale restored;
    restored.load(root);
    assert(restored.id(restored.selected()) == "en-wide");
    assert(restored.text(StudioText::WelcomeNew) == veryLong);
    assert(std::string(restored.text(StudioText::Language)) == "Language");
    assert(std::string(restored.text(StudioText::PreviewReset)) == "Reset preview");
    assert(restored.select(0));
  }
  {
    RecoverySession interrupted;
    assert(interrupted.initialize(root));
    RecoveryDocument document;
    document.project = createDefaultProject("UI recovery fixture");
    document.savedAtMs = nowUnixMs();
    document.snapshots.saveSnapshot(document.project, "Checkpoint", SnapshotKind::Manual);
    assert(interrupted.checkpoint(document));
  }
#if defined(_WIN32)
  const auto oldValue = _wgetenv(L"IMGUI_STUDIO_STATE_DIR");
  const std::wstring original = oldValue ? oldValue : L"";
  _wputenv_s(L"IMGUI_STUDIO_STATE_DIR", root.c_str());
#else
  const auto oldValue = std::getenv("IMGUI_STUDIO_STATE_DIR");
  const std::string original = oldValue ? oldValue : "";
  const bool wasSet = oldValue != nullptr;
  setenv("IMGUI_STUDIO_STATE_DIR", root.c_str(), 1);
#endif
  ImGui::CreateContext();
  auto& io = ImGui::GetIO();
  io.IniFilename = nullptr;
  io.DisplaySize = ImVec2(1440, 860);
  io.DeltaTime = 1.0f / 60.0f;
  unsigned char* pixels = nullptr;
  int width = 0, height = 0;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
  const auto frame = [&](StudioApp& editor) {
    ImGui::NewFrame();
    editor.renderFrame();
    ImGui::Render();
  };
  const auto welcomeContent = [] {
    const auto* window = ImGui::FindWindowByName("##StudioWelcome");
    assert(window && window->Active && window->DC.ChildWindows.Size == 1);
    return window->DC.ChildWindows[0];
  };
  const auto popupOpen = [](const char* name) {
    const auto* parent = ImGui::FindWindowByName("Debug##Default");
    assert(parent);
    return ImGui::IsPopupOpen(ImHashStr(name, 0, parent->ID), ImGuiPopupFlags_AnyPopupLevel);
  };
  const auto addWindow = [&](StudioApp& editor) {
    const auto* palette = ImGui::FindWindowByName("##WidgetPalette");
    assert(palette && palette->Active && palette->DC.ChildWindows.Size == 1);
    const auto* items = palette->DC.ChildWindows[0];
    ImGui::ActivateItemByID(ImHashStr("###PaletteWindow", 0, items->ID));
    frame(editor); frame(editor);
  };
  const auto selectProjectEntry = [&](StudioApp& editor, ImGuiWindow* dialog,
      const std::filesystem::path& path) {
    ProjectFileBrowser browser;
    assert(!browser.selectFolder(path.parent_path()));
    const auto row = std::find_if(browser.entries.begin(), browser.entries.end(), [&](const auto& entry) {
      return entry.path == path;
    });
    assert(row != browser.entries.end());
    assert(dialog->DC.ChildWindows.Size == 1);
    const int index = static_cast<int>(std::distance(browser.entries.begin(), row));
    const auto entryId = ImHashData(&index, sizeof(index), dialog->DC.ChildWindows[0]->ID);
    ImGui::ActivateItemByID(ImHashStr("###project_entry", 0, entryId));
    frame(editor); frame(editor);
  };
  const auto chooseLanguage = [&](StudioApp& editor, const char* option) {
    const auto* content = welcomeContent();
    ImGui::ActivateItemByID(ImHashStr("###Language", 0, content->ID));
    frame(editor);
    auto* languages = ImGui::FindWindowByName("##Combo_00");
    assert(languages && languages->Active);
    ImGui::ActivateItemByID(languages->GetID(option));
    frame(editor); frame(editor);
  };
  {
    StudioApp editor;
    frame(editor); frame(editor);
    assert(popupOpen("###RecoveryTitle"));
    welcomeContent();
    auto* modal = ImGui::FindWindowByName("###RecoveryTitle");
    assert(modal && modal->DC.ChildWindows.Size == 1);
    assert(std::string(modal->Name).find("Recuperar") == 0);
    const auto* list = modal->DC.ChildWindows[0];
    const int first = 0;
    const auto rowId = ImHashData(&first, sizeof(first), list->ID);
    ImGui::ActivateItemByID(ImHashStr("###DiscardRecovery", 0, rowId));
    frame(editor);
    ImGui::ActivateItemByID(modal->GetID("###DiscardConfirm"));
    frame(editor); frame(editor);
    assert(!popupOpen("###RecoveryTitle"));
    chooseLanguage(editor, "###locale_en");
    assert(Json::parse(readTextFile(root / "language.json", 8192).text).at("language") == "en");
    assert(editor.requestClose());
  }
  {
    StudioApp editor;
    frame(editor); frame(editor);
    assert(!popupOpen("###RecoveryTitle"));
    const auto* content = welcomeContent();
    ImGui::ActivateItemByID(ImHashStr("###WelcomeOpen", 0, content->ID));
    frame(editor); frame(editor);
    auto* fileDialog = ImGui::FindWindowByName("###OpenJson");
    assert(fileDialog && fileDialog->Active && std::string(fileDialog->Name).find("Open JSON project") == 0);
    ImGui::ActivateItemByID(fileDialog->GetID("###Cancel"));
    frame(editor); frame(editor);
    const auto invalid = root / "malformed.json";
    assert(writeTextFileAtomically("{malformed", invalid));
    ImGui::ActivateItemByID(ImHashStr("###WelcomeOpen", 0, content->ID));
    frame(editor); frame(editor);
    assert(popupOpen("###OpenJson"));
    selectProjectEntry(editor, fileDialog, invalid);
    ImGui::ActivateItemByID(fileDialog->GetID("###Open"));
    frame(editor); frame(editor);
    assert(popupOpen("###OpenJson"));
    assert(welcomeContent()->Active);
    selectProjectEntry(editor, fileDialog, recent);
    ImGui::ActivateItemByID(fileDialog->GetID("###Open"));
    frame(editor); frame(editor);
    assert(!popupOpen("###OpenJson"));
    assert(projectToJson(editor.project()) == projectToJson(saved));
    addWindow(editor);
    assert(editor.project().nodes.size() == 2);
    io.AddKeyEvent(ImGuiMod_Ctrl, true);
    io.AddKeyEvent(ImGuiKey_S, true);
    frame(editor);
    io.AddKeyEvent(ImGuiKey_S, false);
    io.AddKeyEvent(ImGuiMod_Ctrl, false);
    frame(editor);
    const auto savedAgain = loadProjectFile(recent);
    assert(savedAgain && savedAgain.project->nodes.size() == 2);
    addWindow(editor);
    assert(editor.project().nodes.size() == 3 && !editor.requestClose());
    frame(editor);
    auto* unsaved = ImGui::FindWindowByName("###UnsavedTitle");
    assert(unsaved && popupOpen("###UnsavedTitle"));
    assert(std::string(unsaved->Name).find("Unsaved changes") == 0);
    ImGui::ActivateItemByID(unsaved->GetID("###DiscardChanges"));
    frame(editor);
    assert(editor.shouldClose());
    const auto final = loadProjectFile(recent);
    assert(final && final.project->nodes.size() == 2);
  }
  {
    const auto destinationFolder = root / std::filesystem::u8path(u8"Carpeta de prueba ñ");
    std::filesystem::create_directory(destinationFolder);
    ProjectFileBrowser browser;
    assert(!browser.selectFolder(root));
    browser.filename = "../invalid";
    assert(!browser.destination());
    assert(!browser.selectFolder(destinationFolder));
    browser.filename = u8"Copia portátil";
    assert(browser.destination() == destinationFolder / std::filesystem::u8path(u8"Copia portátil.json"));

    StudioApp editor;
    frame(editor); frame(editor);
    const auto* content = welcomeContent();
    const int first = 0;
    const auto recentId = ImHashData(&first, sizeof(first), content->ID);
    ImGui::ActivateItemByID(ImHashStr("###recent", 0, recentId));
    frame(editor); frame(editor);
    assert(!ImGui::FindWindowByName("##StudioWelcome")->Active);
    const auto launchSaveAs = [&] {
      io.AddKeyEvent(ImGuiMod_Ctrl, true);
      io.AddKeyEvent(ImGuiMod_Shift, true);
      io.AddKeyEvent(ImGuiKey_S, true);
      frame(editor);
      io.AddKeyEvent(ImGuiKey_S, false);
      io.AddKeyEvent(ImGuiMod_Shift, false);
      io.AddKeyEvent(ImGuiMod_Ctrl, false);
      frame(editor);
      auto* dialog = ImGui::FindWindowByName("###SaveAs");
      assert(dialog && dialog->Active && popupOpen("###SaveAs"));
      return dialog;
    };
    auto* dialog = launchSaveAs();
    ImGui::ActivateItemByID(dialog->GetID("###Cancel"));
    frame(editor); frame(editor);
    assert(!popupOpen("###SaveAs"));

    dialog = launchSaveAs();
    selectProjectEntry(editor, dialog, destinationFolder);
    const auto field = dialog->GetID("##SaveFilename");
    ImGui::ActivateItemByID(field);
    frame(editor); frame(editor);
    assert(ImGui::GetActiveID() == field);
    io.AddKeyEvent(ImGuiMod_Ctrl, true); io.AddKeyEvent(ImGuiKey_A, true); frame(editor);
    io.AddKeyEvent(ImGuiKey_A, false); io.AddKeyEvent(ImGuiMod_Ctrl, false); frame(editor);
    io.AddInputCharactersUTF8(u8"Copia portátil"); frame(editor);
    ImGui::ActivateItemByID(dialog->GetID("###Save"));
    frame(editor); frame(editor);
    const auto copy = loadProjectFile(destinationFolder / std::filesystem::u8path(u8"Copia portátil.json"));
    assert(copy && projectToJson(*copy.project) == projectToJson(editor.project()));
    assert(!popupOpen("###SaveAs"));
    addWindow(editor);
    io.AddKeyEvent(ImGuiMod_Ctrl, true); io.AddKeyEvent(ImGuiKey_O, true);
    frame(editor);
    io.AddKeyEvent(ImGuiKey_O, false); io.AddKeyEvent(ImGuiMod_Ctrl, false);
    frame(editor);
    auto* unsaved = ImGui::FindWindowByName("###UnsavedTitle");
    assert(unsaved && popupOpen("###UnsavedTitle"));
    ImGui::ActivateItemByID(unsaved->GetID("###DiscardChanges"));
    frame(editor); frame(editor);
    auto* openDialog = ImGui::FindWindowByName("###OpenJson");
    assert(openDialog && popupOpen("###OpenJson"));
    ImGui::ActivateItemByID(openDialog->GetID("###ParentFolder"));
    frame(editor); frame(editor);
    selectProjectEntry(editor, openDialog, recent);
    ImGui::ActivateItemByID(openDialog->GetID("###Open"));
    frame(editor); frame(editor);
    assert(!popupOpen("###OpenJson"));
    assert(projectToJson(editor.project()) == projectToJson(*loadProjectFile(recent).project));
    assert(editor.requestClose());
  }
  {
    StudioApp editor(true, saved);
    frame(editor); frame(editor);
    addWindow(editor);
    assert(!editor.requestClose());
    frame(editor);
    auto* unsaved = ImGui::FindWindowByName("###UnsavedTitle");
    assert(unsaved && popupOpen("###UnsavedTitle"));
    ImGui::ActivateItemByID(unsaved->GetID("###SaveContinue"));
    frame(editor); frame(editor);
    auto* dialog = ImGui::FindWindowByName("###SaveAs");
    assert(dialog && popupOpen("###SaveAs"));
    ImGui::ActivateItemByID(dialog->GetID("###Cancel"));
    frame(editor); frame(editor);
    assert(!editor.shouldClose() && editor.project().nodes.size() == 2);
    assert(!editor.requestClose());
    frame(editor);
    assert(popupOpen("###UnsavedTitle"));
    ImGui::ActivateItemByID(unsaved->GetID("###DiscardChanges"));
    frame(editor);
    assert(editor.shouldClose());
  }
  {
    assert(saveEditorAppearance(root, EditorAppearance{5, 14}));
    StudioApp editor;
    frame(editor); frame(editor);
    assert(ImGui::GetStyle().WindowRounding == 14.0f);
    assert(editor.workspaceColor().x == editorColor(editorPalettes[5].workspace).x);
    ImGui::ActivateItemByID(welcomeContent()->GetID("###EditorAppearance"));
    frame(editor); frame(editor);
    auto* settings = ImGui::FindWindowByName("###EditorAppearance");
    assert(settings && settings->Active);
    ImGui::ActivateItemByID(settings->GetID("##EditorPalette"));
    frame(editor);
    auto* palette = ImGui::FindWindowByName("##Combo_00");
    assert(palette && palette->Active);
    ImGui::ActivateItemByID(palette->GetID("Ocean"));
    frame(editor); frame(editor);
    assert(loadEditorAppearance(root).palette == 2);
    assert(editor.workspaceColor().x == editorColor(editorPalettes[2].workspace).x);
    assert(editor.requestClose());
  }
  {
    StudioApp editor;
    io.DisplaySize = ImVec2(720, 480);
    frame(editor); frame(editor);
    assert(!popupOpen("###RecoveryTitle"));
    for (const auto* id : {"fr", "ko", "ja", "zh"}) {
      const auto option = std::string("###locale_") + id;
      chooseLanguage(editor, option.c_str());
      assert(Json::parse(readTextFile(root / "language.json", 8192).text).at("language") == id);
      assert(welcomeContent()->ContentSizeIdeal.x < welcomeContent()->Size.x);
    }
    chooseLanguage(editor, "###locale_en-wide");
    assert(Json::parse(readTextFile(root / "language.json", 8192).text).at("language") == "en-wide");
    auto* content = welcomeContent();
    assert(content->ContentSizeIdeal.x < content->Size.x);
    ImGui::ActivateItemByID(content->GetID("###WelcomeNew"));
    frame(editor); frame(editor);
    assert(editor.project().name == "NuevoProyecto");
    assert(editor.requestClose());
  }
  std::string reusableId;
  {
    io.DisplaySize = ImVec2(1440, 860);
    StudioApp editor(true, saved);
    frame(editor); frame(editor);
    const auto activate = [&](ImGuiID id) {
      ImGui::ActivateItemByID(id);
      frame(editor); frame(editor);
    };
    const auto type = [&](ImGuiID id, const std::string& value) {
      activate(id);
      assert(ImGui::GetActiveID() == id);
      io.AddInputCharactersUTF8(value.c_str()); frame(editor);
      io.AddKeyEvent(ImGuiKey_Enter, true); frame(editor);
      io.AddKeyEvent(ImGuiKey_Enter, false); frame(editor);
    };
    const auto selectTab = [&](const char* key) {
      const auto* library = ImGui::FindWindowByName("###Library");
      assert(library && library->Active);
      const auto barId = ImHashStr("##WidgetLibraryTabs", 0, library->ID);
      auto* tabs = ImGui::GetCurrentContext()->TabBars.GetByKey(barId);
      assert(tabs && tabs->Tabs.Size == 4);
      const auto id = ImHashStr(key, 0, barId);
      for (const auto& tab : tabs->Tabs) {
        if (tab.ID != id) continue;
        io.AddMousePosEvent(tabs->BarRect.Min.x + tab.Offset + tab.Width * 0.5f, tabs->BarRect.GetCenter().y);
        frame(editor);
        io.AddMouseButtonEvent(0, true); frame(editor);
        io.AddMouseButtonEvent(0, false); frame(editor); frame(editor);
        assert(tabs->SelectedTabId == id);
        return id;
      }
      std::abort();
    };
    const auto paletteItems = [] {
      const auto* win = ImGui::FindWindowByName("##WidgetPalette");
      assert(win && win->DC.ChildWindows.Size == 1);
      return win->DC.ChildWindows[0]->ID;
    };
    const auto toolbar = [] {
      auto* window = ImGui::FindWindowByName("##MainMenuBar");
      assert(window);
      return window->GetID("##menubar");
    };

    const auto* palette = ImGui::FindWindowByName("##WidgetPalette");
    activate(ImHashStr("###Library", 0, palette->ID));
    const auto createTab = selectTab("###LibraryCreate");
    type(ImHashStr("##LibraryName", 0, createTab), "Grupo reusable");
    activate(ImHashStr("###LibraryCreateDesign", 0, createTab));
    assert(!ImGui::FindWindowByName("###Library")->Active);
    activate(ImHashStr("###PaletteText", 0, paletteItems()));
    activate(ImHashStr("###PaletteButton", 0, paletteItems()));
    activate(ImHashStr("###LibrarySave", 0, toolbar()));
    assert(projectToJson(editor.project()) == projectToJson(saved));
    StudioWidgetLibrary library;
    assert(library.load(root) && library.entries().size() == 1);
    reusableId = library.entries().front().id;
    assert(library.entries().front().node->type == WidgetType::Group);
    assert(library.entries().front().node->children.size() == 2);

    const auto ownTab = selectTab("###LibraryCustom");
    activate(ImHashStr("###LibraryEdit", 0, ownTab));
    const auto nameField = ImHashStr("##WidgetDraftName", 0, toolbar());
    activate(nameField);
    assert(ImGui::GetActiveID() == nameField);
    io.AddKeyEvent(ImGuiMod_Ctrl, true); io.AddKeyEvent(ImGuiKey_A, true); frame(editor);
    io.AddKeyEvent(ImGuiKey_A, false); io.AddKeyEvent(ImGuiMod_Ctrl, false); frame(editor);
    io.AddInputCharactersUTF8("Grupo editado"); frame(editor);
    io.AddKeyEvent(ImGuiKey_Enter, true); frame(editor);
    io.AddKeyEvent(ImGuiKey_Enter, false); frame(editor);
    activate(ImHashStr("###LibrarySave", 0, toolbar()));
    assert(library.load(root) && library.find(reusableId)->name == "Grupo editado");

    selectTab("###LibraryCreate");
    type(ImHashStr("##LibraryName", 0, createTab), "Casilla reusable");
    ImGui::ActivateItemByID(ImHashStr("##LibraryBaseControl", 0, createTab));
    frame(editor);
    auto* templates = ImGui::FindWindowByName("##Combo_00");
    assert(templates && templates->Active);
    activate(templates->GetID("###PaletteCheckbox"));
    activate(ImHashStr("###LibraryCreateDesign", 0, createTab));
    activate(ImHashStr("###LibrarySave", 0, toolbar()));
    assert(library.load(root) && library.entries().size() == 2);
    assert(library.entries().back().node->type == WidgetType::Checkbox);
    assert(projectToJson(editor.project()) == projectToJson(saved));

    const auto path = root / std::filesystem::u8path(u8"widget portátil.json");
    assert(library.exportFile(*library.find(reusableId), path));
    const auto importTab = selectTab("###LibraryImport");
    type(ImHashStr("##LibraryImportPath", 0, importTab), path.u8string());
    activate(ImHashStr("###LibraryImport", 0, importTab));
    assert(library.load(root) && library.entries().size() == 3);

    selectTab("###LibraryCustom");
    activate(ImHashStr("###LibraryInsert", 0, ownTab));
    const auto& children = editor.project().nodes.front().children;
    assert(children.size() == 1 && children.front().type == WidgetType::Group);
    assert(children.front().children.size() == 2 && editor.undoCount() == 1);
    assert(!editor.requestClose());
    frame(editor);
    auto* unsaved = ImGui::FindWindowByName("###UnsavedTitle");
    activate(unsaved->GetID("###DiscardChanges"));
    assert(editor.shouldClose());
  }
  {
    KnobsPackage files;
    for (size_t i = 0; i < files.size(); ++i) files[i] = KnobsSource::files[i].contents;
    assert(installKnobsPackage(root, files));
    StudioApp reopened(true, saved);
    frame(reopened); frame(reopened);
    const auto* palette = ImGui::FindWindowByName("##WidgetPalette");
    assert(palette && palette->DC.ChildWindows.Size == 1);
    auto* list = palette->DC.ChildWindows[0];
    ImGui::SetScrollY(list, list->ScrollMax.y);
    frame(reopened); frame(reopened);
    const auto entry = std::find_if(reopened.editTargets().begin(), reopened.editTargets().end(),
      [&](const auto& item) { return item.nodeId == "library/" + reusableId; });
    assert(entry != reopened.editTargets().end() && entry->paletteTemplate);
    const ImVec2 source((entry->minimum.x + entry->maximum.x) * 0.5f, (entry->minimum.y + entry->maximum.y) * 0.5f);
    assert(source.y >= entry->clipMinimum.y && source.y <= entry->clipMaximum.y);
    const auto& report = reopened.designReport().items;
    const auto anchor = std::find_if(report.begin(), report.end(), [&](const auto& item) {
      return item.nodeId == saved.nodes.front().id && item.optionIndex == -1;
    });
    assert(anchor != report.end());
    const ImVec2 drop(anchor->minimum.x + 90, anchor->minimum.y + 100);
    io.AddMousePosEvent(source.x, source.y); frame(reopened);
    io.AddMouseButtonEvent(0, true); frame(reopened);
    io.AddMousePosEvent(drop.x, drop.y); frame(reopened);
    io.AddMouseButtonEvent(0, false); frame(reopened); frame(reopened);
    assert(reopened.project().nodes.front().children.size() == 1);
    assert(reopened.project().nodes.front().children.front().children.size() == 2);
    assert(reopened.undoCount() == 1);

    ImGui::ActivateItemByID(ImHashStr("###CatalogKnob", 0, list->ID));
    frame(reopened); frame(reopened);
    assert(reopened.project().nodes.front().children.front().children.back().type == WidgetType::Knob);
    assert(reopened.designReport().limitations.empty());
    const auto withDial = projectToJson(reopened.project());

    ImGui::ActivateItemByID(ImHashStr("###Library", 0, palette->ID));
    frame(reopened); frame(reopened);
    const auto* libraryWindow = ImGui::FindWindowByName("###Library");
    assert(libraryWindow && libraryWindow->Active);
    const auto barId = ImHashStr("##WidgetLibraryTabs", 0, libraryWindow->ID);
    const auto catalogTab = ImHashStr("###ExtensionCatalog", 0, barId);
    ImGui::ActivateItemByID(catalogTab);
    frame(reopened); frame(reopened);
    ImGui::ActivateItemByID(ImHashStr("###CatalogRemove", 0, catalogTab));
    frame(reopened); frame(reopened);
    assert(!hasInstalledKnobs(root));
    assert(!reopened.designReport().limitations.empty() && projectToJson(reopened.project()) == withDial);
  }
  ImGui::DestroyContext();
#if defined(_WIN32)
  _wputenv_s(L"IMGUI_STUDIO_STATE_DIR", original.c_str());
#else
  if (wasSet) setenv("IMGUI_STUDIO_STATE_DIR", original.c_str(), 1);
  else unsetenv("IMGUI_STUDIO_STATE_DIR");
#endif
  assert(std::filesystem::weakly_canonical(root).parent_path() ==
         std::filesystem::weakly_canonical(std::filesystem::temp_directory_path()));
  assert(root.filename().string().find("studio_ui_test_") == 0);
  std::filesystem::remove_all(root);
  std::cout << "[OK] Welcome, recovery, languages and the reusable widget workflow.\n";
}
