#pragma once

#include <QtGlobal>

/// Declaration of macros required for exporting symbols
/// into shared libraries
#if defined(SHVBROKER_BUILD_DLL)
#  define SHVBROKER_DECL_EXPORT
#else
#  define SHVBROKER_DECL_EXPORT
#endif

