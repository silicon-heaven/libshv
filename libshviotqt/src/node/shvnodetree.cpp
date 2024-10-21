#include <shv/iotqt/node/shvnodetree.h>

#include <shv/core/utils/shvfilejournal.h>
#include <shv/chainpack/accessgrant.h>
#include <shv/chainpack/metamethod.h>
#include <shv/chainpack/rpcvalue.h>
#include <shv/core/exception.h>
#include <shv/core/stringview.h>
#include <shv/coreqt/log.h>
#include <shv/core/utils/shvpath.h>

namespace shv::iotqt::node {

ShvNodeTree::ShvNodeTree(QObject *parent)
	: QObject(parent)
	, m_root(new ShvRootNode(this))
{
}

ShvNodeTree::ShvNodeTree(ShvNode *root, QObject *parent)
	: QObject(parent)
	, m_root(root)
{
	Q_ASSERT(root);
	if(!root->isRootNode())
		SHV_EXCEPTION("Node tree must begin with root node!");
	if(!m_root->parent())
		m_root->setParent(this);
}

ShvNodeTree::~ShvNodeTree() = default;

ShvNode* ShvNodeTree::root()
{
	return m_root;
}

const ShvNode* ShvNodeTree::root() const
{
	return m_root;
}

ShvNode *ShvNodeTree::mkdir(const ShvNode::String &path)
{
	ShvNode::StringViewList lst = core::utils::ShvPath::split(path);
	return mkdir(lst);
}

ShvNode *ShvNodeTree::mkdir(const ShvNode::StringViewList &path)
{
	return mdcd(path, true, nullptr);
}

ShvNode *ShvNodeTree::cd(const ShvNode::String &path)
{
	std::string path_rest;
	ShvNode::StringViewList lst = core::utils::ShvPath::split(path);
	ShvNode *nd = mdcd(lst, false, &path_rest);
	if(path_rest.empty())
		return nd;
	return nullptr;
}

ShvNode *ShvNodeTree::cd(const ShvNode::String &path, ShvNode::String *path_rest)
{
	ShvNode::StringViewList lst = core::utils::ShvPath::split(path);
	return mdcd(lst, false, path_rest);
}

ShvNode *ShvNodeTree::mdcd(const ShvNode::StringViewList &path, bool create_dirs, ShvNode::String *path_rest)
{
	ShvNode *ret = m_root;
	size_t ix;
	for (ix = 0; ix < path.size(); ++ix) {
		const shv::core::StringView &s = path[ix];
		ShvNode *nd2 = ret->childNode(std::string{s}, !shv::core::Exception::Throw);
		if(nd2 == nullptr) {
			if(create_dirs) {
				ret = new ShvNode(ret);
				ret->setNodeId(std::string{path[ix]});
			}
			else {
				break;
			}
		}
		else {
			ret = nd2;
		}
	}
	if(path_rest) {
		auto path2 = ShvNode::StringViewList(path.begin() + static_cast<long>(ix), path.end());
		*path_rest = shv::core::string::join(path2, '/');
	}
	return ret;
}

bool ShvNodeTree::mount(const ShvNode::String &path, ShvNode *node)
{
	ShvNode::StringViewList lst = core::utils::ShvPath::split(path);
	if(lst.empty()) {
		shvError() << "Cannot mount node on empty path:" << path;
		return false;
	}
	shv::core::StringView last_id = lst.back();
	lst.pop_back();
	ShvNode *parent_nd = nullptr;
	if(lst.empty()) {
		parent_nd = m_root;
	}
	else {
		parent_nd = mkdir(lst);
	}
	ShvNode *ch = parent_nd->childNode(std::string{last_id}, !shv::core::Exception::Throw);
	if(ch) {
		shvError() << "Node exist allready:" << path;
		return false;
	}
	node->setParentNode(parent_nd);
	node->setNodeId(std::string{last_id});
	return true;
}

chainpack::RpcValue ShvNodeTree::invokeMethod(const std::string &shv_path, const std::string &method, const chainpack::RpcValue &params, const chainpack::RpcValue &user_id)
{
	chainpack::RpcRequest rq;
	rq.setShvPath(shv_path);
	rq.setMethod(method);
	rq.setParams(params);
	rq.setUserId(user_id);
	rq.setAccessGrant(chainpack::AccessGrant(chainpack::AccessLevel::Service));
	if(auto *root_nd = root()) {
		try {
			return root_nd->handleRpcRequestImpl(rq);
		}
		catch (const std::exception &e) {
			shvError().nospace() << "Method call exception: " << shv_path << ":" << method << " - " << e.what();
		}
	}
	shvError() << "Root node is NULL";
	return {};
}

namespace {
std::string dump_node(ShvNode *parent, int indent)
{
	std::string ret;
	for(const std::string &pn : parent->childNames()) {
		ShvNode *chnd = parent->childNode(pn);
		ret += '\n' + std::string(static_cast<size_t>(indent), '\t') + pn;// + ": " + parent->propertyValue(pn).toPrettyString();
		if(chnd) {
			ret += dump_node(chnd, ++indent);
		}
	}
	return ret;
}
}

std::string ShvNodeTree::dumpTree()
{
	return dump_node(m_root, 0);
}

}
