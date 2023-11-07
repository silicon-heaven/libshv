#include <iostream>
#pragma once

#ifdef WITH_SHV_LDAP
#include <shv/broker/ldap/ldapconfig.h>
#endif

#include <shv/iotqt/node/shvnode.h>
#include <shv/iotqt/acl/aclroleaccessrules.h>

namespace shv {
namespace broker {

class EtcAclRootNode : public shv::iotqt::node::MethodsTableNode
{
	using Super = shv::iotqt::node::MethodsTableNode;
public:
	EtcAclRootNode(shv::iotqt::node::ShvNode *parent = nullptr);

	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params, const shv::chainpack::RpcValue &user_id) override;
};

class BrokerAclNode : public shv::iotqt::node::MethodsTableNode
{
	using Super = shv::iotqt::node::MethodsTableNode;
public:
	BrokerAclNode(const std::string &config_name, shv::iotqt::node::ShvNode *parent = nullptr);

	virtual std::string saveConfigFile() = 0;
protected:
	virtual const std::vector<shv::chainpack::MetaMethod> *metaMethodsForPath(const StringViewList &shv_path);
	size_t methodCount(const StringViewList &shv_path) override;
	const shv::chainpack::MetaMethod* metaMethod(const StringViewList &shv_path, size_t ix) override;

	std::string saveConfigFile(const std::string &file_name, const shv::chainpack::RpcValue val);
};

class MountsAclNode : public BrokerAclNode
{
	using Super = BrokerAclNode;
public:
	MountsAclNode(shv::iotqt::node::ShvNode *parent = nullptr);

protected:
	StringList childNames(const ShvNode::StringViewList &shv_path) override;
	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params, const shv::chainpack::RpcValue &user_id) override;

	std::string saveConfigFile() override;
};

class RolesAclNode : public BrokerAclNode
{
	using Super = BrokerAclNode;
public:
	RolesAclNode(shv::iotqt::node::ShvNode *parent = nullptr);

protected:
	StringList childNames(const ShvNode::StringViewList &shv_path) override;
	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params, const shv::chainpack::RpcValue &user_id) override;

	std::string saveConfigFile() override;
};

class UsersAclNode : public BrokerAclNode
{
	using Super = BrokerAclNode;
public:
	UsersAclNode(shv::iotqt::node::ShvNode *parent = nullptr);
protected:
	StringList childNames(const ShvNode::StringViewList &shv_path) override;
	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params, const shv::chainpack::RpcValue &user_id) override;

	std::string saveConfigFile() override;
};

class AccessAclNode : public BrokerAclNode
{
	using Super = BrokerAclNode;
public:
	AccessAclNode(shv::iotqt::node::ShvNode *parent = nullptr);

protected:
	StringList childNames(const ShvNode::StringViewList &shv_path) override;
	const std::vector<shv::chainpack::MetaMethod> *metaMethodsForPath(const StringViewList &shv_path) override;
	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params, const shv::chainpack::RpcValue &user_id) override;

	std::string saveConfigFile() override;
private:
	std::string ruleKey(unsigned rule_ix, unsigned rules_cnt, const shv::iotqt::acl::AclAccessRule &rule) const;
	static constexpr auto InvalidIndex = std::numeric_limits<unsigned>::max();
	static unsigned keyToRuleIndex(const std::string &key);
};

#ifdef WITH_SHV_LDAP
class LdapAclNode : public shv::iotqt::node::MethodsTableNode
{
	using Super = shv::iotqt::node::MethodsTableNode;
public:
	LdapAclNode(const LdapConfig& ldap_config, shv::iotqt::node::ShvNode *parent = nullptr);

protected:
	shv::chainpack::RpcValue callMethodRq(const shv::chainpack::RpcRequest &rq) override;
private:
	LdapConfig m_ldapConfig;
};
#endif
}}
