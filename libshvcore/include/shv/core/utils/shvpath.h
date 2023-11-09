#pragma once

#include "../shvcoreglobal.h"

#include "../string.h"

#include <functional>
#include <map>

namespace shv {
namespace core {
namespace utils {

namespace shvpath {

SHVCORE_DECL_EXPORT bool startsWithPath(const std::string_view &str, const std::string_view &path, size_t *pos = nullptr);

}

class SHVCORE_DECL_EXPORT ShvPath : public shv::core::String
{
	using Super = shv::core::String;
public:
	static constexpr auto SHV_PATH_QUOTE = '\'';
	static constexpr auto SHV_PATH_DELIM = '/';
	static constexpr auto SHV_PATH_METHOD_DELIM = ':';
public:
	ShvPath();
	ShvPath(shv::core::String &&o);
	ShvPath(const shv::core::String &o);
	using Super::Super;

	const std::string &asString() const;

	bool startsWithPath(const StringView &path, size_t *pos = nullptr) const;
	static bool startsWithPath(const StringView &str, const StringView &path, size_t *pos = nullptr);

	ShvPath appendPath(const StringView &path) const;

	static ShvPath joinDirs(const std::vector<std::string> &dirs);
	static ShvPath joinDirs(const shv::core::StringViewList &dirs);
	static ShvPath joinDirs(std::vector<StringView>::const_iterator first, std::vector<StringView>::const_iterator last);
	static ShvPath joinDirs(StringView dir1, StringView dir2);
	static ShvPath appendDir(StringView path1, StringView dir);
	ShvPath appendDir(StringView dir) const;

	// split is deprecated, use splitPath instead
	static shv::core::StringViewList split(const shv::core::StringView &shv_path);
	static shv::core::StringViewList splitPath(const shv::core::StringView &shv_path);
	static shv::core::StringView firstDir(const shv::core::StringView &shv_path, size_t *next_dir_pos = nullptr);
	static shv::core::StringView takeFirsDir(shv::core::StringView &shv_path);

	bool matchWild(const std::string &pattern) const;
	bool matchWild(const shv::core::StringViewList &pattern_lst) const;
	static bool matchWild(const shv::core::StringViewList &path_lst, const shv::core::StringViewList &pattern_lst);

	template<typename T>
	static void forEachDirAndSubdirs(const T &map, const std::string &root_dir, std::function<void (typename T::const_iterator)> fn)
	{
		for (auto it = map.lower_bound(root_dir); it != map.end(); ) {
			const auto &key = it->first;
			if(key.rfind(root_dir, 0) == 0) {
				// key starts with key
				// key must be either equal to root or ended with slash
				if(key.size() == root_dir.size() || key[root_dir.size()] == '/') {
					fn(it);
				}
				++it;
			}
			else {
				break;
			}
		}
	}

	template<typename T>
	static void forEachDirAndSubdirsDeleteIf(T &map, const std::string &root_dir, std::function<bool (typename T::const_iterator)> fn)
	{
		for (auto it = map.lower_bound(root_dir); it != map.end(); ) {
			const auto &key = it->first;
			if(key.rfind(root_dir, 0) == 0) {
				// key starts with key
				// key must be either equal to root or ended with slash
				if(key.size() == root_dir.size() || key[root_dir.size()] == '/') {
					if(fn(it)) {
						it = map.erase(it);
						continue;
					}
				}
				++it;
			}
			else {
				break;
			}
		}
	}
};

}}}

