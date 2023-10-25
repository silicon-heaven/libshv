#pragma once

#include <shv/chainpack/rpcvalue.h>
#include <shv/chainpack/rpc.h>

#include <shv/chainpack/exception.h>
#include <shv/chainpack/shvchainpackglobal.h>

#include <functional>

namespace shv {
namespace chainpack {

class AbstractStreamWriter;
class TunnelCtl;

class SHVCHAINPACK_DECL_EXPORT RpcMessage
{
public:
	class MetaType : public meta::MetaType
	{
		using Super = meta::MetaType;
	public:
		enum {ID = meta::GlobalNS::MetaTypeId::ChainPackRpcMessage};
		struct Tag { enum Enum {RequestId = meta::Tag::USER, // 8
								ShvPath, // 9
								Method,  // 10
								CallerIds, // 11
								ProtocolType, //needed when dest client is using different version than source one to translate raw message data to correct format
								RevCallerIds,
								AccessGrant,
								TunnelCtl,
								UserId,
								MAX};};
		struct Key { enum Enum {Params = 1, Result, Error, MAX};};

		MetaType();

		static void registerMetaType();
	};
public:
	RpcMessage();
	RpcMessage(const RpcValue &val);
	virtual ~RpcMessage();

	const RpcValue& value() const;
protected:
	bool hasKey(RpcValue::Int key) const;
	RpcValue value(RpcValue::Int key) const;
	void setValue(RpcValue::Int key, const RpcValue &val);
public:
	bool isValid() const;
	bool isRequest() const;
	bool isResponse() const;
	bool isSignal() const;

	static bool isRequest(const RpcValue::MetaData &meta);
	static bool isResponse(const RpcValue::MetaData &meta);
	static bool isSignal(const RpcValue::MetaData &meta);

	static bool hasRequestId(const RpcValue::MetaData &meta);
	static RpcValue requestId(const RpcValue::MetaData &meta);
	static void setRequestId(RpcValue::MetaData &meta, const RpcValue &requestId);
	bool hasRequestId() const;
	RpcValue requestId() const;
	void setRequestId(const RpcValue &requestId);

	static bool hasMethod(const RpcValue::MetaData &meta);
	static RpcValue method(const RpcValue::MetaData &meta);
	static void setMethod(RpcValue::MetaData &meta, const RpcValue::String &method);
	bool hasMethod() const;
	RpcValue method() const;
	void setMethod(const RpcValue::String &method);

	static RpcValue shvPath(const RpcValue::MetaData &meta);
	static void setShvPath(RpcValue::MetaData &meta, const RpcValue::String &path);
	RpcValue shvPath() const;
	void setShvPath(const RpcValue::String &path);

	static RpcValue accessGrant(const RpcValue::MetaData &meta);
	static void setAccessGrant(RpcValue::MetaData &meta, const RpcValue &ag);
	RpcValue accessGrant() const;
	void setAccessGrant(const RpcValue &ag);

	static TunnelCtl tunnelCtl(const RpcValue::MetaData &meta);
	static void setTunnelCtl(RpcValue::MetaData &meta, const TunnelCtl &tc);
	TunnelCtl tunnelCtl() const;
	void setTunnelCtl(const TunnelCtl &tc);

	static RpcValue callerIds(const RpcValue::MetaData &meta);
	static void setCallerIds(RpcValue::MetaData &meta, const RpcValue &caller_id);
	static void pushCallerId(RpcValue::MetaData &meta, RpcValue::Int caller_id);
	static RpcValue popCallerId(const RpcValue &caller_ids, RpcValue::Int &id);
	static RpcValue::Int popCallerId(RpcValue::MetaData &meta);
	RpcValue::Int popCallerId();
	static RpcValue::Int peekCallerId(const RpcValue::MetaData &meta);
	RpcValue::Int peekCallerId() const;
	RpcValue callerIds() const;
	void setCallerIds(const RpcValue &callerIds);

	static RpcValue revCallerIds(const RpcValue::MetaData &meta);
	static void setRevCallerIds(RpcValue::MetaData &meta, const RpcValue &caller_ids);
	static void pushRevCallerId(RpcValue::MetaData &meta, RpcValue::Int caller_id);
	RpcValue revCallerIds() const;

	void setRegisterRevCallerIds();
	static bool isRegisterRevCallerIds(const RpcValue::MetaData &meta);

	RpcValue userId() const;
	void setUserId(const RpcValue &user_id);
	static RpcValue userId(RpcValue::MetaData &meta);
	static void setUserId(RpcValue::MetaData &meta, const RpcValue &user_id);

	static Rpc::ProtocolType protocolType(const RpcValue::MetaData &meta);
	static void setProtocolType(RpcValue::MetaData &meta, shv::chainpack::Rpc::ProtocolType ver);
	Rpc::ProtocolType protocolType() const;
	void setProtocolType(shv::chainpack::Rpc::ProtocolType ver);

	std::string toPrettyString() const;
	std::string toCpon() const;

	const RpcValue::MetaData& metaData() const;
	RpcValue metaValue(RpcValue::Int key) const;
	void setMetaValue(RpcValue::Int key, const RpcValue &val);
	RpcValue metaValue(const RpcValue::String &key) const;
	void setMetaValue(const RpcValue::String &key, const RpcValue &val);

	void write(AbstractStreamWriter &wr) const;

	static void registerMetaTypes();
protected:
	void checkMetaValues();
protected:
	RpcValue m_value;
	static const bool m_isMetaTypeExplicit;
};

class RpcResponse;

class SHVCHAINPACK_DECL_EXPORT RpcRequest : public RpcMessage
{
private:
	using Super = RpcMessage;
public:
	RpcRequest();
	RpcRequest(const RpcMessage &msg);
	~RpcRequest() override;
public:
	RpcRequest& setMethod(const RpcValue::String &met);
	RpcRequest& setMethod(RpcValue::String &&met);
	RpcRequest& setParams(const RpcValue &p);
	RpcValue params() const;
	RpcRequest& setRequestId(const RpcValue::Int id);

	RpcResponse makeResponse() const;
};

class SHVCHAINPACK_DECL_EXPORT RpcSignal : public RpcRequest
{
private:
	using Super = RpcRequest;
public:
	RpcSignal();
	RpcSignal(const RpcMessage &msg);
	~RpcSignal() override;
public:
	RpcRequest& setRequestId(const RpcValue::Int requestId) = delete;

	static void write(AbstractStreamWriter &wr, const std::string &method, std::function<void (AbstractStreamWriter &)> write_params_callback);
};

class SHVCHAINPACK_DECL_EXPORT RpcException : public Exception
{
	using Super = Exception;
public:
	RpcException(int err_code, const std::string& _msg, const std::string& _where = std::string());
	~RpcException() override;

	int errorCode() const;
protected:
	int m_errorCode;
};

class SHVCHAINPACK_DECL_EXPORT RpcError
{
private:
	enum { KeyCode = 1, KeyMessage, KeyData };
public:
	enum ErrorCode {
		NoError = 0,
		InvalidRequest,	// The JSON sent is not a valid Request object.
		MethodNotFound,	// The method does not exist / is not available.
		InvalidParams,		// Invalid method parameter(s).
		InternalError,		// Internal JSON-RPC error.
		ParseError,		// Invalid JSON was received by the server. An error occurred on the server while parsing the JSON text.
		MethodCallTimeout,
		MethodCallCancelled,
		MethodCallException,
		Unknown,
		UserCode = 32
	};
public:
	RpcError();
	RpcError(const std::string &msg, int code = ErrorCode::Unknown, const RpcValue &data = {});
	//RpcError(const RpcValue &v);
	RpcError& setCode(int c);
	int code() const;
	RpcValue::String message() const;
	RpcError& setMessage(const RpcValue::String &m);
	RpcValue data() const;
	RpcError& setData(const RpcValue &mv);
	static std::string errorCodeToString(int code);
	RpcValue::String toString() const;

	RpcValue::Map toJson() const;
	static RpcError fromJson(const RpcValue::Map &json);

	RpcValue toRpcValue() const;
	static RpcError fromRpcValue(const RpcValue &rv);
public:
	static RpcError create(int c, const RpcValue::String &msg, const RpcValue &data = {});
	static RpcError createMethodCallExceptionError(const RpcValue::String &msg = RpcValue::String());
	static RpcError createInternalError(const RpcValue::String &msg = RpcValue::String());
	static RpcError createSyncMethodCallTimeout(const RpcValue::String &msg = RpcValue::String());
	bool isValid() const;
	bool operator==(const RpcError &) const = default;
private:
	RpcValue value(int key) const;
	RpcError& setValue(int key, const RpcValue &v);
private:
	RpcValue m_value;
};

class SHVCHAINPACK_DECL_EXPORT RpcResponse : public RpcMessage
{
private:
	using Super = RpcMessage;
public:
	using Error = RpcError;
public:
	RpcResponse();
	RpcResponse(const RpcMessage &msg);
	RpcResponse(const RpcResponse &msg);
	~RpcResponse() override;

	RpcResponse& operator=(const RpcResponse &msg) = default;

	static RpcResponse forRequest(const RpcValue::MetaData &meta);
	static RpcResponse forRequest(const RpcRequest &rq);
public:
	/// hasRetVal() is deprecated, use hasResult() instead
	bool hasRetVal() const;

	bool hasResult() const;
	bool isSuccess() const;
	bool isError() const;
	std::string errorString() const;
	RpcResponse& setError(const Error &err);
	Error error() const;
	RpcResponse& setResult(const RpcValue &res);
	RpcValue result() const;
	RpcResponse& setRequestId(const RpcValue &id);
};

} // namespace chainpack
} // namespace shv
