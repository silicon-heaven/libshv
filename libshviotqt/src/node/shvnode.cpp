#include <shv/iotqt/node/shvnode.h>

#include <shv/core/utils/shvfilejournal.h>
#include <shv/coreqt/log.h>
#include <shv/coreqt/utils.h>
#include <shv/chainpack/metamethod.h>
#include <shv/chainpack/rpcmessage.h>
#include <shv/chainpack/rpcvalue.h>
#include <shv/chainpack/rpcdriver.h>
#include <shv/chainpack/cponreader.h>
#include <shv/chainpack/cponwriter.h>
#include <shv/chainpack/accessgrant.h>
#include <shv/core/exception.h>
#include <shv/core/stringview.h>
#include <shv/core/string.h>
#include <shv/core/utils/shvpath.h>
#include <shv/core/utils/shvurl.h>

#include <QTimer>
#include <QFile>
#include <cstring>
#include <fstream>

using namespace shv::chainpack;

#define logConfig() shvCMessage("Config")

namespace shv::iotqt::node {

//===========================================================
// ShvNode
//===========================================================
const std::string ShvNode::LOCAL_NODE_HACK = ".local";
const std::string ShvNode::ADD_LOCAL_TO_LS_RESULT_HACK_META_KEY = "__add_local_to_ls_hack";

ShvNode::ShvNode(ShvNode *parent)
	: QObject(parent)
{
	shvDebug() << __FUNCTION__ << this;
}

ShvNode::ShvNode(const std::string &node_id, ShvNode *parent)
	: ShvNode(parent)
{
	setNodeId(node_id);
}

ShvNode::~ShvNode() = default;

ShvNode *ShvNode::parentNode() const
{
	return qobject_cast<ShvNode*>(parent());
}

ShvNode *ShvNode::childNode(const ShvNode::String &name, bool throw_exc) const
{
	auto *nd = findChild<ShvNode*>(QString::fromStdString(name), Qt::FindDirectChildrenOnly);
	if(throw_exc && !nd)
		SHV_EXCEPTION("Child node id: " + name + " doesn't exist, parent node: " + shvPath());
	return nd;
}

void ShvNode::setParentNode(ShvNode *parent)
{
	setParent(parent);
}

ShvNode::String ShvNode::nodeId() const
{
	return m_nodeId;
}

void ShvNode::setNodeId(ShvNode::String &&n)
{
	setObjectName(QString::fromStdString(n));
	shvDebug() << __FUNCTION__ << this << n;
	m_nodeId = std::move(n);
}

void ShvNode::setNodeId(const ShvNode::String &n)
{
	setObjectName(QString::fromStdString(n));
	shvDebug() << __FUNCTION__ << this << n;
	m_nodeId = n;
}

shv::core::utils::ShvPath ShvNode::shvPath() const
{
	shv::core::utils::ShvPath ret;
	const ShvNode *nd = this;
	while(nd) {
		if(!nd->isRootNode()) {
			if(!ret.empty())
				ret = '/' + ret;
			ret = nd->nodeId() + ret;
		}
		else {
			break;
		}
		nd = nd->parentNode();
	}
	return ret;
}

void ShvNode::setSortedChildren(bool b)
{
	m_isSortedChildren = b;
}

void ShvNode::deleteIfEmptyWithParents()
{
	ShvNode *nd = this;
	if(!nd->isRootNode() && nd->childNames().empty()) {
		ShvNode *lowest_empty_parent_nd = nd;
		nd = nd->parentNode();
		while(nd && !nd->isRootNode() && nd->childNames().size() == 1) {
			lowest_empty_parent_nd = nd;
			nd = nd->parentNode();
		}
		delete lowest_empty_parent_nd;
	}
}

bool ShvNode::isRootNode() const
{
	return m_isRootNode;
}

void ShvNode::handleRpcFrame(RpcFrame &&frame)
{
	shvLogFuncFrame() << "node:" << nodeId() << "meta:" << frame.meta.toPrettyString();
	using ShvPath = shv::core::utils::ShvPath;
	using namespace std;
	using namespace shv::core::utils;
	const chainpack::RpcValue::String method = RpcMessage::method(frame.meta).toString();
	const chainpack::RpcValue::String shv_path_str = RpcMessage::shvPath(frame.meta).toString();
	ShvUrl shv_url(RpcMessage::shvPath(frame.meta).asString());
	core::StringViewList shv_path = ShvPath::split(shv_url.pathPart());
	const bool ls_hook = frame.meta.hasKey(ADD_LOCAL_TO_LS_RESULT_HACK_META_KEY);
	RpcResponse resp = RpcResponse::forRequest(frame.meta);
	try {
		if(!shv_path.empty()) {
			ShvNode *nd = childNode(std::string{shv_path.at(0)}, !shv::core::Exception::Throw);
			if(nd) {
				shvDebug() << "Child node:" << shv_path.at(0) << "on path:" << shv_path.join('/') << "FOUND";
				std::string new_path = ShvUrl::makeShvUrlString(shv_url.type(),
																shv_url.service(),
																shv_url.fullBrokerId(),
																ShvPath::joinDirs(++shv_path.begin(), shv_path.end()));
				RpcMessage::setShvPath(frame.meta, new_path);
				nd->handleRpcFrame(std::move(frame));
				return;
			}
		}
		const chainpack::MetaMethod *mm = metaMethod(shv_path, method);
		if(mm) {
			shvDebug() << "Metamethod:" << method << "on path:" << ShvPath::joinDirs(shv_path) << "FOUND";
			std::string errmsg;
			RpcMessage rpc_msg = frame.toRpcMessage(&errmsg);
			if(!errmsg.empty())
				SHV_EXCEPTION(errmsg);

			RpcRequest rq(rpc_msg);
			chainpack::RpcValue ret_val = processRpcRequest(rq);
			if(ret_val.isValid()) {
				resp.setResult(ret_val);
			}
		}
		else {
			string path = shv::core::utils::joinPath(shvPath(), shv_path_str);
			throw chainpack::RpcException(RpcResponse::Error::MethodNotFound,
										  "Method: '" + method + "' on path '" + path + "' doesn't exist",
										  std::string(__FILE__) + ":" + std::to_string(__LINE__));
		}
	}
	catch (const chainpack::RpcException &e) {
		shvError() << "method:"  << method << "path:" << shv_path_str << "err code:" << e.errorCode() << "msg:" << e.message();
		RpcResponse::Error err(e.message(), e.errorCode(), e.data());
		resp.setError(err);
	}
	catch (const std::exception &e) {
		std::string err_str = "method: " + method + " path: " + shv_path_str + " what: " +  e.what();
		shvError() << err_str;
		RpcResponse::Error err(err_str, RpcResponse::Error::MethodCallException);
		resp.setError(err);
	}
	if(resp.hasResult()) {
		ShvNode *root = rootNode();
		if(root) {
			if (ls_hook && resp.isSuccess()) {
				chainpack::RpcValue::List res_list = resp.result().asList();
				if (!res_list.empty() && !res_list[0].isList())
					res_list.insert(res_list.begin(), LOCAL_NODE_HACK);
				else
					res_list.insert(res_list.begin(), chainpack::RpcValue::List{ LOCAL_NODE_HACK, true });
				resp.setResult(res_list);
				shvDebug() << resp.toCpon();
			}
			root->emitSendRpcMessage(resp);
		}
	}
}

void ShvNode::handleRpcRequest(const chainpack::RpcRequest &rq)
{
	shvLogFuncFrame() << "node:" << nodeId() << metaObject()->className();
	using ShvPath = shv::core::utils::ShvPath;
	const chainpack::RpcValue::String method = rq.method().asString();
	const chainpack::RpcValue::String shv_path_str = rq.shvPath().asString();
	core::StringViewList shv_path = ShvPath::split(shv_path_str);
	RpcResponse resp = RpcResponse::forRequest(rq);
	try {
		chainpack::RpcValue ret_val = handleRpcRequestImpl(rq);
		if(ret_val.isValid())
			resp.setResult(ret_val);
	}
	catch (const chainpack::RpcException &e) {
		shvDebug() << "method:"  << method << "path:" << shv_path_str << "err code:" << e.errorCode() << "msg:" << e.message();
		resp.setError(RpcResponse::Error(e.message(), e.errorCode(), e.data()));
	}
	catch (const chainpack::Exception &e) {
		shvDebug() << "method:"  << method << "path:" << shv_path_str << "error:" << e.what();
		resp.setError(RpcResponse::Error(e.message(), RpcResponse::Error::MethodCallException, e.data()));
	}
	catch (const std::exception &e) {
		shvError() << e.what();
		resp.setError(RpcResponse::Error(e.what(), RpcResponse::Error::MethodCallException));
	}
	if(resp.hasResult()) {
		ShvNode *root = rootNode();
		if(root) {
			shvDebug() << "emit resp:"  << resp.toCpon();
			root->emitSendRpcMessage(resp);
		}
	}
}

chainpack::RpcValue ShvNode::handleRpcRequestImpl(const chainpack::RpcRequest &rq)
{
	shvLogFuncFrame() << "node:" << nodeId() << metaObject()->className();
	using ShvPath = shv::core::utils::ShvPath;
	const chainpack::RpcValue::String method = rq.method().asString();
	const chainpack::RpcValue::String shv_path_str = rq.shvPath().asString();
	core::StringViewList shv_path = ShvPath::split(shv_path_str);
	if(!shv_path.empty()) {
		ShvNode *nd = childNode(std::string{shv_path.at(0)}, !shv::core::Exception::Throw);
		if(nd) {
			shvDebug() << "Child node:" << shv_path.at(0) << "on path:" << ShvPath::joinDirs(shv_path) << "FOUND";
			std::string new_path = ShvPath::joinDirs(++shv_path.begin(), shv_path.end());
			chainpack::RpcRequest rq2(rq);
			rq2.setShvPath(new_path);
			return nd->handleRpcRequestImpl(rq2);
		}
	}
	shvDebug() << "Metamethod:" << method << "on path:" << shv_path.join('/');
	return processRpcRequest(rq);
}

chainpack::RpcValue ShvNode::processRpcRequest(const chainpack::RpcRequest &rq)
{
	shvLogFuncFrame() << rq.shvPath() << rq.method();
	core::StringViewList shv_path = core::utils::ShvPath::split(rq.shvPath().asString());
	const chainpack::RpcValue::String method = rq.method().asString();
	const chainpack::MetaMethod *mm = metaMethod(shv_path, method);
	if (!mm) {
		throw chainpack::RpcException(RpcResponse::Error::MethodNotFound,
									  "Method: '" + method + "' on path '" + shvPath() + '/' + rq.shvPath().toString() + "' doesn't exist",
									  std::string(__FILE__) + ":" + std::to_string(__LINE__));
	}
	const chainpack::RpcValue &rq_grant = rq.accessGrant();
	const RpcValue &mm_grant = mm->accessGrant();
	auto rq_access_level = grantToAccessLevel(rq_grant);
	auto mm_access_level = grantToAccessLevel(mm_grant);
	if(mm_access_level > rq_access_level)
		SHV_EXCEPTION(std::string("Call method: '") + method + "' on path '" + shvPath() + '/' + rq.shvPath().toString() + "' permission denied, grant: " + rq_grant.toCpon() + " required: " + mm_grant.toCpon());

	if(mm_access_level > MetaMethod::AccessLevel::Write) {


		shv::core::utils::ShvJournalEntry e(shv::core::utils::joinPath(shvPath(), rq.shvPath().asString())
											, method + '(' + rq.params().toCpon() + ')'
											, shv::chainpack::Rpc::SIG_COMMAND_LOGGED
											, shv::core::utils::ShvJournalEntry::NO_SHORT_TIME
											, (1 << DataChange::ValueFlag::Spontaneous));
		e.epochMsec = RpcValue::DateTime::now().msecsSinceEpoch();
		chainpack::RpcValue user_id = rq.userId();
		if(user_id.isValid())
			e.userId = user_id.toCpon();
		rootNode()->emitLogUserCommand(e);
	}

	return callMethodRq(rq);
}

chainpack::RpcValue ShvNode::callMethodRq(const chainpack::RpcRequest &rq)
{
	core::StringViewList shv_path = shv::core::utils::ShvPath::split(rq.shvPath().asString());
	const chainpack::RpcValue::String method = rq.method().asString();
	chainpack::RpcValue ret_val = callMethod(shv_path, method, rq.params(), rq.userId());
	return ret_val;
}

QList<ShvNode *> ShvNode::ownChildren() const
{
	QList<ShvNode*> lst = findChildren<ShvNode*>(QString(), Qt::FindDirectChildrenOnly);
	return lst;
}

ShvNode::StringList ShvNode::childNames()
{
	return childNames(StringViewList());
}

ShvNode::StringList ShvNode::childNames(const StringViewList &shv_path)
{
	shvLogFuncFrame() << "node:" << nodeId() << "shv_path:" << shv_path.join('/');
	ShvNode::StringList ret;
	if(shv_path.empty()) {
		for (ShvNode *nd : ownChildren()) {
			ret.push_back(nd->nodeId());
			if(m_isSortedChildren)
				std::sort(ret.begin(), ret.end());
		}
	}
	else if(shv_path.size() == 1) {
		ShvNode *nd = childNode(std::string{shv_path.at(0)}, !shv::core::Exception::Throw);
		if(nd)
			ret = nd->childNames(StringViewList());
	}
	shvDebug() << "\tret:" << shv::core::String::join(ret, '+');
	return ret;
}

chainpack::RpcValue ShvNode::hasChildren(const StringViewList &shv_path)
{
	shvLogFuncFrame() << "node:" << nodeId() << "shv_path:" << shv_path.join('/');
	if(shv_path.size() == 1) {
		ShvNode *nd = childNode(std::string{shv_path.at(0)}, !shv::core::Exception::Throw);
		if(nd) {
			return nd->hasChildren(StringViewList());
		}
	}
	return !childNames(shv_path).empty();
}

MetaMethod::AccessLevel ShvNode::basicGrantToAccessLevel(const RpcValue &access_grant)
{
	if(access_grant.isString()) {
		auto acc_level = shv::chainpack::MetaMethod::AccessLevel::None;
		auto roles = shv::core::utils::split(access_grant.asString(), ',');
		for(const auto &role : roles) {
			if(role == Rpc::ROLE_BROWSE) acc_level = std::max(acc_level, MetaMethod::AccessLevel::Browse);
			else if(role == Rpc::ROLE_READ) acc_level = std::max(acc_level, MetaMethod::AccessLevel::Read);
			else if(role == Rpc::ROLE_WRITE) acc_level = std::max(acc_level, MetaMethod::AccessLevel::Write);
			else if(role == Rpc::ROLE_COMMAND) acc_level = std::max(acc_level, MetaMethod::AccessLevel::Command);
			else if(role == Rpc::ROLE_CONFIG) acc_level = std::max(acc_level, MetaMethod::AccessLevel::Config);
			else if(role == Rpc::ROLE_SERVICE) acc_level = std::max(acc_level, MetaMethod::AccessLevel::Service);
			else if(role == Rpc::ROLE_SUPER_SERVICE) acc_level = std::max(acc_level, MetaMethod::AccessLevel::SuperService);
			else if(role == Rpc::ROLE_DEVEL) acc_level = std::max(acc_level, MetaMethod::AccessLevel::Devel);
			else if(role == Rpc::ROLE_ADMIN) acc_level = std::max(acc_level, MetaMethod::AccessLevel::Admin);
		}
		return acc_level;
	}
	if(access_grant.isInt()) {
		return static_cast<MetaMethod::AccessLevel>(access_grant.toInt());
	}
	return shv::chainpack::MetaMethod::AccessLevel::None;
}

MetaMethod::AccessLevel ShvNode::grantToAccessLevel(const RpcValue &acces_grant) const
{
	return basicGrantToAccessLevel(acces_grant);
}

void ShvNode::treeWalk(std::function<void (ShvNode *parent_nd, const StringViewList &shv_path)> callback)
{
	treeWalk_helper(callback, this, {});
}

void ShvNode::treeWalk_helper(std::function<void (ShvNode *, const ShvNode::StringViewList &)> callback, ShvNode *parent_nd, const ShvNode::StringViewList &shv_path)
{
	callback(parent_nd, shv_path);
	const auto child_names = parent_nd->childNames(shv_path);
	for (const std::string &child_name : child_names) {
		shv::iotqt::node::ShvNode *child_nd = nullptr;
		if(shv_path.empty()) {
			child_nd = parent_nd->childNode(child_name, false);
			if (child_nd)
				treeWalk_helper(callback, child_nd, {});
		}
		if(!child_nd) {
			shv::core::StringViewList new_shv_path = shv_path;
			new_shv_path.push_back(child_name);
			treeWalk_helper(callback, parent_nd, new_shv_path);
		}
	}
}

chainpack::RpcValue ShvNode::dir(const StringViewList &shv_path, const chainpack::RpcValue &methods_params)
{
	auto method_name = methods_params.asString();
	size_t cnt = methodCount(shv_path);
	if(method_name.empty()) {
		RpcValue::List ret;
		for (size_t ix = 0; ix < cnt; ++ix) {
			const chainpack::MetaMethod *mm = metaMethod(shv_path, ix);
			ret.push_back(mm->toRpcValue());
		}
		return RpcValue{ret};
	}
	for (size_t ix = 0; ix < cnt; ++ix) {
		const chainpack::MetaMethod *mm = metaMethod(shv_path, ix);
		if(mm->name() == method_name) {
			return RpcValue{mm->toRpcValue()};
		}
	}
	return RpcValue{nullptr};
}

chainpack::RpcValue ShvNode::ls(const StringViewList &shv_path, const chainpack::RpcValue &methods_params)
{
	const std::string &child_name = methods_params.asString();
	if(child_name.empty()) {
		RpcValue::List ret;
		for(const std::string &ch_name : childNames(shv_path)) {
			ret.push_back(ch_name);
		}
		return chainpack::RpcValue{ret};
	}
	for(const std::string &ch_name : childNames(shv_path)) {
		if(ch_name == child_name)
			return true;
	}
	return false;
}

static const std::vector<MetaMethod> meta_methods {
	shv::chainpack::methods::DIR,
	shv::chainpack::methods::LS,
};

size_t ShvNode::methodCount(const StringViewList &shv_path)
{
	if(shv_path.empty())
		return meta_methods.size();
	return 0;
}

const chainpack::MetaMethod *ShvNode::metaMethod(const StringViewList &shv_path, size_t ix)
{
	if(shv_path.empty())
		return &(meta_methods.at(ix));
	return nullptr;
}

const chainpack::MetaMethod *ShvNode::metaMethod(const ShvNode::StringViewList &shv_path, const std::string &name)
{
	size_t cnt = methodCount(shv_path);
	for (size_t i = 0; i < cnt; ++i) {
		const chainpack::MetaMethod *mm = metaMethod(shv_path, i);
		if(mm && name == mm->name())
			return mm;
	}
	return nullptr;
}

chainpack::RpcValue ShvNode::callMethod(const ShvNode::StringViewList &shv_path, const std::string &method, const chainpack::RpcValue &params, const shv::chainpack::RpcValue &user_id)
{
	if(method == Rpc::METH_DIR)
		return dir(shv_path, params);
	if(method == Rpc::METH_LS)
		return ls(shv_path, params);

	SHV_EXCEPTION("Node: " + shvPath() + " - method: " + method + " not exists on path: " + shv_path.join('/') + " user id: " + user_id.toCpon());
}

ShvNode *ShvNode::rootNode()
{
	ShvNode *nd = this;
	while(nd) {
		if(nd->isRootNode())
			return nd;
		nd = nd->parentNode();
	}
	return nullptr;
}

void ShvNode::emitSendRpcMessage(const chainpack::RpcMessage &msg)
{
	if(isRootNode()) {
		if(msg.isResponse()) {
			RpcResponse resp(msg);
			// RPC calls with requestID == 0 does not expect response
			// whoo is using this?
			if(resp.requestId().toInt() == 0) {
				shvWarning() << "throwing away response with invalid request ID:" << resp.requestId().toCpon();
				return;
			}
		}
		emit sendRpcMessage(msg);
	}
	else {
		ShvNode *rnd = rootNode();
		if(rnd) {
			rnd->emitSendRpcMessage(msg);
		}
		else {
			shvError() << "Cannot find root node to send RPC message";
		}
	}
}

void ShvNode::emitLogUserCommand(const shv::core::utils::ShvJournalEntry &e)
{
	if(isRootNode()) {
		emit logUserCommand(e);

		// emit also as change to have commands in HP dirty-log
		// only HP should have this
		RpcSignal sig;
		sig.setMethod(Rpc::SIG_COMMAND_LOGGED);
		sig.setShvPath(e.path);
		sig.setParams(e.toRpcValue());
		emitSendRpcMessage(sig);
	}
	else {
		ShvNode *rnd = rootNode();
		if(rnd) {
			rnd->emitLogUserCommand(e);
		}
		else {
			shvError() << "Cannot find root node to send RPC message";
		}
	}
}

//===========================================================
// ShvRootNode
//===========================================================
ShvRootNode::ShvRootNode(QObject *parent)
 : Super(nullptr)
{
	setNodeId("<ROOT>");
	setParent(parent);
	m_isRootNode = true;
}

ShvRootNode::~ShvRootNode() = default;

//===========================================================
// MethodsTableNode
//===========================================================
MethodsTableNode::MethodsTableNode(const std::string &node_id, const std::vector<shv::chainpack::MetaMethod> *methods, shv::iotqt::node::ShvNode *parent)
	: Super(node_id, parent)
	, m_methods(methods)
{
}

size_t MethodsTableNode::methodCount(const shv::iotqt::node::ShvNode::StringViewList &shv_path)
{
	if(m_methods == nullptr)
		SHV_EXCEPTION("Methods table not set!");
	if(shv_path.empty()) {
		return m_methods->size();
	}
	return Super::methodCount(shv_path);
}

const shv::chainpack::MetaMethod *MethodsTableNode::metaMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, size_t ix)
{
	if(m_methods == nullptr)
		SHV_EXCEPTION("Methods table not set!");
	if(shv_path.empty()) {
		if(m_methods->size() <= ix)
			SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(m_methods->size()));
		return &(m_methods->operator[](ix));
	}
	return Super::metaMethod(shv_path, ix);
}


//===========================================================
// RpcValueMapNode
//===========================================================
const std::vector<MetaMethod> meta_methods_value_map_root_node {
	shv::chainpack::methods::DIR,
	shv::chainpack::methods::LS,
};

const std::vector<MetaMethod> meta_methods_value_map_node {
	shv::chainpack::methods::DIR,
	shv::chainpack::methods::LS,
	{Rpc::METH_GET, MetaMethod::Flag::IsGetter, "", "RpcValue", MetaMethod::AccessLevel::Read},
	{Rpc::METH_SET, MetaMethod::Flag::IsSetter, "RpcValue", "Bool", MetaMethod::AccessLevel::Config},
};

RpcValueMapNode::RpcValueMapNode(const std::string &node_id, ShvNode *parent)
	: Super(node_id, parent)
{
}

RpcValueMapNode::RpcValueMapNode(const std::string &node_id, const chainpack::RpcValue &values, ShvNode *parent)
	: Super(node_id, parent)
	, m_valuesLoaded(true)
	, m_values(values)
{
}

size_t RpcValueMapNode::methodCount(const shv::iotqt::node::ShvNode::StringViewList &shv_path)
{
	if(shv_path.empty()) {
		return meta_methods_value_map_root_node.size();
	}
	if(isDir(shv_path))
		return 2;
	if(isReadOnly())
		return meta_methods_value_map_node.size() - 1;

	return meta_methods_value_map_node.size();
}

const shv::chainpack::MetaMethod *RpcValueMapNode::metaMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, size_t ix)
{
	size_t size;
	const std::vector<shv::chainpack::MetaMethod> &methods = shv_path.empty()? meta_methods_value_map_root_node: meta_methods_value_map_node;
	if(shv_path.empty()) {
		size = meta_methods_value_map_root_node.size();
	}
	else {
		size = isDir(shv_path)? 2: meta_methods_value_map_node.size();
	}
	if(size <= ix)
		SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(size));
	return &(methods[ix]);
}

shv::iotqt::node::ShvNode::StringList RpcValueMapNode::childNames(const shv::iotqt::node::ShvNode::StringViewList &shv_path)
{
	shv::chainpack::RpcValue val = valueOnPath(shv_path);
	ShvNode::StringList lst;
	for(const auto &kv : val.asMap()) {
		lst.push_back(kv.first);
	}
	return lst;
}

shv::chainpack::RpcValue RpcValueMapNode::hasChildren(const shv::iotqt::node::ShvNode::StringViewList &shv_path)
{
	return isDir(shv_path);
}

chainpack::RpcValue RpcValueMapNode::callMethod(const ShvNode::StringViewList &shv_path, const std::string &method, const chainpack::RpcValue &params, const shv::chainpack::RpcValue &user_id)
{
	if(shv_path.empty()) {
		if(method == M_LOAD) {
			m_valuesLoaded = false;
			return values();
		}
		if(method == M_SAVE) {
			m_valuesLoaded = true;
			m_values = params;
			saveValues();
			return true;
		}
		if(method == M_COMMIT) {
			commitChanges();
			return true;
		}
	}
	if(method == Rpc::METH_GET) {
		shv::chainpack::RpcValue rv = valueOnPath(shv_path);
		return rv;
	}
	if(method == Rpc::METH_SET) {
		setValueOnPath(shv_path, params);
		return true;
	}
	return Super::callMethod(shv_path, method, params, user_id);
}

void RpcValueMapNode::loadValues()
{
	m_valuesLoaded = true;
}

void RpcValueMapNode::saveValues()
{
	emit configSaved();
}

const chainpack::RpcValue &RpcValueMapNode::values()
{
	if(!m_valuesLoaded) {
		loadValues();
	}
	return m_values;
}

chainpack::RpcValue RpcValueMapNode::valueOnPath(const chainpack::RpcValue &val, const ShvNode::StringViewList &shv_path, bool throw_exc)
{
	shv::chainpack::RpcValue v = val;
	for(const auto & dir : shv_path) {
		const shv::chainpack::RpcValue::Map &m = v.asMap();
		v = m.value(std::string{dir});
		if(!v.isValid()) {
			if(throw_exc)
				SHV_EXCEPTION("Invalid path: " + shv_path.join('/'));
			return v;
		}
	}
	return v;
}

chainpack::RpcValue RpcValueMapNode::valueOnPath(const std::string &shv_path, bool throw_exc)
{
	StringViewList lst = shv::core::utils::ShvPath::split(shv_path);
	return  valueOnPath(lst, throw_exc);
}

shv::chainpack::RpcValue RpcValueMapNode::valueOnPath(const shv::iotqt::node::ShvNode::StringViewList &shv_path, bool throw_exc)
{
	return valueOnPath(values(), shv_path, throw_exc);
}

void RpcValueMapNode::setValueOnPath(const std::string &shv_path, const chainpack::RpcValue &val)
{
	StringViewList lst = shv::core::utils::ShvPath::split(shv_path);
	setValueOnPath(lst, val);
}

void RpcValueMapNode::setValueOnPath(const shv::iotqt::node::ShvNode::StringViewList &shv_path, const shv::chainpack::RpcValue &val)
{
	values();
	if(shv_path.empty())
		SHV_EXCEPTION("Empty path");
	shv::chainpack::RpcValue v = values();
	for (size_t i = 0; i < shv_path.size()-1; ++i) {
		auto dir = shv_path.at(i);
		const shv::chainpack::RpcValue::Map &m = v.asMap();
		v = m.value(std::string{dir});
		if(!v.isValid())
			SHV_EXCEPTION("Invalid path: " + shv_path.join('/'));
	}
	v.set(std::string{shv_path.at(shv_path.size() - 1)}, val);
}

void RpcValueMapNode::commitChanges()
{
	saveValues();
}

void RpcValueMapNode::clearValuesCache()
{
	m_valuesLoaded = false;
}

bool RpcValueMapNode::isDir(const shv::iotqt::node::ShvNode::StringViewList &shv_path)
{
	return valueOnPath(shv_path).isMap();
}

//===========================================================
// RpcValueConfigNode
//===========================================================
namespace {
RpcValue mergeMaps(const RpcValue &template_val, const RpcValue &user_val)
{
	if(template_val.isMap() && user_val.isMap()) {
		const shv::chainpack::RpcValue::Map &template_map = template_val.asMap();
		const shv::chainpack::RpcValue::Map &user_map = user_val.asMap();
		RpcValue::Map map = template_map;
		for(const auto &kv : user_map) {
			if(map.hasKey(kv.first)) {
				map[kv.first] = mergeMaps(map.value(kv.first), kv.second);
			}
			else {
				shvWarning() << "user key:" << kv.first << "not found in template map";
			}
		}
		return RpcValue(map);
	}
	return user_val;
}

RpcValue mergeTemplateMaps(const RpcValue &template_base, const RpcValue &template_over)
{
	if(template_over.isMap() && template_base.isMap()) {
		const shv::chainpack::RpcValue::Map &map_base = template_base.asMap();
		const shv::chainpack::RpcValue::Map &map_over = template_over.asMap();
		RpcValue::Map map = map_base;
		for(const auto &kv : map_over) {
			map[kv.first] = mergeTemplateMaps(map.value(kv.first), kv.second);
		}
		return RpcValue(map);
	}
	return template_over;
}

RpcValue diffMaps(const RpcValue &template_vals, const RpcValue &vals)
{
	if(template_vals.isMap() && vals.isMap()) {
		const shv::chainpack::RpcValue::Map &templ_map = template_vals.asMap();
		const shv::chainpack::RpcValue::Map &vals_map = vals.asMap();
		RpcValue::Map map;
		for(const auto &kv : templ_map) {
			RpcValue v = diffMaps(kv.second, vals_map.value(kv.first));
			if(v.isValid())
				map[kv.first] = v;
		}
		return map.empty()? RpcValue(): RpcValue(map);
	}
	if(template_vals == vals)
		return RpcValue();
	return vals;
}

const auto METH_ORIG_VALUE = "origValue";
const auto METH_RESET_TO_ORIG_VALUE = "resetValue";

const std::vector<MetaMethod> meta_methods_root_node {
	shv::chainpack::methods::DIR,
	shv::chainpack::methods::LS,
	{shv::iotqt::node::RpcValueMapNode::M_LOAD, MetaMethod::Flag::None, "", "RpcValue", MetaMethod::AccessLevel::Service},
	{shv::iotqt::node::RpcValueMapNode::M_SAVE, MetaMethod::Flag::None, "RpcValue", "Bool", MetaMethod::AccessLevel::Admin},
	{shv::iotqt::node::RpcValueMapNode::M_COMMIT, MetaMethod::Flag::None, "", "Bool", MetaMethod::AccessLevel::Admin},
};

const std::vector<MetaMethod> meta_methods_node {
	shv::chainpack::methods::DIR,
	shv::chainpack::methods::LS,
	{Rpc::METH_GET, MetaMethod::Flag::IsGetter, "", "RpcValue", MetaMethod::AccessLevel::Config},
	{Rpc::METH_SET, MetaMethod::Flag::IsSetter, "RpcValue", "bool", MetaMethod::AccessLevel::Devel},
	{METH_ORIG_VALUE, MetaMethod::Flag::IsGetter, "", "RpcValue", MetaMethod::AccessLevel::Read},
	{METH_RESET_TO_ORIG_VALUE, MetaMethod::Flag::None, "RpcValue", "Bool", MetaMethod::AccessLevel::Write},
};
}

RpcValueConfigNode::RpcValueConfigNode(const std::string &node_id, ShvNode *parent)
	: Super(node_id, parent)
{
	shvDebug() << "creating: RpcValueConfigNode" << nodeId();
}

size_t RpcValueConfigNode::methodCount(const shv::iotqt::node::ShvNode::StringViewList &shv_path)
{
	if(shv_path.empty()) {
		return meta_methods_root_node.size();
	}

	return isDir(shv_path)? 2: meta_methods_node.size();
}

const shv::chainpack::MetaMethod *RpcValueConfigNode::metaMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, size_t ix)
{
	size_t size;
	const std::vector<shv::chainpack::MetaMethod> &methods = shv_path.empty()? meta_methods_root_node: meta_methods_node;
	if(shv_path.empty()) {
		size = meta_methods_root_node.size();
	}
	else {
		size = isDir(shv_path)? 2: meta_methods_node.size();
	}
	if(size <= ix)
		SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(size));
	return &(methods[ix]);
}

shv::chainpack::RpcValue RpcValueConfigNode::callMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params, const shv::chainpack::RpcValue &user_id)
{
	if(method == METH_ORIG_VALUE) {
		return valueOnPath(m_templateValues, shv_path, shv::core::Exception::Throw);
	}
	if(method == METH_RESET_TO_ORIG_VALUE) {
		RpcValue orig_val = valueOnPath(m_templateValues, shv_path, shv::core::Exception::Throw);
		setValueOnPath(shv_path, orig_val);
		return true;
	}
	if(method == Rpc::METH_SET) {
		if(!params.isValid())
			SHV_EXCEPTION("Invalid value to set on key: " + shv_path.join('/'));
	}
	return Super::callMethod(shv_path, method, params, user_id);
}

shv::chainpack::RpcValue RpcValueConfigNode::loadConfigTemplate(const std::string &file_name)
{
	shvLogFuncFrame() << file_name;
	logConfig() << "Loading template config file:" << file_name;
	std::ifstream is(file_name);
	if(is) {
		CponReader rd(is);
		std::string err;
		shv::chainpack::RpcValue rv = rd.read(&err);
		if(err.empty()) {
			const shv::chainpack::RpcValue::Map &map = rv.asMap();
			static const auto BASED_ON = "basedOn";
			const std::string &based_on = map.valref(BASED_ON).asString();
			if(!based_on.empty()) {
				shvDebug() << "based on:" << based_on;
				std::string base_fn = templateDir() + '/' + based_on;
				shv::chainpack::RpcValue rv2 = loadConfigTemplate(base_fn);
				shv::chainpack::RpcValue::Map m = map;
				m.erase(BASED_ON);
				rv = mergeTemplateMaps(rv2, RpcValue(m));
			}
			shvDebug() << "return:" << rv.toCpon("\t");
			return rv;
		}
		shvWarning() << "Cpon parsing error:" << err << "file:" << file_name;
	}
	else {
		shvWarning() << "Cannot open file:" << file_name;
	}
	return shv::chainpack::RpcValue();
}

std::string RpcValueConfigNode::resolvedUserConfigName() const
{
	if(userConfigName().empty())
		return "user." + configName();
	return userConfigName();
}

std::string RpcValueConfigNode::resolvedUserConfigDir() const
{
	if(userConfigDir().empty())
		return configDir();
	return userConfigDir();
}

void RpcValueConfigNode::loadValues()
{
	Super::loadValues();
	{
		std::string cfg_file = configDir() + '/' + configName();
		logConfig() << "Reading config file:" << cfg_file;
		if(!QFile::exists(QString::fromStdString(cfg_file))) {
			logConfig() << "\t not exists";
			if(!templateDir().empty() && !templateConfigName().empty()) {
				cfg_file = templateDir() + '/' + templateConfigName();
				logConfig() << "Reading config template file" << cfg_file;
			}
		}
		RpcValue val = loadConfigTemplate(cfg_file);
		if(val.isMap()) {
			m_templateValues = val;
		}
		else {
			/// file may not exist
			m_templateValues = RpcValue::Map();
		}
	}
	RpcValue new_values = RpcValue();
	{
		std::string cfg_file = resolvedUserConfigDir() + '/' + resolvedUserConfigName();
		logConfig() << "Reading config user file" << cfg_file;
		std::ifstream is(cfg_file);
		if(is) {
			CponReader rd(is);
			new_values = rd.read();
		}
		if(!new_values.isMap()) {
			/// file may not exist
			new_values = RpcValue::Map();
		}
	}
	m_values = mergeMaps(m_templateValues, new_values);
}

void RpcValueConfigNode::saveValues()
{
	RpcValue new_values = diffMaps(m_templateValues, m_values);
	std::string cfg_file = resolvedUserConfigDir() + '/' + resolvedUserConfigName();
	std::ofstream os(cfg_file);
	if(os) {
		CponWriterOptions opts;
		opts.setIndent("\t");
		CponWriter wr(os, opts);
		wr.write(new_values);
		wr.flush();
		Super::saveValues();
		return;
	}
	SHV_EXCEPTION("Cannot open file '" + cfg_file + "' for writing!");
}

//===========================================================
// PropertyShvNode
//===========================================================
static const std::vector<MetaMethod> meta_methods_pn {
	shv::chainpack::methods::DIR,
	shv::chainpack::methods::LS,
	{Rpc::METH_GET, MetaMethod::Flag::IsGetter, "", "RpcValue", MetaMethod::AccessLevel::Read, {{Rpc::SIG_VAL_CHANGED}}},
	{Rpc::METH_SET, MetaMethod::Flag::IsSetter, "RpcValue", "Bool", MetaMethod::AccessLevel::Write},
};

enum {
	IX_DIR = 0,
	IX_LS,
	IX_GET,
	IX_SET,
	IX_CHNG,
	IX_Count,
};

ValueProxyShvNode::Handle::Handle() = default;

ValueProxyShvNode::Handle::~Handle() = default;

const shv::chainpack::RpcRequest& ValueProxyShvNode::Handle::servedRpcRequest() const
{
	return m_servedRpcRequest;
}

ValueProxyShvNode::ValueProxyShvNode(const std::string &node_id, int value_id, ValueProxyShvNode::Type type, ValueProxyShvNode::Handle *handled_obj, ShvNode *parent)
	: Super(node_id, parent)
	, m_valueId(value_id)
	, m_type(type)
	, m_handledObject(handled_obj)
{
	auto *handled_qobj = dynamic_cast<QObject*>(handled_obj);
	if(handled_qobj) {
		bool ok = connect(handled_qobj, SIGNAL(shvValueChanged(int,shv::chainpack::RpcValue)), this, SLOT(onShvValueChanged(int,shv::chainpack::RpcValue)), Qt::QueuedConnection);
		if(!ok)
			shvWarning() << nodeId() << "cannot connect shvValueChanged signal";
	}
	else {
		shvWarning() << nodeId() << "CHNG notification cannot be delivered, because handle object is not QObject";
	}
}

void ValueProxyShvNode::onShvValueChanged(int value_id, chainpack::RpcValue val)
{
	if(value_id == m_valueId && isSignal()) {
		RpcSignal sig;
		sig.setMethod(Rpc::SIG_VAL_CHANGED);
		sig.setShvPath(shvPath());
		sig.setParams(val);
		emitSendRpcMessage(sig);
	}
}

void ValueProxyShvNode::addMetaMethod(chainpack::MetaMethod &&mm)
{
	m_extraMetaMethods.push_back(std::move(mm));
}

static const std::map<int, std::vector<size_t>> method_indexes = {
	{static_cast<int>(ValueProxyShvNode::Type::Invalid), {IX_DIR, IX_LS} },
	{static_cast<int>(ValueProxyShvNode::Type::Read), {IX_DIR, IX_LS, IX_GET} },
	{static_cast<int>(ValueProxyShvNode::Type::Write), {IX_DIR, IX_LS, IX_SET} },
	{static_cast<int>(ValueProxyShvNode::Type::ReadWrite), {IX_DIR, IX_LS, IX_GET, IX_SET} },
	{static_cast<int>(ValueProxyShvNode::Type::ReadSignal), {IX_DIR, IX_LS, IX_GET, IX_CHNG} },
	{static_cast<int>(ValueProxyShvNode::Type::WriteSignal), {IX_DIR, IX_LS, IX_SET, IX_CHNG} },
	{static_cast<int>(ValueProxyShvNode::Type::Signal), {IX_DIR, IX_LS, IX_CHNG} },
	{static_cast<int>(ValueProxyShvNode::Type::ReadWriteSignal), {IX_DIR, IX_LS, IX_GET, IX_SET, IX_CHNG} },
};

size_t ValueProxyShvNode::methodCount(const ShvNode::StringViewList &shv_path)
{
	if(shv_path.empty()) {
		size_t ret = IX_LS + 1;
		if(isReadable())
			ret++;
		if(isWriteable())
			ret++;
		if(isSignal())
			ret++;
		return ret + m_extraMetaMethods.size();
	}
	return  Super::methodCount(shv_path);
}

const chainpack::MetaMethod *ValueProxyShvNode::metaMethod(const ShvNode::StringViewList &shv_path, size_t ix)
{
	if(shv_path.empty()) {
		const std::vector<size_t> &ixs = method_indexes.at(static_cast<int>(m_type));
		if(ix < ixs.size()) {
			return &(meta_methods_pn[ixs[ix]]);
		}
		size_t extra_ix = ix - ixs.size();
		if(extra_ix < m_extraMetaMethods.size()) {
			return &(m_extraMetaMethods[extra_ix]);
		}
		SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " on path: " + shv_path.join('/'));
	}
	return  Super::metaMethod(shv_path, ix);

}

chainpack::RpcValue ValueProxyShvNode::callMethodRq(const chainpack::RpcRequest &rq)
{
	m_handledObject->m_servedRpcRequest = rq;
	RpcValue ret = Super::callMethodRq(rq);
	m_handledObject->m_servedRpcRequest = RpcRequest();
	return ret;
}

chainpack::RpcValue ValueProxyShvNode::callMethod(const ShvNode::StringViewList &shv_path, const std::string &method, const chainpack::RpcValue &params, const shv::chainpack::RpcValue &user_id)
{
	if(shv_path.empty()) {
		if(method == Rpc::METH_GET) {
			if(isReadable())
				return m_handledObject->shvValue(m_valueId);
			SHV_EXCEPTION("Property " + nodeId() + " on path: " + shv_path.join('/') + " is not readable");
		}
		if(method == Rpc::METH_SET) {
			if(isWriteable()) {
				m_handledObject->setShvValue(m_valueId, params);
				return true;
			}
			SHV_EXCEPTION("Property " + nodeId() + " on path: " + shv_path.join('/') + " is not writeable");
		}
	}
	return  Super::callMethod(shv_path, method, params, user_id);
}

bool ValueProxyShvNode::isWriteable()
{
	return static_cast<int>(m_type) & static_cast<int>(Type::Write);
}

bool ValueProxyShvNode::isReadable()
{
	return static_cast<int>(m_type) & static_cast<int>(Type::Read);
}

bool ValueProxyShvNode::isSignal()
{
	return static_cast<int>(m_type) & static_cast<int>(Type::Signal);
}

}
