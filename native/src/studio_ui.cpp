#include "studio_ui.hpp"
#include "studio_python_export.hpp"
#include "studio_csharp_export.hpp"
#include "studio_java_export.hpp"
#include "studio_c_export.hpp"
#include "studio_android_export.hpp"
#include "studio_apple_export.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "misc/cpp/imgui_stdlib.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <utility>
#ifdef _WIN32
#include <commdlg.h>
#endif

namespace ImGuiStudio {

namespace {

constexpr size_t kRecentProjectLimit = 10;
constexpr size_t kRecentFileLimit = 256 * 1024;

void selectCppRenderer(const std::string& label, CppRenderer& renderer) {
  if (ImGui::BeginCombo(label.c_str(),cppRendererLabel(renderer))) {
    for (const auto choice : {CppRenderer::OpenGL,CppRenderer::Vulkan})
      if (ImGui::Selectable(cppRendererLabel(choice), choice == renderer)) renderer = choice;
    ImGui::EndCombo();
  }
}

StudioText buildStatusText(BuildPhase phase) {
  switch (phase) {
    case BuildPhase::Idle: return StudioText::BuildIdle;
    case BuildPhase::Configuring: return StudioText::BuildConfiguring;
    case BuildPhase::Restoring: return StudioText::BuildRestoring;
    case BuildPhase::Building: return StudioText::BuildBuilding;
    case BuildPhase::Checking: return StudioText::BuildChecking;
    case BuildPhase::CheckingJava: return StudioText::BuildCheckingJava;
    case BuildPhase::CheckingRust: return StudioText::BuildCheckingRust;
    case BuildPhase::Ready: return StudioText::BuildReady;
    case BuildPhase::Running: return StudioText::BuildRunning;
    case BuildPhase::Stopping: return StudioText::BuildStopping;
    case BuildPhase::Cancelled: return StudioText::BuildCancelled;
    case BuildPhase::Failed: return StudioText::BuildFailed;
  }
  return StudioText::BuildFailed;
}

bool canvasWindowHovered() {
  const auto* hovered = ImGui::GetCurrentContext()->HoveredWindow;
  return hovered && (hovered->RootWindow == ImGui::GetCurrentWindow() ||
    std::strstr(hovered->RootWindow->Name, "###canvas_") != nullptr);
}

void sameLineIfFits(const char* nextLabel, float minimumWidth = 0.0f) {
  const auto& style = ImGui::GetStyle();
  const float needed = std::max(minimumWidth, ImGui::CalcTextSize(nextLabel, nullptr, true).x + 2 * style.FramePadding.x);
  const float rightEdge = ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x;
  if (ImGui::GetItemRectMax().x + style.ItemSpacing.x + needed <= rightEdge) ImGui::SameLine();
}

void clipCanvasWindow(ImVec2 minimum, ImVec2 maximum) {
  auto* window = ImGui::GetCurrentWindow();
  const ImRect viewport(minimum, maximum);
  window->OuterRectClipped.ClipWithFull(viewport);
  if (window->OuterRectClipped.GetWidth() <= 0 || window->OuterRectClipped.GetHeight() <= 0)
    window->Flags |= ImGuiWindowFlags_NoMouseInputs;

  // Begin already drew the window decorations. Clip those commands as well as
  // widgets and nested child windows, which inherit the current ImGui clip.
  for (auto& command : window->DrawList->CmdBuffer) {
    if (command.ElemCount == 0) continue;
    ImRect clipped(command.ClipRect);
    clipped.ClipWithFull(viewport);
    command.ClipRect = clipped.ToVec4();
  }
  ImGui::PushClipRect(minimum, maximum, true);
}

void copyStringToBuffer(const std::string& src, char* dst, size_t dstSize) {
  if (dstSize == 0) return;
  std::strncpy(dst, src.c_str(), dstSize - 1);
  dst[dstSize - 1] = '\0';
}

#ifdef _WIN32
std::optional<std::filesystem::path> browseForJsonFile(const StudioLocale& locale, std::string& error) {
  std::array<wchar_t, 32768> selected{};
  auto filter = std::filesystem::u8path(locale.text(StudioText::JsonFilter)).wstring();
  filter.append(L"\0*.json\0", 8);
  filter.append(std::filesystem::u8path(locale.text(StudioText::AllFiles)).wstring());
  filter.append(L"\0*.*\0", 5);
  OPENFILENAMEW picker{};
  picker.lStructSize = sizeof(picker);
  picker.hwndOwner = GetActiveWindow();
  picker.lpstrFilter = filter.c_str();
  picker.lpstrFile = selected.data();
  picker.nMaxFile = static_cast<DWORD>(selected.size());
  picker.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
  if (GetOpenFileNameW(&picker)) {
    error.clear();
    return std::filesystem::path(selected.data());
  }
  if (CommDlgExtendedError()) error = locale.text(StudioText::BrowseError);
  return std::nullopt;
}
#endif

struct PaletteEntry {
  WidgetType type;
  StudioText label;
  StudioText category;
  const char* preset = nullptr;
};

constexpr PaletteEntry kPalette[] = {
    {WidgetType::Window, StudioText::PaletteWindow, StudioText::PaletteContainers},
    {WidgetType::ChildWindow, StudioText::PaletteChild, StudioText::PaletteContainers},
    {WidgetType::Loader, StudioText::PaletteLoader, StudioText::PaletteContainers},
    {WidgetType::Component, StudioText::PaletteComponent, StudioText::PaletteContainers},
    {WidgetType::Repeater, StudioText::PaletteRepeater, StudioText::PaletteContainers},
    {WidgetType::Animation, StudioText::PaletteAnimation, StudioText::PaletteContainers},
    {WidgetType::MenuBar, StudioText::PaletteMenuBar, StudioText::PaletteContainers},
    {WidgetType::TabBar, StudioText::PaletteTabs, StudioText::PaletteContainers},
    {WidgetType::CollapsingHeader, StudioText::PaletteCollapsible, StudioText::PaletteContainers},
    {WidgetType::Table, StudioText::PaletteTable, StudioText::PaletteContainers},
    {WidgetType::Group, StudioText::PaletteGroup, StudioText::PaletteContainers},
    {WidgetType::TreeNode, StudioText::PaletteTree, StudioText::PaletteContainers},
    {WidgetType::RowLayout, StudioText::PaletteRow, StudioText::PalettePositioners},
    {WidgetType::PositionerFlow, StudioText::PaletteFlow, StudioText::PalettePositioners},
    {WidgetType::ColumnLayout, StudioText::PaletteColumn, StudioText::PalettePositioners},
    {WidgetType::GridLayout, StudioText::PaletteGrid, StudioText::PalettePositioners},
    {WidgetType::ListView, StudioText::PaletteListView, StudioText::PaletteViews},
    {WidgetType::GridView, StudioText::PaletteGridView, StudioText::PaletteViews},
    {WidgetType::PathView, StudioText::PalettePathView, StudioText::PaletteViews},
    {WidgetType::Text, StudioText::PaletteText, StudioText::PaletteControls},
    {WidgetType::TextColored, StudioText::PaletteColoredText, StudioText::PaletteControls},
    {WidgetType::Button, StudioText::PaletteButton, StudioText::PaletteControls},
    {WidgetType::TitleBarButton, StudioText::PaletteTitleButton, StudioText::PaletteControls},
    {WidgetType::Image, StudioText::Image, StudioText::PaletteControls},
    {WidgetType::ImageButton, StudioText::PaletteImageButton, StudioText::PaletteControls},
    {WidgetType::SmallButton, StudioText::PaletteSmallButton, StudioText::PaletteControls},
    {WidgetType::ArrowButton, StudioText::PaletteArrowButton, StudioText::PaletteControls},
    {WidgetType::ColorButton, StudioText::PaletteColorButton, StudioText::PaletteControls},
    {WidgetType::Selectable, StudioText::PaletteSelectable, StudioText::PaletteControls},
    {WidgetType::TextWrapped, StudioText::PaletteWrappedText, StudioText::PaletteControls},
    {WidgetType::BulletText, StudioText::PaletteBulletText, StudioText::PaletteControls},
    {WidgetType::HelpMarker, StudioText::PaletteHelpMarker, StudioText::PaletteControls},
    {WidgetType::Checkbox, StudioText::PaletteCheckbox, StudioText::PaletteControls},
    {WidgetType::RadioButton, StudioText::PaletteRadio, StudioText::PaletteControls},
    {WidgetType::SliderFloat, StudioText::PaletteFloatSlider, StudioText::PaletteControls},
    {WidgetType::SliderInt, StudioText::PaletteIntSlider, StudioText::PaletteControls},
    {WidgetType::InputText, StudioText::PaletteTextInput, StudioText::PaletteControls},
    {WidgetType::InputMultiline, StudioText::PaletteMultiline, StudioText::PaletteControls},
    {WidgetType::InputFloat, StudioText::PaletteFloatInput, StudioText::PaletteControls},
    {WidgetType::InputInt, StudioText::PaletteIntInput, StudioText::PaletteControls},
    {WidgetType::SliderFloat2, StudioText::PaletteSlider2, StudioText::PaletteControls},
    {WidgetType::SliderFloat3, StudioText::PaletteSlider3, StudioText::PaletteControls},
    {WidgetType::SliderAngle, StudioText::PaletteAngleSlider, StudioText::PaletteControls},
    {WidgetType::DragFloat, StudioText::PaletteDragFloat, StudioText::PaletteControls},
    {WidgetType::DragFloat3, StudioText::PaletteDragFloat3, StudioText::PaletteControls},
    {WidgetType::DragInt, StudioText::PaletteDragInt, StudioText::PaletteControls},
    {WidgetType::ColorEdit3, StudioText::PaletteRgb, StudioText::PaletteControls},
    {WidgetType::Listbox, StudioText::PaletteListBox, StudioText::PaletteControls},
    {WidgetType::Combo, StudioText::PaletteCombo, StudioText::PaletteControls},
    {WidgetType::ColorEdit4, StudioText::PaletteRgba, StudioText::PaletteControls},
    {WidgetType::ProgressBar, StudioText::PaletteProgress, StudioText::PaletteControls},
    {WidgetType::Toggle, StudioText::PaletteToggle, StudioText::PaletteIndicatorGroup},
    {WidgetType::Spinner, StudioText::PaletteSpinner, StudioText::PaletteIndicatorGroup},
    {WidgetType::CircularProgress, StudioText::PaletteCircularProgress, StudioText::PaletteIndicatorGroup},
    {WidgetType::Chart2d, StudioText::PaletteLine2d, StudioText::PaletteCharts2d, "line"},
    {WidgetType::Chart2d, StudioText::PaletteArea2d, StudioText::PaletteCharts2d, "area"},
    {WidgetType::Chart2d, StudioText::PaletteBar2d, StudioText::PaletteCharts2d, "bar"},
    {WidgetType::Chart2d, StudioText::PaletteDonut, StudioText::PaletteCharts2d, "pie"},
    {WidgetType::Chart2d, StudioText::PaletteScatter2d, StudioText::PaletteCharts2d, "scatter"},
    {WidgetType::Chart2d, StudioText::PaletteSpline, StudioText::PaletteCharts2d, "spline"},
    {WidgetType::Chart3d, StudioText::PaletteBar3d, StudioText::PaletteCharts3d, "bars"},
    {WidgetType::Chart3d, StudioText::PaletteScatter3d, StudioText::PaletteCharts3d, "scatter"},
    {WidgetType::Chart3d, StudioText::PaletteSurface3d, StudioText::PaletteCharts3d, "surface"},
    {WidgetType::PlotLines, StudioText::PaletteCompactLine, StudioText::PaletteCompactCharts},
    {WidgetType::PlotHistogram, StudioText::PaletteCompactHist, StudioText::PaletteCompactCharts},
    {WidgetType::Separator, StudioText::PaletteSeparator, StudioText::PaletteUtilities},
    {WidgetType::Spacing, StudioText::PaletteSpacing, StudioText::PaletteUtilities},
    {WidgetType::SeparatorText, StudioText::PaletteTextSeparator, StudioText::PaletteUtilities},
    {WidgetType::Dummy, StudioText::PaletteDummy, StudioText::PaletteUtilities},
    {WidgetType::SameLine, StudioText::PaletteSameLine, StudioText::PaletteUtilities},
    {WidgetType::Knob, StudioText::CatalogKnob, StudioText::ExtensionsInstalled},
};

}  // namespace

StudioApp::StudioApp(bool persistenceEnabled, std::optional<StudioProject> initialProject, NativeImageCache* images)
  : previewRenderer_(images), designRenderer_(images), welcomeOpen_(persistenceEnabled && !initialProject),
    images_(images), persistenceEnabled_(persistenceEnabled) {
  if (persistenceEnabled_) {
    try { locale_.load(defaultStudioStateDirectory()); }
    catch (const std::filesystem::filesystem_error&) { /* The default language is still usable. */ }
    catch (const std::runtime_error&) { /* The state directory may be unavailable. */ }
  }
  project_ = initialProject ? std::move(*initialProject) : createDefaultProject("ImGuiStudioCpp");
  const auto validInitial = projectFromJson(projectToJson(project_));
  if (!validInitial) throw std::runtime_error("Invalid initial project: " + validInitial.error);
  if (!project_.nodes.empty()) {
    selectNode(project_.nodes.front().id);
  }
  snapshots_.saveSnapshot(project_, tr(StudioText::ProjectCreated), SnapshotKind::Milestone, true);
  savedState_ = projectToJson(project_).dump();
  if (persistenceEnabled_) {
    try {
      const auto result = widgetLibrary_.load(defaultStudioStateDirectory());
      if (!result) libraryStatus_ = result.error;
      extensions_->load(defaultStudioStateDirectory());
    } catch (const std::exception& e) { libraryStatus_ = e.what(); }
    try {
      if (!buildProfiles_.load(defaultStudioStateDirectory() / "build_profiles.json",
          (std::filesystem::current_path() / "build" / "studio-projects").u8string()))
        buildProfileStatus_ = "Perfiles en modo de sesion: " + buildProfiles_.error();
      const auto initialized = recoverySession_.initialize(defaultStudioStateDirectory());
      if (!initialized) fileStatus_ = std::string(tr(StudioText::RecoveryUnavailable)) + initialized.error;
      else {
        std::vector<std::string> errors;
        recoveryCandidates_ = recoverySession_.scan(errors);
        openRecoveryPopup_ = !recoveryCandidates_.empty();
        for (const auto& error : errors) fileStatus_ += "\n" + error;
      }
    } catch (const std::exception& e) { fileStatus_ = e.what(); }
    try {
      const auto recent = readTextFile(defaultStudioStateDirectory() / "recent_projects.json", kRecentFileLimit);
      if (recent) {
        const auto list = Json::parse(recent.text, nullptr, false);
        if (list.is_array()) for (const auto& item : list) {
          if (recentProjects_.size() == kRecentProjectLimit) break;
          if (!item.is_string()) continue;
          const auto& name = item.get_ref<const std::string&>();
          if (!name.empty() && name.find('\0') == std::string::npos)
            recentProjects_.push_back(std::filesystem::u8path(name));
        }
      }
    } catch (const std::exception&) {
      fileStatus_ += std::string("\n") + tr(StudioText::ReadRecentsError);
    }
  }
}

void StudioApp::commitChange(const StudioProject& next, bool recordUndo) {
  if (projectToJson(next) == projectToJson(project_)) return;
  if (recordUndo) {
    interactiveEdit_ = false;
    past_.push_back(project_);
    if (past_.size() > 40) past_.erase(past_.begin());
  }
  future_.clear();
  project_ = next;
  codePreview_.reset();
  canvasNeedsSync_ = true;
  previewProject_.reset(); previewRenderer_.reset();
  lastEditMs_ = nowUnixMs();
  pendingAutoSave_ = true;
  recoveryDirty_ = true;
}

void StudioApp::undo() {
  if (past_.empty()) return;
  future_.insert(future_.begin(), project_);
  project_ = past_.back();
  codePreview_.reset();
  past_.pop_back();
  canvasNeedsSync_ = true;
  previewProject_.reset(); previewRenderer_.reset();
  pendingAutoSave_ = true;
  recoveryDirty_ = true;
  lastEditMs_ = nowUnixMs();
}

void StudioApp::redo() {
  if (future_.empty()) return;
  past_.push_back(project_);
  project_ = future_.front();
  codePreview_.reset();
  future_.erase(future_.begin());
  canvasNeedsSync_ = true;
  previewProject_.reset(); previewRenderer_.reset();
  pendingAutoSave_ = true;
  recoveryDirty_ = true;
  lastEditMs_ = nowUnixMs();
}

void StudioApp::selectNode(const std::string& id) {
  selectedIds_ = id.empty() ? std::vector<std::string>{} : std::vector<std::string>{id};
}

void StudioApp::addWidgetToSelection(WidgetType type, const char* preset) {
  insertWidgetIntoSelection(createPaletteNode(type, preset));
}

void StudioApp::insertWidgetIntoSelection(ImGuiNode node) {
  const auto id = node.id;
  DocumentEditResult inserted;
  if (node.type == WidgetType::Window) {
    node.x = node.y = 48.0f + static_cast<float>(project_.nodes.size() % 5) * 32.0f;
    inserted = insertDocumentNode(project_, std::move(node), {}, project_.nodes.size());
  } else if (project_.nodes.empty()) {
    auto window = createDefaultNode(WidgetType::Window);
    window.children.push_back(std::move(node));
    inserted = insertDocumentNode(project_, std::move(window), {}, 0);
  } else {
    const auto* target = findNodeById(project_.nodes, selectedNodeId());
    if (!target || !canAcceptChild(target->type, node.type)) target = &project_.nodes.front();
    inserted = insertDocumentNode(project_, std::move(node), target->id, target->children.size());
  }
  if (inserted) {
    selectNode(id);
    commitChange(*inserted.project);
  } else if (!inserted.error.empty()) {
    fileStatus_ = inserted.error;
  }
}

void StudioApp::commitInteractiveChange(const StudioProject& next) {
  commitChange(next, !interactiveEdit_);
  interactiveEdit_ = true;
}

void StudioApp::captureSnapshot(const std::string& label, SnapshotKind kind, bool skipSame) {
  if (!snapshots_.saveSnapshot(project_, label, kind, skipSame)) return;
  historyDirty_ = true;
  recoveryDirty_ = true;
  if (kind != SnapshotKind::Auto) persistSession();
}

void StudioApp::bindHistory(const std::filesystem::path& path, bool loadExisting) {
  historyFile_.emplace();
  SnapshotManager existing;
  const auto result = historyFile_->open(path, existing);
  if (!result) {
    fileStatus_ += "\nHistorial conservado sin sobrescribir: " + result.error;
    historyDirty_ = true;
    return;
  }
  if (loadExisting) snapshots_ = std::move(existing);
  else if (!existing.snapshots().empty()) {
    // Save As may replace a project after confirmation, but must not silently
    // replace/prune another project's existing history.
    historyFile_->protectExisting();
    fileStatus_ += "\nEl destino ya tiene historial y se conserva. El historial actual queda en recuperacion; guarde otra copia para trasladarlo.";
  }
  historyDirty_ = true;
}

void StudioApp::persistSession() {
  if (!persistenceEnabled_ || (!recoveryDirty_ && !historyDirty_)) return;
  lastPersistAttemptMs_ = nowUnixMs();
  auto report = [&](const std::string& message) {
    if (fileStatus_.find(message) == std::string::npos) fileStatus_ += "\n" + message;
  };
  if (historyDirty_) {
    if (!historyFile_) historyDirty_ = false; // Untitled histories live in recovery.
    else {
      const auto saved = historyFile_->save(snapshots_);
      if (saved) historyDirty_ = false;
      else report("No se pudo guardar el historial: " + saved.error);
    }
  }
  if (!hasUnsavedChanges()) {
    const auto cleared = recoverySession_.clear();
    if (cleared) recoveryDirty_ = false;
    else report(std::string(tr(StudioText::RecoveryClearError)) + cleared.error);
    return;
  }
  RecoveryDocument document;
  document.project = project_; document.snapshots = snapshots_;
  document.past = past_; document.future = future_;
  document.originalPath = projectPath_.empty() ? recoveryOriginPath_ : projectPath_.u8string();
  document.savedAtMs = nowUnixMs();
  const auto saved = recoverySession_.checkpoint(document);
  if (saved) recoveryDirty_ = false;
  else report(std::string(tr(StudioText::RecoverySaveError)) + saved.error);
}

void StudioApp::finishSession(bool discard) {
  persistSession();
  if (persistenceEnabled_ && (discard || !hasUnsavedChanges())) {
    const auto cleared = recoverySession_.clear();
    if (!cleared) fileStatus_ += "\n" + cleared.error;
  }
}

void StudioApp::resetRecoverySession() {
  if (!persistenceEnabled_) return;
  try {
    recoverySession_ = RecoverySession{};
    const auto result = recoverySession_.initialize(defaultStudioStateDirectory());
    if (!result) fileStatus_ += std::string("\n") + tr(StudioText::RecoveryUnavailable) + result.error;
  } catch (const std::exception& e) { fileStatus_ += "\n" + std::string(e.what()); }
}

void StudioApp::renderRecoveryDialog() {
  if (openRecoveryPopup_) {
    ImGui::OpenPopup(label(StudioText::RecoveryTitle).c_str());
    openRecoveryPopup_ = false;
  }
  if (ImGui::BeginPopupModal(label(StudioText::RecoveryTitle).c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    const float width = std::clamp(ImGui::GetMainViewport()->WorkSize.x - 64.0f, 150.0f, 660.0f);
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + width);
    ImGui::TextUnformatted(tr(StudioText::RecoveryIntro));
    ImGui::TextDisabled("%s", tr(StudioText::RecoveryCopyHint));
    ImGui::PopTextWrapPos();
    ImGui::BeginChild("RecoveryList", ImVec2(width, 260), true);
    for (size_t i = 0; i < recoveryCandidates_.size(); ++i) {
      const auto& candidate = recoveryCandidates_[i];
      ImGui::PushID(static_cast<int>(i));
      ImGui::TextUnformatted(candidate.name.c_str());
      ImGui::TextWrapped("%s", candidate.originalPath.empty() ? tr(StudioText::UntitledProject) : candidate.originalPath.c_str());
      ImGui::TextDisabled("%s: %lld", tr(StudioText::CheckpointTime), static_cast<long long>(candidate.savedAtMs));
      if (ImGui::SmallButton(label(StudioText::RecoverCopy, width - 36.0f).c_str())) {
        RecoveryDocument recovered;
        const auto result = recoverySession_.recover(candidate, recovered);
        if (result) {
          designRenderer_.reset(); canvasController_.cancel();
          assistant_.resetConversation();
          resetCanvasView();
          project_ = std::move(recovered.project);
          welcomeOpen_ = false;
          codePreview_.reset();
          snapshots_ = std::move(recovered.snapshots);
          past_ = std::move(recovered.past); future_ = std::move(recovered.future);
          projectPath_.clear(); savedState_.clear(); historyFile_.reset();
          recoveryOriginPath_ = recovered.originalPath;
          historyDirty_ = false; recoveryDirty_ = true;
          selectNode(project_.nodes.empty() ? "" : project_.nodes.front().id);
          previewProject_.reset(); previewRenderer_.reset(); previewMode_ = false;
          interactiveEdit_ = false; canvasNeedsSync_ = true;
          pendingAutoSave_ = true; lastEditMs_ = nowUnixMs();
          fileStatus_ = tr(StudioText::RecoverySuccess);
          if (!result.error.empty()) fileStatus_ += "\n" + result.error;
          persistSession();
          recoveryCandidates_.erase(recoveryCandidates_.begin() + static_cast<std::ptrdiff_t>(i));
          // EndPopup must apply to the modal, not to the scrolling child.
          ImGui::PopID(); ImGui::EndChild();
          ImGui::CloseCurrentPopup(); ImGui::EndPopup();
          return;
        }
        fileStatus_ = std::string(tr(StudioText::RecoveryError)) + result.error;
      }
      const auto discardLabel = label(StudioText::DiscardRecovery, width - 36.0f);
      sameLineIfFits(discardLabel.c_str());
      if (ImGui::SmallButton(discardLabel.c_str())) discardRecoveryIndex_ = i;
      ImGui::Separator();
      ImGui::PopID();
    }
    ImGui::EndChild();
    if (discardRecoveryIndex_ && *discardRecoveryIndex_ < recoveryCandidates_.size()) {
      ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + width);
      ImGui::TextUnformatted(tr(StudioText::DiscardQuestion));
      ImGui::PopTextWrapPos();
      if (ImGui::Button(label(StudioText::DiscardConfirm, width - 12.0f).c_str())) {
        const auto index = *discardRecoveryIndex_;
        const auto result = recoverySession_.discard(recoveryCandidates_[index]);
        if (result) recoveryCandidates_.erase(recoveryCandidates_.begin() + static_cast<std::ptrdiff_t>(index));
        else fileStatus_ = std::string(tr(StudioText::DiscardError)) + result.error;
        discardRecoveryIndex_.reset();
      }
      const auto keepLabel = label(StudioText::KeepCopy, width - 12.0f);
      sameLineIfFits(keepLabel.c_str());
      if (ImGui::Button(keepLabel.c_str())) discardRecoveryIndex_.reset();
    }
    if (recoveryCandidates_.empty() || ImGui::Button(label(StudioText::Later, width - 12.0f).c_str())) {
      discardRecoveryIndex_.reset();
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

bool StudioApp::hasUnsavedChanges() const {
  return projectToJson(project_).dump() != savedState_ ||
    (projectPath_.empty() && snapshots_.snapshots().size() > 1);
}

bool StudioApp::requestClose() {
  if (widgetEditor_) {
    closeAfterWidgetDraft_ = true;
    widgetEditor_->widgetDraft_->confirmDiscard = true;
    return false;
  }
  if (quitRequested_ || !hasUnsavedChanges()) { finishSession(); return true; }
  requestAction(PendingAction::Exit);
  return false;
}

void StudioApp::requestAction(PendingAction action) {
  if (fileDialog_ != FileDialog::None || pendingAction_ != PendingAction::None) return;
  pendingAction_ = action;
  if (hasUnsavedChanges()) openUnsavedPopup_ = true;
  else continuePendingAction();
}

void StudioApp::continuePendingAction() {
  const auto action = pendingAction_;
  pendingAction_ = PendingAction::None;
  if (action == PendingAction::Open) {
    fileDialog_ = FileDialog::Open;
    fileError_.clear();
    projectPathBuf_[0] = '\0';
    openFilePopup_ = true;
  } else if (action == PendingAction::New) {
    finishSession(true);
    resetRecoverySession();
    historyFile_.reset(); recoveryOriginPath_.clear();
    historyDirty_ = false; recoveryDirty_ = false;
    designRenderer_.reset(); canvasController_.cancel();
    assistant_.resetConversation();
    resetCanvasView();
    project_ = createDefaultProject("NuevoProyecto");
    welcomeOpen_ = false;
    codePreview_.reset();
    projectPath_.clear();
    savedState_ = projectToJson(project_).dump();
    fileStatus_ = tr(StudioText::NewProjectStatus);
    past_.clear(); future_.clear();
    snapshots_ = SnapshotManager{};
    snapshots_.saveSnapshot(project_, tr(StudioText::ProjectCreated), SnapshotKind::Milestone);
    selectNode(project_.nodes.front().id);
    previewProject_.reset(); previewRenderer_.reset(); previewMode_ = false;
    pendingAutoSave_ = false; interactiveEdit_ = false; canvasNeedsSync_ = true;
  } else if (action == PendingAction::Exit) {
    finishSession(true);
    quitRequested_ = true;
  }
}

void StudioApp::rememberProject(const std::filesystem::path& path) {
  if (!persistenceEnabled_) return;
  try {
    std::error_code error;
    auto normalized = std::filesystem::weakly_canonical(path, error);
    if (error || !normalized.is_absolute()) normalized = std::filesystem::absolute(path);
    const auto position = std::find(recentProjects_.begin(), recentProjects_.end(), normalized);
    if (position == recentProjects_.begin() && position != recentProjects_.end()) return;
    if (position != recentProjects_.end()) recentProjects_.erase(position);
    recentProjects_.insert(recentProjects_.begin(), std::move(normalized));
    if (recentProjects_.size() > kRecentProjectLimit) recentProjects_.pop_back();
    auto list = Json::array();
    for (const auto& recent : recentProjects_) list.push_back(recent.u8string());
    const auto saved = writeTextFileAtomically(list.dump(), defaultStudioStateDirectory() / "recent_projects.json", kRecentFileLimit);
    if (!saved) fileStatus_ += std::string("\n") + tr(StudioText::SaveRecentsError) + saved.error;
  } catch (const std::exception&) {
    fileStatus_ += std::string("\n") + tr(StudioText::UpdateRecentsError);
  }
}

bool StudioApp::openProject(const std::filesystem::path& path) {
  auto result = loadProjectFile(path);
  if (!result) {
    fileError_ = std::string(tr(StudioText::OpenError)) + result.error;
    return false;
  }
  finishSession(true);
  resetRecoverySession();
  designRenderer_.reset(); canvasController_.cancel();
  assistant_.resetConversation();
  resetCanvasView();
  project_ = std::move(*result.project);
  welcomeOpen_ = false;
  codePreview_.reset();
  projectPath_ = path;
  savedState_ = projectToJson(project_).dump();
  past_.clear(); future_.clear();
  snapshots_ = SnapshotManager{};
  historyDirty_ = false; recoveryDirty_ = false; recoveryOriginPath_.clear();
  bindHistory(path, true);
  captureSnapshot(tr(StudioText::ProjectOpened), SnapshotKind::Milestone, true);
  selectNode(project_.nodes.empty() ? "" : project_.nodes.front().id);
  previewProject_.reset(); previewRenderer_.reset(); previewMode_ = false;
  pendingAutoSave_ = false; interactiveEdit_ = false; canvasNeedsSync_ = true;
  fileError_.clear();
  for (const auto& warning : result.warnings) fileStatus_ += "\n" + warning;
  rememberProject(path);
  return true;
}

std::string StudioApp::label(StudioText key, float maxWidth) const {
  auto translated = locale_.label(key);
  const auto* start = tr(key);
  if (maxWidth > 0 && ImGui::CalcTextSize(start).x > maxWidth) {
    const char* end = start;
    const float space = std::max(1.0f, maxWidth - ImGui::CalcTextSize("...").x);
    ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), space, 0.0f, start, nullptr, &end);
    translated.replace(0, std::strlen(start), std::string(start, end) + "...");
  }
  return translated;
}

void StudioApp::renderLanguageChoices() {
  for (size_t i = 0; i < locale_.size(); ++i) {
    const auto option = locale_.name(i) + "###locale_" + locale_.id(i);
    if (ImGui::Selectable(option.c_str(), locale_.selected() == i)) pendingLanguage_ = i;
  }
}

void StudioApp::renderWelcome() {
  const auto* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);
  const auto flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
  if (ImGui::Begin("##StudioWelcome", nullptr, flags)) {
    const float width = std::min(680.0f, ImGui::GetContentRegionAvail().x);
    ImGui::SetCursorPosX(std::max(ImGui::GetCursorPosX(), (viewport->WorkSize.x - width) * 0.5f));
    ImGui::SetCursorPosY(std::max(ImGui::GetCursorPosY(), viewport->WorkSize.y * 0.12f));
    if (ImGui::BeginChild("##WelcomeContent", ImVec2(width, 0))) {
      ImGui::TextUnformatted("ImGui Studio");
      ImGui::SetNextItemWidth(std::min(210.0f, width * 0.45f));
      if (ImGui::BeginCombo(label(StudioText::Language, width * 0.44f).c_str(),
                           locale_.name(locale_.selected()).c_str())) {
        renderLanguageChoices();
        ImGui::EndCombo();
      }
      ImGui::Spacing();
      const float buttonMin = std::min(155.0f, width * 0.4f);
      const auto welcomeButton = [&](const std::string& title) {
        const float textWidth = ImGui::CalcTextSize(title.c_str(), nullptr, true).x;
        const float width = std::max(buttonMin, textWidth + 2 * ImGui::GetStyle().FramePadding.x);
        return ImGui::Button(title.c_str(), ImVec2(width, 38));
      };
      if (welcomeButton(label(StudioText::WelcomeNew, width - 30.0f))) requestAction(PendingAction::New);
      const auto openLabel = label(StudioText::WelcomeOpen, width - 30.0f);
      sameLineIfFits(openLabel.c_str(), buttonMin);
      if (welcomeButton(openLabel)) requestAction(PendingAction::Open);
      ImGui::Spacing();
      ImGui::SeparatorText(tr(StudioText::Recents));
      if (recentProjects_.empty()) {
        ImGui::PushTextWrapPos(0.0f);
        ImGui::TextDisabled("%s", tr(StudioText::RecentsEmpty));
        ImGui::PopTextWrapPos();
      }
      std::optional<std::filesystem::path> chosen;
      for (size_t i = 0; i < recentProjects_.size(); ++i) {
        const auto& recent = recentProjects_[i];
        ImGui::PushID(static_cast<int>(i));
        if (ImGui::Selectable((recent.filename().u8string() + "###recent").c_str())) chosen = recent;
        ImGui::TextWrapped("%s", recent.u8string().c_str());
        ImGui::PopID();
        if (chosen) break;
      }
      if (chosen) openProject(*chosen);
      if (fileDialog_ == FileDialog::None && !fileError_.empty()) ImGui::TextWrapped("%s", fileError_.c_str());
    }
    ImGui::EndChild();
  }
  ImGui::End();
}

bool StudioApp::saveProject(bool saveAs) {
  if (saveAs || projectPath_.empty()) {
    if (fileDialog_ != FileDialog::None) return false;
    fileDialog_ = FileDialog::SaveAs;
    confirmOverwrite_ = false;
    fileError_.clear();
    copyStringToBuffer(projectPath_.empty() ? (recoveryOriginPath_.empty() ? "proyecto.json" : "proyecto_recuperado.json") : projectPath_.u8string(),
                       projectPathBuf_, sizeof(projectPathBuf_));
    openFilePopup_ = true;
    return false;
  }
  const auto result = saveProjectIfUnchanged(project_, projectPath_, savedState_);
  if (!result) {
    fileStatus_ = std::string(tr(StudioText::SaveError)) + result.error;
    fileError_ = result.error;
    return false;
  }
  fileError_.clear();
  savedState_ = projectToJson(project_).dump();
  fileStatus_ = std::string(tr(StudioText::SavedStatus)) + projectPath_.u8string();
  rememberProject(projectPath_);
  captureSnapshot(tr(StudioText::ProjectSaved), SnapshotKind::Manual, true);
  recoveryDirty_ = true;
  persistSession();
  return true;
}

void StudioApp::renderProjectDialogs() {
  if (!fileStatus_.empty()) {
    ImGui::SetNextWindowSize(ImVec2(600, 0), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(label(StudioText::ProjectStatus).c_str())) {
      ImGui::TextWrapped("%s", fileStatus_.c_str());
      if (ImGui::SmallButton(label(StudioText::CloseMessage).c_str())) fileStatus_.clear();
    }
    ImGui::End();
  }
  if (openUnsavedPopup_) {
    ImGui::OpenPopup(label(StudioText::UnsavedTitle).c_str());
    openUnsavedPopup_ = false;
  }
  if (ImGui::BeginPopupModal(label(StudioText::UnsavedTitle).c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    const float width = std::clamp(ImGui::GetMainViewport()->WorkSize.x - 40.0f, 140.0f, 500.0f);
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + width);
    ImGui::TextUnformatted(tr(StudioText::UnsavedPrompt));
    if (!fileError_.empty()) ImGui::TextWrapped("%s", fileError_.c_str());
    ImGui::PopTextWrapPos();
    ImGui::Dummy(ImVec2(width, 0.0f));
    if (ImGui::Button(label(StudioText::SaveContinue, width).c_str())) {
      if (saveProject()) {
        ImGui::CloseCurrentPopup();
        continuePendingAction();
      } else if (fileDialog_ == FileDialog::SaveAs) {
        ImGui::CloseCurrentPopup();
      }
    }
    const auto discardLabel = label(StudioText::DiscardChanges, width);
    sameLineIfFits(discardLabel.c_str());
    if (ImGui::Button(discardLabel.c_str())) {
      ImGui::CloseCurrentPopup();
      continuePendingAction();
    }
    const auto cancelLabel = label(StudioText::Cancel, width);
    sameLineIfFits(cancelLabel.c_str());
    if (ImGui::Button(cancelLabel.c_str())) {
      pendingAction_ = PendingAction::None;
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
  if (openFilePopup_) {
    ImGui::OpenPopup(label(StudioText::JsonDialog).c_str());
    openFilePopup_ = false;
  }
  if (ImGui::BeginPopupModal(label(StudioText::JsonDialog).c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    const float width = std::clamp(ImGui::GetMainViewport()->WorkSize.x - 64.0f, 140.0f, 620.0f);
    const bool opening = fileDialog_ == FileDialog::Open;
    const bool python = fileDialog_ == FileDialog::ExportPython;
    const bool csharp = fileDialog_ == FileDialog::ExportCSharp;
    const bool java = fileDialog_ == FileDialog::ExportJava;
    const bool c = fileDialog_ == FileDialog::ExportC;
    const bool rust = fileDialog_ == FileDialog::ExportRust;
    const bool android = fileDialog_ == FileDialog::ExportAndroid;
    const bool macos = fileDialog_ == FileDialog::ExportMacOS;
    const bool ios = fileDialog_ == FileDialog::ExportIOS;
    const bool usesPack = android || macos || ios;
    auto& packPath = cppPackPaths_[macos ? 1 : ios ? 2 : 0];
    const bool exporting = python || csharp || java || c || rust || usesPack || fileDialog_ == FileDialog::ExportCpp;
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + width);
    if (exporting) ImGui::TextWrapped("%s (%s)", tr(StudioText::ExportProjectHint),
      python ? "Python" : csharp ? "C#" : java ? "Java" : c ? "C" : rust ? "Rust" :
      android ? "Android C++" : macos ? "macOS C++" : ios ? "iOS C++" : "C++");
    else ImGui::TextUnformatted(opening ? tr(StudioText::OpenProjectHint) : tr(StudioText::SaveProjectHint));
    ImGui::PopTextWrapPos();
    ImGui::TextWrapped("%s", tr(StudioText::Path));
    ImGui::SetNextItemWidth(width);
    ImGui::InputText("##ProjectPath", projectPathBuf_, sizeof(projectPathBuf_));
    if (fileDialog_ == FileDialog::ExportCpp) {
      ImGui::SetNextItemWidth(std::min(180.0f,width * 0.5f));
      selectCppRenderer(surfaceLabel(StudioText::CppRenderer),exportCppRenderer_);
    }
    if (usesPack) {
      ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + width);
      ImGui::TextUnformatted(tr(android ? StudioText::AndroidPackFolder : StudioText::ApplePackFolder));
      ImGui::PopTextWrapPos();
      ImGui::SetNextItemWidth(width);
      ImGui::InputText("##CppTemplateFolder", &packPath);
    }
#ifdef _WIN32
    if (opening && ImGui::Button(label(StudioText::Browse, width - 12.0f).c_str())) {
      if (const auto picked = browseForJsonFile(locale_, fileError_)) {
        const auto chosen = picked->u8string();
        if (chosen.size() < sizeof(projectPathBuf_)) {
          copyStringToBuffer(chosen, projectPathBuf_, sizeof(projectPathBuf_));
          fileError_.clear();
        } else fileError_ = tr(StudioText::PathTooLong);
      }
    }
#endif
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + width);
    ImGui::TextDisabled("%s", exporting ? tr(StudioText::ExportPathHint) : tr(StudioText::FilePathHint));
    ImGui::PopTextWrapPos();
    if (!opening && !exporting)
      ImGui::Checkbox(label(StudioText::ConfirmOverwrite, width - 45.0f).c_str(), &confirmOverwrite_);
    if (!fileError_.empty()) ImGui::TextWrapped("%s", fileError_.c_str());
    if (ImGui::Button(label(exporting ? StudioText::Export : opening ? StudioText::Open : StudioText::Save, width - 12.0f).c_str())) {
      const auto path = std::filesystem::u8path(projectPathBuf_);
      bool success = false;
      if (exporting) {
        const auto result = android ? exportAndroidCppBundle(project_,std::filesystem::u8path(packPath),path) :
          macos || ios ? exportAppleCppBundle(project_,std::filesystem::u8path(packPath),path,
                                              ios ? AppleCppTarget::IOS : AppleCppTarget::MacOS) :
          python ? exportPythonBundle(project_,path) : csharp ? exportCSharpBundle(project_,path) :
          java ? exportJavaBundle(project_,path) : c ? exportCBundle(project_,path) :
          rust ? exportRustBundle(project_,path) : exportCppBundle(project_,path,exportCppRenderer_);
        if (result) {
          fileStatus_ = std::string(tr(StudioText::Exported)) + path.u8string() + "\n" + tr(StudioText::CheckExportReport);
          success = true;
        }
        else fileError_ = std::string(tr(StudioText::ExportError)) + result.error;
      } else if (opening) {
        success = openProject(path);
      } else {
        std::error_code pathError;
        const bool replacingOther = path != projectPath_ && std::filesystem::exists(path, pathError);
        const auto result = replacingOther && !confirmOverwrite_
          ? ProjectSaveResult{false, tr(StudioText::FileExists)}
          : saveProjectIfUnchanged(project_, path, path == projectPath_
              ? std::optional<std::string>(savedState_) : std::nullopt);
        if (result) {
          const bool changedPath = projectPath_ != path;
          projectPath_ = path;
          savedState_ = projectToJson(project_).dump();
          fileStatus_ = std::string(tr(StudioText::SavedStatus)) + path.u8string();
          rememberProject(path);
          if (changedPath || !historyFile_) bindHistory(path, false);
          recoveryOriginPath_.clear();
          captureSnapshot(tr(StudioText::ProjectSaved), SnapshotKind::Manual, true);
          recoveryDirty_ = true;
          persistSession();
          success = true;
        } else fileError_ = std::string(tr(StudioText::SaveError)) + result.error;
      }
      if (success) {
        fileDialog_ = FileDialog::None;
        ImGui::CloseCurrentPopup();
        continuePendingAction();
      }
    }
    const auto cancelLabel = label(StudioText::Cancel, width - 12.0f);
    sameLineIfFits(cancelLabel.c_str());
    if (ImGui::Button(cancelLabel.c_str())) {
      fileDialog_ = FileDialog::None;
      pendingAction_ = PendingAction::None;
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

void StudioApp::renderFrame() {
  const ScopedStudioLocale language(locale_);
  if (pendingLanguage_) {
    const auto result = locale_.select(*pendingLanguage_);
    if (!result) fileStatus_ = std::string(tr(StudioText::LanguageSaveError)) + result.error;
    pendingLanguage_.reset();
  }
  nativeBuild_.poll();
  assistant_.poll();
  extensions_->poll();
  previewRenderer_.setKnobsInstalled(extensions_->installed());
  designRenderer_.setKnobsInstalled(extensions_->installed());
  if (widgetEditor_) {
    if ((recoveryDirty_ || historyDirty_) && nowUnixMs() - lastPersistAttemptMs_ >= 2000) persistSession();
    widgetEditor_->renderFrame();
    handleWidgetDraftAction();
    return;
  }
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigWindowsMoveFromTitleBarOnly = true;
  const bool shortcut = (io.KeyCtrl || io.KeySuper) &&
    !ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId);
  if (welcomeOpen_) {
    if (shortcut && ImGui::IsKeyPressed(ImGuiKey_O)) requestAction(PendingAction::Open);
    renderWelcome();
    renderProjectDialogs();
    renderRecoveryDialog();
    return;
  }
  if (shortcut && ImGui::IsKeyPressed(ImGuiKey_S)) {
    if (widgetDraft_) widgetDraft_->action = WidgetDraft::Action::Save;
    else saveProject(io.KeyShift);
  }
  if (shortcut && !widgetDraft_ && ImGui::IsKeyPressed(ImGuiKey_O)) requestAction(PendingAction::Open);
  if (shortcut && !io.WantTextInput && fileDialog_ == FileDialog::None) {
    if (ImGui::IsKeyPressed(ImGuiKey_Z)) {
      interactiveEdit_ = false;
      if (io.KeyShift) redo(); else undo();
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Y)) { interactiveEdit_ = false; redo(); }
  }

  // Debounced 15s auto-snapshot
  if (pendingAutoSave_ && (nowUnixMs() - lastEditMs_) >= 15000) {
    captureSnapshot(tr(StudioText::AutoSnapshot), SnapshotKind::Auto, true);
    pendingAutoSave_ = false;
  }

  renderTopMenuBar();
  if (previewMode_ != lastPreviewMode_) {
    canvasNeedsSync_ = true;
    canvasPan_.reset();
    lastPreviewMode_ = previewMode_;
  }
  if (previewMode_ && !previewProject_) previewProject_ = project_;
  if (!previewMode_) { previewProject_.reset(); previewRenderer_.reset(); }
  previewRenderer_.beginFrame();
  hierarchyItems_.clear();
  selectedIds_.erase(std::remove_if(selectedIds_.begin(), selectedIds_.end(), [&](const std::string& id) {
    return !findNodeById(project_.nodes, id);
  }), selectedIds_.end());

  const ImGuiViewport* vp = ImGui::GetMainViewport();
  const float availW = vp->WorkSize.x;
  const float availH = vp->WorkSize.y;
  const float sideScale = std::clamp((availW - 280.0f) / 560.0f, 0.45f, 1.0f);
  const float leftW = std::floor(260.0f * sideScale);
  const float rightW = std::floor(300.0f * sideScale);
  const float centerW = availW - leftW - rightW;
  const float bottomH = std::floor(std::min(availH * 0.36f, 224.0f));
  const float workspaceH = availH - bottomH;
  const float centerX = vp->WorkPos.x + leftW;
  const float bottomY = vp->WorkPos.y + workspaceH;

  renderWidgetPalette(vp->WorkPos.x, vp->WorkPos.y, leftW, workspaceH);
  renderHierarchy(vp->WorkPos.x, bottomY, leftW, bottomH);
  renderCenterCanvas(centerX, vp->WorkPos.y, centerW, showCodePanel_ ? workspaceH : availH);
  if (showWidgetLibrary_ && !widgetDraft_) renderWidgetLibrary();
  if (showCodePanel_) {
    renderCodeDrawer(centerX, bottomY, centerW, bottomH);
  }
  if (!previewMode_) {
    auto items = designRenderer_.report().items;
    items.insert(items.end(), hierarchyItems_.begin(), hierarchyItems_.end());
    auto update = canvasController_.process(project_, items, selectedIds_, canvasOrigin_, snapToGrid_, canvasScale());
    if (update.selection) selectedIds_ = std::move(*update.selection);
    if (update.edit.project) commitChange(*update.edit.project);
    else if (!update.edit.error.empty()) fileStatus_ = update.edit.error;
  } else canvasController_.cancel();
  renderRightInspector(centerX + centerW, vp->WorkPos.y, rightW, availH);

  if (showHistoryPanel_) {
    renderVersionHistoryWindow();
  }
  if (showBuildPanel_) renderBuildWindow();
  if (showAssistant_) {
    auto edit = assistant_.render(project_,selectedNodeId(),codeLanguage_,showAssistant_,persistenceEnabled_);
    if (edit.project) {
      selectNode(edit.project->nodes[project_.nodes.size()].id);
      commitChange(*edit.project);
    }
  }
  if (showDiagnosticSource_) renderDiagnosticSource();
  renderProjectDialogs();
  renderRecoveryDialog();
  if (widgetDraft_) {
    if (!widgetDraft_->error.empty()) {
      ImGui::SetNextWindowSize(ImVec2(std::min(availW - 36.0f, 480.0f), 0), ImGuiCond_Appearing);
      if (ImGui::Begin(label(StudioText::LibraryDesigner).c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped("%s", widgetDraft_->error.c_str());
        if (ImGui::Button(label(StudioText::CloseMessage).c_str())) widgetDraft_->error.clear();
      }
      ImGui::End();
    }
    const auto popup = label(StudioText::LibraryDiscardTitle);
    if (widgetDraft_->confirmDiscard) {
      ImGui::OpenPopup(popup.c_str());
      widgetDraft_->confirmDiscard = false;
    }
    if (ImGui::BeginPopupModal(popup.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::TextWrapped("%s", tr(StudioText::LibraryDiscardBody));
      if (ImGui::Button(label(StudioText::DiscardChanges).c_str())) {
        widgetDraft_->action = WidgetDraft::Action::Discard;
        ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine();
      if (ImGui::Button(label(StudioText::Cancel).c_str())) {
        widgetDraft_->action = WidgetDraft::Action::KeepEditing;
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }
  }
  if ((recoveryDirty_ || historyDirty_) && nowUnixMs() - lastPersistAttemptMs_ >= 2000)
    persistSession();
  if (!ImGui::IsAnyItemActive() && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) interactiveEdit_ = false;
}

void StudioApp::openWidgetDraft(std::string name, const ImGuiNode* base, const LibraryWidget* original) {
  WidgetDraft draft;
  draft.name = std::move(name);
  draft.initialName = draft.name;
  draft.blank = !base;
  if (original) draft.original = *original;

  StudioProject design;
  design.name = draft.name;
  design.style = project_.style;
  design.canvas = project_.canvas;
  if (base && base->type == WidgetType::Window) {
    design.nodes.push_back(*base);
    design.nodes.front().x = design.nodes.front().y = 32;
  } else {
    auto canvas = createDefaultNode(WidgetType::Window, 24, 24);
    canvas.label = tr(StudioText::LibraryDraftContainer);
    canvas.width = 560;
    canvas.height = 440;
    canvas.props.noMove = true;
    canvas.props.noResize = true;
    draft.canvasId = draft.wrapperId = canvas.id;
    if (base && base->type == WidgetType::TabItem) {
      auto tabs = createDefaultNode(WidgetType::TabBar);
      draft.wrapperId = tabs.id;
      tabs.children.push_back(*base);
      canvas.children.push_back(std::move(tabs));
    } else if (base) {
      canvas.children.push_back(*base);
    }
    design.nodes.push_back(std::move(canvas));
  }
  widgetEditor_ = std::make_unique<StudioApp>(false, std::move(design), images_);
  widgetEditor_->locale_ = locale_;
  widgetEditor_->extensions_ = extensions_;
  widgetEditor_->showCodePanel_ = false;
  widgetEditor_->selectNode(base ? base->id : draft.canvasId);
  widgetEditor_->widgetDraft_ = std::move(draft);
}

void StudioApp::renderWidgetDraftToolbar() {
  auto& draft = *widgetDraft_;
  const float width = ImGui::GetWindowWidth();
  if (ImGui::Button(label(StudioText::LibraryBack, width * 0.18f).c_str())) {
    if (hasUnsavedChanges() || draft.name != draft.initialName) draft.confirmDiscard = true;
    else draft.action = WidgetDraft::Action::Discard;
  }
  ImGui::SameLine();
  if (ImGui::Button(label(previewMode_ ? StudioText::Ide : StudioText::Preview, width * 0.12f).c_str()))
    previewMode_ = !previewMode_;
  ImGui::SameLine();
  if (ImGui::Button(label(StudioText::LibrarySave, width * 0.22f).c_str())) draft.action = WidgetDraft::Action::Save;
  if (draft.original) {
    ImGui::SameLine();
    if (ImGui::Button(label(StudioText::LibrarySaveCopy, width * 0.19f).c_str())) draft.action = WidgetDraft::Action::SaveCopy;
  }
  ImGui::SameLine();
  ImGui::SetNextItemWidth(std::clamp(width * 0.18f, 110.0f, 240.0f));
  ImGui::InputTextWithHint("##WidgetDraftName", tr(StudioText::LibraryName), &draft.name);
  if (ImGui::IsItemHovered()) ImGui::SetItemTooltip("%s", tr(StudioText::LibraryName));
}

void StudioApp::handleWidgetDraftAction() {
  auto& editor = *widgetEditor_;
  auto& draft = *editor.widgetDraft_;
  const auto action = std::exchange(draft.action, WidgetDraft::Action::None);
  if (action == WidgetDraft::Action::None) return;
  if (action == WidgetDraft::Action::KeepEditing) {
    closeAfterWidgetDraft_ = false;
    return;
  }
  if (action == WidgetDraft::Action::Save || action == WidgetDraft::Action::SaveCopy) {
    const auto& roots = editor.project_.nodes;
    const ImGuiNode* node = nullptr;
    std::optional<ImGuiNode> group;
    if (roots.size() == 1) {
      const auto& root = roots.front();
      if (draft.wrapperId.empty()) {
        node = &root;
      } else if (root.id == draft.canvasId) {
        const auto* wrapper = findNodeById(roots, draft.wrapperId);
        if (wrapper && wrapper->children.size() == 1) node = &wrapper->children.front();
        else if (wrapper && draft.blank) {
          group = createDefaultNode(WidgetType::Group);
          group->label = draft.name;
          group->children = wrapper->children;
          node = &*group;
        }
      }
    }
    if (!node) {
      draft.error = tr(StudioText::LibraryOneRoot);
      return;
    }
    const auto* expected = action == WidgetDraft::Action::Save && draft.original ? &*draft.original : nullptr;
    const auto saved = widgetLibrary_.save(draft.name, *node, expected);
    if (!saved) {
      draft.error = saved.error;
      return;
    }
    librarySelection_ = saved.id;
    libraryStatus_ = tr(StudioText::LibrarySaved);
    libraryName_.clear();
  }
  widgetEditor_.reset();
  canvasNeedsSync_ = true;
  canvasController_.cancel();
  if (std::exchange(closeAfterWidgetDraft_, false) && requestClose()) quitRequested_ = true;
}

void StudioApp::renderWidgetLibrary() {
  ImGui::SetNextWindowSize(ImVec2(610, 450), ImGuiCond_FirstUseEver);
  if (ImGui::Begin(label(StudioText::Library).c_str(), &showWidgetLibrary_)) {
    if (!libraryStatus_.empty()) ImGui::TextWrapped("%s", libraryStatus_.c_str());
    if (ImGui::BeginTabBar("##WidgetLibraryTabs")) {
      if (ImGui::BeginTabItem(label(StudioText::LibraryCustom).c_str())) {
        if (ImGui::BeginChild("##LibraryList", ImVec2(0, 150), ImGuiChildFlags_Borders)) {
          if (widgetLibrary_.entries().empty()) ImGui::TextWrapped("%s", tr(StudioText::LibraryEmpty));
          for (const auto& entry : widgetLibrary_.entries()) {
            const auto caption = entry.name + "###item_" + entry.id;
            if (ImGui::Selectable(caption.c_str(), entry.id == librarySelection_)) {
              librarySelection_ = entry.id;
              exportLibraryWidget_ = false;
              libraryStatus_.clear();
            }
          }
        }
        ImGui::EndChild();
        if (const auto* selected = widgetLibrary_.find(librarySelection_)) {
          if (ImGui::Button(label(StudioText::LibraryInsert).c_str()))
            insertWidgetIntoSelection(instantiateLibraryWidget(*selected->node));
          const auto editLabel = label(StudioText::LibraryEdit);
          sameLineIfFits(editLabel.c_str());
          if (ImGui::Button(editLabel.c_str())) openWidgetDraft(selected->name, selected->node.get(), selected);
          const auto exportLabel = label(StudioText::LibraryExport);
          sameLineIfFits(exportLabel.c_str());
          if (ImGui::Button(exportLabel.c_str())) exportLibraryWidget_ = !exportLibraryWidget_;
          if (exportLibraryWidget_) {
            ImGui::TextWrapped("%s", tr(StudioText::LibraryFileNew));
            ImGui::SetNextItemWidth(-1.0f);
            ImGui::InputTextWithHint("##LibraryExportPath", tr(StudioText::Path), &libraryExportPath_);
            ImGui::BeginDisabled(libraryExportPath_.empty());
            if (ImGui::Button(label(StudioText::Export).c_str())) {
              const auto result = widgetLibrary_.exportFile(*selected, std::filesystem::u8path(libraryExportPath_));
              libraryStatus_ = result ? std::string(tr(StudioText::Exported)) + libraryExportPath_ : result.error;
              if (result) exportLibraryWidget_ = false;
            }
            ImGui::EndDisabled();
          }
        }
        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem(label(StudioText::LibraryCreate).c_str())) {
        ImGui::TextUnformatted(tr(StudioText::LibraryName));
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputText("##LibraryName", &libraryName_);
        const auto* selected = findNodeById(project_.nodes, selectedNodeId());
        if (selected && (selected->type == WidgetType::Unknown || selected->type == WidgetType::SameLine)) selected = nullptr;
        const char* base = widgetBaseIndex_ == 0 ? tr(StudioText::LibraryBlank) : widgetBaseIndex_ == 1
          ? tr(StudioText::LibraryFromSelection) : tr(kPalette[widgetBaseIndex_ - 2].label);
        ImGui::TextUnformatted(tr(StudioText::LibraryBaseControl));
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::BeginCombo("##LibraryBaseControl", base, ImGuiComboFlags_HeightLarge)) {
          if (ImGui::Selectable(label(StudioText::LibraryBlank).c_str(), widgetBaseIndex_ == 0)) widgetBaseIndex_ = 0;
          if (ImGui::Selectable(label(StudioText::LibraryFromSelection).c_str(), widgetBaseIndex_ == 1,
                                selected ? ImGuiSelectableFlags_None : ImGuiSelectableFlags_Disabled)) widgetBaseIndex_ = 1;
          for (size_t i = 0; i < std::size(kPalette); ++i) {
            if (kPalette[i].type == WidgetType::SameLine ||
                (kPalette[i].type == WidgetType::Knob && !extensions_->installed())) continue;
            if (ImGui::Selectable(label(kPalette[i].label).c_str(), widgetBaseIndex_ == i + 2)) widgetBaseIndex_ = i + 2;
          }
          ImGui::EndCombo();
        }
        if (ImGui::Button(label(StudioText::LibraryCreateDesign).c_str())) {
          if (libraryName_.find_first_not_of(" \t") == std::string::npos) libraryStatus_ = tr(StudioText::LibraryNameRequired);
          else if (widgetBaseIndex_ == 0) openWidgetDraft(libraryName_, nullptr);
          else if (widgetBaseIndex_ == 1 && selected) openWidgetDraft(libraryName_, selected);
          else if (widgetBaseIndex_ >= 2) {
            const auto& entry = kPalette[widgetBaseIndex_ - 2];
            if (entry.type != WidgetType::Knob || extensions_->installed()) {
              const auto widget = createPaletteNode(entry.type, entry.preset);
              openWidgetDraft(libraryName_, &widget);
            }
          }
        }
        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem(label(StudioText::LibraryImport).c_str())) {
        ImGui::TextWrapped("%s", tr(StudioText::LibraryFileHint));
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputTextWithHint("##LibraryImportPath", tr(StudioText::Path), &libraryImportPath_);
#ifdef _WIN32
        if (ImGui::Button(label(StudioText::Browse).c_str()))
          if (const auto picked = browseForJsonFile(locale_, libraryStatus_)) libraryImportPath_ = picked->u8string();
        ImGui::SameLine();
#endif
        ImGui::BeginDisabled(libraryImportPath_.empty());
        if (ImGui::Button(label(StudioText::LibraryImport).c_str())) {
          const auto result = widgetLibrary_.importFile(std::filesystem::u8path(libraryImportPath_));
          libraryStatus_ = result ? tr(StudioText::LibrarySaved) : result.error;
          if (result) {
            librarySelection_ = result.id;
            libraryImportPath_.clear();
            exportLibraryWidget_ = false;
          }
        }
        ImGui::EndDisabled();
        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem(label(StudioText::ExtensionCatalog).c_str())) {
        renderExtensionCatalog();
        ImGui::EndTabItem();
      }
      ImGui::EndTabBar();
    }
  }
  ImGui::End();
}

void StudioApp::renderExtensionCatalog() {
  ImGui::SeparatorText("imgui-knobs");
  ImGui::TextWrapped("%s", tr(StudioText::CatalogKnobDescription));
  ImGui::Text("%s: MIT", tr(StudioText::CatalogLicense));
  ImGui::Text("%s: %.7s", tr(StudioText::CatalogRevision), KnobsSource::revision);
  const auto source = std::string(KnobsSource::repository) + "/tree/" + KnobsSource::revision;
  ImGui::TextLinkOpenURL(label(StudioText::CatalogSource).c_str(), source.c_str());
  ImGui::Spacing();
  ImGui::TextWrapped("%s", tr(StudioText::CatalogPreviewTargets));
  ImGui::TextWrapped("%s", tr(StudioText::CatalogExportTargets));
  ImGui::TextWrapped("%s", tr(StudioText::CatalogOtherTargets));
  ImGui::Spacing();
  if (extensions_->busy()) {
    ImGui::TextWrapped("%s", tr(StudioText::CatalogDownloading));
    if (ImGui::Button(label(StudioText::Cancel).c_str())) extensions_->cancel();
  } else if (extensions_->installed()) {
    ImGui::TextUnformatted(tr(StudioText::CatalogInstalled));
    if (ImGui::Button(label(StudioText::CatalogRemove).c_str())) extensions_->remove();
    ImGui::TextWrapped("%s", tr(StudioText::CatalogRemoveHint));
  } else {
    ImGui::BeginDisabled(!extensions_->canInstall());
    if (ImGui::Button(label(StudioText::CatalogInstall).c_str())) extensions_->install();
    ImGui::EndDisabled();
    if (!catalogHttpAvailable()) ImGui::TextWrapped("%s", tr(StudioText::CatalogOffline));
  }
  if (!extensions_->error().empty()) ImGui::TextWrapped("%s", extensions_->error().c_str());
}

void StudioApp::renderTopMenuBar() {
  if (ImGui::BeginMainMenuBar()) {
    if (widgetDraft_) {
      renderWidgetDraftToolbar();
      ImGui::EndMainMenuBar();
      return;
    }
    const float menuWidth = ImGui::GetWindowWidth() * 0.14f;
    if (ImGui::BeginMenu(label(StudioText::File, menuWidth).c_str())) {
      if (ImGui::MenuItem(label(StudioText::NewEmpty).c_str())) requestAction(PendingAction::New);
      if (ImGui::MenuItem(label(StudioText::OpenJson).c_str(), "Ctrl+O")) requestAction(PendingAction::Open);
      if (ImGui::MenuItem(label(StudioText::SaveProject).c_str(), "Ctrl+S")) saveProject();
      if (ImGui::MenuItem(label(StudioText::SaveAs).c_str(), "Ctrl+Shift+S")) saveProject(true);
      ImGui::Separator();
      if (ImGui::MenuItem(label(StudioText::SaveSnapshot).c_str())) {
        captureSnapshot(tr(StudioText::ManualSnapshot), SnapshotKind::Manual);
      }
      if (ImGui::MenuItem(label(StudioText::RecoverEarlier).c_str(), nullptr, false,
                           persistenceEnabled_ && !hasUnsavedChanges())) {
        std::vector<std::string> errors;
        recoveryCandidates_ = recoverySession_.scan(errors);
        openRecoveryPopup_ = !recoveryCandidates_.empty();
        if (recoveryCandidates_.empty()) fileStatus_ = tr(StudioText::NoRecovery);
        for (const auto& error : errors) fileStatus_ += "\n" + error;
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu(label(StudioText::Edit, menuWidth).c_str())) {
      if (ImGui::MenuItem(label(StudioText::Undo).c_str(), "Ctrl+Z", false, !past_.empty())) undo();
      if (ImGui::MenuItem(label(StudioText::Redo).c_str(), "Ctrl+Y", false, !future_.empty())) redo();
      ImGui::Separator();
      if (ImGui::MenuItem(label(StudioText::NewWindow).c_str())) addWidgetToSelection(WidgetType::Window);
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu(label(StudioText::View, menuWidth).c_str())) {
      ImGui::MenuItem(label(StudioText::PreviewMode).c_str(), nullptr, &previewMode_);
      ImGui::MenuItem(label(StudioText::GridSnap).c_str(), nullptr, &snapToGrid_);
      const auto zoom = std::string(tr(StudioText::CanvasZoom)) + " (" +
        std::to_string(canvasZoomPercent_) + "%)###CanvasZoomMenu";
      if (ImGui::BeginMenu(zoom.c_str(), !previewMode_)) {
        if (ImGui::MenuItem(label(StudioText::ZoomIn).c_str(), nullptr, false, canvasZoomPercent_ < 150))
          setCanvasZoom(canvasZoomPercent_ + 10, {});
        if (ImGui::MenuItem(label(StudioText::ZoomOut).c_str(), nullptr, false, canvasZoomPercent_ > 50))
          setCanvasZoom(canvasZoomPercent_ - 10, {});
        if (ImGui::MenuItem(label(StudioText::ZoomReset).c_str(), nullptr, false, canvasZoomPercent_ != 100))
          setCanvasZoom(100, {});
        ImGui::TextDisabled("%s", tr(StudioText::ZoomHint));
        ImGui::EndMenu();
      }
      if (ImGui::MenuItem(label(StudioText::CanvasReset).c_str(), tr(StudioText::HomeKey), false, !previewMode_))
        resetCanvasView();
      ImGui::MenuItem(label(StudioText::CodePanel).c_str(), nullptr, &showCodePanel_);
      ImGui::MenuItem(label(StudioText::VersionHistory).c_str(), nullptr, &showHistoryPanel_);
      ImGui::MenuItem(label(StudioText::RunBuild).c_str(), nullptr, &showBuildPanel_);
      ImGui::MenuItem(label(StudioText::Assistant).c_str(), nullptr, &showAssistant_);
      if (ImGui::BeginMenu(label(StudioText::Language).c_str())) {
        renderLanguageChoices();
        ImGui::EndMenu();
      }
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu(label(StudioText::NativeProject, menuWidth).c_str())) {
      if (ImGui::MenuItem(label(StudioText::RunBuildOpen).c_str())) showBuildPanel_ = true;
      ImGui::TextDisabled("%s", tr(buildStatusText(nativeBuild_.phase())));
      ImGui::EndMenu();
    }

    ImGui::Separator();
    const auto modeButton = [&](const char* label, bool target) {
      const bool current = previewMode_ == target;
      if (current) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
      if (ImGui::Button(label)) previewMode_ = target;
      if (current) ImGui::PopStyleColor();
    };
    modeButton(label(StudioText::Ide, menuWidth).c_str(), false);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tr(StudioText::IdeHint));
    ImGui::SameLine();
    modeButton(label(StudioText::Preview, menuWidth).c_str(), true);
    if (previewMode_) {
      ImGui::SameLine();
      if (ImGui::Button(label(StudioText::PreviewReset, menuWidth).c_str())) {
        previewProject_ = project_; previewRenderer_.reset(); canvasNeedsSync_ = true;
      }
    }
    ImGui::Separator();
    const std::string title = project_.name + (hasUnsavedChanges() ? " *" : "");
    const auto position = ImGui::GetCursorScreenPos();
    const float available = ImGui::GetWindowPos().x + ImGui::GetWindowWidth() -
      ImGui::GetStyle().WindowPadding.x - position.x;
    if (available > 12) {
      ImGui::PushClipRect(position, ImVec2(position.x + available, position.y + ImGui::GetFrameHeight()), true);
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted(title.c_str());
      ImGui::PopClipRect();
      if (ImGui::IsItemHovered() && ImGui::CalcTextSize(title.c_str()).x > available)
        ImGui::SetTooltip("%s", title.c_str());
    }
    ImGui::EndMainMenuBar();
  }
}

void StudioApp::renderWidgetPalette(float x, float y, float width, float height) {
  ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
  const ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
    ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;
  if (ImGui::Begin("##WidgetPalette", nullptr, flags)) {
    ImGui::SeparatorText(tr(StudioText::Widgets));
    if (!widgetDraft_ && ImGui::Button(label(StudioText::Library, std::max(36.0f, width - 36.0f)).c_str(),
                                      ImVec2(-1.0f, 0.0f))) showWidgetLibrary_ = true;
    if (ImGui::BeginChild("##PaletteItems")) {
      if (!widgetDraft_) ImGui::SeparatorText(tr(StudioText::LibraryBase));
      std::optional<StudioText> lastCat;
      for (const auto& entry : kPalette) {
        if (widgetDraft_ && entry.type == WidgetType::Window) continue;
        if (entry.type == WidgetType::Knob && !extensions_->installed()) continue;
        if (lastCat != entry.category) {
          lastCat = entry.category;
          ImGui::SeparatorText(tr(entry.category));
        }
        const auto caption = label(entry.label, ImGui::GetContentRegionAvail().x - 2 * ImGui::GetStyle().FramePadding.x);
        if (ImGui::Button(caption.c_str(), ImVec2(-1.0f, 0.0f)) && !canvasController_.dragging())
          addWidgetToSelection(entry.type, entry.preset);
        if (ImGui::IsItemHovered() && ImGui::CalcTextSize(tr(entry.label)).x >
            ImGui::GetItemRectSize().x - 2 * ImGui::GetStyle().FramePadding.x)
          ImGui::SetItemTooltip("%s", tr(entry.label));
        ImGuiNode paletteNode;
        paletteNode.id = std::string("palette/") + widgetTypeId(entry.type);
        if (entry.preset) paletteNode.id += std::string("/") + entry.preset;
        paletteNode.type = entry.type;
        auto observation = observeNativeItem(paletteNode, {}, {}, 0);
        observation.paletteType = entry.type;
        observation.palettePreset = entry.preset;
        hierarchyItems_.push_back(std::move(observation));
      }
      if (!widgetDraft_ && !widgetLibrary_.entries().empty()) {
        ImGui::SeparatorText(tr(StudioText::LibraryCustom));
        for (const auto& entry : widgetLibrary_.entries()) {
          const auto caption = entry.name + "###" + entry.id;
          if (ImGui::Button(caption.c_str(), ImVec2(-1.0f, 0.0f)) && !canvasController_.dragging())
            insertWidgetIntoSelection(instantiateLibraryWidget(*entry.node));
          if (ImGui::IsItemHovered()) ImGui::SetItemTooltip("%s", entry.name.c_str());
          ImGuiNode item;
          item.id = "library/" + entry.id;
          item.type = entry.node->type;
          auto observation = observeNativeItem(item, {}, {}, 0);
          observation.paletteType = item.type;
          observation.paletteTemplate = entry.node;
          hierarchyItems_.push_back(std::move(observation));
        }
      }
    }
    ImGui::EndChild();
  }
  ImGui::End();
}

void StudioApp::renderHierarchy(float x, float y, float width, float height) {
  ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
  const ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
    ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;
  if (ImGui::Begin("##Hierarchy", nullptr, flags)) {
    ImGui::SeparatorText(tr(StudioText::Hierarchy));
    const auto location = locateNode(project_.nodes, selectedNodeId());
    const auto* parent = location ? findNodeById(project_.nodes, location->parentId) : nullptr;
    const size_t count = parent ? parent->children.size() : project_.nodes.size();
    const auto reorder = [&](bool down) {
      const auto result = moveDocumentNode(project_, selectedNodeId(), location->parentId,
                                           down ? location->index + 2 : location->index - 1);
      if (result) commitChange(*result.project);
      else if (!result.error.empty()) fileStatus_ = result.error;
    };
    ImGui::BeginDisabled(!location || location->index == 0);
    const float buttonWidth = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f -
      2 * ImGui::GetStyle().FramePadding.x;
    if (ImGui::Button(label(StudioText::Up, buttonWidth).c_str())) reorder(false);
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(!location || location->index + 1 >= count);
    if (ImGui::Button(label(StudioText::Down, buttonWidth).c_str())) reorder(true);
    ImGui::EndDisabled();
    if (ImGui::BeginChild("##HierarchyRows")) {
      if (!previewMode_) canvasController_.scrollHierarchy();
      renderNodeTree(project_.nodes);
    }
    ImGui::EndChild();
  }
  ImGui::End();
}

void StudioApp::renderNodeTree(const std::vector<ImGuiNode>& nodes, const std::string& parent, int depth) {
  for (const auto& node : nodes) {
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (depth == 0) flags |= ImGuiTreeNodeFlags_DefaultOpen;
    if (node.children.empty()) flags |= ImGuiTreeNodeFlags_Leaf;
    if (std::find(selectedIds_.begin(), selectedIds_.end(), node.id) != selectedIds_.end())
      flags |= ImGuiTreeNodeFlags_Selected;

    std::string label = node.label + " [" + widgetTypeName(node.type) + "]###" + node.id;
    if (!previewMode_ && canvasController_.shouldExpandHierarchy(node.id))
      ImGui::SetNextItemOpen(true);
    bool open = ImGui::TreeNodeEx(label.c_str(), flags);
    auto observation = observeNativeItem(node, parent, {}, depth);
    observation.hierarchy = true;
    observation.hierarchyCollapsed = !open && !node.children.empty();
    hierarchyItems_.push_back(std::move(observation));
    if (previewMode_ && ImGui::IsItemClicked()) {
      selectNode(node.id);
    }
    if (open) {
      if (!node.children.empty()) {
        renderNodeTree(node.children, node.id, depth + 1);
      }
      ImGui::TreePop();
    }
  }
}

void StudioApp::resetCanvasView() {
  canvasPan_.reset();
  if (canvasZoomPercent_ != 100 || canvasOffset_.x != 0 || canvasOffset_.y != 0) {
    canvasZoomPercent_ = 100;
    canvasOffset_ = {};
    canvasNeedsSync_ = true;
  }
}

void StudioApp::setCanvasZoom(int percent, ImVec2 anchor) {
  percent = std::clamp(percent, 50, 150);
  if (percent == canvasZoomPercent_) return;
  const float ratio = static_cast<float>(percent) / static_cast<float>(canvasZoomPercent_);
  canvasOffset_ = ImVec2(anchor.x + (canvasOffset_.x - anchor.x) * ratio,
                        anchor.y + (canvasOffset_.y - anchor.y) * ratio);
  canvasZoomPercent_ = percent;
  canvasNeedsSync_ = true;
}

void StudioApp::updateCanvasView(ImVec2 minimum, ImVec2 maximum) {
  if (previewMode_) return;
  const auto& io = ImGui::GetIO();
  const bool blocked = ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId) || io.WantTextInput;
  const bool mouseValid = ImGui::IsMousePosValid();
  const bool canStart = !blocked && mouseValid && !canvasController_.dragging() &&
    !ImGui::IsMouseDown(ImGuiMouseButton_Left) &&
    ImRect(minimum, maximum).Contains(io.MousePos) && canvasWindowHovered();
  if (canStart && ImGui::IsKeyPressed(ImGuiKey_Home)) {
    resetCanvasView();
    return;
  }
  if (canStart && !canvasPan_ && io.KeyCtrl && io.MouseWheel != 0.0f) {
    const int change = static_cast<int>(std::round(std::clamp(io.MouseWheel, -10.0f, 10.0f) * 10.0f));
    setCanvasZoom(canvasZoomPercent_ + change, ImVec2(io.MousePos.x - minimum.x, io.MousePos.y - minimum.y));
    return;
  }
  if (canStart && ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
    canvasPan_ = CanvasPan{io.MousePos, canvasOffset_};
  if (!canvasPan_) return;
  if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
    canvasPan_.reset();
    return;
  }

  ImVec2 next = canvasOffset_;
  if (!mouseValid || blocked || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
    next = canvasPan_->offset;
    canvasPan_.reset();
  } else if (ImGui::IsMouseDown(ImGuiMouseButton_Middle) || ImGui::IsMouseReleased(ImGuiMouseButton_Middle)) {
    next = ImVec2(canvasPan_->offset.x + io.MousePos.x - canvasPan_->pointer.x,
                  canvasPan_->offset.y + io.MousePos.y - canvasPan_->pointer.y);
    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Middle)) canvasPan_.reset();
  } else canvasPan_.reset();

  if (next.x != canvasOffset_.x || next.y != canvasOffset_.y) {
    canvasOffset_ = next;
    canvasNeedsSync_ = true;
  }
}

void StudioApp::renderCenterCanvas(float x, float y, float w, float h) {
  ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(w, h), ImGuiCond_Always);
  ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                           ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
                           ImGuiWindowFlags_NoBringToFrontOnFocus;

  if (ImGui::Begin("##CenterCanvas", nullptr, flags)) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImVec2 avail = ImGui::GetContentRegionAvail();
    const ImVec2 visibleEnd(p0.x + std::max(1.0f, avail.x), p0.y + std::max(1.0f, avail.y));
    updateCanvasView(p0, visibleEnd);
    const float scale = previewMode_ ? 1.0f : canvasScale();
    designRenderer_.beginFrame(true, scale, previewMode_ ? std::nullopt : canvasController_.orbitPreview());
    if (!previewMode_ && !ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId) && canvasWindowHovered()) {
      const auto shift = canvasController_.edgePan(p0, visibleEnd);
      canvasOffset_.x += shift.x;
      canvasOffset_.y += shift.y;
    }
    const ImVec2 offset = previewMode_ ? ImVec2() : ImVec2(std::round(canvasOffset_.x), std::round(canvasOffset_.y));
    const ImVec2 origin(p0.x + offset.x, p0.y + offset.y);
    if (origin.x != canvasOrigin_.x || origin.y != canvasOrigin_.y) canvasNeedsSync_ = true;
    canvasOrigin_ = origin;

    // Keep the grid in project coordinates and draw only the visible portion.
    const float gridStep = 24.0f * scale;
    const auto gridStart = [gridStep](float value) {
      const float remainder = std::fmod(value, gridStep);
      return remainder < 0 ? remainder + gridStep : remainder;
    };
    for (float gx = gridStart(offset.x); gx < avail.x; gx += gridStep) {
      dl->AddLine(ImVec2(p0.x + gx, p0.y), ImVec2(p0.x + gx, p0.y + avail.y),
                  IM_COL32(36, 44, 62, 110));
    }
    for (float gy = gridStart(offset.y); gy < avail.y; gy += gridStep) {
      dl->AddLine(ImVec2(p0.x, p0.y + gy), ImVec2(p0.x + avail.x, p0.y + gy),
                  IM_COL32(36, 44, 62, 110));
    }

    ImGui::InvisibleButton("##CanvasDropZone", ImVec2(std::max(1.0f, avail.x), std::max(1.0f, avail.y)));
    ImGuiNode background;
    background.id = "__studio_canvas__";
    auto backgroundItem = observeNativeItem(background, {}, {}, -1);
    backgroundItem.canvasBackground = true;
    hierarchyItems_.push_back(std::move(backgroundItem));

    // Preview values belong to a disposable copy, never to the design document.
    StudioProject editedCanvas = previewMode_ ? *previewProject_ : project_;
    bool geometryChanged = false;
    const bool synchronize = canvasNeedsSync_;
    canvasNeedsSync_ = false;
    const ImGuiStyle editorStyle = ImGui::GetStyle();
    applyPreviewStyle(editedCanvas.style);
    auto& io = ImGui::GetIO();
    const float editorFontScale = io.FontGlobalScale;
    if (scale != 1.0f) {
      ImGui::GetStyle().ScaleAllSizes(scale);
      // Refresh ImGui's cached font size before drawing titles and child windows.
      io.FontGlobalScale *= scale;
      ImGui::PushFont(ImGui::GetFont());
    }
    for (auto& win : editedCanvas.nodes) {
      if (!win.visible || win.type != WidgetType::Window) continue;
      auto* windowState = previewMode_ ? &previewRenderer_.windowState(win.id) : nullptr;
      if (windowState && windowState->closed) continue;
      const bool fixedPosition = windowState && (windowState->minimized || windowState->maximized);
      const bool restoreGeometry = windowState && std::exchange(windowState->updateGeometry, false);
      const bool minimized = windowState && windowState->minimized;
      const ScopedPreviewWindowStyle windowStyle(win, scale, minimized);
      ImVec2 winScreenPos(canvasOrigin_.x + win.x * scale, canvasOrigin_.y + win.y * scale);
      ImVec2 winScreenSize(win.width * scale, win.height * scale);
      if (windowState && windowState->maximized) {
        winScreenPos = p0;
        winScreenSize = ImVec2(visibleEnd.x - p0.x, visibleEnd.y - p0.y);
        if (restoreGeometry) ImGui::SetNextWindowCollapsed(false, ImGuiCond_Always);
      }
      if (minimized) winScreenSize.y = ImGui::GetFrameHeight();
      if (previewMode_ && synchronize) ImGui::SetNextWindowCollapsed(false, ImGuiCond_Always);
      const ImGuiCond geometry = synchronize || fixedPosition || restoreGeometry ? ImGuiCond_Always : ImGuiCond_Appearing;
      ImGui::SetNextWindowPos(winScreenPos, geometry);
      ImGui::SetNextWindowSize(winScreenSize, geometry);

      ImGuiWindowFlags wflags = ImGuiWindowFlags_NoSavedSettings;
      if (win.props.noTitleBar) wflags |= ImGuiWindowFlags_NoTitleBar;
      if (win.props.noBackground) wflags |= ImGuiWindowFlags_NoBackground;
      if (win.props.noCollapse || !win.props.showCollapseArrow) wflags |= ImGuiWindowFlags_NoCollapse;
      if (win.props.noResize || fixedPosition) wflags |= ImGuiWindowFlags_NoResize;
      if (win.props.noMove || fixedPosition || (!previewMode_ && win.props.noTitleBar)) wflags |= ImGuiWindowFlags_NoMove;
      for (const auto& child : win.children)
        if (!minimized && child.visible && child.type == WidgetType::MenuBar && child.props.menuBarEdge == MenuBarEdge::Top)
          wflags |= ImGuiWindowFlags_MenuBar;

      const bool customTitle = NativePreview::usesCustomTitle(win);
      const std::string windowName = (customTitle ? std::string{} : win.label) +
        (previewMode_ ? "###canvas-preview_" : "###canvas_") + win.id;
      const bool open = ImGui::Begin(windowName.c_str(), nullptr, wflags);
      windowStyle.afterBegin();
      clipCanvasWindow(p0, visibleEnd);
      auto& renderer = previewMode_ ? previewRenderer_ : designRenderer_;
      renderer.drawWindowShadow(win, p0, visibleEnd);
      if (!previewMode_) renderer.recordTitleBar(win);
      const auto titleHit = customTitle ? renderer.renderTitleBar(win, windowState) : TitleBarHit::None;
      const bool titleClicked = !win.props.noTitleBar && ImGui::GetCurrentWindow()->TitleBarRect().Contains(io.MousePos);
      if (titleHit != TitleBarHit::Widget && (previewMode_ || (titleClicked && !io.KeyCtrl)) &&
          ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&
          ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        selectNode(win.id);
      }
      if (open && !minimized) {
        ImVec2 curPos = ImGui::GetWindowPos();
        ImVec2 curSize = ImGui::GetWindowSize();
        const std::array<float, 4> observed{curPos.x, curPos.y, curSize.x, curSize.y};
        const auto previous = canvasGeometry_.find(win.id);
        const bool userGeometryChange = !synchronize && !fixedPosition && !restoreGeometry && previous != canvasGeometry_.end() &&
          previous->second != observed && ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsWindowFocused();
        // ImGui rounds positions and enforces minimum sizes. Merely displaying an
        // imported document must not rewrite its original coordinates/dimensions.
        if (userGeometryChange) {
          const auto position = [&](float coord, float origin) {
            const float logical = (coord - origin) / scale;
            return snapToGrid_ ? std::round(logical / 8.0f) * 8.0f : logical;
          };
          const auto before = std::array<float, 4>{win.x, win.y, win.width, win.height};
          if (observed[0] != previous->second[0]) win.x = position(curPos.x, canvasOrigin_.x);
          if (observed[1] != previous->second[1]) win.y = position(curPos.y, canvasOrigin_.y);
          if (observed[2] != previous->second[2]) win.width = curSize.x / scale;
          if (observed[3] != previous->second[3]) win.height = curSize.y / scale;
          geometryChanged |= before != std::array<float, 4>{win.x, win.y, win.width, win.height};
        }
        canvasGeometry_[win.id] = observed;

        if (!previewMode_) designRenderer_.recordWindow(win);
        renderer.renderWindowBody(win);
        if (!previewMode_ && win.children.empty()) ImGui::TextDisabled("%s", surfaceText(StudioText::CanvasEmptyWindow));
      }
      ImGui::PopClipRect();
      ImGui::End();
    }
    if (scale != 1.0f) {
      io.FontGlobalScale = editorFontScale;
      ImGui::PopFont();
    }
    ImGui::GetStyle() = editorStyle;
    if (previewMode_) previewProject_ = std::move(editedCanvas);
    else if (geometryChanged) {
      commitInteractiveChange(editedCanvas);
      canvasNeedsSync_ = false; // Preserve an in-progress ImGui window drag.
    }
  } else designRenderer_.beginFrame(true, canvasScale());
  ImGui::End();
}

bool StudioApp::renderImageProperties(ImGuiNode& node, const StudioProject& edited) {
  auto& asset = node.props.imageAsset;
  ImGui::SeparatorText(surfaceText(StudioText::Image));
  if (asset) {
    const auto name = asset->find("name");
    if (name != asset->end() && name->is_string()) ImGui::TextWrapped("%s", name->get_ref<const std::string&>().c_str());
    if (ImGui::Button(surfaceLabel(StudioText::RemoveImage).c_str())) {
      asset.reset();
      imageError_.clear();
      return true;
    }
  }
  ImGui::SetNextItemWidth(-FLT_MIN);
  ImGui::InputTextWithHint("##ImagePath", "Ruta local PNG o JPEG", &imagePath_);
  ImGui::BeginDisabled(imagePath_.empty());
  const bool load = ImGui::Button(surfaceLabel(StudioText::ImportImage).c_str());
  ImGui::EndDisabled();
  if (load) {
    auto imported = importImage(std::filesystem::u8path(imagePath_));
    imageError_ = std::move(imported.error);
    if (imported.asset) {
      const auto previous = asset;
      asset = std::move(imported.asset);
      if (projectToJson(edited).dump(2).size() < ProjectJson::kMaxProjectBytes) {
        imagePath_.clear();
        return true;
      }
      asset = previous;
      imageError_ = "La imagen supera el espacio disponible en el JSON del proyecto.";
    }
  }
  if (!imageError_.empty()) ImGui::TextWrapped("%s", imageError_.c_str());
  return false;
}

void StudioApp::renderRightInspector(float x, float y, float w, float h) {
  ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(w, h), ImGuiCond_Always);
  ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                           ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;

  if (ImGui::Begin("##RightInspector", nullptr, flags)) {
    ImGui::SeparatorText(surfaceText(StudioText::Inspector));

    StudioProject edited = project_;
    bool discreteChange = false;
    ImGuiNode* sel = !selectedNodeId().empty() ? findNodeById(edited.nodes, selectedNodeId()) : nullptr;
    if (imageEditNode_ != selectedNodeId()) {
      imageEditNode_ = selectedNodeId();
      imagePath_.clear();
      imageError_.clear();
    }
    if (sel && (!widgetDraft_ || (sel->id != widgetDraft_->canvasId && sel->id != widgetDraft_->wrapperId))) {
      ImGui::InputText(surfaceLabel(StudioText::WidgetLabel).c_str(), &sel->label);
      ImGui::InputText(surfaceLabel(StudioText::Variable).c_str(), &sel->varName);
      ImGui::InputText(surfaceLabel(StudioText::ImGuiId).c_str(), &sel->imguiId);
      const bool roundStatus = isRoundStatusWidget(sel->type);
      ImGui::DragFloat(surfaceLabel(roundStatus ? StudioText::RoundWidgetDiameter : StudioText::Width).c_str(),
        &sel->width, 1.0f, roundStatus ? 16.0f : -1.0f, 16000.0f);
      if (!roundStatus) {
        ImGui::DragFloat(surfaceLabel(StudioText::Height).c_str(), &sel->height, 1.0f, 0.0f, 16000.0f);
        if (!isStatusWidget(sel->type)) ImGui::TextDisabled("%s", surfaceText(StudioText::WidthHint));
      }
      if (isFreeLayoutUtility(sel->type) && sel->type != WidgetType::Dummy && !sel->useCursorPos)
        ImGui::Checkbox(surfaceLabel(StudioText::CustomSize).c_str(), &sel->props.utilityCustomSize);
      if (selectedIds_.size() > 1)
        ImGui::TextDisabled("%zu seleccionados. Propiedades de %s.", selectedIds_.size(), sel->label.c_str());
      ImGui::TextWrapped("%s", tr(isFreeLayoutUtility(sel->type) ? StudioText::FreeDragHint : StudioText::GeneralDragHint));
      ImGui::Checkbox(surfaceLabel(StudioText::Visible).c_str(), &sel->visible);
      ImGui::Checkbox(surfaceLabel(StudioText::FreePosition).c_str(), &sel->useCursorPos);
      ImGui::DragFloat("X", &sel->x, 1, -16000, 16000);
      ImGui::DragFloat("Y", &sel->y, 1, -16000, 16000);
      ImGui::Checkbox(surfaceLabel(StudioText::PaletteSameLine).c_str(), &sel->sameLine);
      ImGui::DragFloat(surfaceLabel(StudioText::SameLineSpacing).c_str(), &sel->sameLineSpacing, 1, 0, 1000);
      ImGui::DragFloat(surfaceLabel(StudioText::Indent).c_str(), &sel->indent, 1, 0, 1000);
      discreteChange = renderWidgetProperties(*sel, plotEditor_);
      if (NativePreview::canRenderInTitleBar(sel->type)) {
        const auto location = locateNode(edited.nodes, sel->id);
        if (location && location->depth == 1) {
          ImGui::Checkbox(surfaceLabel(StudioText::InTitle).c_str(), &sel->props.inTitleBar);
          if (sel->props.inTitleBar) renderTitlePosition(sel->props);
        }
      }
      if (isImageWidget(sel->type)) discreteChange |= renderImageProperties(*sel, edited);
      if (sel->type == WidgetType::Window) {
        ImGui::Checkbox(surfaceLabel(StudioText::HideTitle).c_str(), &sel->props.noTitleBar);
        ImGui::Checkbox(surfaceLabel(StudioText::PreventResize).c_str(), &sel->props.noResize);
        ImGui::Checkbox(surfaceLabel(StudioText::PreventMove).c_str(), &sel->props.noMove);
      }

      if (sel->type == WidgetType::MenuBar) {
        int edgeIdx = static_cast<int>(sel->props.menuBarEdge);
        const char* edges[] = {tr(StudioText::Top), tr(StudioText::Bottom), tr(StudioText::Left),
          tr(StudioText::Right), tr(StudioText::TitleBar), tr(StudioText::Free)};
        if (ImGui::Combo(surfaceLabel(StudioText::MenuPlacement).c_str(), &edgeIdx, edges, 6)) {
          sel->props.menuBarEdge = static_cast<MenuBarEdge>(edgeIdx);
        }
        if (sel->props.menuBarEdge == MenuBarEdge::TitleBar) renderTitlePosition(sel->props);
      }

      ImGui::Spacing();
      if (ImGui::Button(surfaceLabel(StudioText::Duplicate).c_str())) {
        StudioProject next = edited;
        auto res = duplicateNodeById(next.nodes, sel->id);
        if (res.has_value()) {
          selectNode(*res);
          commitChange(next);
          edited = project_;
        }
      }
      ImGui::SameLine();
      if (ImGui::Button(surfaceLabel(StudioText::Delete).c_str())) {
        StudioProject next = project_;
        if (removeNodeById(next.nodes, selectedNodeId())) {
          selectNode(next.nodes.empty() ? "" : next.nodes.front().id);
          commitChange(next);
          edited = project_;
        }
      }
    } else {
      plotEditor_.reset();
      ImGui::TextWrapped("%s", tr(widgetDraft_ && sel ? StudioText::CanvasEmptyWindow : StudioText::SelectWidgetHint));
    }

    if (!widgetDraft_) {
      ImGui::Spacing();
      ImGui::SeparatorText(surfaceText(StudioText::GlobalStyle));
      ImGui::SliderFloat(surfaceLabel(StudioText::WindowCorners).c_str(), &edited.style.windowRounding, 0.0f, 16.0f);
      ImGui::SliderFloat(surfaceLabel(StudioText::FrameCorners).c_str(), &edited.style.frameRounding, 0.0f, 16.0f);
      ImGui::ColorEdit4(surfaceLabel(StudioText::WindowBackground).c_str(), &edited.style.windowBg.r);
      ImGui::ColorEdit4(surfaceLabel(StudioText::ButtonBackground).c_str(), &edited.style.buttonColor.r);
    }
    if (discreteChange) commitChange(edited);
    else if (projectToJson(edited) != projectToJson(project_)) commitInteractiveChange(edited);
    const auto& report = previewMode_ ? previewRenderer_.report() : designRenderer_.report();
    if (!report.limitations.empty()) {
      ImGui::SeparatorText(surfaceText(StudioText::PreviewLimits));
      for (const auto& limitation : report.limitations) ImGui::TextWrapped("%s", limitation.c_str());
    }
  }
  ImGui::End();
}

void StudioApp::renderCodeDrawer(float x, float y, float w, float h) {
  ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(w, h), ImGuiCond_Always);
  ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                           ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;

  if (ImGui::Begin("##CodeDrawer", nullptr, flags)) {
    ImGui::SeparatorText(tr(StudioText::GeneratedCode));
    const bool compact = ImGui::GetContentRegionAvail().x < 450.0f;
    ImGui::SetNextItemWidth(compact ? 95.0f : 135.0f);
    const std::pair<TargetLanguage,const char*> languages[] = {
      {TargetLanguage::Cpp,"C++"},{TargetLanguage::Python,"Python"},
      {TargetLanguage::CSharp,"C#"},{TargetLanguage::Java,"Java"},{TargetLanguage::C,"C"},{TargetLanguage::Rust,"Rust"}
    };
    const char* selected = "C++";
    for (const auto& language : languages) if (codeLanguage_ == language.first) selected = language.second;
    if (ImGui::BeginCombo("##Lenguaje",selected)) {
      for (const auto& language : languages)
        if (ImGui::Selectable(language.second,codeLanguage_ == language.first) && codeLanguage_ != language.first) {
          codeLanguage_ = language.first;
          codePreview_.reset();
        }
      ImGui::EndCombo();
    }
    const bool python = codeLanguage_ == TargetLanguage::Python;
    const bool csharp = codeLanguage_ == TargetLanguage::CSharp;
    const bool java = codeLanguage_ == TargetLanguage::Java;
    const bool c = codeLanguage_ == TargetLanguage::C;
    const bool rust = codeLanguage_ == TargetLanguage::Rust;
    if (codeLanguage_ == TargetLanguage::Cpp) {
      ImGui::SameLine();
      const char* modes[] = {".hpp", "Render()", tr(StudioText::FullSource)};
      int mode = static_cast<int>(cppMode_);
      ImGui::SetNextItemWidth(compact ? 115.0f : 135.0f);
      if (ImGui::Combo("##Modo",&mode,modes,IM_ARRAYSIZE(modes))) {
        cppMode_ = static_cast<CppEmitMode>(mode);
        codePreview_.reset();
      }
    }
    if (!compact) ImGui::SameLine();

    if (!codePreview_) {
      codePreview_ = python ? generatePythonCode(project_) : csharp ? generateCSharpCode(project_) :
        java ? generateJavaCode(project_) : c ? generateCCode(project_) :
        rust ? generateRustCode(project_) : generateCppDetailed(project_,cppMode_);
    }
    const auto& generated = *codePreview_;
    const auto& code = generated.code;
    ImGui::BeginDisabled(!generated.error.empty());
    if (ImGui::Button(surfaceLabel(StudioText::CopyCode).c_str())) {
      ImGui::SetClipboardText(code.c_str());
    }
    const auto exportCaption = std::string(tr(StudioText::Export)) + " " +
      (python ? "Python" : csharp ? ".NET" : java ? "Java" : c ? "C" : rust ? "Rust" : "CMake") + "…###ExportBundle";
    sameLineIfFits(exportCaption.c_str());
    if (ImGui::Button(exportCaption.c_str()) && fileDialog_ == FileDialog::None) {
      fileDialog_ = python ? FileDialog::ExportPython : csharp ? FileDialog::ExportCSharp :
        java ? FileDialog::ExportJava : c ? FileDialog::ExportC : rust ? FileDialog::ExportRust : FileDialog::ExportCpp;
      if (fileDialog_ == FileDialog::ExportCpp) exportCppRenderer_ = buildProfiles_.document().active().settings.cppRenderer;
      projectPathBuf_[0] = '\0'; fileError_.clear(); openFilePopup_ = true;
    }
    if (!python && !csharp && !java && !c && !rust) {
      constexpr std::pair<StudioText, FileDialog> destinations[] = {
        {StudioText::ExportAndroid, FileDialog::ExportAndroid},
        {StudioText::ExportMacOS, FileDialog::ExportMacOS},
        {StudioText::ExportIOS, FileDialog::ExportIOS}
      };
      for (const auto& [text, dialog] : destinations) {
        const auto caption = surfaceLabel(text);
        sameLineIfFits(caption.c_str());
        if (ImGui::Button(caption.c_str()) && fileDialog_ == FileDialog::None) {
          fileDialog_ = dialog;
          projectPathBuf_[0] = '\0'; fileError_.clear(); openFilePopup_ = true;
        }
      }
    }
    ImGui::EndDisabled();
    if (!generated.error.empty()) ImGui::TextWrapped("%s: %s", tr(StudioText::ErrorHeading), generated.error.c_str());
    if (!generated.diagnostics.empty())
      ImGui::TextWrapped("%s: %zu. %s", tr(StudioText::Warnings), generated.diagnostics.size(), tr(StudioText::CheckExportReport));

    ImGui::BeginChild("##CodeScroll", ImVec2(0, 0), ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::TextUnformatted(code.c_str());
    ImGui::EndChild();
  }
  ImGui::End();
}

void StudioApp::renderBuildWindow() {
  ImGui::SetNextWindowSize(ImVec2(820,620),ImGuiCond_FirstUseEver);
  if (ImGui::Begin(surfaceLabel(StudioText::BuildPanel).c_str(),&showBuildPanel_)) {
    ImGui::TextWrapped("%s", surfaceText(StudioText::BuildHint));
    ImGui::BeginDisabled(nativeBuild_.busy());
    auto& profiles = buildProfiles_.document();
    if (ImGui::BeginCombo(surfaceLabel(StudioText::Profile).c_str(),profiles.active().name.c_str())) {
      for (const auto& entry : profiles.profiles) {
        ImGui::PushID(entry.id.c_str());
        if (ImGui::Selectable(entry.name.c_str(),profiles.activeId == entry.id)) profiles.activeId = entry.id;
        ImGui::PopID();
      }
      ImGui::EndCombo();
    }
    ImGui::BeginDisabled(profiles.profiles.size() >= kMaxBuildProfiles);
    if (ImGui::Button(surfaceLabel(StudioText::DuplicateProfile).c_str())) {
      auto copy = profiles.active(); copy.id = nextId("profile"); copy.name += " (copia)";
      profiles.activeId = copy.id; profiles.profiles.push_back(std::move(copy));
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(profiles.profiles.size() <= 1);
    if (ImGui::Button(surfaceLabel(StudioText::DeleteProfile).c_str())) {
      const auto id = profiles.activeId;
      profiles.profiles.erase(std::remove_if(profiles.profiles.begin(),profiles.profiles.end(),[&](const BuildProfile& p) { return p.id == id; }),profiles.profiles.end());
      profiles.activeId = profiles.profiles.front().id;
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(!persistenceEnabled_ || !buildProfiles_.writable());
    if (ImGui::Button(surfaceLabel(StudioText::SaveProfiles).c_str())) {
      const auto result = buildProfiles_.save();
      buildProfileStatus_ = result ? std::string(tr(StudioText::SavedStatus)) + buildProfiles_.path().u8string() : result.error;
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(!persistenceEnabled_ || buildProfiles_.path().empty());
    if (ImGui::Button(surfaceLabel(StudioText::ReloadProfiles).c_str())) {
      const auto path = buildProfiles_.path();
      const bool loaded = buildProfiles_.load(path,(std::filesystem::current_path() / "build" / "studio-projects").u8string());
      buildProfileStatus_ = loaded ? tr(StudioText::ProfilesRestored) : buildProfiles_.error();
    }
    ImGui::EndDisabled();
    ImGui::TextDisabled("%s", surfaceText(StudioText::ProfilesHint));
    if (!buildProfileStatus_.empty()) ImGui::TextWrapped("%s",buildProfileStatus_.c_str());
    if (ImGui::Button(surfaceLabel(StudioText::ImportProfile).c_str())) {
      const auto result = projectBuildProfile(project_);
      if (!result) buildProfileStatus_ = result.error;
      else if (profiles.profiles.size() >= kMaxBuildProfiles) buildProfileStatus_ = tr(StudioText::ProfileLimit);
      else {
        auto copy = *result.profile; copy.id = nextId("profile");
        profiles.activeId = copy.id; profiles.profiles.push_back(std::move(copy));
        buildProfileStatus_ = tr(StudioText::ProfileImported);
      }
    }
    ImGui::SameLine();
    if (ImGui::Button(surfaceLabel(StudioText::EmbedProfile).c_str())) {
      const auto result = attachBuildProfile(project_,profiles.active());
      if (result.project) { commitChange(*result.project); buildProfileStatus_ = tr(StudioText::ProfileEmbedded); }
      else buildProfileStatus_ = result.error;
    }
    ImGui::TextDisabled("%s", surfaceText(StudioText::EmbedProfileHint));
    auto& profile = profiles.active();
    auto& buildSettings_ = profile.settings;
    auto& buildWorkspace_ = profile.workspace;
    ImGui::InputText(surfaceLabel(StudioText::ProfileName).c_str(),&profile.name);
    if (ImGui::BeginCombo(surfaceLabel(StudioText::BuildTarget).c_str(),buildTargetLabel(buildSettings_.target))) {
      for (const auto target : {BuildTarget::Cpp,BuildTarget::C,BuildTarget::CSharp,BuildTarget::Python,BuildTarget::Java,BuildTarget::Rust})
        if (ImGui::Selectable(buildTargetLabel(target),target == buildSettings_.target)) {
          if (target != buildSettings_.target) toolchainStatus_.clear();
          buildSettings_.target = target;
          if (target == BuildTarget::Rust && buildSettings_.configuration != "Release")
            buildSettings_.configuration = "Debug";
        }
      ImGui::EndCombo();
    }
    const bool c = buildSettings_.target == BuildTarget::C;
    const bool csharp = buildSettings_.target == BuildTarget::CSharp;
    const bool python = buildSettings_.target == BuildTarget::Python;
    const bool java = buildSettings_.target == BuildTarget::Java;
    const bool rust = buildSettings_.target == BuildTarget::Rust;
    if (buildSettings_.target == BuildTarget::Cpp)
      selectCppRenderer(surfaceLabel(StudioText::CppRenderer),buildSettings_.cppRenderer);
    ImGui::InputText(surfaceLabel(StudioText::WorkingDirectory).c_str(),&buildWorkspace_);
    if (rust) {
      ImGui::InputTextWithHint(surfaceLabel(StudioText::CargoExecutable).c_str(),tr(StudioText::AutoDetectHint),&buildSettings_.cargo);
      ImGui::Checkbox(surfaceLabel(StudioText::CargoCache).c_str(),&buildSettings_.rustOffline);
      ImGui::TextWrapped("%s", surfaceText(StudioText::RustDepsHint));
    } else if (java) {
      ImGui::InputTextWithHint(surfaceLabel(StudioText::JdkDirectory).c_str(),tr(StudioText::JdkPathHint),&buildSettings_.jdkHome);
      ImGui::InputText(surfaceLabel(StudioText::JarDirectory).c_str(),&buildSettings_.javaBinding);
      ImGui::TextWrapped("%s", surfaceText(StudioText::JavaDepsHint));
    } else if (python) {
      ImGui::InputText(surfaceLabel(StudioText::CPythonExecutable).c_str(),&buildSettings_.python);
      ImGui::InputTextWithHint(surfaceLabel(StudioText::BindingPath).c_str(),tr(StudioText::PythonBindingHint),&buildSettings_.pythonBinding);
      ImGui::TextWrapped("%s", surfaceText(StudioText::PythonDepsHint));
    } else if (csharp) {
      ImGui::InputText(surfaceLabel(StudioText::DotnetExecutable).c_str(),&buildSettings_.dotnet);
      ImGui::TextWrapped("%s", surfaceText(StudioText::DotnetDepsHint));
    } else {
      ImGui::InputText("CMake",&buildSettings_.cmake);
      ImGui::InputTextWithHint(surfaceLabel(StudioText::CMakeGenerator).c_str(),tr(StudioText::GeneratorHint),&buildSettings_.generator);
      ImGui::InputTextWithHint(surfaceLabel(StudioText::CppCompiler).c_str(),tr(StudioText::CompilerHint),&buildSettings_.compiler);
      if (c) ImGui::InputTextWithHint(surfaceLabel(StudioText::CCompiler).c_str(),tr(StudioText::CompilerHint),&buildSettings_.cCompiler);
      if (!c && buildSettings_.cppRenderer == CppRenderer::Vulkan) {
        ImGui::InputTextWithHint(surfaceLabel(StudioText::VulkanSdkDirectory).c_str(),tr(StudioText::AutoDetectHint),&buildSettings_.vulkanRoot);
        ImGui::TextWrapped("%s",surfaceText(StudioText::VulkanDependencies));
      }
    }
    if (!python && !java && ImGui::BeginCombo(surfaceLabel(StudioText::BuildConfig).c_str(),buildSettings_.configuration.c_str())) {
      for (const auto* config : {"Debug","Release","RelWithDebInfo","MinSizeRel"})
        if ((!rust || std::string_view(config) == "Debug" || std::string_view(config) == "Release") &&
            ImGui::Selectable(config,buildSettings_.configuration == config)) buildSettings_.configuration = config;
      ImGui::EndCombo();
    }
    if (ImGui::Button(surfaceLabel(StudioText::DetectTools).c_str())) {
      const auto found = detectToolchain();
      const auto pathOrStatus = [&](const std::filesystem::path& path, StudioText status = StudioText::MissingTool) {
        return path.empty() ? std::string(tr(status)) : path.u8string();
      };
      if (rust) {
        const auto cargo = RustBuild::findCargo(buildSettings_.cargo);
        toolchainStatus_ = "Cargo: " + pathOrStatus(cargo);
      } else if (java) {
        const auto tools = JavaBuild::findTools(buildSettings_.jdkHome);
        toolchainStatus_ = "javac: " + pathOrStatus(tools.javac) + "\njava: " + pathOrStatus(tools.java);
      } else if (python) {
        if (!found.python.empty()) buildSettings_.python = found.python.u8string();
        toolchainStatus_ = "Python (PATH): " + pathOrStatus(found.python);
      } else if (csharp) {
        if (!found.dotnet.empty()) buildSettings_.dotnet = found.dotnet.u8string();
        toolchainStatus_ = "dotnet: " + pathOrStatus(found.dotnet);
      } else {
        if (!found.cmake.empty()) buildSettings_.cmake = found.cmake.u8string();
        toolchainStatus_ = "CMake: " + pathOrStatus(found.cmake) +
          "\nC++ (PATH): " + pathOrStatus(found.compiler, StudioText::DetectByCmake) +
          "\nC (PATH): " + pathOrStatus(found.cCompiler, StudioText::DetectByCmake);
      }
    }
    if (!toolchainStatus_.empty()) ImGui::TextWrapped("%s",toolchainStatus_.c_str());
    if (!python && !csharp && !java && !rust && ImGui::CollapsingHeader(surfaceLabel(StudioText::LocalDependencies).c_str())) {
      ImGui::InputText(surfaceLabel(StudioText::GlfwSources).c_str(),&buildSettings_.glfwSource);
      if (c) {
        ImGui::TextWrapped("%s", surfaceText(StudioText::CimguiDepsHint));
        ImGui::InputText(surfaceLabel(StudioText::CimguiSources).c_str(),&buildSettings_.cimguiSource);
        ImGui::InputText(surfaceLabel(StudioText::CimguiDearImGui).c_str(),&buildSettings_.cImguiSource);
      } else {
        ImGui::TextWrapped("%s", surfaceText(StudioText::CppDepsHint));
        ImGui::InputText(surfaceLabel(StudioText::ImguiSources).c_str(),&buildSettings_.imguiSource);
      }
    }
    if (ImGui::Button(surfaceLabel(StudioText::CompileNow).c_str())) nativeBuild_.build(project_,std::filesystem::u8path(buildWorkspace_),buildSettings_);
    ImGui::SameLine();
    const bool canRun = nativeBuild_.canRun(project_,buildSettings_,std::filesystem::u8path(buildWorkspace_));
    ImGui::BeginDisabled(!canRun);
    if (ImGui::Button(surfaceLabel(StudioText::RunNow).c_str())) nativeBuild_.run(project_,buildSettings_,std::filesystem::u8path(buildWorkspace_));
    ImGui::SameLine();
    if (ImGui::Button(surfaceLabel(StudioText::TestStartup).c_str())) nativeBuild_.run(project_,buildSettings_,std::filesystem::u8path(buildWorkspace_),true);
    ImGui::EndDisabled();
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(!nativeBuild_.busy());
    if (ImGui::Button(surfaceLabel(StudioText::StopBuild).c_str())) nativeBuild_.cancel();
    ImGui::EndDisabled();
    ImGui::Text("%s: %s", tr(StudioText::Status), tr(buildStatusText(nativeBuild_.phase())));
    if (nativeBuild_.exitCode()) ImGui::Text("%s: %d", tr(StudioText::ExitCode), *nativeBuild_.exitCode());
    if (!nativeBuild_.executable().empty() && !canRun && !nativeBuild_.busy())
      ImGui::TextWrapped("%s", surfaceText(StudioText::RebuildHint));
    if (!nativeBuild_.error().empty()) ImGui::TextWrapped("%s",nativeBuild_.error().c_str());
    if (!nativeBuild_.sourceDirectory().empty())
      ImGui::TextWrapped("%s: %s", tr(StudioText::OutputPath), nativeBuild_.sourceDirectory().u8string().c_str());
    if (ImGui::CollapsingHeader(("Diagnosticos ("+std::to_string(nativeBuild_.diagnostics().size())+")###BuildDiagnostics").c_str())) {
      ImGui::BeginChild("DiagnosticsList",ImVec2(0,170),true);
      ImGui::TextDisabled("%s", surfaceText(StudioText::BuildLogHint));
      const bool currentBuild = nativeBuild_.matches(project_);
      size_t index = 0;
      for (const auto& d : nativeBuild_.diagnostics()) {
        ImGui::PushID(static_cast<int>(index++));
        auto summary = d.message.substr(0,d.message.find('\n')); if (summary.size() > 180) summary = summary.substr(0,180)+"...";
        const bool expanded = ImGui::TreeNode("##Diagnostic","[%s] %s:%zu %s",diagnosticSeverityLabel(d.severity),d.file.filename().u8string().c_str(),d.line,summary.c_str());
        if (expanded) {
        ImGui::TextWrapped("[%s] %s%s%s",d.stage.c_str(),d.code.c_str(),d.code.empty()?"":": ",d.message.c_str());
        if (!d.file.empty()) ImGui::TextWrapped("%s:%zu:%zu",d.file.u8string().c_str(),d.line,d.column);
        const bool mapped = !d.nodeId.empty() && currentBuild && findNodeById(project_.nodes,d.nodeId);
        if (!d.nodeId.empty()) {
          ImGui::BeginDisabled(!mapped);
          if (ImGui::SmallButton(surfaceLabel(StudioText::SelectWidget).c_str())) { selectNode(d.nodeId); previewMode_ = false; }
          ImGui::EndDisabled();
          if (!mapped) ImGui::TextDisabled("%s", surfaceText(StudioText::MissingDiagnosticWidget));
        }
        const auto source = nativeBuild_.diagnosticSource(d);
        if (source && ImGui::SmallButton(surfaceLabel(StudioText::ShowExportedCode).c_str())) {
          diagnosticSourceLines_.clear(); std::istringstream lines(*source); std::string line;
          while (std::getline(lines,line)) diagnosticSourceLines_.push_back(line);
          diagnosticSourceTitle_ = d.file.u8string(); diagnosticSourceLine_ = d.line;
          showDiagnosticSource_ = true; scrollDiagnosticSource_ = true;
        }
        ImGui::TreePop(); }
        ImGui::PopID();
      }
      ImGui::EndChild();
    }
    ImGui::BeginChild("BuildOutput",ImVec2(0,0),true,ImGuiWindowFlags_HorizontalScrollbar);
    const bool atEnd = ImGui::GetScrollY() >= ImGui::GetScrollMaxY();
    const auto& log = nativeBuild_.log();
    ImGui::TextUnformatted(log.data(),log.data()+log.size());
    if (atEnd) ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();
  }
  ImGui::End();
}

void StudioApp::renderDiagnosticSource() {
  ImGui::SetNextWindowSize(ImVec2(860,520),ImGuiCond_FirstUseEver);
  if (ImGui::Begin(surfaceLabel(StudioText::BuildSource).c_str(),&showDiagnosticSource_)) {
    ImGui::TextWrapped("%s",diagnosticSourceTitle_.c_str());
    ImGui::TextWrapped("%s", surfaceText(StudioText::BuildSourceHint));
    ImGui::BeginChild("DiagnosticSourceLines",ImVec2(0,0),true,ImGuiWindowFlags_HorizontalScrollbar);
    const float height = ImGui::GetTextLineHeightWithSpacing();
    if (scrollDiagnosticSource_) {
      ImGui::SetScrollY(std::max(0.0f,(static_cast<float>(diagnosticSourceLine_)-4)*height));
      scrollDiagnosticSource_ = false;
    }
    ImGuiListClipper clipper; clipper.Begin(static_cast<int>(diagnosticSourceLines_.size()),height);
    while (clipper.Step()) for (int i=clipper.DisplayStart;i<clipper.DisplayEnd;++i) {
      const bool selected = static_cast<size_t>(i)+1 == diagnosticSourceLine_;
      if (selected) ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(1,0.7f,0.2f,1));
      ImGui::Text("%6d",i+1); ImGui::SameLine(); ImGui::TextUnformatted(diagnosticSourceLines_[static_cast<size_t>(i)].c_str());
      if (selected) ImGui::PopStyleColor();
    }
    ImGui::EndChild();
  }
  ImGui::End();
}

void StudioApp::renderVersionHistoryWindow() {
  ImGui::SetNextWindowSize(ImVec2(380.0f, 440.0f), ImGuiCond_FirstUseEver);
  if (ImGui::Begin(surfaceLabel(StudioText::HistoryPanel).c_str(), &showHistoryPanel_)) {
    ImGui::InputTextWithHint("##SnapLabel", "Nombre de snapshot...", snapshotLabelBuf_,
                             sizeof(snapshotLabelBuf_));
    ImGui::SameLine();
    if (ImGui::Button(surfaceLabel(StudioText::Save).c_str())) {
      captureSnapshot(snapshotLabelBuf_, SnapshotKind::Manual);
      snapshotLabelBuf_[0] = '\0';
    }

    ImGui::Separator();
    for (const auto& snap : snapshots_.snapshots()) {
      ImGui::PushID(snap.id.c_str());
      ImGui::Text("[%s] %s", snapshotKindLabel(snap.kind).c_str(), snap.label.c_str());
      ImGui::TextDisabled("%d vent. · %d widgets · %s", snap.windowCount, snap.widgetCount,
                          snapshots_.describeDiff(snap, project_).c_str());
      if (ImGui::SmallButton(surfaceLabel(StudioText::RestoreSnapshot).c_str())) {
        auto restored = snapshots_.revertToSnapshot(snap.id, project_);
        historyDirty_ = true; recoveryDirty_ = true;
        if (restored.has_value()) {
          commitChange(*restored);
          selectNode(project_.nodes.empty() ? "" : project_.nodes.front().id);
          persistSession();
          ImGui::PopID();
          break;
        }
      }
      ImGui::SameLine();
      if (ImGui::SmallButton(surfaceLabel(StudioText::Delete).c_str())) {
        snapshots_.deleteSnapshot(snap.id);
        historyDirty_ = true; recoveryDirty_ = true;
        persistSession();
        ImGui::PopID();
        break;
      }
      ImGui::Separator();
      ImGui::PopID();
    }
  }
  ImGui::End();
}

}  // namespace ImGuiStudio
