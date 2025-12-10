#pragma once

#include <shv/chainpack/abstractstreamreader.h>
#include <shv/chainpack/rpcvalue.h>

namespace shv::chainpack {

class LIBSHVCHAINPACK_CPP_EXPORT CponReaderOptions
{
};

class LIBSHVCHAINPACK_CPP_EXPORT CponReader : public AbstractStreamReader
{
	using Super = AbstractStreamReader;
public:
	CponReader(std::istream &in);

	CponReader& operator >>(RpcValue &value);
	CponReader& operator >>(RpcValue::MetaData &meta_data);

	using Super::read;
	void read(RpcValue::MetaData &meta_data) override;
	void read(RpcValue &val) override;

	static RpcValue readFile(const std::string &file_name, std::string *error = nullptr);
private:
	void unpackNext();

	void parseList(RpcValue &val);
	void parseMetaData(RpcValue::MetaData &meta_data);
	void parseMap(RpcValue &val);
	void parseIMap(RpcValue &val);

	void throwParseException(const std::string &msg = {});
};
} // namespace shv::chainpack
