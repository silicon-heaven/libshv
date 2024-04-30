#pragma once

#include <string_view>

namespace shv {
namespace chainpack {

class Cpon
{
public:
	static constexpr auto STR_NULL = "null";
	static constexpr auto STR_TRUE = "true";
	static constexpr auto STR_FALSE = "false";
	static constexpr auto STR_IMAP_BEGIN = "i{";
	static constexpr auto STR_ESC_BLOB_BEGIN = "b\"";
	static constexpr auto STR_HEX_BLOB_BEGIN = "x\"";
	static constexpr auto STR_DATETIME_BEGIN = "d\"";

	static constexpr auto C_LIST_BEGIN = '[';
	static constexpr auto C_LIST_END = ']';
	static constexpr auto C_MAP_BEGIN = '{';
	static constexpr auto C_MAP_END = '}';
	static constexpr auto C_META_BEGIN = '<';
	static constexpr auto C_META_END = '>';
	static constexpr auto C_DECIMAL_END = 'n';
	static constexpr auto C_UNSIGNED_END = 'u';
};

} // namespace chainpack
} // namespace shv
