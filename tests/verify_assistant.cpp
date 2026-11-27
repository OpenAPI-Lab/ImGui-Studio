#include "services/studio_assistant_http.hpp"
#include "imguistudio/services/studio_assistant_proposal.hpp"
#include "services/studio_assistant_panel.hpp"
#include "services/studio_assistant_vault.hpp"
#include "imgui.h"
#include <cassert>
#include <chrono>
#include <future>
#include <iostream>
#include <thread>

using namespace ImGuiStudio;

int main(int argc, char** argv) {
  if (argc == 2 && std::string_view(argv[1]) == "--vault-test") {
    if (!assistantVaultAvailable()) return 1;
    const auto endpoint = "https://verification.invalid/" + nextId("assistant_vault_test");
    AssistantConnection probe{AssistantProvider::OpenAI,endpoint,{},"synthetic-probe"};
    if (loadAssistantKey(probe).error.empty()) return 1;
    if (const auto error = storeAssistantKey(probe); !error.empty()) {
      std::cerr << error << '\n';
      return 1;
    }
    const auto loaded = loadAssistantKey(probe);
    const auto other = loadAssistantKey({AssistantProvider::OpenAI,endpoint + "/other",{},{}});
    auto replacement = probe;
    replacement.key = "synthetic-replacement";
    const auto updateError = storeAssistantKey(replacement);
    const auto updated = loadAssistantKey(probe);
    AssistantVaultCancellation cancellation;
    const bool cancellable = cancellation.nativeHandle() != nullptr;
    AssistantVaultResult cancelled;
    if (cancellable) {
      cancellation.cancel();
      cancelled = loadAssistantKey(probe, &cancellation);
    }
    const auto removed = deleteAssistantKey(probe);
    const auto missing = loadAssistantKey(probe);
    if (!removed.empty()) { std::cerr << removed << '\n'; return 1; }
    assert(loaded.error.empty() && loaded.key == probe.key);
    assert(updateError.empty() && updated.error.empty() && updated.key == replacement.key);
    assert(!other.error.empty() && !missing.error.empty());
    assert(!cancellable || !cancelled.error.empty());
    std::cout << "System credential vault write, read, replacement, endpoint isolation and cleanup verified.\n";
    return 0;
  }
  if (argc == 1) {
    AssistantConnection invalid{AssistantProvider::OpenAI,"http://example.invalid","test","mock-key"};
    const auto rejects = [](const AssistantConnection& connection) {
      try { makeAssistantRequest(connection, true); }
      catch (const std::runtime_error&) { return true; }
      return false;
    };
    assert(rejects(invalid));
    invalid.endpoint = "https://name:password@example.invalid";
    assert(rejects(invalid));
    invalid.endpoint = "https://example.invalid/path?token=value";
    assert(rejects(invalid));
    invalid.endpoint.clear();
    invalid.key = "a\r\nInjected: b";
    assert(rejects(invalid));
    const auto defaults = makeAssistantRequest({AssistantProvider::Ollama,{}, {}, {}}, true);
    assert(defaults.url == "http://localhost:11434/api/tags" && defaults.body.empty());
    const auto target = assistantCredentialTarget({AssistantProvider::OpenAI, "https://example.invalid/Team", {}, {}});
    assert(target == assistantCredentialTarget({AssistantProvider::OpenAI, "https://example.invalid/Team/v1/", "other-model", {}}));
    assert(target != assistantCredentialTarget({AssistantProvider::OpenAI, "https://example.invalid/team", {}, {}}));
    assert(target != assistantCredentialTarget({AssistantProvider::LmStudio, "https://example.invalid/Team", {}, {}}));
    if (!assistantHttpAvailable()) {
      std::atomic<bool> cancelled{false};
      assert(!sendAssistantHttp(defaults, cancelled).error.empty());
    }
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = ImVec2(1280, 800);
    unsigned char* pixels = nullptr;
    int width = 0, height = 0;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    {
      StudioAssistantPanel panel;
      const auto project = createDefaultProject("Panel");
      const auto snapshot = projectToJson(project);
      bool open = true;
      ImGui::NewFrame();
      const auto edit = panel.render(project, {}, TargetLanguage::Cpp, open, false);
      ImGui::Render();
      assert(!edit && open && projectToJson(project) == snapshot);
    }
    ImGui::DestroyContext();
    std::cout << "Assistant endpoint and credential handling verified.\n";
    return 0;
  }

  assert(argc == 2 && assistantHttpAvailable());
  const std::string server = argv[1];
  const std::vector<AssistantTurn> turns = {{false,"Primero"},{true,"Respuesta"},{false,"Consulta final"}};
  std::atomic<bool> cancel{false};
  for (const auto& provider : kAssistantProviders) {
    AssistantConnection connection{provider.id,server + "/" + provider.key,"fixture","mock-key"};
    if (provider.id == AssistantProvider::LmStudio) connection.endpoint += "/v1/";
    const auto found = sendAssistantHttp(makeAssistantRequest(connection,true), cancel);
    assert(found.error.empty());
    assert(assistantModels(provider.id,Json::parse(found.body)) == std::vector<std::string>{"fixture"});
    const auto result = sendAssistantHttp(makeAssistantRequest(connection,false,turns,"Instruccion"), cancel);
    assert(result.error.empty());
    const auto text = assistantText(provider.id,Json::parse(result.body));
    const auto proposal = readAssistantProposal(text);
    assert(proposal.error.empty() && proposal.windows.size() == 1);
    auto project = createDefaultProject("Test");
    const auto before = projectToJson(project);
    const auto applied = applyAssistantProposal(project,proposal,before);
    assert(applied && applied.project->nodes.size() == project.nodes.size() + 1);
    assert(projectToJson(project) == before);
    std::cout << provider.label << " models, chat and proposal verified.\n";
  }
  const auto endpoint = [&](const char* path) {
    return makeAssistantRequest({AssistantProvider::OpenAI,server + "/" + path,"fixture","mock-key"},true);
  };
  const auto denied = sendAssistantHttp(endpoint("denied"),cancel);
  assert(denied.body.empty() && denied.error.find("401") != std::string::npos);
  const auto redirect = sendAssistantHttp(endpoint("redirect"),cancel);
  assert(redirect.body.empty() && redirect.error.find("307") != std::string::npos);
  const auto large = sendAssistantHttp(endpoint("large"),cancel);
  assert(large.body.empty() && large.error.find("tamano") != std::string::npos);
  const auto slow = endpoint("slow");
  auto pending = std::async(std::launch::async,[&] { return sendAssistantHttp(slow,cancel); });
  std::this_thread::sleep_for(std::chrono::milliseconds(180));
  cancel.store(true);
  assert(pending.wait_for(std::chrono::seconds(4)) == std::future_status::ready);
  assert(pending.get().error == "Consulta cancelada.");
  std::cout << "Assistant HTTP errors, redirects, response limit and cancellation verified.\n";
}
