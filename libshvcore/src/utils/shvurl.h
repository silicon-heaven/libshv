#ifndef SHV_CORE_UTILS_SHVURL_H
#define SHV_CORE_UTILS_SHVURL_H

#include "../stringview.h"
#include <optional>

namespace shv {
namespace core {
namespace utils {

class SHVCORE_DECL_EXPORT ShvUrl
{
public:
	enum class Type { Plain, AbsoluteService, MountPointRelativeService, DownTreeService };
public:
	ShvUrl(const std::string &shv_path);

	bool isServicePath() const;
	bool isUpTreeMountPointRelative() const;
	bool isUpTreeAbsolute() const;
	bool isPlain() const;
	bool isDownTree() const;
	bool isUpTree() const;
	Type type() const;
	const char* typeString() const;
	StringView service() const;
	/// broker ID including '@' separator
	StringView fullBrokerId() const;
	StringView brokerId() const;
	StringView pathPart() const;
	std::string toPlainPath(const StringView &path_part_prefix = {}) const;
	std::string toString(const StringView &path_part_prefix = {}) const;
	const std::string& shvPath() const;

	static std::string makeShvUrlString(Type type, const StringView &service, const StringView &full_broker_id, const StringView &path_rest);
private:
	static constexpr char END_MARK = ':';
	static constexpr char RELATIVE_MARK = '~';
	static constexpr char ABSOLUTE_MARK = '|';
	static constexpr char DOWNTREE_MARK = '>';

	static size_t serviceProviderMarkIndex(const std::string &path);
	std::string typeMark() const;
	static std::string typeMark(Type t);
private:
	const std::string &m_shvPath;
	Type m_type = Type::Plain;
	StringView m_service;
	std::optional<StringView> m_fullBrokerId; // including @, like '@mpk'
	StringView m_pathPart;
};

} // namespace utils
} // namespace core
} // namespace shv

#endif // SHV_CORE_UTILS_SHVURL_H
