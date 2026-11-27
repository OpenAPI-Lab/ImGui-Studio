#pragma once

#include "imguistudio/services/studio_assistant_api.hpp"
#include <memory>

namespace ImGuiStudio {

struct AssistantVaultResult {
  std::string key, error;
};

class AssistantVaultCancellation {
 public:
  AssistantVaultCancellation();
  void cancel() const;
  void* nativeHandle() const { return handle_.get(); }

 private:
  std::shared_ptr<void> handle_;
};

bool assistantVaultAvailable();
std::wstring assistantCredentialTarget(const AssistantConnection& connection);
AssistantVaultResult loadAssistantKey(const AssistantConnection& connection,
                                    const AssistantVaultCancellation* cancellation = nullptr);
std::string storeAssistantKey(const AssistantConnection& connection,
                              const AssistantVaultCancellation* cancellation = nullptr);
std::string deleteAssistantKey(const AssistantConnection& connection,
                               const AssistantVaultCancellation* cancellation = nullptr);

} // namespace ImGuiStudio
