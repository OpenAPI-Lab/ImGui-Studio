#pragma once

#include "studio_assistant_http.hpp"
#include "imguistudio/services/studio_assistant_proposal.hpp"
#include "studio_assistant_vault.hpp"
#include "imguistudio/export/studio_codegen_native.hpp"
#include <array>
#include <future>

namespace ImGuiStudio {

class StudioAssistantPanel {
 public:
  ~StudioAssistantPanel() { cancel_.store(true); vaultCancellation_.cancel(); }
  void resetConversation();
  void poll();
  DocumentEditResult render(const StudioProject& project, const std::string& selected,
                            TargetLanguage language, bool& open, bool persistenceEnabled);
 private:
  enum class VaultAction { Load, Store, Remove };
  struct Preference { std::string endpoint, model; };
  std::array<Preference, std::size(kAssistantProviders)> preferences_;
  AssistantProvider provider_ = AssistantProvider::Ollama;
  bool loaded_ = false, listing_ = false;
  std::atomic<bool> cancel_{false};
  std::future<AssistantHttpResult> pending_;
  AssistantVaultCancellation vaultCancellation_;
  std::future<AssistantVaultResult> pendingVault_;
  VaultAction vaultAction_ = VaultAction::Load;
  std::string prompt_, key_, notice_, proposalJson_;
  std::vector<AssistantTurn> turns_;
  std::vector<std::string> models_;
  AssistantProposal proposal_;
  Json reviewed_;
  void loadSettings();
  void saveSettings();
  void startVaultAction(VaultAction action, const Preference& prefs);
  void start(const StudioProject& project, const std::string& selected, TargetLanguage language, bool models);
};

} // namespace ImGuiStudio
