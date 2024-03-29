#pragma once

#include <shv/chainpack/shvchainpackglobal.h>

#include <shv/chainpack/metamethod.h>
#include <shv/chainpack/rpcmessage.h>
#include <shv/chainpack/rpcvalue.h>

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
	AccessLevel accessLevel = AccessLevel::None;
	std::string access;

	AccessGrant() = default;
	AccessGrant(std::optional<AccessLevel> level, std::string_view access_ = {});

	static AccessGrant fromShv2Access(std::string_view shv2_access, int access_level = 0);
	std::string toShv2Access() const;
	std::string toPrettyString() const;
};

} // namespace chainpack
} // namespace shv
