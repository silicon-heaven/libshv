#pragma once

#define SHV_LOG_NECROLOG

#ifdef SHV_LOG_NECROLOG

#ifdef QT_NO_DEBUG
#define SHV_NO_DEBUG_LOG
#endif

#ifdef SHV_NO_DEBUG_LOG
#define NECROLOG_NO_DEBUG_LOG
#endif

#include "stringview.h"
#include <necrolog.h>

#define shvCDebug(category) nCDebug(category)
#define shvCMessage(category) nCMessage(category)
#define shvCInfo(category) nCInfo(category)
#define shvCWarning(category) nCWarning(category)
#define shvCError(category) nCError(category)

#define shvLogFuncFrame() nLogFuncFrame()
#define shvLogScope(name) nLogScope(name)

#else

#include "shvlog.h"

#ifndef SHV_NO_DEBUG_LOG
#ifdef QT_NO_DEBUG
#define SHV_NO_DEBUG
#endif
#endif

#ifdef SHV_NO_DEBUG_LOG
#define shvCDebug(category) while(false) shv::core::ShvLog(shv::core::ShvLog::Level::Debug, shv::core::ShvLog::LogContext{__FILE__, __LINE__, __FUNCTION__, category})
#else
#define shvCDebug(category) for(bool en = shv::core::ShvLog::isMatchingLogFilter(shv::core::ShvLog::Level::Debug, shv::core::ShvLog::LogContext{__FILE__, __LINE__, __FUNCTION__, category}); en; en = false) \
shv::core::ShvLog(shv::core::ShvLog::Level::Debug, shv::core::ShvLog::LogContext{__FILE__, __LINE__, __FUNCTION__, category})
#endif
#define shvCInfo(category) for(bool en = shv::core::ShvLog::isMatchingLogFilter(shv::core::ShvLog::Level::Info, shv::core::ShvLog::LogContext{__FILE__, __LINE__, __FUNCTION__, category}); en; en = false) \
shv::core::ShvLog(shv::core::ShvLog::Level::Info, shv::core::ShvLog::LogContext{__FILE__, __LINE__, __FUNCTION__, category})
#define shvCWarning(category) for(bool en = shv::core::ShvLog::isMatchingLogFilter(shv::core::ShvLog::Level::Warning, shv::core::ShvLog::LogContext{__FILE__, __LINE__, __FUNCTION__, category}); en; en = false) \
shv::core::ShvLog(shv::core::ShvLog::Level::Warning, shv::core::ShvLog::LogContext{__FILE__, __LINE__, __FUNCTION__, category})
#define shvCError(category) for(bool en = shv::core::ShvLog::isMatchingLogFilter(shv::core::ShvLog::Level::Error, shv::core::ShvLog::LogContext{__FILE__, __LINE__, __FUNCTION__, category}); en; en = false) \
shv::core::ShvLog(shv::core::ShvLog::Level::Error, shv::core::ShvLog::LogContext{__FILE__, __LINE__, __FUNCTION__, category})

#ifdef SHV_NO_DEBUG_LOG
#define shvLogFuncFrame() while(0) shvDebug()
#else
#define shvLogFuncFrame() shv::core::ShvLog __shv_func_frame_exit_logger__ = shv::core::ShvLog::isMatchingLogFilter(shv::core::ShvLog::Level::Debug, shv::core::ShvLog::LogContext{__FILE__, __LINE__, __FUNCTION__})? \
shv::core::ShvLog(shv::core::ShvLog::Level::Debug, shv::core::ShvLog::LogContext{__FILE__, __LINE__, __FUNCTION__}) << "     EXIT FN" << __FUNCTION__: \
shv::core::ShvLog(shv::core::ShvLog::Level::Invalid, shv::core::ShvLog::LogContext{__FILE__, __LINE__, __FUNCTION__}); \
shvDebug() << ">>>> ENTER FN" << __FUNCTION__
#endif

#endif

#define shvDebug() shvCDebug("")
#define shvMessage() shvCMessage("")
#define shvInfo() shvCInfo("")
#define shvWarning() shvCWarning("")
#define shvError() shvCError("")

#define logRpcMsg() shvCMessage("RpcMsg")

NecroLog SHVCORE_DECL_EXPORT operator<<(NecroLog log, const shv::core::StringView &v);

