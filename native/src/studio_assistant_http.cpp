#include "studio_assistant_http.hpp"

#if defined(STUDIO_HTTP_CURL)
#include <curl/curl.h>
#include <memory>
#endif

namespace ImGuiStudio {

#if defined(STUDIO_HTTP_CURL)
namespace {
struct CurlRuntime {
  CURLcode status = curl_global_init(CURL_GLOBAL_DEFAULT);
  ~CurlRuntime() { if (status == CURLE_OK) curl_global_cleanup(); }
};

struct ResponseBuffer { std::string text; bool tooLarge = false; };
size_t appendResponse(char* data, size_t size, size_t count, void* target) {
  auto& buffer = *static_cast<ResponseBuffer*>(target);
  constexpr size_t limit = 2 * 1024 * 1024;
  if (size && count > (limit - buffer.text.size()) / size) {
    buffer.tooLarge = true;
    return 0;
  }
  const auto bytes = size * count;
  try { buffer.text.append(data, bytes); }
  catch (const std::exception&) { return 0; }
  return bytes;
}
int checkCancellation(void* state, curl_off_t, curl_off_t, curl_off_t, curl_off_t) {
  return static_cast<const std::atomic<bool>*>(state)->load() ? 1 : 0;
}
AssistantHttpResult performStudioHttp(const AssistantRequest& request, const std::atomic<bool>& cancel,
                                      const char* protocols) {
  static const CurlRuntime runtime;
  if (cancel.load()) return {{}, "Consulta cancelada."};
  if (runtime.status != CURLE_OK) return {{}, "No se pudo inicializar libcurl."};
  std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(curl_easy_init(), &curl_easy_cleanup);
  if (!curl) return {{}, "No se pudo preparar la consulta."};
  std::unique_ptr<curl_slist, decltype(&curl_slist_free_all)> headers(nullptr, &curl_slist_free_all);
  for (const auto& header : request.headers) {
    auto* next = curl_slist_append(headers.get(), header.c_str());
    if (!next) return {{}, "No se pudieron preparar los encabezados de la consulta."};
    headers.release();
    headers.reset(next);
  }
  auto set = [&](CURLoption option, auto value) {
    return curl_easy_setopt(curl.get(), option, value) == CURLE_OK;
  };
  ResponseBuffer response;
  if (!set(CURLOPT_URL, request.url.c_str()) || !set(CURLOPT_HTTPHEADER, headers.get()) ||
      !set(CURLOPT_PROTOCOLS_STR, protocols) || !set(CURLOPT_FOLLOWLOCATION, 0L) ||
      !set(CURLOPT_SSL_VERIFYPEER, 1L) || !set(CURLOPT_SSL_VERIFYHOST, 2L) ||
      !set(CURLOPT_NOPROXY, "localhost,127.0.0.1,::1") || !set(CURLOPT_NOSIGNAL, 1L) ||
      !set(CURLOPT_CONNECTTIMEOUT_MS, 5000L) || !set(CURLOPT_TIMEOUT_MS, 120000L) ||
      !set(CURLOPT_NOPROGRESS, 0L) || !set(CURLOPT_XFERINFOFUNCTION, &checkCancellation) ||
      !set(CURLOPT_XFERINFODATA, const_cast<std::atomic<bool>*>(&cancel)) ||
      !set(CURLOPT_WRITEFUNCTION, &appendResponse) || !set(CURLOPT_WRITEDATA, &response))
    return {{}, "No se pudo configurar la consulta HTTP."};
  if (!request.body.empty() &&
      (!set(CURLOPT_POSTFIELDS, request.body.c_str()) ||
       !set(CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(request.body.size()))))
    return {{}, "No se pudo configurar el mensaje de la consulta."};
#if defined(_WIN32)
  if (!set(CURLOPT_SSL_OPTIONS, static_cast<long>(CURLSSLOPT_NATIVE_CA)))
    return {{}, "No se pudo configurar el almacen de certificados de Windows."};
#endif
  const auto result = curl_easy_perform(curl.get());
  if (cancel.load()) return {{}, "Consulta cancelada."};
  if (response.tooLarge) return {{}, "La respuesta del proveedor supera el limite de tamano."};
  if (result != CURLE_OK) return {{}, std::string("Error HTTP: ") + curl_easy_strerror(result)};
  long status = 0;
  curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &status);
  if (status < 200 || status >= 300)
    return {{}, "El proveedor respondio HTTP " + std::to_string(status) + "."};
  return {std::move(response.text), {}};
}
}
#endif

#if defined(STUDIO_ASSISTANT_CURL)
bool assistantHttpAvailable() { return true; }
AssistantHttpResult sendAssistantHttp(const AssistantRequest& request, const std::atomic<bool>& cancel) {
  return performStudioHttp(request, cancel, "HTTP,HTTPS");
}
#else
bool assistantHttpAvailable() { return false; }
AssistantHttpResult sendAssistantHttp(const AssistantRequest&, const std::atomic<bool>&) {
  return {{}, "Este editor se compilo sin libcurl."};
}
#endif

#if defined(STUDIO_CATALOG_CURL)
bool catalogHttpAvailable() { return true; }
AssistantHttpResult downloadCatalogFile(const std::string& url, const std::atomic<bool>& cancel) {
  return performStudioHttp(AssistantRequest{url, {}, {}}, cancel, "HTTPS");
}
#else
bool catalogHttpAvailable() { return false; }
AssistantHttpResult downloadCatalogFile(const std::string&, const std::atomic<bool>&) {
  return {{}, "Catalog downloads require libcurl."};
}
#endif

} // namespace ImGuiStudio
