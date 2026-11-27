# Assistant, credentials, and extensions

[Español](SERVICES.md)

Network transport, secrets, and applying edits have separate owners. The catalog and assistant can share transport while enforcing distinct data contracts.

## File contracts

| File | What it does | Why it exists and what to preserve |
| --- | --- | --- |
| [studio_assistant_api.hpp](../../include/imguistudio/services/studio_assistant_api.hpp) | Defines provider formats, endpoints, authentication, and response conversion. | Validates connections before building a request. A new provider needs request and response fixtures. |
| [studio_assistant_proposal.hpp](../../include/imguistudio/services/studio_assistant_proposal.hpp) | `readAssistantProposal` interprets the proposal. `applyAssistantProposal` proposes a tree edit. | Response content is validated as project data and the proposal must match its base document. |
| [studio_assistant_http.hpp](../../src/services/studio_assistant_http.hpp) | Declares the HTTP result and transport boundary. | Callers can handle cancellation and errors without using libcurl directly. |
| [studio_assistant_http.cpp](../../src/services/studio_assistant_http.cpp) | Performs the exchange, accumulates a bounded reply, and checks cancellation. | Connection failure must return as a result. Preserve protocol restrictions and TLS verification. |
| [studio_assistant_panel.hpp](../../src/services/studio_assistant_panel.hpp) | Declares preferences, requests, conversation, and UI state. | The panel keeps temporary credentials separate from serialized settings. |
| [studio_assistant_panel.cpp](../../src/services/studio_assistant_panel.cpp) | Loads provider state and settings, starts work, polls, and reviews proposals. | The frame loop remains responsive during work. Applying requires user action and a matching base design. |
| [studio_assistant_vault.hpp](../../src/services/studio_assistant_vault.hpp) | Declares vault results, availability, and cancellation. | The UI can handle a result without knowing WinCred, Secret Service, or Keychain details. |
| [studio_assistant_vault.cpp](../../src/services/studio_assistant_vault.cpp) | Implements platform-specific credential storage and loading. | An entry is keyed by provider and base URL. Project files and generated source are outside the vault. |
| [studio_extensions.hpp](../../src/services/studio_extensions.hpp) | Declares `StudioExtensionCatalog` and the installation interface used by the UI. | Palette availability depends on the catalog. Design nodes retain their data. |
| [studio_extensions.cpp](../../src/services/studio_extensions.cpp) | Fetches and compares pinned imgui-knobs files and persists installation metadata. | Enables the entry after checking bytes and committing metadata. Rendering uses the compiled adapter. |

### C++ example

This uses a local sample reply. It shows that review is tied to the captured design and that a later edit invalidates the proposal.

```cpp
#include "imguistudio/services/studio_assistant_proposal.hpp"
#include <cassert>

int main() {
    using namespace ImGuiStudio;
    StudioProject project = createDefaultProject();
    const auto reviewed = projectToJson(project);
    const auto proposal = readAssistantProposal(
        "```imgui-json\n{\"type\":\"text\",\"label\":\"Status\"}\n```");
    assert(proposal.error.empty());
    assert(proposal.windows.size() == 1);

    const auto accepted = applyAssistantProposal(project, proposal, reviewed);
    assert(accepted);
    project.name = "Changed after review";
    const auto rejected = applyAssistantProposal(project, proposal, reviewed);
    assert(!rejected && !rejected.error.empty());
}
```

## Verification

Use `AssistantApiTest`, `AssistantLoopbackTest`, and `NativeExtensionCatalogTest`. Provider changes should work against the local test server and cover rejected replies. Extension changes must retain provenance and license in exports that include them. [Assistant](../ASSISTANT.en.md) · [Library](../WIDGET_LIBRARY.en.md)

