#include <shv/iotqt/node/filenode.h>

#include <shv/chainpack/rpcmessage.h>
#include <shv/chainpack/metamethod.h>
#include <shv/core/exception.h>
#include <shv/core/stringview.h>
#include <shv/coreqt/utils.h>
#include <shv/coreqt/log.h>

#include <QCryptographicHash>

namespace cp = shv::chainpack;

namespace shv::iotqt::node {

namespace {
const auto M_HASH = "hash";
const auto M_SIZE = "size";
const auto M_SIZE_COMPRESSED = "sizeCompressed";
const auto M_READ = "read";
const auto M_READ_COMPRESSED = "readCompressed";
}

const std::vector<shv::chainpack::MetaMethod> FileNode::meta_methods_file_base = {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_BROWSE},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_BROWSE},
	{M_HASH, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{M_SIZE, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::LargeResultHint, cp::Rpc::ROLE_BROWSE},
	{M_SIZE_COMPRESSED, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_BROWSE
	 , "Parameters\n"
	   "  read() parameters\n"
	   "  compressionType: gzip (default) | qcompress"
	},
	{M_READ, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::LargeResultHint, cp::Rpc::ROLE_READ
	 , "Parameters\n"
	 "  offset: file offset to start read, default is 0\n"
	 "  size: number of bytes to read starting on offset, default is till end of file\n"
	},
	{M_READ_COMPRESSED, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ
	 , "Parameters\n"
	   "  read() parameters\n"
	   "  compressionType: gzip (default) | qcompress"
	},
};

enum class CompressionType {
	Invalid,
	GZip,
	QCompress,
};

namespace {
CompressionType compression_type_from_string(const std::string &type_str, CompressionType default_type)
{
	if (type_str.empty())
		return default_type;
	if (type_str == "gzip")
		return CompressionType::GZip;
	if (type_str == "qcompress")
		return CompressionType::QCompress;

	return CompressionType::Invalid;
}
}

FileNode::FileNode(const std::string &node_id, shv::iotqt::node::FileNode::Super *parent)
	: Super(node_id, parent)
{
}

cp::RpcValue FileNode::callMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params, const shv::chainpack::RpcValue &user_id)
{
	if (method == M_READ) {
		return read(shv_path, params);
	}
	if (method == M_READ_COMPRESSED) {
		return readFileCompressed(shv_path, params);
	}
	if(method == M_HASH) {
		shv::chainpack::RpcValue::Blob bytes = read(shv_path, params).asBlob();
		QCryptographicHash h(QCryptographicHash::Sha1);
#if QT_VERSION_MAJOR >= 6 && QT_VERSION_MINOR >= 3
		h.addData(QByteArrayView(reinterpret_cast<const char*>(bytes.data()), static_cast<int>(bytes.size())));
#else
		h.addData(reinterpret_cast<const char*>(bytes.data()), static_cast<int>(bytes.size()));
#endif
		return h.result().toHex().toStdString();
	}
	if(method == M_SIZE) {
		return size(shv_path);
	}
	if(method == M_SIZE_COMPRESSED) {
		return static_cast<unsigned>(readFileCompressed(shv_path, params).asBlob().size());
	}

	return Super::callMethod(shv_path, method, params, user_id);
}

size_t FileNode::methodCount(const ShvNode::StringViewList &shv_path)
{
	Q_UNUSED(shv_path)

	return meta_methods_file_base.size();
}

const chainpack::MetaMethod *FileNode::metaMethod(const ShvNode::StringViewList &shv_path, size_t ix)
{
	Q_UNUSED(shv_path)

	if(meta_methods_file_base.size() <= ix)
		SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(meta_methods_file_base.size()));
	return &meta_methods_file_base[ix];
}

std::string FileNode::fileName(const ShvNode::StringViewList &shv_path) const
{
	Q_UNUSED(shv_path)
	return nodeId();
}

chainpack::RpcValue FileNode::size(const StringViewList &shv_path) const
{
#pragma GCC diagnostic push
#if defined __GNUC__ && !defined(__clang__)
#pragma GCC diagnostic ignored "-Wuseless-cast"
#endif
	return static_cast<uint64_t>(readContent(shv_path, 0, std::numeric_limits<int64_t>::max()).asBlob().size());
#pragma GCC diagnostic pop
}

chainpack::RpcValue FileNode::read(const ShvNode::StringViewList &shv_path, const chainpack::RpcValue &params) const
{
	int64_t offset = params.asMap().value("offset").toInt64();
	int64_t size = params.asMap().value("size", std::numeric_limits<int64_t>::max()).toInt64();
	cp::RpcValue ret_value = readContent(shv_path, offset, size);
	ret_value.setMetaValue("fileName", fileName(shv_path));
	ret_value.setMetaValue("offset", offset);
#pragma GCC diagnostic push
#if defined __GNUC__ && !defined(__clang__)
#pragma GCC diagnostic ignored "-Wuseless-cast"
#endif
	ret_value.setMetaValue("size", static_cast<uint64_t>(ret_value.asBlob().size()));
#pragma GCC diagnostic pop
	return ret_value;
}

chainpack::RpcValue FileNode::readFileCompressed(const ShvNode::StringViewList &shv_path, const chainpack::RpcValue &params) const
{
	const auto compression_type_str = params.asMap().value("compressionType").toString();
	const auto compression_type = compression_type_from_string(compression_type_str, CompressionType::GZip);
	if (compression_type == CompressionType::Invalid) {
		SHV_EXCEPTION("Invalid compression type: " + compression_type_str);
	}

	cp::RpcValue result;
	int64_t offset = params.asMap().value("offset").toInt64();
	int64_t size = params.asMap().value("size", std::numeric_limits<int64_t>::max()).toInt64();
	const cp::RpcValue::Blob blob = readContent(shv_path, offset, size).asBlob();
	if (compression_type == CompressionType::QCompress) {
		const auto compressed_blob = qCompress(blob.data(), static_cast<int>(blob.size()));
		result = shv::chainpack::RpcValue::Blob(compressed_blob.cbegin(), compressed_blob.cend());

		result.setMetaValue("compressionType", "qcompress");
		result.setMetaValue("fileName", fileName(shv_path) + ".qcompress");
	}
	else if (compression_type == CompressionType::GZip) {
		result = shv::coreqt::Utils::compressGZip(blob);
		result.setMetaValue("compressionType", "gzip");
		result.setMetaValue("fileName", fileName(shv_path) + ".gz");
	}
	result.setMetaValue("offset", offset);
#pragma GCC diagnostic push
#if defined __GNUC__ && !defined(__clang__)
#pragma GCC diagnostic ignored "-Wuseless-cast"
#endif
	result.setMetaValue("size", static_cast<uint64_t>(result.asBlob().size()));
#pragma GCC diagnostic pop

	return result;
}

}
