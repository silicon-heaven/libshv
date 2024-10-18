#pragma once

#include <shv/chainpack/abstractstreamwriter.h>

#include <vector>

namespace shv::chainpack {

class SHVCHAINPACK_DECL_EXPORT CponWriterOptions
{
	bool m_translateIds = false;
	bool m_hexBlob = false;
	std::string m_indent;
	bool m_jsonFormat = false;
public:
	bool isTranslateIds() const;
	CponWriterOptions& setTranslateIds(bool b);

	bool isHexBlob() const;
	CponWriterOptions& setHexBlob(bool b);

	const std::string& indent() const;
	CponWriterOptions& setIndent(const std::string& i);

	bool isJsonFormat() const;
	CponWriterOptions& setJsonFormat(bool b);
};

class SHVCHAINPACK_DECL_EXPORT CponWriter : public AbstractStreamWriter
{
	using Super = AbstractStreamWriter;

public:
	CponWriter(std::ostream &out);
	CponWriter(std::ostream &out, const CponWriterOptions &opts);

	static bool writeFile(const std::string &file_name, const shv::chainpack::RpcValue &rv, std::string *err = nullptr);

	CponWriter& operator <<(const RpcValue &value);
	CponWriter& operator <<(const RpcValue::MetaData &meta_data);

	void write(const RpcValue &val) override;
	void write(const RpcValue::MetaData &meta_data) override;

	void writeContainerBegin(RpcValue::Type container_type, bool is_oneliner = false) override;
	void writeContainerEnd() override;

	void writeMapKey(const std::string &key) override;
	void writeIMapKey(RpcValue::Int key) override;
	void writeListElement(const RpcValue &val) override;
	void writeMapElement(const std::string &key, const RpcValue &val) override;
	void writeMapElement(RpcValue::Int key, const RpcValue &val) override;
	void writeRawData(const std::string &data) override;
private:
	void writeMetaBegin(bool is_oneliner);
	void writeMetaEnd();

	CponWriter& write_p(std::nullptr_t);
	CponWriter& write_p(bool value);
	CponWriter& write_p(int32_t value);
	CponWriter& write_p(uint32_t value);
	CponWriter& write_p(int64_t value);
	CponWriter& write_p(uint64_t value);
	CponWriter& write_p(double value);
	CponWriter& write_p(RpcValue::Decimal value);
	CponWriter& write_p(RpcValue::DateTime value);
	CponWriter& write_p(const std::string &value);
	CponWriter& write_p(const RpcValue::Blob &value);
	CponWriter& write_p(const RpcList &values);
	CponWriter& write_p(const RpcValue::Map &values);
	CponWriter& write_p(const RpcValue::IMap &values, const RpcValue::MetaData *meta_data = nullptr);
private:
	CponWriterOptions m_opts;

	struct ContainerState
	{
		RpcValue::Type containerType = RpcValue::Type::Invalid;
		int elementCount = 0;
		bool isOneLiner = false;
		ContainerState();
		ContainerState(RpcValue::Type t, bool one_liner);
	};
	std::vector<ContainerState> m_containerStates;
};
} // namespace shv::chainpack
