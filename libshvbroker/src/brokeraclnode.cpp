#include "brokeraclnode.h"
#include "brokerapp.h"

#include <shv/core/utils/shvpath.h>
#include <shv/chainpack/cponwriter.h>
#include <shv/chainpack/metamethod.h>
#include <shv/chainpack/rpc.h>
#include <shv/core/string.h>
#include <shv/core/log.h>
#include <shv/core/exception.h>
#include <shv/iotqt/acl/aclroleaccessrules.h>

#include <regex>
#include <fstream>

namespace cp = shv::chainpack;
namespace acl = shv::iotqt::acl;

namespace shv::broker {

//========================================================
// EtcAclNode
//========================================================
static std::vector<cp::MetaMethod> meta_methods_dir_ls {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
};

static std::vector<cp::MetaMethod> meta_methods_property {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{cp::Rpc::METH_GET, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_READ},
};

static std::vector<cp::MetaMethod> meta_methods_property_rw {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{cp::Rpc::METH_GET, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_READ},
	{cp::Rpc::METH_SET, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsSetter, cp::Rpc::ROLE_CONFIG},
};

static const std::string M_VALUE = "value";
static const std::string M_SET_VALUE = "setValue";
static const std::string M_SAVE_TO_CONFIG_FILE = "saveToConfigFile";

static std::vector<cp::MetaMethod> meta_methods_acl_node {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{M_SET_VALUE, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_CONFIG},
	{M_SAVE_TO_CONFIG_FILE, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_CONFIG},
};

static std::vector<cp::MetaMethod> meta_methods_acl_subnode {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{M_VALUE, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
};

static const std::string M_SAVE_TO_CONFIG_FILES = "saveToConfigFiles";
static std::vector<cp::MetaMethod> meta_methods_acl_root {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{M_SAVE_TO_CONFIG_FILES, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_CONFIG},
};

//========================================================
// EtcAclRootNode
//========================================================
EtcAclRootNode::EtcAclRootNode(shv::iotqt::node::ShvNode *parent)
	: Super("acl", &meta_methods_acl_root, parent)
{
	setSortedChildren(false);
	{
		new MountsAclNode(this);
	}
	{
		new UsersAclNode(this);
	}
	{
		new RolesAclNode(this);
	}
	{
		new AccessAclNode(this);
	}
}

chainpack::RpcValue EtcAclRootNode::callMethod(const iotqt::node::ShvNode::StringViewList &shv_path, const std::string &method, const chainpack::RpcValue &params, const chainpack::RpcValue &user_id)
{
	if(shv_path.empty()) {
		if(method == M_SAVE_TO_CONFIG_FILES) {
			cp::RpcValue::List ret;
			for(auto *nd : findChildren<BrokerAclNode*>(QString(), Qt::FindDirectChildrenOnly))
				ret.push_back(nd->saveConfigFile());
			return ret;
		}
	}
	return Super::callMethod(shv_path, method, params, user_id);
}

//========================================================
// BrokerAclNode
//========================================================
BrokerAclNode::BrokerAclNode(const std::string &config_name, shv::iotqt::node::ShvNode *parent)
	: Super(config_name, &meta_methods_dir_ls, parent)
{
}

std::vector<chainpack::MetaMethod> *BrokerAclNode::metaMethodsForPath(const iotqt::node::ShvNode::StringViewList &shv_path)
{
	if(shv_path.size() == 0)
		return &meta_methods_acl_node;
	if(shv_path.size() == 1)
		return &meta_methods_acl_subnode;
	if(shv_path.size() == 2)
		return &meta_methods_property_rw;
	return &meta_methods_dir_ls;
}

size_t BrokerAclNode::methodCount(const iotqt::node::ShvNode::StringViewList &shv_path)
{
	return metaMethodsForPath(shv_path)->size();
}

const chainpack::MetaMethod *BrokerAclNode::metaMethod(const iotqt::node::ShvNode::StringViewList &shv_path, size_t ix)
{
	std::vector<cp::MetaMethod> *mm = metaMethodsForPath(shv_path);
	if(ix < mm->size())
		return &mm->at(ix);
	SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " on shv path: " + shv_path.join('/'));
}

std::string BrokerAclNode::saveConfigFile(const std::string &file_name, const chainpack::RpcValue val)
{
	BrokerApp *app = BrokerApp::instance();
	std::string fn = app->cliOptions()->configDir() + '/' + file_name;
	std::ofstream os(fn, std::ios::out | std::ios::binary);
	if(!os)
		SHV_EXCEPTION("Cannot open file: '" + fn + "' for writing.");
	cp::CponWriterOptions opts;
	opts.setIndent("\t");
	cp::CponWriter wr(os, opts);
	wr << val;
	return fn;
}

//========================================================
// MountsAclNode
//========================================================
static const std::string ACL_MOUNTS_DESCR = "description";
static const std::string ACL_MOUNTS_MOUNT_POINT = "mountPoint";

MountsAclNode::MountsAclNode(iotqt::node::ShvNode *parent)
	: Super("mounts", parent)
{

}

iotqt::node::ShvNode::StringList MountsAclNode::childNames(const iotqt::node::ShvNode::StringViewList &shv_path)
{
	if(shv_path.empty()) {
		BrokerApp *app = BrokerApp::instance();
		AclManager *mng = app->aclManager();
		return mng->mountDeviceIds();
	}
	if(shv_path.size() == 1) {
		return iotqt::node::ShvNode::StringList{ACL_MOUNTS_DESCR, ACL_MOUNTS_MOUNT_POINT};
	}
	return Super::childNames(shv_path);
}

chainpack::RpcValue MountsAclNode::callMethod(const iotqt::node::ShvNode::StringViewList &shv_path, const std::string &method, const chainpack::RpcValue &params, const chainpack::RpcValue &user_id)
{
	if(shv_path.empty()) {
		if(method == M_SET_VALUE) {
			if(params.isList()) {
				const auto &lst = params.asList();
				const std::string dev_id = lst.value(0).asString();
				chainpack::RpcValue rv = lst.value(1);
				auto v = acl::AclMountDef::fromRpcValue(rv);
				if(rv.isValid() && !rv.isNull() && !v.isValid())
					throw shv::core::Exception("Invalid device ID: " + dev_id + " definition: " + rv.toCpon());
				AclManager *mng = BrokerApp::instance()->aclManager();
				mng->setMountDef(dev_id, v);
				return true;
			}
			SHV_EXCEPTION("Invalid parameters, method: " + method);
		}
		if(method == M_SAVE_TO_CONFIG_FILE) {
			return saveConfigFile();
		}
	}
	else if(shv_path.size() == 1) {
		if(method == M_VALUE) {
			AclManager *mng = BrokerApp::instance()->aclManager();
			auto v = mng->mountDef(shv_path.value(0).toString());
			return v.toRpcValue();
		}
	}
	else if(shv_path.size() == 2) {
		if(method == cp::Rpc::METH_GET) {
			AclManager *mng = BrokerApp::instance()->aclManager();
			acl::AclMountDef u = mng->mountDef(shv_path.value(0).toString());
			auto pn = shv_path.value(1);
			if(pn == ACL_MOUNTS_DESCR)
				return u.description;
			if(pn == ACL_MOUNTS_MOUNT_POINT)
				return u.mountPoint;
		}
		if(method == cp::Rpc::METH_SET) {
			AclManager *mng = BrokerApp::instance()->aclManager();
			std::string device_id = shv_path.value(0).toString();
			acl::AclMountDef md = mng->mountDef(device_id);
			auto pn = shv_path.value(1);
			if(pn == ACL_MOUNTS_DESCR) {
				md.description = params.toString();
				return callMethod(StringViewList{}, M_SET_VALUE, cp::RpcValue::List{device_id, md.toRpcValue()}, user_id);
			}
			if(pn == ACL_MOUNTS_MOUNT_POINT) {
				md.mountPoint = params.toString();
				return callMethod(StringViewList{}, M_SET_VALUE, cp::RpcValue::List{device_id, md.toRpcValue()}, user_id);
			}
		}
	}
	return Super::callMethod(shv_path, method, params, user_id);
}

std::string MountsAclNode::saveConfigFile()
{
	cp::RpcValue::Map m;
	AclManager *mng = BrokerApp::instance()->aclManager();
	for(const std::string &n : childNames(StringViewList{})) {
		auto md = mng->mountDef(n);
		m[n] = md.toRpcValue();
	}
	return Super::saveConfigFile("mounts.cpon", m);
}

//========================================================
// RolesAclNode
//========================================================
static const std::string ACL_ROLE_WEIGHT = "weight";
static const std::string ACL_ROLE_ROLES = "roles";
static const std::string ACL_ROLE_PROFILE = "profile";

RolesAclNode::RolesAclNode(shv::iotqt::node::ShvNode *parent)
	: Super("roles", parent)
{
}

iotqt::node::ShvNode::StringList RolesAclNode::childNames(const iotqt::node::ShvNode::StringViewList &shv_path)
{
	if(shv_path.empty()) {
		BrokerApp *app = BrokerApp::instance();
		AclManager *mng = app->aclManager();
		return mng->roles();
	}
	if(shv_path.size() == 1) {
		return iotqt::node::ShvNode::StringList{ACL_ROLE_WEIGHT, ACL_ROLE_ROLES, ACL_ROLE_PROFILE};
	}
	return Super::childNames(shv_path);
}

chainpack::RpcValue RolesAclNode::callMethod(const iotqt::node::ShvNode::StringViewList &shv_path, const std::string &method, const chainpack::RpcValue &params, const chainpack::RpcValue &user_id)
{
	if(shv_path.empty()) {
		if(method == M_SET_VALUE) {
			if(params.isList()) {
				const auto &lst = params.asList();
				const std::string role_name = lst.value(0).asString();

				chainpack::RpcValue rv = lst.value(1);
				auto v = acl::AclRole::fromRpcValue(rv);
				AclManager *mng = BrokerApp::instance()->aclManager();
				mng->setRole(role_name, v);
				return true;
			}
			SHV_EXCEPTION("Invalid parameters, method: " + method);
		}
		if(method == M_SAVE_TO_CONFIG_FILE) {
			return saveConfigFile();
		}
	}
	else if(shv_path.size() == 1) {
		if(method == M_VALUE) {
			AclManager *mng = BrokerApp::instance()->aclManager();
			auto v = mng->role(shv_path.value(0).toString());
			return v.toRpcValue();
		}
	}
	else if(shv_path.size() == 2) {
		std::string role_name = shv_path.value(0).toString();
		auto pn = shv_path.value(1);
		AclManager *mng = BrokerApp::instance()->aclManager();
		acl::AclRole role_def = mng->role(role_name);
		if(method == cp::Rpc::METH_GET) {
			if(pn == ACL_ROLE_WEIGHT)
				return role_def.weight;
			if(pn == ACL_ROLE_ROLES)
				return shv::chainpack::RpcValue::List::fromStringList(role_def.roles);
			if(pn == ACL_ROLE_PROFILE)
				return role_def.profile;
		}
		if(method == cp::Rpc::METH_SET) {
			if(pn == ACL_ROLE_WEIGHT) {
				role_def.weight = params.toInt();
				return callMethod(StringViewList{}, M_SET_VALUE, cp::RpcValue::List{role_name, role_def.toRpcValue()}, user_id);
			}
			if(pn == ACL_ROLE_ROLES) {
				role_def.roles.clear();
				for(const auto &rv : params.asList())
					role_def.roles.push_back(rv.toString());
				return callMethod(StringViewList{}, M_SET_VALUE, cp::RpcValue::List{role_name, role_def.toRpcValue()}, user_id);
			}
			if(pn == ACL_ROLE_PROFILE) {
				role_def.profile = params;
				return callMethod(StringViewList{}, M_SET_VALUE, cp::RpcValue::List{role_name, role_def.toRpcValue()}, user_id);
			}
		}
	}
	return Super::callMethod(shv_path, method, params, user_id);
}

std::string RolesAclNode::saveConfigFile()
{
	cp::RpcValue::Map m;
	AclManager *mng = BrokerApp::instance()->aclManager();
	for(const std::string &n : childNames(StringViewList{})) {
		auto role = mng->role(n);
		m[n] = role.toRpcValue();
	}
	return Super::saveConfigFile("roles.cpon", m);
}
//========================================================
// UsersAclNode
//========================================================
static const std::string ACL_USER_PASSWORD = "password";
static const std::string ACL_USER_ROLES = "roles";
static const std::string ACL_USER_PASSWORD_FORMAT = "passwordFormat";

UsersAclNode::UsersAclNode(shv::iotqt::node::ShvNode *parent)
	: Super("users", parent)
{
}

iotqt::node::ShvNode::StringList UsersAclNode::childNames(const iotqt::node::ShvNode::StringViewList &shv_path)
{
	if(shv_path.empty()) {
		BrokerApp *app = BrokerApp::instance();
		AclManager *mng = app->aclManager();
		return mng->users();
	}
	if(shv_path.size() == 1) {
		return iotqt::node::ShvNode::StringList{ACL_USER_PASSWORD, ACL_USER_PASSWORD_FORMAT, ACL_USER_ROLES};
	}
	return Super::childNames(shv_path);
}

chainpack::RpcValue UsersAclNode::callMethod(const iotqt::node::ShvNode::StringViewList &shv_path, const std::string &method, const chainpack::RpcValue &params, const chainpack::RpcValue &user_id)
{
	if(shv_path.empty()) {
		if(method == M_SET_VALUE) {
			if(params.isList()) {
				const auto &lst = params.asList();
				const std::string name = lst.value(0).asString();
				chainpack::RpcValue rv = lst.value(1);
				auto user = acl::AclUser::fromRpcValue(rv);
				if(rv.isValid() && !rv.isNull() && !user.isValid())
					throw shv::core::Exception("Invalid user: " + name + " definition: " + rv.toCpon());
				AclManager *mng = BrokerApp::instance()->aclManager();
				mng->setUser(name, user);
				return true;
			}
			SHV_EXCEPTION("Invalid parameters, method: " + method);
		}
		if(method == M_SAVE_TO_CONFIG_FILE) {
			return saveConfigFile();
		}
	}
	else if(shv_path.size() == 1) {
		if(method == M_VALUE) {
			AclManager *mng = BrokerApp::instance()->aclManager();
			acl::AclUser u = mng->user(shv_path.value(0).toString());
			return u.toRpcValue();
		}
	}
	else if(shv_path.size() == 2) {
		std::string user_name = shv_path.value(0).toString();
		auto pn = shv_path.value(1);
		AclManager *mng = BrokerApp::instance()->aclManager();
		acl::AclUser user_def = mng->user(user_name);
		if(method == cp::Rpc::METH_GET) {
			if(pn == ACL_USER_PASSWORD)
				return user_def.password.password;
			if(pn == ACL_USER_PASSWORD_FORMAT)
				return acl::AclPassword::formatToString(user_def.password.format);
			if(pn == ACL_USER_ROLES)
				return shv::chainpack::RpcValue::List::fromStringList(user_def.roles);
		}
		if(method == cp::Rpc::METH_SET) {
			if(pn == ACL_USER_PASSWORD) {
				user_def.password.password = params.toString();
				return callMethod(StringViewList{}, M_SET_VALUE, cp::RpcValue::List{user_name, user_def.toRpcValue()}, user_id);
			}
			if(pn == ACL_USER_PASSWORD_FORMAT) {
				user_def.password.format = acl::AclPassword::formatFromString(params.asString());
				return callMethod(StringViewList{}, M_SET_VALUE, cp::RpcValue::List{user_name, user_def.toRpcValue()}, user_id);
			}
			if(pn == ACL_USER_ROLES) {
				user_def.roles.clear();
				for(const auto &rv : params.asList())
					user_def.roles.push_back(rv.toString());
				return callMethod(StringViewList{}, M_SET_VALUE, cp::RpcValue::List{user_name, user_def.toRpcValue()}, user_id);
			}
		}
	}
	return Super::callMethod(shv_path, method, params, user_id);
}

std::string UsersAclNode::saveConfigFile()
{
	cp::RpcValue::Map m;
	AclManager *mng = BrokerApp::instance()->aclManager();
	for(const std::string &n : childNames(StringViewList{})) {
		auto user = mng->user(n);
		m[n] = user.toRpcValue();
	}
	return Super::saveConfigFile("users.cpon", m);
}

//========================================================
// AccessAclNode
//========================================================

static const std::string M_GET_GRANT = "grant";
static const std::string M_GET_GRANT_TYPE = "grantType";
static const std::string M_GET_PATH_PATTERN = "pathPattern";
static const std::string M_GET_METHOD = "method";

static const std::string M_SET_GRANT = "setGrant";
static const std::string M_SET_PATH_PATTERN = "setPathPattern";
static const std::string M_SET_METHOD = "setMethod";

static std::vector<cp::MetaMethod> meta_methods_role_access {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},

	{M_GET_PATH_PATTERN, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_READ},
	{M_SET_PATH_PATTERN, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::IsSetter, cp::Rpc::ROLE_CONFIG},
	{M_GET_METHOD, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_READ},
	{M_SET_METHOD, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::IsSetter, cp::Rpc::ROLE_CONFIG},
	{M_GET_GRANT, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_READ},
	{M_SET_GRANT, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::IsSetter, cp::Rpc::ROLE_CONFIG},
	{M_GET_GRANT_TYPE, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_READ},
};

AccessAclNode::AccessAclNode(shv::iotqt::node::ShvNode *parent)
	: Super("access", parent)
{

}

std::vector<chainpack::MetaMethod> *AccessAclNode::metaMethodsForPath(const iotqt::node::ShvNode::StringViewList &shv_path)
{
	if(shv_path.size() == 0)
		return &meta_methods_acl_node;
	if(shv_path.size() == 1)
		return &meta_methods_acl_subnode;
	if(shv_path.size() == 2)
		return &meta_methods_role_access;
	return &meta_methods_dir_ls;
}

iotqt::node::ShvNode::StringList AccessAclNode::childNames(const iotqt::node::ShvNode::StringViewList &shv_path)
{
	if(shv_path.empty()) {
		AclManager *mng = BrokerApp::instance()->aclManager();
		return mng->accessRoles();
	}
	if(shv_path.size() == 1) {
		AclManager *mng = BrokerApp::instance()->aclManager();
		const acl::AclRoleAccessRules role_rules = mng->accessRoleRules(shv_path.value(0).toString());
		iotqt::node::ShvNode::StringList ret;
		unsigned ix = 0;
		for(const auto &rule : role_rules) {
			ret.push_back('\'' + ruleKey(ix++, static_cast<unsigned int>(role_rules.size()), rule) + '\'');
		}
		return ret;
	}
	return Super::childNames(shv_path);
}

chainpack::RpcValue AccessAclNode::callMethod(const iotqt::node::ShvNode::StringViewList &shv_path, const std::string &method, const chainpack::RpcValue &params, const chainpack::RpcValue &user_id)
{
	if(shv_path.empty()) {
		if(method == M_SET_VALUE) {
			if(params.isList()) {
				const auto &lst = params.asList();
				const std::string role_name = lst.value(0).asString();
				chainpack::RpcValue rv = lst.value(1);
				auto v = acl::AclRoleAccessRules::fromRpcValue(rv);
				AclManager *mng = BrokerApp::instance()->aclManager();
				mng->setAccessRoleRules(role_name, v);
				return true;
			}
			SHV_EXCEPTION("Invalid parameters, method: " + method);
		}
		if(method == M_SAVE_TO_CONFIG_FILE) {
			return saveConfigFile();
		}
	}
	else if(shv_path.size() == 1) {
		if(method == M_VALUE) {
			AclManager *mng = BrokerApp::instance()->aclManager();
			auto v = mng->accessRoleRules(shv_path.value(0).toString());
			return v.toRpcValue();
		}
	}
	else if(shv_path.size() == 2) {
		AclManager *mng = BrokerApp::instance()->aclManager();
		std::string rule_name = shv_path.value(0).toString();
		acl::AclRoleAccessRules role_rules = mng->accessRoleRules(rule_name);
		std::string key = shv_path.value(1).toString();
		if(key.size() > 1 && key[0] == '\'' && key[key.size() - 1] == '\'')
			key = key.substr(1, key.size() - 2);
		unsigned i = keyToRuleIndex(key);
		if(i >= role_rules.size())
			SHV_EXCEPTION("Invalid access rule key: " + key);
		auto &rule = role_rules[i];

		if(method == M_GET_PATH_PATTERN)
			return rule.pathPattern;
		if(method == M_GET_METHOD)
			return rule.method;
		if(method == M_GET_GRANT)
			return rule.grant.toRpcValue();
		if(method == M_GET_GRANT_TYPE)
			return cp::AccessGrant::typeToString(rule.grant.type);

		using namespace shv::core;
		if(method == M_SET_PATH_PATTERN) {
			rule.pathPattern = params.toString();
			return callMethod(StringViewList{}, M_SET_VALUE, cp::RpcValue::List{rule_name, rule.toRpcValue()}, user_id);
		}
		if(method == M_SET_METHOD) {
			rule.method = params.toString();
			return callMethod(StringViewList{}, M_SET_VALUE, cp::RpcValue::List{rule_name, rule.toRpcValue()}, user_id);
		}
		if(method == M_SET_GRANT) {
			rule.grant = cp::AccessGrant::fromRpcValue(params);
			return callMethod(StringViewList{}, M_SET_VALUE, cp::RpcValue::List{rule_name, rule.toRpcValue()}, user_id);
		}
	}
	return Super::callMethod(shv_path, method, params, user_id);
}

std::string AccessAclNode::saveConfigFile()
{
	cp::RpcValue::Map m;
	AclManager *mng = BrokerApp::instance()->aclManager();
	for(const std::string &n : childNames(StringViewList{})) {
		auto rules = mng->accessRoleRules(n);
		m[n] = rules.toRpcValue();
	}
	return Super::saveConfigFile("access.cpon", m);
}

std::string AccessAclNode::ruleKey(unsigned rule_ix, unsigned rules_cnt, const acl::AclAccessRule &rule) const
{
	int width = 1;
	while (rules_cnt > 9) {
		width++;
		rules_cnt /= 10;
	}
	std::string ret = QStringLiteral("[%1] ").arg(rule_ix, width, 10, QChar('0')).toStdString();
	ret += rule.pathPattern;
	if(!rule.method.empty())
		ret += core::utils::ShvPath::SHV_PATH_METHOD_DELIM + rule.method;
	return ret;
}

unsigned AccessAclNode::keyToRuleIndex(const std::string &key)
{
	static const std::regex color_regex(R"RX(^\[([0-9]+)\])RX");
	// show contents of marked subexpressions within each match
	std::smatch color_match;
	if(std::regex_search(key, color_match, color_regex)) {
		if (color_match.size() > 1) {
			bool ok;
			unsigned ix = static_cast<unsigned>(shv::core::String::toInt(color_match[1], &ok));
			if(ok)
				return ix;
		}
	}
	return  InvalidIndex;
}

}
