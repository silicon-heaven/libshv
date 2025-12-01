#pragma once

#include <QtGlobal>

/// Declaration of macros required for exporting symbols
/// into shared libraries
#if defined(SHVIOTQT_BUILD_DLL)
#  define SHVIOTQT_DECL_EXPORT
#else
#  define SHVIOTQT_DECL_EXPORT
#endif

