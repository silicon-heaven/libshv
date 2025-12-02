#pragma once

#include <shv/chainpack/abstractstreamwriter.h>

namespace shv::chainpack {

class SHVCHAINPACK_DECL_EXPORT ChainPackWriter : public shv::chainpack::AbstractStreamWriter
{
	using Super = AbstractStreamWriter;
public:
	ChainPackWriter(std::ostream &out, const std::function<void(int)>& progress_callback = nullptr);

	ChainPackWriter& operator <<(const RpcValue &value);
	ChainPackWriter& operator <<(const RpcValue::MetaData &meta_data);

	void write(const RpcValue &val) override;
	void write(const RpcValue::MetaData &meta_data) override;

	void writeUIntData(uint64_t n);

	void writeContainerBegin(RpcValue::Type container_type, bool /*is_oneliner*/ = false) override;
	/// ChainPack doesn't need to know container type to close it
	void writeContainerEnd() override;
	void writeMapKey(const std::string &key) override;
	void writeIMapKey(RpcValue::Int key) override;
	void writeListElement(const RpcValue &val) override;
	void writeMapElement(const std::string &key, const RpcValue &val) override;
	void writeMapElement(RpcValue::Int key, const RpcValue &val) override;
	void writeRawData(const std::string &data) override;
private:
	ChainPackWriter& write_p(std::nullptr_t);
	ChainPackWriter& write_p(bool value);
	ChainPackWriter& write_p(int32_t value);
	ChainPackWriter& write_p(uint32_t value);
	ChainPackWriter& write_p(int64_t value);
	ChainPackWriter& write_p(uint64_t value);
	ChainPackWriter& write_p(double value);
	ChainPackWriter& write_p(RpcValue::Decimal value);
	ChainPackWriter& write_p(RpcValue::DateTime value);
	ChainPackWriter& write_p(const std::string &value);
	ChainPackWriter& write_p(const RpcValue::Blob &value);
	ChainPackWriter& write_p(const RpcList &values);
	ChainPackWriter& write_p(const RpcValue::Map &values);
	ChainPackWriter& write_p(const RpcValue::IMap &values);

	std::function<void(std::streamoff)> m_progressCallback;
};
} // namespace shv::chainpack
