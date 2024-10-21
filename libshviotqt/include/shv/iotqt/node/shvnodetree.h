#pragma once

#include <shv/iotqt/node/shvnode.h>

#include <QObject>

namespace shv::iotqt::node {

class SHVIOTQT_DECL_EXPORT ShvNodeTree : public QObject
{
	Q_OBJECT
public:
	explicit ShvNodeTree(QObject *parent = nullptr);
	explicit ShvNodeTree(ShvNode *root, QObject *parent = nullptr);
	~ShvNodeTree() override;

	const ShvNode* root() const;
	ShvNode* root();

	ShvNode* mkdir(const ShvNode::String &path);
	ShvNode* mkdir(const ShvNode::StringViewList &path);
	ShvNode* cd(const ShvNode::String &path);
	ShvNode* cd(const ShvNode::String &path, ShvNode::String *path_rest);
	bool mount(const ShvNode::String &path, ShvNode *node);

	shv::chainpack::RpcValue invokeMethod(const std::string &shv_path,
										  const std::string &method,
										  const shv::chainpack::RpcValue &params = {},
										  const shv::chainpack::RpcValue &user_id = {});

	std::string dumpTree();
protected:
	ShvNode* mdcd(const ShvNode::StringViewList &path, bool create_dirs, ShvNode::String *path_rest);
protected:
	ShvNode* m_root = nullptr;
};
}
