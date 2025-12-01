#pragma once

#if defined _WIN32
#define SHVCHAINPACK_DECL_EXPORT_IMPL     __declspec(dllexport)
#define SHVCHAINPACK_DECL_IMPORT_IMPL     __declspec(dllimport)
#else
#define SHVCHAINPACK_DECL_EXPORT_IMPL     __attribute__((visibility("default")))
#define SHVCHAINPACK_DECL_IMPORT_IMPL     __attribute__((visibility("default")))
#define SHVCHAINPACK_DECL_HIDDEN_IMPL     __attribute__((visibility("hidden")))
#endif

/// Declaration of macros required for exporting symbols
/// into shared libraries
#if defined(SHVCHAINPACK_BUILD_DLL)
#define SHVCHAINPACK_DECL_EXPORT
#else
#define SHVCHAINPACK_DECL_EXPORT
#endif

