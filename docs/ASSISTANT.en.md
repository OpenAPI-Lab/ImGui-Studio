# Assistant and providers

[Español](ASSISTANT.md)


Open **View → AI assistant** and choose a provider and model. **Detect models** queries the configured server. Successful discovery confirms that the server returned its model list. Sending a prompt and applying a proposal are separate actions.

## Connection and credentials

| Input | Behavior |
| --- | --- |
| Local Ollama or LM Studio | The editor suggests a loopback URL. Credentials are optional. |
| OpenAI, Anthropic, or Gemini | Requests require a key. The editor reads `OPENAI_API_KEY`, `ANTHROPIC_API_KEY`, or `GEMINI_API_KEY` for the corresponding official endpoint when the temporary field is empty. |
| Custom URL | The editor validates the base URL, adds the provider path, and requires HTTPS when sending a key to a remote server. |
| Temporary key | Stays in the session field and clears when you change provider or URL. |
| Saved key | An explicit save action uses the system credential store. Entries are associated with a provider and URL. |

`assistant_settings.json` stores provider, endpoints, and models in the state directory. Load a vault entry through **Keys stored in the system**. Linux needs libsecret and an active Secret Service session to use the vault. Windows uses WinCred. Keychain still needs validation on macOS.

## From prompt to design

Selecting **Send** transmits the prompt, recent context, and a summary of the design. Consider those fields when working on a private project. The editor shows the proposal before modifying the document. **Add to design** checks that the proposal still matches the current design and applies it as an undoable edit. Request a new proposal if the design has changed.

## Tests and diagnosis

CMake looks for libcurl 7.85 or later. With networking enabled and Python available, `AssistantLoopbackTest` uses a simulated local server and dummy credentials to check all five variants. `AssistantApiTest` covers formats, URL rules, and proposal validation. When changing a provider, maintain a mocked case and record external-service validation separately.

[Request formats](../include/imguistudio/services/studio_assistant_api.hpp) · [Proposal validation](../include/imguistudio/services/studio_assistant_proposal.hpp) · [Reporting policy](../SECURITY.en.md)
