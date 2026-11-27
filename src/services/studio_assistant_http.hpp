#pragma once

#include "imguistudio/services/studio_assistant_api.hpp"
#include <atomic>

namespace ImGuiStudio {

struct AssistantHttpResult {
  std::string body, error;
};

bool assistantHttpAvailable();
AssistantHttpResult sendAssistantHttp(const AssistantRequest& request, const std::atomic<bool>& cancel);
bool catalogHttpAvailable();
AssistantHttpResult downloadCatalogFile(const std::string& url, const std::atomic<bool>& cancel);

} // namespace ImGuiStudio
