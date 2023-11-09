#pragma once

#include <shv/chainpack/abstractstreamreader.h>

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT ChainPackReader : public AbstractStreamReader
{
	using Super = AbstractStreamReader;
public:
	ChainPackReader(std::istream &in);

	ChainPackReader& operator >>(RpcValue &value);
	ChainPackReader& operator >>(RpcValue::MetaData &meta_data);

	using Super::read;
	void read(RpcValue::MetaData &meta_data) override;
	void read(RpcValue &val) override;

	uint64_t readUIntData(int *err_code);
	static uint64_t readUIntData(std::istream &in, int *err_code);

	using ItemType = ccpcp_item_types;

	ItemType peekNext();
	ItemType unpackNext();
	static const char* itemTypeToString(ItemType it);
private:
	void parseList(RpcValue &val);
	void parseMetaData(RpcValue::MetaData &meta_data);
	void parseMap(RpcValue &val);
	void parseIMap(RpcValue &val);
};

} // namespace chainpack
} // namespace shv
