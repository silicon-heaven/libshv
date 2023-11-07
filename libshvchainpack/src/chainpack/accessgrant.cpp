#include "accessgrant.h"
#include "irpcconnection.h"

#include <necrolog.h>

namespace shv {
namespace chainpack {

static bool str_eq(const std::string &s1, const char *s2)
{
	size_t i;
	for (i = 0; i < s1.size(); ++i) {
		char c2 = s2[i];
		if(!c2)
			return false;
		if(toupper(c2 != toupper(s1[i])))
			return false;
	}
	return s2[i] == 0;
}

//================================================================
// UserLoginContext
//================================================================
const RpcValue::Map& UserLoginContext::loginParams() const
{
	return loginRequest.params().asMap();
}

UserLogin UserLoginContext::userLogin() const
{
	return UserLogin::fromRpcValue(loginParams().value(Rpc::KEY_LOGIN));
}

//================================================================
// UserLogin
//================================================================
const char* UserLogin::loginTypeToString(UserLogin::LoginType t)
{
	switch(t) {
	case LoginType::Plain: return "PLAIN";
	case LoginType::Sha1: return "SHA1";
	case LoginType::RsaOaep: return "RSAOAEP";
	default: return "INVALID";
	}
}

UserLogin::LoginType UserLogin::loginTypeFromString(const std::string &s)
{
	if(str_eq(s, loginTypeToString(LoginType::Plain)))
		return LoginType::Plain;
	if(str_eq(s, loginTypeToString(LoginType::Sha1)))
		return LoginType::Sha1;
	if(str_eq(s, loginTypeToString(LoginType::RsaOaep)))
		return LoginType::RsaOaep;
	return LoginType::Invalid;
}

RpcValue UserLogin::toRpcValueMap() const
{
	RpcValue::Map ret;
	ret["user"] = user;
	ret["password"] = password;
	ret["loginType"] = loginTypeToString(loginType);
	return RpcValue(std::move(ret));
}

UserLogin UserLogin::fromRpcValue(const RpcValue &val)
{
	UserLogin ret;
	if(val.isMap()) {
		const RpcValue::Map &m = val.asMap();
		ret.user = m.value("user").asString();
		ret.password = m.value("password").asString();
		const RpcValue::String lts = m.value("type").asString();
		if(lts.empty())
			ret.loginType = LoginType::Sha1;
		else
			ret.loginType = loginTypeFromString(lts);
	}
	return ret;
}

//================================================================
// UserLoginResult
//================================================================
RpcValue UserLoginResult::toRpcValue() const
{
	RpcValue::Map m;
	if(passwordOk) {
		if(clientId > 0)
			m["clientId"] = clientId;
	}
	return RpcValue(std::move(m));
}

//================================================================
// AccessGrant
//================================================================
AccessGrant::MetaType::MetaType()
	: Super("AccessGrant")
{
	m_keys = {
		RPC_META_KEY_DEF(Type),
		RPC_META_KEY_DEF(NotResolved),
		RPC_META_KEY_DEF(Role),
		RPC_META_KEY_DEF(AccessLevel),
		RPC_META_KEY_DEF(User),
		RPC_META_KEY_DEF(Password),
		RPC_META_KEY_DEF(LoginType),
	};
	//m_tags = {
	//	RPC_META_TAG_DEF(shvPath)
	//};
}

void AccessGrant::MetaType::registerMetaType()
{
	static bool is_init = false;
	if(!is_init) {
		is_init = true;
		static MetaType s;
		shv::chainpack::meta::registerType(shv::chainpack::meta::GlobalNS::ID, MetaType::ID, &s);
	}
}

bool AccessGrant::isValid() const
{
	return type != Type::Invalid;
}

bool AccessGrant::isUserLogin() const
{
	return type == Type::UserLogin;
}

bool AccessGrant::isRole() const
{
	return type == Type::Role;
}

bool AccessGrant::isAccessLevel() const
{
	return type == Type::AccessLevel;
}

RpcValue AccessGrant::toRpcValue() const
{
	if(!isValid())
		return RpcValue();
	if(isAccessLevel())
		return RpcValue(accessLevel);
	if(isRole())
		return RpcValue(role);
	RpcValue ret(RpcValue::IMap{});
	MetaType::registerMetaType();
	ret.setMetaValue(chainpack::meta::Tag::MetaTypeId, MetaType::ID);
	ret.set(MetaType::Key::Type, (int)type);
	switch (type) {
	case Type::AccessLevel:
		ret.set(MetaType::Key::AccessLevel, accessLevel);
		break;
	case Type::Role:
		ret.set(MetaType::Key::Role, role);
		break;
	case Type::UserLogin:
		ret.set(MetaType::Key::User, login.user);
		ret.set(MetaType::Key::Password, login.password);
		ret.set(MetaType::Key::LoginType, (int)login.loginType);
		break;
	case Type::Invalid:
		break;
	}
	return ret;
}

static const std::string KEY_TYPE = "type";
static const std::string KEY_ACCESS_LEVEL = "accessLevel";
static const std::string KEY_ROLE = "role";
//static const std::string KEY_USER = "user";
//static const std::string KEY_PASSWORD = "password";
static const std::string KEY_LOGIN = "login";
static const std::string KEY_NOT_RESOLVED = "notResolved";

RpcValue AccessGrant::toRpcValueMap() const
{
	RpcValue::Map ret;
	switch (type) {
	case Type::AccessLevel:
		ret[KEY_ACCESS_LEVEL] = accessLevel;
		break;
	case Type::Role:
		ret[KEY_ROLE] = role;
		break;
	case Type::UserLogin:
		ret[KEY_LOGIN] = login.toRpcValueMap();
		break;
	case Type::Invalid:
		break;
	}
	return RpcValue(std::move(ret));
}

AccessGrant AccessGrant::fromRpcValue(const RpcValue &rpcval)
{
	nLogFuncFrame() << rpcval.toCpon();
	AccessGrant ret;
	switch (rpcval.type()) {
	case RpcValue::Type::Invalid:
	case RpcValue::Type::Null:
		return AccessGrant();
	case RpcValue::Type::UInt:
	case RpcValue::Type::Int:
		ret.type = Type::AccessLevel;
		ret.accessLevel = rpcval.toInt();
		break;
	case RpcValue::Type::String:
		ret.type = Type::Role;
		ret.role = rpcval.toString();
		break;
	case RpcValue::Type::IMap: {
		const RpcValue::IMap &m = rpcval.toIMap();
		ret.type = static_cast<Type>(m.value(MetaType::Key::Type).toInt());
		switch (ret.type) {
		case Type::AccessLevel:
			ret.accessLevel = m.value(MetaType::Key::AccessLevel).toInt();
			break;
		case Type::Role:
			ret.role = m.value(MetaType::Key::Role).toString();
			break;
		case Type::UserLogin:
			ret.login.user = m.value(MetaType::Key::User).toString();
			ret.login.password = m.value(MetaType::Key::Password).toString();
			ret.login.loginType = static_cast<UserLogin::LoginType>(m.value(MetaType::Key::LoginType).toInt());
			break;
		default:
			nError() << "Invalid access grant type:" << rpcval.toCpon();
			break;
		}
		break;
	}
	case RpcValue::Type::Map: {
		const RpcValue::Map &m = rpcval.asMap();
		do {
			{
				auto access_level = m.value(KEY_ACCESS_LEVEL).toInt();
				if(access_level > 0) {
					ret.type = Type::AccessLevel;
					ret.accessLevel = access_level;
					break;
				}
			}
			{
				auto role = m.value(KEY_ROLE).asString();
				if(!role.empty()) {
					ret.type = Type::Role;
					ret.role = role;
					break;
				}
			}
			{
				// legacy configs used 'grant' intead of 'role'
				auto role = m.value("grant").asString();
				if(!role.empty()) {
					ret.type = Type::Role;
					ret.role = role;
					break;
				}
			}
			{
				auto login = m.value(KEY_LOGIN);
				if(login.isValid()) {
					ret.type = Type::UserLogin;
					ret.login = UserLogin::fromRpcValue(login);
					break;
				}
			}
			ret.type = Type::Invalid;
		} while(false);
		break;
	}
	default:
		break;
	}
	if(ret.type == Type::Invalid)
		nError() << "Invalid access grant:" << rpcval.toCpon();
	return ret;
}

const char *AccessGrant::typeToString(AccessGrant::Type t)
{
	switch (t) {
	case Type::Invalid: return "Invalid";
	case Type::AccessLevel: return "AccessLevel";
	case Type::Role: return "Role";
	case Type::UserLogin: return "UserLogin";
	}
	return "???";
}

AccessGrant::Type AccessGrant::typeFromString(const std::string &s)
{
	if(s == "AccessLevel")
		return Type::AccessLevel;
	if(s == "Role")
		return Type::Role;
	if(s == "UserLogin")
		return Type::UserLogin;
	return Type::Invalid;
}

} // namespace chainpack
} // namespace shv
