#pragma once

#include <shv/core/exception.h>
#include <shv/core/log.h>

#define SHV_EXCEPTION_STRING(cond, message) shvError() << "EXCEPTION:" << "\"" cond"\" -" << message << Q_FUNC_INFO
#define SHV_ASSERT_STRING(cond, message) shvError() << "\"" cond"\" -" << message << Q_FUNC_INFO
#define SHV_CHECK_STRING(cond, message) shvError() << "\"" cond"\" -" << message << Q_FUNC_INFO

// The 'do {...} while (0)' idiom is not used for the main block here to be
// able to use 'break' and 'continue' as 'actions'.

#define SHV_ASSERT(cond, message, action) if (cond) {} else { SHV_ASSERT_STRING(#cond, message); action; } do {} while (0)
#define SHV_ASSERT_EX(cond, message) if (cond) {} else { SHV_EXCEPTION_STRING(#cond, message); SHV_EXCEPTION(message); } do {} while (0)
#define SHV_CHECK(cond, message) if (cond) {} else { SHV_CHECK_STRING(#cond, message); } do {} while (0)

