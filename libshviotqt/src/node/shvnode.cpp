#include "shvnode.h"

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
#include <shv/core/stringview.h>
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

//#define logConfig() shvCDebug("Config").color(NecroLog::Color::Yellow)
#define logConfig() shvCMessage("Config")

namespace shv {
namespace iotqt {
namespace node {

//===========================================================
// ShvNode
//===========================================================
std::string ShvNode::LOCAL_NODE_HACK = ".local";
std::string ShvNode::ADD_LOCAL_TO_LS_RESULT_HACK_META_KEY = "__add_local_to_ls_hack";

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

ShvNode::~ShvNode()
{
	/*
	ShvNode *pnd = this->parentNode();
	if(pnd && !pnd->isRootNode() && pnd->ownChildren().isEmpty()) {
		pnd->deleteLater();
	}
	setParentNode(nullptr);
	*/
}

ShvNode *ShvNode::parentNode() const
{
	return qobject_cast<ShvNode*>(parent());
}

ShvNode *ShvNode::childNode(const ShvNode::String &name, bool throw_exc) const
{
	ShvNode *nd = findChild<ShvNode*>(QString::fromStdString(name), Qt::FindDirectChildrenOnly);
	if(throw_exc && !nd)
		SHV_EXCEPTION("Child node id: " + name + " doesn't exist, parent node: " + shvPath());
	return nd;
}

void ShvNode::setParentNode(ShvNode *parent)
{
	setParent(parent);
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
		if(lowest_empty_parent_nd)
			delete lowest_empty_parent_nd;
	}
}

void ShvNode::handleRawRpcRequest(RpcValue::MetaData &&meta, std::string &&data)
{
	shvLogFuncFrame() << "node:" << nodeId() << "meta:" << meta.toPrettyString();
	using ShvPath = shv::core::utils::ShvPath;
	using namespace std;
	using namespace shv::core::utils;
	const chainpack::RpcValue::String method = RpcMessage::method(meta).toString();
	const chainpack::RpcValue::String shv_path_str = RpcMessage::shvPath(meta).toString();
	ShvUrl shv_url(RpcMessage::shvPath(meta).asString());
	core::StringViewList shv_path = ShvPath::split(shv_url.pathPart());
	const bool ls_hook = meta.hasKey(ADD_LOCAL_TO_LS_RESULT_HACK_META_KEY);
	RpcResponse resp = RpcResponse::forRequest(meta);
	try {
		if(!shv_path.empty()) {
			ShvNode *nd = childNode(shv_path.at(0).toString(), !shv::core::Exception::Throw);
			if(nd) {
				shvDebug() << "Child node:" << shv_path.at(0).toString() << "on path:" << shv_path.join('/') << "FOUND";
				std::string new_path = ShvUrl::makeShvUrlString(shv_url.type(),
																shv_url.service(),
																shv_url.fullBrokerId(),
																ShvPath::joinDirs(++shv_path.begin(), shv_path.end()));
				RpcMessage::setShvPath(meta, new_path);
				nd->handleRawRpcRequest(std::move(meta), std::move(data));
				return;
			}
		}
		const chainpack::MetaMethod *mm = metaMethod(shv_path, method);
		if(mm) {
			shvDebug() << "Metamethod:" << method << "on path:" << ShvPath::joinDirs(shv_path) << "FOUND";
			std::string errmsg;
			RpcMessage rpc_msg = RpcDriver::composeRpcMessage(std::move(meta), data, &errmsg);
			if(!errmsg.empty())
				SHV_EXCEPTION(errmsg);

			RpcRequest rq(rpc_msg);
			chainpack::RpcValue ret_val = processRpcRequest(rq);
			if(ret_val.isValid()) {
				resp.setResult(ret_val);
			}
		}
		else {
			string path = shv::core::Utils::joinPath(shvPath(), shv_path_str);
			SHV_EXCEPTION("Method: '" + method + "' on path '" + path + "' doesn't exist");
		}
	}
	catch (const chainpack::RpcException &e) {
		shvDebug() << "method:"  << method << "path:" << shv_path_str << "err code:" << e.errorCode() << "msg:" << e.message();
		RpcResponse::Error err = RpcResponse::Error::create(e.errorCode(), e.message());
		resp.setError(err);
	}
	catch (const std::exception &e) {
		std::string err_str = "method: " + method + " path: " + shv_path_str + " what: " +  e.what();
		shvError() << err_str;
		RpcResponse::Error err = RpcResponse::Error::create(RpcResponse::Error::MethodCallException , err_str);
		resp.setError(err);
	}
	if(resp.hasRetVal()) {
		ShvNode *root = rootNode();
		if(root) {
			if (ls_hook && resp.isSuccess()) {
				chainpack::RpcValue::List res_list = resp.result().asList();
				if (res_list.size() && !res_list[0].isList())
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
		RpcResponse::Error err = RpcResponse::Error::create(e.errorCode(), e.message());
		resp.setError(err);
	}
	catch (const std::exception &e) {
		shvError() << e.what();
		resp.setError(RpcResponse::Error::create(RpcResponse::Error::MethodCallException, e.what()));
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
	RpcResponse resp = RpcResponse::forRequest(rq);
	if(!shv_path.empty()) {
		ShvNode *nd = childNode(shv_path.at(0).toString(), !shv::core::Exception::Throw);
		if(nd) {
			shvDebug() << "Child node:" << shv_path.at(0).toString() << "on path:" << ShvPath::joinDirs(shv_path) << "FOUND";
			std::string new_path = ShvPath::joinDirs(++shv_path.begin(), shv_path.end());
			chainpack::RpcRequest rq2(rq);
			//RpcValue::MetaData meta2(meta);
			rq2.setShvPath(new_path);
			return nd->handleRpcRequestImpl(rq2);
		}
	}
	/*
	const chainpack::MetaMethod *mm = metaMethod(shv_path, method);
	if(!mm) {
		core::utils::ShvPath path = shvPath();
		if(!path.empty() && !shv_path_str.empty())
			path += '/';
		path += shv_path_str;
		SHV_EXCEPTION("Method: '" + method + "' on path '" + path + "' doesn't exist");
	}
	*/
	shvDebug() << "Metamethod:" << method << "on path:" << shv_path.join('/');
	return processRpcRequest(rq);
}

chainpack::RpcValue ShvNode::processRpcRequest(const chainpack::RpcRequest &rq)
{
	shvLogFuncFrame() << rq.shvPath() << rq.method();
	core::StringViewList shv_path = core::utils::ShvPath::split(rq.shvPath().asString());
	const chainpack::RpcValue::String method = rq.method().asString();
	const chainpack::MetaMethod *mm = metaMethod(shv_path, method);
	if(!mm)
		SHV_EXCEPTION(std::string("Method: '") + method + "' on path '" + shvPath() + '/' + rq.shvPath().toString() + "' doesn't exist.");
	const chainpack::RpcValue &rq_grant = rq.accessGrant();
	const RpcValue &mm_grant = mm->accessGrant();
	int rq_access_level = grantToAccessLevel(rq_grant);
	int mm_access_level = grantToAccessLevel(mm_grant);
	if(mm_access_level > rq_access_level)
		SHV_EXCEPTION(std::string("Call method: '") + method + "' on path '" + shvPath() + '/' + rq.shvPath().toString() + "' permission denied, grant: " + rq_grant.toCpon() + " required: " + mm_grant.toCpon());

	if(mm_access_level > MetaMethod::AccessLevel::Write) {


		shv::core::utils::ShvJournalEntry e(shv::core::Utils::joinPath(shvPath(), rq.shvPath().asString())
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
/*
static std::string join_str(const ShvNode::StringList &sl, char sep)
{
	std::string ret;
	for(const std::string &s : sl) {
		if(ret.empty())
			ret = s;
		else
			ret += sep + s;
	}
	return ret;
}
*/
QList<ShvNode *> ShvNode::ownChildren() const
{
	QList<ShvNode*> lst = findChildren<ShvNode*>(QString(), Qt::FindDirectChildrenOnly);
	return lst;
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
		ShvNode *nd = childNode(shv_path.at(0).toString(), !shv::core::Exception::Throw);
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
		ShvNode *nd = childNode(shv_path.at(0).toString(), !shv::core::Exception::Throw);
		if(nd) {
			return nd->hasChildren(StringViewList());
		}
	}
	return !childNames(shv_path).empty();
}

chainpack::RpcValue ShvNode::lsAttributes(const StringViewList &shv_path, unsigned attributes)
{
	shvLogFuncFrame() << "node:" << nodeId() << "attributes:" << attributes << "shv path:" << shv_path.join('/');
	RpcValue::List ret;
	if(attributes & MetaMethod::LsAttribute::HasChildren)
		ret.push_back(hasChildren(shv_path));
	return ret;
}

int ShvNode::basicGrantToAccessLevel(const chainpack::RpcValue &acces_grant)
{
	AccessGrant grant = AccessGrant::fromRpcValue(acces_grant);
	if(grant.isRole()) {
		const char *acces_level = grant.role.data();
		if(std::strcmp(acces_level, Rpc::ROLE_BROWSE) == 0) return MetaMethod::AccessLevel::Browse;
		if(std::strcmp(acces_level, Rpc::ROLE_READ) == 0) return MetaMethod::AccessLevel::Read;
		if(std::strcmp(acces_level, Rpc::ROLE_WRITE) == 0) return MetaMethod::AccessLevel::Write;
		if(std::strcmp(acces_level, Rpc::ROLE_COMMAND) == 0) return MetaMethod::AccessLevel::Command;
		if(std::strcmp(acces_level, Rpc::ROLE_CONFIG) == 0) return MetaMethod::AccessLevel::Config;
		if(std::strcmp(acces_level, Rpc::ROLE_SERVICE) == 0) return MetaMethod::AccessLevel::Service;
		if(std::strcmp(acces_level, Rpc::ROLE_SUPER_SERVICE) == 0) return MetaMethod::AccessLevel::SuperService;
		if(std::strcmp(acces_level, Rpc::ROLE_DEVEL) == 0) return MetaMethod::AccessLevel::Devel;
		if(std::strcmp(acces_level, Rpc::ROLE_ADMIN) == 0) return MetaMethod::AccessLevel::Admin;
		return shv::chainpack::MetaMethod::AccessLevel::None;
	}
	else if(grant.isAccessLevel()) {
		return grant.accessLevel;
	}
	return shv::chainpack::MetaMethod::AccessLevel::None;
}

int ShvNode::grantToAccessLevel(const chainpack::RpcValue &acces_grant) const
{
	return basicGrantToAccessLevel(acces_grant);
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
/*
chainpack::RpcValue ShvNode::call(const std::string &method, const chainpack::RpcValue &params)
{
	shvLogFuncFrame() << "method:" << method << "params:" << params.toCpon() << "shv path:" << shvPath();
	if(method == Rpc::METH_LS) {
		return ls(params);
	}
	if(method == Rpc::METH_DIR) {
		return dir(params);
	}
	SHV_EXCEPTION("Invalid method: " + method + " called for node: " + shvPath());
}
*/
chainpack::RpcValue ShvNode::dir(const StringViewList &shv_path, const chainpack::RpcValue &methods_params)
{
	RpcValue::List ret;
	chainpack::RpcValueGenList params(methods_params);
	const std::string method = params.value(0).toString();
	unsigned attrs = params.value(1).toUInt();
	size_t cnt = methodCount(shv_path);
	for (size_t ix = 0; ix < cnt; ++ix) {
		const chainpack::MetaMethod *mm = metaMethod(shv_path, ix);
		if(method.empty()) {
			ret.push_back(mm->attributes(attrs));
		}
		else if(method == mm->name()) {
				ret.push_back(mm->attributes(attrs));
				break;
		}
	}
	return RpcValue{ret};
}

chainpack::RpcValue ShvNode::ls(const StringViewList &shv_path, const chainpack::RpcValue &methods_params)
{
	//shvInfo() << __FUNCTION__ << "path:" << shvPath() << "shvPath:" << shv_path.join('/');
	RpcValue::List ret;
	chainpack::RpcValueGenList mpl(methods_params);
	const std::string child_name_pattern = mpl.value(0).toString();
	unsigned attrs = mpl.value(1).toUInt();
	for(const std::string &child_name : childNames(shv_path)) {
		//shvInfo() << "\tchild_name:" << child_name;
		if(child_name_pattern.empty() || child_name_pattern == child_name) {
			//try {
				StringViewList ch_shv_path = shv_path;
				ch_shv_path.push_back(shv::core::StringView(child_name));
				RpcValue::List attrs_result = lsAttributes(ch_shv_path, attrs).toList();
				if(attrs_result.empty()) {
					ret.push_back(child_name);
				}
				else {
					attrs_result.insert(attrs_result.begin(), child_name);
					ret.push_back(attrs_result);
				}
			//}
			//catch (std::exception &e) {
			//	shvError() << "ShvNode::ls exception - " + std::string(e.what());
			//	ret.push_back(nullptr);
			//}
		}
	}
	//shvInfo() << "\t return:" << chainpack::RpcValue{ret}.toCpon();
	return chainpack::RpcValue{ret};
}

static std::vector<MetaMethod> meta_methods {
	{Rpc::METH_DIR, MetaMethod::Signature::RetParam, 0, Rpc::ROLE_BROWSE},
	{Rpc::METH_LS, MetaMethod::Signature::RetParam, 0, Rpc::ROLE_BROWSE},
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
/*
ShvNode::StringList ShvNode::methodNames(const StringViewList &shv_path)
{
	ShvNode::StringList ret;
	size_t cnt = methodCount();
	for (size_t ix = 0; ix < cnt; ++ix) {
		ret.push_back(metaMethod(ix)->name());
	}
	return ret;
}
*/

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
	//SHV_EXCEPTION("Cannot find root node!");
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

ShvRootNode::~ShvRootNode()
{
}

//===========================================================
// MethodsTableNode
//===========================================================
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
//static const char M_SIZE[] = "size";
const char *RpcValueMapNode::M_LOAD = "loadFile";
const char *RpcValueMapNode::M_SAVE = "saveFile";
const char *RpcValueMapNode::M_COMMIT = "commitChanges";
//static const char M_RELOAD[] = "serverReload";

static std::vector<MetaMethod> meta_methods_value_map_root_node {
	{Rpc::METH_DIR, MetaMethod::Signature::RetParam, 0, Rpc::ROLE_CONFIG},
	{Rpc::METH_LS, MetaMethod::Signature::RetParam, 0, Rpc::ROLE_CONFIG},
	/// load, save, commit were exposed in value node, do not know why, they should be in config node
	//{RpcValueMapNode::M_LOAD, MetaMethod::Signature::RetVoid, 0, Rpc::ROLE_SERVICE},
	//{RpcValueMapNode::M_SAVE, MetaMethod::Signature::RetVoid, 0, Rpc::ROLE_ADMIN},
	//{RpcValueMapNode::M_COMMIT, MetaMethod::Signature::RetVoid, 0, Rpc::ROLE_ADMIN},
};

static std::vector<MetaMethod> meta_methods_value_map_node {
	{Rpc::METH_DIR, MetaMethod::Signature::RetParam, 0, Rpc::ROLE_READ},
	{Rpc::METH_LS, MetaMethod::Signature::RetParam, 0, Rpc::ROLE_READ},
	{Rpc::METH_GET, MetaMethod::Signature::RetVoid, MetaMethod::Flag::IsGetter, Rpc::ROLE_READ},
	{Rpc::METH_SET, MetaMethod::Signature::RetVoid, MetaMethod::Flag::IsSetter, Rpc::ROLE_CONFIG},
	//{M_WRITE, MetaMethod::Signature::RetParam, 0, MetaMethod::AccessLevel::Service},
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
	else {
		if(isDir(shv_path))
			return 2;
		if(isReadOnly())
			return meta_methods_value_map_node.size() - 1;
	}
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
	for(const auto &kv : val.toMap()) {
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
		const shv::chainpack::RpcValue::Map &m = v.toMap();
		v = m.value(dir.toString());
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
		const shv::chainpack::RpcValue::Map &m = v.toMap();
		v = m.value(dir.toString());
		if(!v.isValid())
			SHV_EXCEPTION("Invalid path: " + shv_path.join('/'));
	}
	v.set(shv_path.at(shv_path.size() - 1).toString(), val);
}

bool RpcValueMapNode::isDir(const shv::iotqt::node::ShvNode::StringViewList &shv_path)
{
	return valueOnPath(shv_path).isMap();
}

//===========================================================
// RpcValueConfigNode
//===========================================================
static RpcValue mergeMaps(const RpcValue &template_val, const RpcValue &user_val)
{
	if(template_val.isMap() && user_val.isMap()) {
		const shv::chainpack::RpcValue::Map &template_map = template_val.toMap();
		const shv::chainpack::RpcValue::Map &user_map = user_val.toMap();
		RpcValue::Map map = template_map;
		//for(const auto &kv : template_map)
		//	map[kv.first] = kv.second;
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

static RpcValue mergeTemplateMaps(const RpcValue &template_base, const RpcValue &template_over)
{
	if(template_over.isMap() && template_base.isMap()) {
		const shv::chainpack::RpcValue::Map &map_base = template_base.toMap();
		const shv::chainpack::RpcValue::Map &map_over = template_over.toMap();
		RpcValue::Map map = map_base;
		//for(const auto &kv : template_map)
		//	map[kv.first] = kv.second;
		for(const auto &kv : map_over) {
			map[kv.first] = mergeTemplateMaps(map.value(kv.first), kv.second);
		}
		return RpcValue(map);
	}
	return template_over;
}

static RpcValue diffMaps(const RpcValue &template_vals, const RpcValue &vals)
{
	if(template_vals.isMap() && vals.isMap()) {
		const shv::chainpack::RpcValue::Map &templ_map = template_vals.toMap();
		const shv::chainpack::RpcValue::Map &vals_map = vals.toMap();
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

static const char METH_ORIG_VALUE[] = "origValue";
static const char METH_RESET_TO_ORIG_VALUE[] = "resetValue";

static std::vector<MetaMethod> meta_methods_root_node {
	{Rpc::METH_DIR, MetaMethod::Signature::RetParam, 0, Rpc::ROLE_CONFIG},
	{Rpc::METH_LS, MetaMethod::Signature::RetParam, 0, Rpc::ROLE_CONFIG},
	{shv::iotqt::node::RpcValueMapNode::M_LOAD, MetaMethod::Signature::RetVoid, 0, Rpc::ROLE_SERVICE},
	{shv::iotqt::node::RpcValueMapNode::M_SAVE, MetaMethod::Signature::RetVoid, 0, Rpc::ROLE_ADMIN},
	{shv::iotqt::node::RpcValueMapNode::M_COMMIT, MetaMethod::Signature::RetVoid, 0, Rpc::ROLE_ADMIN},
};

static std::vector<MetaMethod> meta_methods_node {
	{Rpc::METH_DIR, MetaMethod::Signature::RetParam, 0, Rpc::ROLE_CONFIG},
	{Rpc::METH_LS, MetaMethod::Signature::RetParam, 0, Rpc::ROLE_CONFIG},
	{Rpc::METH_GET, MetaMethod::Signature::RetVoid, MetaMethod::Flag::IsGetter, Rpc::ROLE_CONFIG},
	{Rpc::METH_SET, MetaMethod::Signature::RetVoid, MetaMethod::Flag::IsSetter, Rpc::ROLE_DEVEL},
	{METH_ORIG_VALUE, MetaMethod::Signature::RetVoid, MetaMethod::Flag::IsGetter, Rpc::ROLE_READ},
	{METH_RESET_TO_ORIG_VALUE, MetaMethod::Signature::RetVoid, MetaMethod::Flag::None, Rpc::ROLE_WRITE},
};

RpcValueConfigNode::RpcValueConfigNode(const std::string &node_id, ShvNode *parent)
	: Super(node_id, parent)
{
	shvDebug() << "creating:" << metaObject()->className() << nodeId();
}

size_t RpcValueConfigNode::methodCount(const shv::iotqt::node::ShvNode::StringViewList &shv_path)
{
	if(shv_path.empty()) {
		return meta_methods_root_node.size();
	}
	else {
		return isDir(shv_path)? 2: meta_methods_node.size();
	}
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
			const shv::chainpack::RpcValue::Map &map = rv.toMap();
			static const char BASED_ON[] = "basedOn";
			const std::string based_on = map.value(BASED_ON).asString();
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
		else {
			shvWarning() << "Cpon parsing error:" << err << "file:" << file_name;
		}
	}
	else {
		shvWarning() << "Cannot open file:" << file_name;
	}
	return shv::chainpack::RpcValue();
}

std::string RpcValueConfigNode::resolvedUserConfigName()
{
	if(userConfigName().empty())
		return "user." + configName();
	return userConfigName();
}

std::string RpcValueConfigNode::resolvedUserConfigDir()
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
			//shvWarning() << "Cannot open template config file" << cfg_file << "for reading!";
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
	//shvWarning() << "diff:" << new_values.toPrettyString();
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
// ObjectPropertyProxyShvNode
//===========================================================
static std::vector<MetaMethod> meta_methods_pn {
	{Rpc::METH_DIR, MetaMethod::Signature::RetParam, 0, Rpc::ROLE_BROWSE},
	{Rpc::METH_LS, MetaMethod::Signature::RetParam, 0, Rpc::ROLE_BROWSE},
	{Rpc::METH_GET, MetaMethod::Signature::RetVoid, MetaMethod::Flag::IsGetter, Rpc::ROLE_READ},
	{Rpc::METH_SET, MetaMethod::Signature::RetParam, MetaMethod::Flag::IsSetter, Rpc::ROLE_WRITE},
	{Rpc::SIG_VAL_CHANGED, MetaMethod::Signature::VoidParam, MetaMethod::Flag::IsSignal, Rpc::ROLE_READ},
};

enum {
	IX_DIR = 0,
	IX_LS,
	IX_GET,
	IX_SET,
	IX_CHNG,
	IX_Count,
};

ObjectPropertyProxyShvNode::ObjectPropertyProxyShvNode(const char *property_name, QObject *property_obj, shv::iotqt::node::ShvNode *parent)
	: Super(std::string(property_name), parent)
	, m_propertyObj(property_obj)
{
	shvWarning() << "ObjectPropertyProxyShvNode is deprecated use PropertyShvNode instead.";
	const QMetaObject *mo = m_propertyObj->metaObject();
	m_metaProperty = mo->property(mo->indexOfProperty(property_name));
}

size_t ObjectPropertyProxyShvNode::methodCount(const shv::iotqt::node::ShvNode::StringViewList &shv_path)
{
	if(shv_path.empty()) {
		if(m_metaProperty.isWritable() && m_metaProperty.hasNotifySignal())
			return IX_Count;
		if(m_metaProperty.isWritable() && !m_metaProperty.hasNotifySignal())
			return IX_CHNG;
		if(!m_metaProperty.isWritable() && m_metaProperty.hasNotifySignal())
			return IX_CHNG;
		return IX_SET;
	}
	return  Super::methodCount(shv_path);
}

const shv::chainpack::MetaMethod *ObjectPropertyProxyShvNode::metaMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, size_t ix)
{
	if(shv_path.empty()) {
		if(ix < methodCount(shv_path)) {
			if(!m_metaProperty.isWritable() && m_metaProperty.hasNotifySignal())
				return &(meta_methods_pn[ix+1]);
			return &(meta_methods_pn[ix]);
		}
		else {
			SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " on path: " + shv_path.join('/'));
		}
	}
	return  Super::metaMethod(shv_path, ix);
}

shv::chainpack::RpcValue ObjectPropertyProxyShvNode::callMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params, const chainpack::RpcValue &user_id)
{
	if(shv_path.empty()) {
		if(method == Rpc::METH_GET) {
			QVariant qv = m_propertyObj->property(m_metaProperty.name());
			return shv::coreqt::Utils::qVariantToRpcValue(qv);
		}
		if(method == Rpc::METH_SET) {
			bool ok;
			QVariant qv = shv::coreqt::Utils::rpcValueToQVariant(params, &ok);
			if(ok)
				ok = m_propertyObj->setProperty(m_metaProperty.name(), qv);
			return ok;
		}
	}
	return  Super::callMethod(shv_path, method, params, user_id);
}

//===========================================================
// PropertyShvNode
//===========================================================
ValueProxyShvNode::Handle::~Handle()
{
}

ValueProxyShvNode::ValueProxyShvNode(const std::string &node_id, int value_id, ValueProxyShvNode::Type type, ValueProxyShvNode::Handle *handled_obj, ShvNode *parent)
	: Super(node_id, parent)
	, m_valueId(value_id)
	, m_type(type)
	, m_handledObject(handled_obj)
{
	QObject *handled_qobj = dynamic_cast<QObject*>(handled_obj);
	if(handled_qobj) {
		bool ok = connect(handled_qobj, SIGNAL(shvValueChanged(int, shv::chainpack::RpcValue)), this, SLOT(onShvValueChanged(int, shv::chainpack::RpcValue)), Qt::QueuedConnection);
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

static std::map<int, std::vector<size_t>> method_indexes = {
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
		const std::vector<size_t> &ixs = method_indexes[static_cast<int>(m_type)];
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

}}}
