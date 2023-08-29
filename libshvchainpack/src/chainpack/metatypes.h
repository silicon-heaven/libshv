#pragma once

#include "../shvchainpackglobal.h"

#include <cstddef>
#include <map>

#define RPC_META_TAG_DEF(tag_name) {static_cast<int>(Tag::tag_name), {static_cast<int>(Tag::tag_name), #tag_name }}
#define RPC_META_KEY_DEF(key_name) {static_cast<int>(Key::key_name), {static_cast<int>(Key::key_name), #key_name }}

namespace shv::chainpack::meta {

struct Tag {
	enum Enum {
		Invalid = -1,
		MetaTypeId = 1,
		MetaTypeNameSpaceId,
		USER = 8
	};
};

class SHVCHAINPACK_DECL_EXPORT MetaInfo
{
public:
	int id = 0;
	const char *name = nullptr;

	MetaInfo();
	MetaInfo(int id_, const char *name_);

	bool isValid() const;
};

class MetaType;
class SHVCHAINPACK_DECL_EXPORT MetaNameSpace
{
public:
	MetaNameSpace(const char *name = nullptr);
	const char *name() const;
	bool isValid() const;

	std::map<int, MetaType*>& types();
	const std::map<int, MetaType*>& types() const;
protected:
	const char *m_name;
	std::map<int, MetaType*> m_types;
};

class SHVCHAINPACK_DECL_EXPORT MetaType
{
public:
	MetaType(const char *name);
	const char *name() const;
	const MetaInfo& tagById(int id) const;
	const MetaInfo& keyById(int id) const;
	bool isValid() const;
protected:
	const char *m_name;
	std::map<int, MetaInfo> m_tags;
	std::map<int, MetaInfo> m_keys;
};

SHVCHAINPACK_DECL_EXPORT void registerNameSpace(int ns_id, MetaNameSpace *ns);
SHVCHAINPACK_DECL_EXPORT void registerType(int ns_id, int type_id, MetaType *tid);
SHVCHAINPACK_DECL_EXPORT const MetaNameSpace& registeredNameSpace(int ns_id);
SHVCHAINPACK_DECL_EXPORT const MetaType& registeredType(int ns_id, int type_id);

enum class NameSpaceID
{
	Global = 0,
	Elesys,
	Eyas,
};

class SHVCHAINPACK_DECL_EXPORT GlobalNS : public meta::MetaNameSpace
{
	using Super = meta::MetaNameSpace;
public:
	enum {ID = static_cast<int>(NameSpaceID::Global)};
	GlobalNS();

	struct MetaTypeId
	{
		enum Enum {
			ChainPackRpcMessage = 1,
			RpcConnectionParams,
			TunnelCtl,
			AccessGrant,
			DataChange,
			NodeDrop,
			ValueNotAvailable, // DEPRECATED WE ARE USING null INSTEAD
			ShvJournalEntry,
			NodePropertyMap,
		};
	};
	static void registerMetaTypes();
};

class SHVCHAINPACK_DECL_EXPORT ElesysNS : public meta::MetaNameSpace
{
	using Super = meta::MetaNameSpace;
public:
	enum {ID = static_cast<int>(NameSpaceID::Elesys)};
	ElesysNS();

	struct MetaTypeId
	{
		enum Enum {
			VTKEventData = 1,
		};
	};
};

}
