#pragma once

#if defined _WIN32
	#define SHV_DECL_EXPORT     __declspec(dllexport)
	#define SHV_DECL_IMPORT     __declspec(dllimport)
#else
	#define SHV_DECL_EXPORT     __attribute__((visibility("default")))
	#define SHV_DECL_IMPORT     __attribute__((visibility("default")))
	#define SHV_DECL_HIDDEN     __attribute__((visibility("hidden")))
#endif

/// Declaration of macros required for exporting symbols
/// into shared libraries
#if defined(SHVCORE_BUILD_DLL)
	#define SHVCORE_DECL_EXPORT SHV_DECL_EXPORT
#else
	#define SHVCORE_DECL_EXPORT SHV_DECL_IMPORT
#endif

