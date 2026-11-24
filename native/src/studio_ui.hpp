#pragma once

#include "studio_codegen.hpp"
#include "studio_cpp_export.hpp"
#include "studio_build.hpp"
#include "studio_build_profiles.hpp"
#include "studio_snapshots.hpp"
#include "studio_types.hpp"
#include "studio_project_io.hpp"
#include "studio_locale.hpp"
#include "studio_widget_library.hpp"
#include "studio_session_store.hpp"
#include "studio_preview.hpp"
#include "studio_canvas.hpp"
#include "studio_assistant_panel.hpp"
#include "studio_extensions.hpp"

#include <string>
#include <vector>

namespace ImGuiStudio {

class StudioApp {
 public:
  explicit StudioApp(bool persistenceEnabled = true, std::optional<StudioProject> initialProject = std::nullopt,
                     NativeImageCache* images = nullptr);
  void renderFrame();
  bool requestClose();
  bool shouldClose() const { return quitRequested_; }
  const StudioProject& project() const { return project_; }
  const std::string& selectedNodeId() const {
    static const std::string empty;
    return selectedIds_.empty() ? empty : selectedIds_.back();
  }
  const std::vector<std::string>& selectedNodeIds() const { return selectedIds_; }
  size_t undoCount() const { return past_.size(); }
  const PreviewReport& designReport() const { return designRenderer_.report(); }
  const std::vector<PreviewItem>& editTargets() const { return hierarchyItems_; }

 private:
  StudioProject project_;
  std::vector<StudioProject> past_;
  std::vector<StudioProject> future_;
  SnapshotManager snapshots_;
  std::optional<StudioProject> previewProject_;
  NativePreview previewRenderer_;
  NativePreview designRenderer_;
  PlotSeriesEditor plotEditor_;
  NativeCanvas canvasController_;
  std::vector<PreviewItem> hierarchyItems_;
  ImVec2 canvasOrigin_{};
  ImVec2 canvasOffset_{};
  int canvasZoomPercent_ = 100;
  struct CanvasPan { ImVec2 pointer, offset; };
  std::optional<CanvasPan> canvasPan_;
  std::filesystem::path projectPath_;
  bool welcomeOpen_ = false;
  StudioLocale locale_;
  std::optional<size_t> pendingLanguage_;
  NativeImageCache* images_ = nullptr;
  StudioWidgetLibrary widgetLibrary_;
  std::shared_ptr<StudioExtensionCatalog> extensions_ = std::make_shared<StudioExtensionCatalog>();
  bool showWidgetLibrary_ = false;
  bool exportLibraryWidget_ = false;
  size_t widgetBaseIndex_ = 0;
  std::string librarySelection_, libraryName_, libraryImportPath_, libraryExportPath_, libraryStatus_;
  struct WidgetDraft {
    std::optional<LibraryWidget> original;
    std::string name, initialName, canvasId, wrapperId, error;
    enum class Action { None, Save, SaveCopy, Discard, KeepEditing };
    Action action = Action::None;
    bool blank = false;
    bool confirmDiscard = false;
  };
  std::optional<WidgetDraft> widgetDraft_;
  std::unique_ptr<StudioApp> widgetEditor_;
  bool closeAfterWidgetDraft_ = false;
  std::vector<std::filesystem::path> recentProjects_;
  std::string savedState_;
  std::string fileStatus_;
  std::string fileError_;
  char projectPathBuf_[4096] = "";
  std::string imagePath_, imageEditNode_, imageError_;
  enum class PendingAction { None, New, Open, Exit };
  enum class FileDialog { None, Open, SaveAs, ExportCpp, ExportAndroid, ExportMacOS, ExportIOS,
                          ExportPython, ExportCSharp, ExportJava, ExportC, ExportRust };
  PendingAction pendingAction_ = PendingAction::None;
  FileDialog fileDialog_ = FileDialog::None;
  CppRenderer exportCppRenderer_ = CppRenderer::OpenGL;
  std::array<std::string, 3> cppPackPaths_;
  bool openFilePopup_ = false;
  bool openUnsavedPopup_ = false;
  bool quitRequested_ = false;
  bool interactiveEdit_ = false;
  bool canvasNeedsSync_ = true;
  std::map<std::string, std::array<float, 4>> canvasGeometry_;
  bool confirmOverwrite_ = false;
  bool persistenceEnabled_ = true;
  bool recoveryDirty_ = false;
  bool historyDirty_ = false;
  int64_t lastPersistAttemptMs_ = 0;
  RecoverySession recoverySession_;
  std::optional<ProjectHistoryFile> historyFile_;
  std::vector<RecoveryCandidate> recoveryCandidates_;
  bool openRecoveryPopup_ = false;
  std::optional<size_t> discardRecoveryIndex_;
  std::string recoveryOriginPath_;

  std::vector<std::string> selectedIds_;
  bool previewMode_ = false;
  bool lastPreviewMode_ = false;
  bool snapToGrid_ = true;
  bool showCodePanel_ = true;
  bool showHistoryPanel_ = false;
  bool showBuildPanel_ = false;
  bool showAssistant_ = false;
  StudioAssistantPanel assistant_;
  NativeBuild nativeBuild_;
  BuildProfileStore buildProfiles_;
  std::string buildProfileStatus_;
  std::string toolchainStatus_;
  bool showDiagnosticSource_ = false;
  std::vector<std::string> diagnosticSourceLines_;
  std::string diagnosticSourceTitle_;
  size_t diagnosticSourceLine_ = 0;
  bool scrollDiagnosticSource_ = false;
  CppEmitMode cppMode_ = CppEmitMode::Header;
  TargetLanguage codeLanguage_ = TargetLanguage::Cpp;
  std::optional<GeneratedCpp> codePreview_;
  int64_t lastEditMs_ = 0;
  bool pendingAutoSave_ = false;

  char snapshotLabelBuf_[128] = "";

  void commitChange(const StudioProject& next, bool recordUndo = true);
  void commitInteractiveChange(const StudioProject& next);
  bool hasUnsavedChanges() const;
  void requestAction(PendingAction action);
  void continuePendingAction();
  bool saveProject(bool saveAs = false);
  bool openProject(const std::filesystem::path& path);
  void rememberProject(const std::filesystem::path& path);
  void renderWelcome();
  void renderLanguageChoices();
  const char* tr(StudioText key) const { return locale_.text(key); }
  std::string label(StudioText key, float maxWidth = 0.0f) const;
  void renderProjectDialogs();
  void captureSnapshot(const std::string& label, SnapshotKind kind, bool skipSame = false);
  void bindHistory(const std::filesystem::path& path, bool loadExisting);
  void persistSession();
  void finishSession(bool discard = false);
  void resetRecoverySession();
  void renderRecoveryDialog();
  void undo();
  void redo();
  void selectNode(const std::string& id);
  void addWidgetToSelection(WidgetType type, const char* preset = nullptr);
  void insertWidgetIntoSelection(ImGuiNode node);
  void renderWidgetLibrary();
  void renderExtensionCatalog();
  void openWidgetDraft(std::string name, const ImGuiNode* base, const LibraryWidget* original = nullptr);
  void renderWidgetDraftToolbar();
  void handleWidgetDraftAction();
  void renderTopMenuBar();
  void renderWidgetPalette(float x, float y, float width, float height);
  void renderHierarchy(float x, float y, float width, float height);
  void renderCenterCanvas(float x, float y, float w, float h);
  void updateCanvasView(ImVec2 minimum, ImVec2 maximum);
  float canvasScale() const { return static_cast<float>(canvasZoomPercent_) / 100.0f; }
  void setCanvasZoom(int percent, ImVec2 anchor);
  void resetCanvasView();
  void renderRightInspector(float x, float y, float w, float h);
  bool renderImageProperties(ImGuiNode& node, const StudioProject& edited);
  void renderCodeDrawer(float x, float y, float w, float h);
  void renderVersionHistoryWindow();
  void renderBuildWindow();
  void renderDiagnosticSource();
  void renderNodeTree(const std::vector<ImGuiNode>& nodes, const std::string& parent = {}, int depth = 0);
};

}  // namespace ImGuiStudio
