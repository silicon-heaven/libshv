#pragma once

#include <QtGlobal>

/// Declaration of macros required for exporting symbols
/// into shared libraries
#if defined(SHVBROKER_BUILD_DLL)
#  define LIBSHVBROKER_EXPORT Q_DECL_EXPORT
#else
#  define LIBSHVBROKER_EXPORT Q_DECL_IMPORT
#endif

