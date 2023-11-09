#pragma once

#include <QtGlobal>

/// Declaration of macros required for exporting symbols
/// into shared libraries
#if defined(SHVCOREQT_BUILD_DLL)
#  define SHVCOREQT_DECL_EXPORT Q_DECL_EXPORT
#else
#  define SHVCOREQT_DECL_EXPORT Q_DECL_IMPORT
#endif

