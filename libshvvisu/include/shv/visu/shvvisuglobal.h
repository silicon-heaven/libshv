#pragma once

#include <QtGlobal>

/// Declaration of macros required for exporting symbols
/// into shared libraries
#if defined(SHVVISU_BUILD_DLL)
#  define SHVVISU_DECL_EXPORT
#else
#  define SHVVISU_DECL_EXPORT
#endif
