#include "studio_assistant_vault.hpp"
#include "imguistudio/export/studio_codegen_native.hpp"

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <wincred.h>
#elif defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#include <Security/Security.h>
#include <type_traits>
#elif defined(STUDIO_ASSISTANT_LIBSECRET)
#include <libsecret/secret.h>
#endif

namespace ImGuiStudio {
namespace {
constexpr const char* kMissingKey = "No hay una clave guardada para este proveedor y URL.";

std::string credentialId(const AssistantConnection& connection) {
  return std::string("ImGuiStudio/Asistente/") + assistantProviderInfo(connection.provider).key +
    "/" + CppCodegen::hexId(assistantEndpoint(connection));
}
}

std::wstring assistantCredentialTarget(const AssistantConnection& connection) {
  // WinCred compares names case-insensitively; encode endpoint bytes so
  // distinct paths such as /Team and /team do not share an entry.
  const auto name = credentialId(connection);
  return {name.begin(), name.end()};
}

AssistantVaultCancellation::AssistantVaultCancellation() {
#if defined(STUDIO_ASSISTANT_LIBSECRET)
  handle_ = std::shared_ptr<void>(g_cancellable_new(), g_object_unref);
#endif
}

void AssistantVaultCancellation::cancel() const {
#if defined(STUDIO_ASSISTANT_LIBSECRET)
  g_cancellable_cancel(static_cast<GCancellable*>(nativeHandle()));
#endif
}

#if defined(_WIN32)
namespace {
std::string vaultError(DWORD error) {
  if (error == ERROR_NOT_FOUND) return kMissingKey;
  return "El Administrador de credenciales devolvio el error " + std::to_string(error) + ".";
}
std::wstring checkedTarget(const AssistantConnection& connection) {
  auto target = assistantCredentialTarget(connection);
  if (target.size() > CRED_MAX_GENERIC_TARGET_NAME_LENGTH)
    throw std::runtime_error("La URL es demasiado larga para el almacen de credenciales.");
  return target;
}
}

bool assistantVaultAvailable() { return true; }

AssistantVaultResult loadAssistantKey(const AssistantConnection& connection, const AssistantVaultCancellation*) {
  try {
    const auto target = checkedTarget(connection);
    PCREDENTIALW raw = nullptr;
    if (!CredReadW(target.c_str(), CRED_TYPE_GENERIC, 0, &raw)) return {{}, vaultError(GetLastError())};
    const auto release = [](PCREDENTIALW entry) {
      if (entry->CredentialBlob) SecureZeroMemory(entry->CredentialBlob, entry->CredentialBlobSize);
      CredFree(entry);
    };
    std::unique_ptr<CREDENTIALW, decltype(release)> entry(raw, release);
    if (!entry->CredentialBlob || !entry->CredentialBlobSize) return {{}, "La clave guardada esta vacia."};
    std::string key(reinterpret_cast<const char*>(entry->CredentialBlob), entry->CredentialBlobSize);
    validateAssistantKey(key);
    return {std::move(key), {}};
  } catch (const std::exception& error) { return {{}, error.what()}; }
}

std::string storeAssistantKey(const AssistantConnection& connection, const AssistantVaultCancellation*) {
  try {
    auto target = checkedTarget(connection);
    validateAssistantKey(connection.key);
    if (connection.key.empty() || connection.key.size() > CRED_MAX_CREDENTIAL_BLOB_SIZE)
      return "La clave debe tener un tamano valido para el almacen de Windows.";
    CREDENTIALW entry{};
    entry.Type = CRED_TYPE_GENERIC;
    entry.TargetName = target.data();
    entry.Persist = CRED_PERSIST_LOCAL_MACHINE;
    entry.CredentialBlobSize = static_cast<DWORD>(connection.key.size());
    entry.CredentialBlob = reinterpret_cast<LPBYTE>(const_cast<char*>(connection.key.data()));
    return CredWriteW(&entry, 0) ? std::string{} : vaultError(GetLastError());
  } catch (const std::exception& error) { return error.what(); }
}

std::string deleteAssistantKey(const AssistantConnection& connection, const AssistantVaultCancellation*) {
  try {
    const auto target = checkedTarget(connection);
    return CredDeleteW(target.c_str(), CRED_TYPE_GENERIC, 0) ? std::string{} : vaultError(GetLastError());
  } catch (const std::exception& error) { return error.what(); }
}
#elif defined(__APPLE__)
namespace {
template<class T>
auto ownCF(T value) {
  return std::unique_ptr<std::remove_pointer_t<T>, decltype(&CFRelease)>(value, CFRelease);
}

std::string vaultError(OSStatus status) {
  if (status == errSecItemNotFound) return kMissingKey;
  if (status == errSecUserCanceled) return "Operacion del llavero cancelada.";
  if (status == errSecInteractionNotAllowed) return "Desbloquea el llavero de macOS y vuelve a intentarlo.";
  return "El llavero de macOS devolvio el error " + std::to_string(status) + ".";
}

auto credentialQuery(const AssistantConnection& connection) {
  const auto accountName = credentialId(connection);
  const auto account = ownCF(CFStringCreateWithCString(kCFAllocatorDefault, accountName.c_str(), kCFStringEncodingASCII));
  auto query = ownCF(CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
                                               &kCFTypeDictionaryValueCallBacks));
  if (!account || !query) throw std::runtime_error("No se pudo preparar la consulta del llavero de macOS.");
  CFDictionarySetValue(query.get(), kSecClass, kSecClassGenericPassword);
  CFDictionarySetValue(query.get(), kSecAttrService, CFSTR("org.imguistudio.desktop.assistant"));
  CFDictionarySetValue(query.get(), kSecAttrAccount, account.get());
  return query;
}
}

bool assistantVaultAvailable() { return true; }

AssistantVaultResult loadAssistantKey(const AssistantConnection& connection, const AssistantVaultCancellation*) {
  try {
    const auto query = credentialQuery(connection);
    CFDictionarySetValue(query.get(), kSecReturnData, kCFBooleanTrue);
    CFDictionarySetValue(query.get(), kSecMatchLimit, kSecMatchLimitOne);
    CFTypeRef raw = nullptr;
    const auto status = SecItemCopyMatching(query.get(), &raw);
    const auto result = ownCF(raw);
    if (status != errSecSuccess) return {{}, vaultError(status)};
    if (!result || CFGetTypeID(result.get()) != CFDataGetTypeID())
      return {{}, "El llavero de macOS devolvio un valor inesperado."};
    const auto data = static_cast<CFDataRef>(result.get());
    const auto length = CFDataGetLength(data);
    if (length == 0) return {{}, "La clave guardada esta vacia."};
    std::string key(reinterpret_cast<const char*>(CFDataGetBytePtr(data)), static_cast<size_t>(length));
    validateAssistantKey(key);
    return {std::move(key), {}};
  } catch (const std::exception& error) { return {{}, error.what()}; }
}

std::string storeAssistantKey(const AssistantConnection& connection, const AssistantVaultCancellation*) {
  try {
    validateAssistantKey(connection.key);
    if (connection.key.empty()) return "Escribe una clave antes de guardarla.";
    const auto query = credentialQuery(connection);
    const auto data = ownCF(CFDataCreate(kCFAllocatorDefault, reinterpret_cast<const UInt8*>(connection.key.data()),
                                        static_cast<CFIndex>(connection.key.size())));
    if (!data) return "No se pudo preparar la clave para el llavero de macOS.";
    CFDictionarySetValue(query.get(), kSecValueData, data.get());
    auto status = SecItemAdd(query.get(), nullptr);
    if (status == errSecDuplicateItem) {
      CFDictionaryRemoveValue(query.get(), kSecValueData);
      const void* keys[] = {kSecValueData};
      const void* values[] = {data.get()};
      const auto updates = ownCF(CFDictionaryCreate(kCFAllocatorDefault, keys, values, 1,
                                                    &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));
      if (!updates) return "No se pudo preparar la actualizacion del llavero de macOS.";
      status = SecItemUpdate(query.get(), updates.get());
    }
    return status == errSecSuccess ? std::string{} : vaultError(status);
  } catch (const std::exception& error) { return error.what(); }
}

std::string deleteAssistantKey(const AssistantConnection& connection, const AssistantVaultCancellation*) {
  try {
    const auto query = credentialQuery(connection);
    const auto status = SecItemDelete(query.get());
    return status == errSecSuccess ? std::string{} : vaultError(status);
  } catch (const std::exception& error) { return error.what(); }
}
#elif defined(STUDIO_ASSISTANT_LIBSECRET)
namespace {
constexpr SecretSchema kCredentialSchema = {
  "org.imguistudio.AssistantCredential", SECRET_SCHEMA_NONE, {{"connection", SECRET_SCHEMA_ATTRIBUTE_STRING}},
  0, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr
};

std::string vaultError(GError* raw) {
  const std::unique_ptr<GError, decltype(&g_error_free)> error(raw, g_error_free);
  if (!error) return kMissingKey;
  if (g_error_matches(error.get(), G_IO_ERROR, G_IO_ERROR_CANCELLED)) return "Operacion del llavero cancelada.";
  if (g_error_matches(error.get(), SECRET_ERROR, SECRET_ERROR_IS_LOCKED))
    return "El llavero de Linux esta bloqueado.";
  return "No se pudo acceder al llavero de Linux. Comprueba que haya un servicio de secretos activo y desbloqueado.";
}

GCancellable* nativeCancellation(const AssistantVaultCancellation* cancellation) {
  return cancellation ? static_cast<GCancellable*>(cancellation->nativeHandle()) : nullptr;
}
}

bool assistantVaultAvailable() { return true; }

AssistantVaultResult loadAssistantKey(const AssistantConnection& connection, const AssistantVaultCancellation* cancellation) {
  try {
    const auto target = credentialId(connection);
    GError* error = nullptr;
    const std::unique_ptr<SecretValue, decltype(&secret_value_unref)> value(
      secret_password_lookup_binary_sync(&kCredentialSchema, nativeCancellation(cancellation), &error,
                                         "connection", target.c_str(), nullptr),
      secret_value_unref);
    if (!value) return {{}, vaultError(error)};
    gsize length = 0;
    const auto* data = secret_value_get(value.get(), &length);
    if (!data || !length) return {{}, "La clave guardada esta vacia."};
    std::string key(data, length);
    validateAssistantKey(key);
    return {std::move(key), {}};
  } catch (const std::exception& error) { return {{}, error.what()}; }
}

std::string storeAssistantKey(const AssistantConnection& connection, const AssistantVaultCancellation* cancellation) {
  try {
    const auto target = credentialId(connection);
    validateAssistantKey(connection.key);
    if (connection.key.empty()) return "Escribe una clave antes de guardarla.";
    const std::unique_ptr<SecretValue, decltype(&secret_value_unref)> value(
      secret_value_new(connection.key.data(), connection.key.size(), "application/octet-stream"), secret_value_unref);
    GError* error = nullptr;
    const auto label = std::string("ImGui Studio - ") + assistantProviderInfo(connection.provider).label;
    if (secret_password_store_binary_sync(&kCredentialSchema, SECRET_COLLECTION_DEFAULT, label.c_str(),
                                         value.get(), nativeCancellation(cancellation), &error,
                                         "connection", target.c_str(), nullptr))
      return {};
    return error ? vaultError(error) : "El llavero de Linux no guardo la clave.";
  } catch (const std::exception& error) { return error.what(); }
}

std::string deleteAssistantKey(const AssistantConnection& connection, const AssistantVaultCancellation* cancellation) {
  try {
    const auto target = credentialId(connection);
    GError* error = nullptr;
    if (secret_password_clear_sync(&kCredentialSchema, nativeCancellation(cancellation), &error,
                                   "connection", target.c_str(), nullptr))
      return {};
    return vaultError(error);
  } catch (const std::exception& error) { return error.what(); }
}
#else
bool assistantVaultAvailable() { return false; }
AssistantVaultResult loadAssistantKey(const AssistantConnection&, const AssistantVaultCancellation*) { return {{}, "Almacen del sistema no disponible."}; }
std::string storeAssistantKey(const AssistantConnection&, const AssistantVaultCancellation*) { return "Almacen del sistema no disponible."; }
std::string deleteAssistantKey(const AssistantConnection&, const AssistantVaultCancellation*) { return "Almacen del sistema no disponible."; }
#endif

} // namespace ImGuiStudio
