#pragma once

#if defined _WIN32
#define _SHVCHAINPACK_DECL_EXPORT     __declspec(dllexport)
#define _SHVCHAINPACK_DECL_IMPORT     __declspec(dllimport)
#else
#define _SHVCHAINPACK_DECL_EXPORT     __attribute__((visibility("default")))
#define _SHVCHAINPACK_DECL_IMPORT     __attribute__((visibility("default")))
#define _SHVCHAINPACK_DECL_HIDDEN     __attribute__((visibility("hidden")))
#endif

/// Declaration of macros required for exporting symbols
/// into shared libraries
#if defined(SHVCHAINPACK_BUILD_DLL)
#define SHVCHAINPACK_DECL_EXPORT _SHVCHAINPACK_DECL_EXPORT
#else
#define SHVCHAINPACK_DECL_EXPORT _SHVCHAINPACK_DECL_IMPORT
#endif

