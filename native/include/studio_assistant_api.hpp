#pragma once

#include "studio_serialization.hpp"
#include <string_view>

namespace ImGuiStudio {

enum class AssistantProvider { Ollama, LmStudio, OpenAI, Anthropic, Gemini };
struct AssistantProviderInfo {
  AssistantProvider id;
  const char* key;
  const char* label;
  const char* endpoint;
  const char* environmentKey;
};
inline constexpr AssistantProviderInfo kAssistantProviders[] = {
  {AssistantProvider::Ollama,"ollama","Ollama","http://localhost:11434",""},
  {AssistantProvider::LmStudio,"lmstudio","LM Studio","http://localhost:1234",""},
  {AssistantProvider::OpenAI,"openai","OpenAI","https://api.openai.com","OPENAI_API_KEY"},
  {AssistantProvider::Anthropic,"anthropic","Anthropic","https://api.anthropic.com","ANTHROPIC_API_KEY"},
  {AssistantProvider::Gemini,"gemini","Google Gemini","https://generativelanguage.googleapis.com","GEMINI_API_KEY"}
};
inline const AssistantProviderInfo& assistantProviderInfo(AssistantProvider provider) {
  return kAssistantProviders[static_cast<size_t>(provider)];
}

struct AssistantTurn { bool fromAssistant; std::string text; };
struct AssistantConnection { AssistantProvider provider; std::string endpoint, model, key; };
struct AssistantRequest {
  std::string url, body;
  std::vector<std::string> headers;
};

inline bool assistantLoopback(std::string_view host) {
  for (const std::string_view name : {"localhost","127.0.0.1","[::1]"}) {
    if (host == name) return true;
    if (host.size() <= name.size() + 1 || host.substr(0, name.size()) != name || host[name.size()] != ':') continue;
    const auto port = host.substr(name.size() + 1);
    if (std::all_of(port.begin(), port.end(), [](char c) { return c >= '0' && c <= '9'; })) return true;
  }
  return false;
}

inline std::string assistantEndpoint(const AssistantConnection& connection) {
  auto base = connection.endpoint.empty() ? std::string(assistantProviderInfo(connection.provider).endpoint) : connection.endpoint;
  const bool secure = base.rfind("https://",0) == 0;
  const bool http = base.rfind("http://",0) == 0;
  const auto authorityStart = secure ? 8u : 7u;
  if ((!secure && !http) || base.find_first_of("?#\\") != std::string::npos ||
      std::any_of(base.begin(),base.end(),[](unsigned char c) { return c <= 32 || c == 127; }))
    throw std::runtime_error("Usa una URL base HTTP(S) sin consultas ni fragmentos.");
  const auto authority = std::string_view(base).substr(authorityStart,
    base.find('/',authorityStart) - authorityStart);
  if (authority.empty() || authority.find('@') != std::string_view::npos)
    throw std::runtime_error("La URL necesita un servidor y admite las credenciales en el campo separado.");
  if (!connection.key.empty() && !secure && !assistantLoopback(authority))
    throw std::runtime_error("Usa HTTPS para enviar una clave a un servidor remoto.");
  while (base.size() > authorityStart && base.back() == '/') base.pop_back();
  const std::string_view suffix = connection.provider == AssistantProvider::Gemini ? "/v1beta" : "/v1";
  if (connection.provider != AssistantProvider::Ollama && base.size() >= authorityStart + suffix.size() &&
      std::string_view(base).substr(base.size() - suffix.size()) == suffix)
    base.resize(base.size() - suffix.size());
  return base;
}

inline void validateAssistantKey(std::string_view key) {
  if (std::any_of(key.begin(), key.end(), [](unsigned char c) { return c < 32 || c == 127; }))
    throw std::runtime_error("La clave contiene caracteres de control.");
}

inline AssistantRequest makeAssistantRequest(const AssistantConnection& connection, bool models,
    const std::vector<AssistantTurn>& turns = {}, const std::string& instruction = {}) {
  const auto provider = connection.provider;
  const auto base = assistantEndpoint(connection);
  validateAssistantKey(connection.key);
  if (provider != AssistantProvider::Ollama && provider != AssistantProvider::LmStudio && connection.key.empty())
    throw std::runtime_error("Configura la clave del proveedor.");
  AssistantRequest request;
  request.headers.push_back("Accept: application/json");
  if (!connection.key.empty()) {
    const char* prefix = provider == AssistantProvider::Anthropic ? "x-api-key: " :
      provider == AssistantProvider::Gemini ? "x-goog-api-key: " : "Authorization: Bearer ";
    request.headers.push_back(prefix + connection.key);
  }
  if (provider == AssistantProvider::Anthropic) request.headers.push_back("anthropic-version: 2023-06-01");
  if (models) {
    request.url = base + (provider == AssistantProvider::Ollama ? "/api/tags" :
      provider == AssistantProvider::Gemini ? "/v1beta/models" : "/v1/models");
    return request;
  }
  if (turns.empty() || connection.model.empty()) throw std::runtime_error("Elige un modelo y escribe una consulta.");
  request.headers.push_back("Content-Type: application/json");
  Json messages = Json::array(), payload;
  if (provider == AssistantProvider::Gemini) {
    auto model = connection.model;
    if (model.rfind("models/",0) == 0) model.erase(0,7);
    if (model.empty() || model.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.") != std::string::npos)
      throw std::runtime_error("Nombre de modelo Gemini invalido.");
    for (const auto& turn : turns)
      messages.push_back({{"role",turn.fromAssistant ? "model" : "user"},{"parts",Json::array({{{"text",turn.text}}})}});
    payload = {{"contents",std::move(messages)},{"systemInstruction",{{"parts",Json::array({{{"text",instruction}}})}}}};
    request.url = base + "/v1beta/models/" + model + ":generateContent";
  } else {
    if (provider != AssistantProvider::Anthropic && !instruction.empty())
      messages.push_back({{"role","system"},{"content",instruction}});
    for (const auto& turn : turns)
      messages.push_back({{"role",turn.fromAssistant ? "assistant" : "user"},{"content",turn.text}});
    payload = {{"model",connection.model},{"messages",std::move(messages)},{"stream",false}};
    if (provider == AssistantProvider::Anthropic) {
      request.url = base + "/v1/messages";
      payload["max_tokens"] = 2048;
      if (!instruction.empty()) payload["system"] = instruction;
    } else request.url = base + (provider == AssistantProvider::Ollama ? "/api/chat" : "/v1/chat/completions");
  }
  request.body = payload.dump();
  return request;
}

inline std::vector<std::string> assistantModels(AssistantProvider provider, const Json& reply) {
  const auto& rows = reply.at(provider == AssistantProvider::Ollama || provider == AssistantProvider::Gemini ? "models" : "data");
  if (!rows.is_array()) throw std::runtime_error("La respuesta no contiene una lista de modelos.");
  std::vector<std::string> models;
  for (const auto& row : rows) {
    if (!row.is_object()) continue;
    if (provider == AssistantProvider::Gemini && row.contains("supportedGenerationMethods") &&
        row.at("supportedGenerationMethods").is_array()) {
      const auto& methods = row.at("supportedGenerationMethods");
      if (std::find(methods.begin(), methods.end(), Json("generateContent")) == methods.end()) continue;
    }
    auto name = row.value(provider == AssistantProvider::Ollama || provider == AssistantProvider::Gemini ? "name" : "id",std::string{});
    if (provider == AssistantProvider::Gemini && name.rfind("models/",0) == 0) name.erase(0,7);
    if (!name.empty()) models.push_back(std::move(name));
  }
  std::sort(models.begin(),models.end());
  models.erase(std::unique(models.begin(),models.end()),models.end());
  return models;
}

inline std::string assistantText(AssistantProvider provider, const Json& reply) {
  if (provider == AssistantProvider::Ollama) return reply.at("message").at("content").get<std::string>();
  if (provider == AssistantProvider::OpenAI || provider == AssistantProvider::LmStudio)
    return reply.at("choices").at(0).at("message").at("content").get<std::string>();
  const auto& parts = provider == AssistantProvider::Anthropic ? reply.at("content") :
    reply.at("candidates").at(0).at("content").at("parts");
  std::string text;
  for (const auto& part : parts)
    if (part.is_object() && part.contains("text") && part.at("text").is_string() && !part.value("thought",false))
      text += part.at("text").get<std::string>();
  return text;
}

} // namespace ImGuiStudio
