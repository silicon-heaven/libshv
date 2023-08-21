#include "metamethod.h"

namespace shv::chainpack {
MetaMethod::MetaMethod() = default;

MetaMethod::MetaMethod(std::string name, Signature ms, unsigned flags, const RpcValue &access_grant
					   , const std::string &description, const RpcValue::Map &tags)
	: m_name(std::move(name))
	, m_signature(ms)
	, m_flags(flags)
	, m_accessGrant(access_grant)
	, m_description(description)
	, m_tags(tags)
{
	auto descr = m_tags.take("description").toString();
	if(m_description.empty())
		m_description = descr;
	auto lbl = m_tags.take("label").toString();
	if(m_label.empty())
		m_label = lbl;
	auto access = m_tags.take("access");
	if(!m_accessGrant.isValid())
		m_accessGrant = access;
}

bool MetaMethod::isValid() const
{
	return !name().empty();
}

const std::string& MetaMethod::name() const
{
	return m_name;
}

const std::string& MetaMethod::label() const
{
	return m_label;
}

MetaMethod& MetaMethod::setLabel(const std::string &label)
{
	m_label = label;
	return *this;
}

const std::string& MetaMethod::description() const
{
	return m_description;
}

MetaMethod::Signature MetaMethod::signature() const
{
	return m_signature;
}

unsigned MetaMethod::flags() const
{
	return m_flags;
}

const RpcValue& MetaMethod::accessGrant() const
{
	return m_accessGrant;
}

RpcValue MetaMethod::attributes(unsigned mask) const
{
	RpcValue::List lst;
	if(mask & static_cast<unsigned>(DirAttribute::Signature))
		lst.push_back(static_cast<unsigned>(m_signature));
	if(mask & DirAttribute::Flags)
		lst.push_back(m_flags);
	if(mask & DirAttribute::AccessGrant)
		lst.push_back(m_accessGrant);
	if(mask & DirAttribute::Description)
		lst.push_back(m_description);
	if(mask & DirAttribute::Tags)
		lst.push_back(m_tags);
	if(lst.empty())
		return name();
	lst.insert(lst.begin(), name());
	return RpcValue{lst};
}

const RpcValue::Map& MetaMethod::tags() const
{
	return m_tags;
}

RpcValue MetaMethod::tag(const std::string &key, const RpcValue& default_value) const
{
	return m_tags.value(key, default_value);
}

MetaMethod& MetaMethod::setTag(const std::string &key, const RpcValue& value)
{
	m_tags.setValue(key, value); return *this;
}


RpcValue MetaMethod::toRpcValue() const
{
	RpcValue::Map ret;
	ret["name"] = m_name;
	ret["signature"] = static_cast<int>(m_signature);
	ret["flags"] = m_flags;
	ret["accessGrant"] = m_accessGrant;
	if(!m_label.empty())
		ret["label"] = m_label;
	if(!m_description.empty())
		ret["description"] = m_description;
	RpcValue::Map tags = m_tags;
	ret.merge(tags);
	return ret;
}

MetaMethod MetaMethod::fromRpcValue(const RpcValue &rv)
{
	MetaMethod ret;
	ret.applyAttributesMap(rv.asMap());
	return ret;
}

void MetaMethod::applyAttributesMap(const RpcValue::Map &attr_map)
{
	RpcValue::Map map = attr_map;
	RpcValue::Map tags = map.take("tags").asMap();
	map.merge(tags);
	if(auto rv = map.take("name"); rv.isValid())
		m_name = rv.asString();
	if(auto rv = map.take("signature"); rv.isValid())
		m_signature = static_cast<Signature>(rv.toInt());
	if(auto rv = map.take("flags"); rv.isValid())
		m_flags = rv.toInt();
	if(auto rv = map.take("access"); rv.isValid())
		m_accessGrant = rv.asString();
	if(auto rv = map.take("accessGrant"); rv.isValid())
		m_accessGrant = rv.asString();
	if(auto rv = map.take("label"); rv.isValid())
		m_label = rv.asString();
	if(auto rv = map.take("description"); rv.isValid())
		m_description = rv.asString();
	m_tags = map;
}

MetaMethod::Signature MetaMethod::signatureFromString(const std::string &sigstr)
{
	if(sigstr == "VoidParam") return Signature::VoidParam;
	if(sigstr == "RetVoid") return Signature::RetVoid;
	if(sigstr == "RetParam") return Signature::RetParam;
	return Signature::VoidVoid;
}

const char *MetaMethod::signatureToString(MetaMethod::Signature sig)
{
	switch(sig) {
	case Signature::VoidVoid: return "VoidVoid";
	case Signature::VoidParam: return "VoidParam";
	case Signature::RetVoid: return "RetVoid";
	case Signature::RetParam: return "RetParam";
	}
	return "";
}

std::string MetaMethod::flagsToString(unsigned flags)
{
	std::string ret;
	auto add_str = [&ret](const char *str) {
		if(ret.empty()) {
			ret = str;
		}
		else {
			ret += ',';
			ret += str;
		}
		return ret;
	};
	if(flags & Flag::IsGetter)
		add_str("Getter");
	if(flags & Flag::IsSetter)
		add_str("Setter");
	if(flags & Flag::IsSignal)
		add_str("Signal");
	return ret;
}

const char *MetaMethod::accessLevelToString(int access_level)
{
	switch(access_level) {
	case AccessLevel::None: return "";
	case AccessLevel::Browse: return shv::chainpack::Rpc::ROLE_BROWSE;
	case AccessLevel::Read: return shv::chainpack::Rpc::ROLE_READ;
	case AccessLevel::Write: return shv::chainpack::Rpc::ROLE_WRITE;
	case AccessLevel::Command: return shv::chainpack::Rpc::ROLE_COMMAND;
	case AccessLevel::Config: return shv::chainpack::Rpc::ROLE_CONFIG;
	case AccessLevel::Service: return shv::chainpack::Rpc::ROLE_SERVICE;
	case AccessLevel::SuperService: return shv::chainpack::Rpc::ROLE_SUPER_SERVICE;
	case AccessLevel::Devel: return shv::chainpack::Rpc::ROLE_DEVEL;
	case AccessLevel::Admin: return shv::chainpack::Rpc::ROLE_ADMIN;
	}
	return "";
}


} // namespace shv
