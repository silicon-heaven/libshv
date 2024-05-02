#pragma once

#include "filenode.h"

#include <QDir>
#include <QMap>

namespace shv::iotqt::node {

class SHVIOTQT_DECL_EXPORT LocalFSNode : public FileNode
{
	Q_OBJECT

	using Super = FileNode;
public:
	LocalFSNode(const QString &root_path, ShvNode *parent = nullptr);
	LocalFSNode(const QString &root_path, const std::string &node_id, ShvNode *parent = nullptr);

	chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params, const shv::chainpack::RpcValue &user_id) override;

	StringList childNames(const ShvNode::StringViewList &shv_path) override;
	chainpack::RpcValue hasChildren(const StringViewList &shv_path) override;

	size_t methodCount(const StringViewList &shv_path) override;
	const shv::chainpack::MetaMethod* metaMethod(const StringViewList &shv_path, size_t ix) override;
private:
	QString makeAbsolutePath(const QString &relative_path) const;
	std::string fileName(const ShvNode::StringViewList &shv_path) const override;
	shv::chainpack::RpcValue readContent(const ShvNode::StringViewList &shv_path, int64_t offset, int64_t size) const override;
	shv::chainpack::RpcValue size(const ShvNode::StringViewList &shv_path) const override;

	bool isDir(const ShvNode::StringViewList &shv_path) const;
	void checkPathIsBoundedToFsRoot(const QString &path) const;

	QFileInfo ndFileInfo(const QString &path) const;
	chainpack::RpcValue ndSize(const QString &path) const;
	chainpack::RpcValue ndRead(const QString &path, qint64 offset, qint64 size) const;
	chainpack::RpcValue ndWrite(const QString &path, const chainpack::RpcValue &methods_params);
	chainpack::RpcValue ndDelete(const QString &path);
	chainpack::RpcValue ndMkfile(const QString &path, const shv::chainpack::RpcValue &methods_params);
	chainpack::RpcValue ndMkdir(const QString &path, const shv::chainpack::RpcValue &methods_params);
	chainpack::RpcValue ndRmdir(const QString &path, bool recursively);
	chainpack::RpcValue ndLsDir(const QString &path, const shv::chainpack::RpcValue &methods_params);
protected:
	QDir m_rootDir;
};
} // namespace shv::iotqt::node
