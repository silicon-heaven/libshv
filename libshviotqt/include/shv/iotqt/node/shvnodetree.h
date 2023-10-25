#pragma once

#include "shvnode.h"

#include <QObject>

namespace shv {
namespace iotqt {
namespace node {

class SHVIOTQT_DECL_EXPORT ShvNodeTree : public QObject
{
	Q_OBJECT
public:
	explicit ShvNodeTree(QObject *parent = nullptr);
	explicit ShvNodeTree(ShvNode *root, QObject *parent = nullptr);
	~ShvNodeTree() override;

	ShvNode* root() const;

	ShvNode* mkdir(const ShvNode::String &path);
	ShvNode* mkdir(const ShvNode::StringViewList &path);
	ShvNode* cd(const ShvNode::String &path);
	ShvNode* cd(const ShvNode::String &path, ShvNode::String *path_rest);
	bool mount(const ShvNode::String &path, ShvNode *node);

	std::string dumpTree();
protected:
	ShvNode* mdcd(const ShvNode::StringViewList &path, bool create_dirs, ShvNode::String *path_rest);
protected:
	ShvNode* m_root = nullptr;
};

}}}
