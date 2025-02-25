#include <shv/chainpack/accessgrant.h>

#include <necrolog.h>

namespace shv::chainpack {

namespace {
bool str_eq(const std::string &s1, const char *s2)
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
bool UserLogin::isValid() const
{
	return !user.empty();
}

const char* UserLogin::loginTypeToString(UserLogin::LoginType t)
{
	switch(t) {
	case LoginType::None: return "NONE";
	case LoginType::Plain: return "PLAIN";
	case LoginType::Sha1: return "SHA1";
	case LoginType::RsaOaep: return "RSAOAEP";
	case LoginType::AzureAccessToken: return "AZURE";
	case LoginType::Token: return "TOKEN";
	default: return "INVALID";
	}
}

UserLogin::LoginType UserLogin::loginTypeFromString(const std::string &s)
{
	if(str_eq(s, loginTypeToString(LoginType::None)))
		return LoginType::None;
	if(str_eq(s, loginTypeToString(LoginType::Plain)))
		return LoginType::Plain;
	if(str_eq(s, loginTypeToString(LoginType::Sha1)))
		return LoginType::Sha1;
	if(str_eq(s, loginTypeToString(LoginType::RsaOaep)))
		return LoginType::RsaOaep;
	if(str_eq(s, loginTypeToString(LoginType::AzureAccessToken)))
		return LoginType::AzureAccessToken;
	if(str_eq(s, loginTypeToString(LoginType::Token)))
		return LoginType::Token;
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
		ret.user = m.value("user").toString();
		ret.password = m.value("password").toString();
		const RpcValue::String lts = m.value("type").toString();
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
UserLoginResult::UserLoginResult() = default;

UserLoginResult::UserLoginResult(bool password_ok) : UserLoginResult(password_ok, std::string()) {}

UserLoginResult::UserLoginResult(bool password_ok, std::string login_error)
	: passwordOk(password_ok)
	, loginError(std::move(login_error))
{
}

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
AccessGrant::AccessGrant(std::optional<AccessLevel> level, std::string_view access_)
	: accessLevel(level.value_or(AccessLevel::None))
	, access(access_)
{
}

AccessGrant AccessGrant::fromShv2Access(std::string_view shv2_access, int access_level)
{
	AccessGrant ret(accessLevelFromInt(access_level));
	while (!shv2_access.empty()) {
		auto first_comma = shv2_access.find(',');
		auto level_str = shv2_access.substr(0, first_comma);
		shv2_access.remove_prefix(level_str.size());
		if (first_comma != std::string_view::npos) {
			shv2_access.remove_prefix(1);
		}
		if (!level_str.empty()) {
			if (auto level = accessLevelFromAccessString(level_str); level.has_value()) {
				if (level.value() > ret.accessLevel) {
					// keep highest level
					ret.accessLevel = level.value();
				}
			}
			else {
				if (!ret.access.empty()) {
					ret.access += ',';
				}
				ret.access += level_str;
			}
		}
	}
	return ret;
}

std::string AccessGrant::toShv2Access() const
{
	if (std::string level_str = accessLevelToAccessString(accessLevel); !level_str.empty()) {
		if (!access.empty()) {
			level_str = level_str + ',' + access;
		}
		return level_str;
	}
	return {};
}

std::string AccessGrant::toPrettyString() const
{
	std::string s = accessLevelToAccessString(accessLevel);
	if (s.empty()) {
		s = s + '(' + std::to_string(static_cast<int>(accessLevel)) + ')';
	}
	if (!access.empty()) {
		s += "," + access;
	}
	return s;
}

} // namespace shv
