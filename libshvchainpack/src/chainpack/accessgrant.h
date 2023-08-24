#pragma once

#include "../shvchainpackglobal.h"

#include "metamethod.h"
#include "rpcmessage.h"
#include "rpcvalue.h"

#include <optional>

namespace shv {
namespace chainpack {

struct UserLogin;

struct SHVCHAINPACK_DECL_EXPORT UserLoginContext
{
	std::string serverNounce;
	std::string clientType;
	shv::chainpack::RpcRequest loginRequest;
	int connectionId = 0;

	const RpcValue::Map &loginParams() const;
	shv::chainpack::UserLogin userLogin() const;
};

struct SHVCHAINPACK_DECL_EXPORT UserLoginResult
{
	bool passwordOk = false;
	std::string loginError;
	int clientId = 0;
	std::string brokerId;
	std::optional<std::string> userNameOverride;

	UserLoginResult();
	UserLoginResult(bool password_ok);
	UserLoginResult(bool password_ok, std::string login_error);

	shv::chainpack::RpcValue toRpcValue() const;
};

struct SHVCHAINPACK_DECL_EXPORT UserLogin
{
public:
	enum class LoginType {Invalid = 0, Plain, Sha1, RsaOaep, None, AzureAccessToken};

	std::string user;
	std::string password;
	LoginType loginType;

	bool isValid() const;

	static const char *loginTypeToString(LoginType t);
	static LoginType loginTypeFromString(const std::string &s);
	chainpack::RpcValue toRpcValueMap() const;
	static UserLogin fromRpcValue(const RpcValue &val);
};

struct SHVCHAINPACK_DECL_EXPORT AccessGrant
{

	enum class Type { Invalid = 0, AccessLevel, Role, UserLogin, };
	Type type = Type::Invalid;
	int accessLevel = shv::chainpack::MetaMethod::AccessLevel::None;
	std::string role;
	UserLogin login;
public:
	class MetaType : public chainpack::meta::MetaType
	{
		using Super = chainpack::meta::MetaType;
	public:
		enum {ID = chainpack::meta::GlobalNS::MetaTypeId::AccessGrant};
		struct Key { enum Enum {Type = 1, NotResolved /*reserved NOT USED*/, Role, AccessLevel, User, Password, LoginType, MAX};};

		MetaType();
		static void registerMetaType();
	};
public:
	AccessGrant();
	AccessGrant(const std::string &role_);

	bool isValid() const;
	bool isUserLogin() const;
	bool isRole() const;
	bool isAccessLevel() const;

	std::vector<std::string_view> roles() const;

	chainpack::RpcValue toRpcValue() const;
	chainpack::RpcValue toRpcValueMap() const;
	static AccessGrant fromRpcValue(const chainpack::RpcValue &rpcval);
	static const char* typeToString(Type t);
	static Type typeFromString(const std::string &s);
};

} // namespace chainpack
} // namespace shv
