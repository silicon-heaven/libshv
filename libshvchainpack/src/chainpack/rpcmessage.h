#pragma once

#include "rpcvalue.h"
#include "rpc.h"

#include "utils.h"
#include "../shvchainpackglobal.h"

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
		struct Key { enum Enum {Params = 1, Result, Error, ErrorCode, ErrorMessage, MAX};};

		MetaType();

		static void registerMetaType();
	};
public:
	RpcMessage();
	RpcMessage(const RpcValue &val);
	RpcMessage(const RpcMessage &val) = default;
	virtual ~RpcMessage();

	const RpcValue& value() const {return m_value;}
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

	const RpcValue::MetaData& metaData() const {return m_value.metaData();}
	RpcValue metaValue(RpcValue::Int key) const;
	void setMetaValue(RpcValue::Int key, const RpcValue &val);
	RpcValue metaValue(const RpcValue::String &key) const;
	void setMetaValue(const RpcValue::String &key, const RpcValue &val);

	virtual void write(AbstractStreamWriter &wr) const;

	//std::string callerFingerprint() const;

	static void registerMetaTypes();
protected:
	void checkMetaValues();
protected:
	RpcValue m_value;
	static bool m_isMetaTypeExplicit;
};

class RpcResponse;

class SHVCHAINPACK_DECL_EXPORT RpcRequest : public RpcMessage
{
private:
	using Super = RpcMessage;
public:
	RpcRequest() : Super() {}
	RpcRequest(const RpcMessage &msg) : Super(msg) {}
	~RpcRequest() override;
public:
	RpcRequest& setMethod(const RpcValue::String &met);
	RpcRequest& setMethod(RpcValue::String &&met);
	//RpcValue::String method() const;
	RpcRequest& setParams(const RpcValue &p);
	RpcValue params() const;
	RpcRequest& setRequestId(const RpcValue::Int id) {Super::setRequestId(id); return *this;}

	RpcResponse makeResponse() const;

	//size_t write(AbstractStreamWriter &wr) const override;
};

class SHVCHAINPACK_DECL_EXPORT RpcSignal : public RpcRequest
{
private:
	using Super = RpcRequest;
public:
	RpcSignal() : Super() {}
	RpcSignal(const RpcMessage &msg) : Super(msg) {}
	~RpcSignal() override;
public:
	RpcRequest& setRequestId(const RpcValue::Int requestId) = delete;

	using RpcRequest::write;
	static void write(AbstractStreamWriter &wr, const std::string &method, std::function<void (AbstractStreamWriter &)> write_params_callback);
};

class SHVCHAINPACK_DECL_EXPORT RpcException : public Exception
{
	using Super = Exception;
public:
	RpcException(int err_code, const std::string& _msg, const std::string& _where = std::string());
	~RpcException() override;

	int errorCode() const { return m_errorCode; }
protected:
	int m_errorCode;
};

class SHVCHAINPACK_DECL_EXPORT RpcResponse : public RpcMessage
{
private:
	using Super = RpcMessage;
public:
	class SHVCHAINPACK_DECL_EXPORT Error : public RpcValue::IMap
	{
	private:
		using Super = RpcValue::IMap;
		enum {KeyCode = 1, KeyMessage};
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
		Error(const Super &m = Super()) : Super(m) {}
		Error& setCode(int c);
		int code() const;
		Error& setMessage(RpcValue::String &&m);
		RpcValue::String message() const;
		//Error& setData(const Value &data);
		//Value data() const;
		static std::string errorCodeToString(int code);
		RpcValue::String toString() const {return std::string("RPC ERROR ") + errorCodeToString(code()) + ": " + message();}
		RpcValue::Map toJson() const
		{
			return RpcValue::Map {
				{Rpc::JSONRPC_ERROR_CODE, (int)code()},
				{Rpc::JSONRPC_ERROR_MESSAGE, message()},
			};
		}
		static Error fromJson(const RpcValue::Map &json)
		{
			/*
			Error ret;
			ret.setCode(json.value(Rpc::JSONRPC_ERROR_CODE).toInt());
			ret.setMessage(json.value(Rpc::JSONRPC_ERROR_MESSAGE).toString());
			return ret;
			*/
			return RpcValue::IMap {
				{KeyCode, json.value(Rpc::JSONRPC_ERROR_CODE).toInt()},
				{KeyMessage, json.value(Rpc::JSONRPC_ERROR_MESSAGE).asString()},
			};
		}
	public:
		static Error create(int c, RpcValue::String msg);
		/*
		static Error createParseError(const Value::String &msg = Value::String(), const Value &data = Value()) {
			return createError(ParseError,
							   (msg.isEmpty())? "Parse error": msg,
							   (data.isValid())? data: "Invalid JSON was received by the server. An error occurred on the server while parsing the JSON text.");
		}
		static Error createInvalidRequestError(const Value::String &msg = Value::String(), const Value &data = Value()) {
			return createError(InvalidRequest,
							   (msg.isEmpty())? "Invalid Request": msg,
							   (data.isValid())? data: "The JSON sent is not a valid Request object.");
		}
		static Error createMethodNotFoundError(const Value::String &msg = Value::String(), const Value &data = Value()) {
			return createError(MethodNotFound,
							   (msg.isEmpty())? "Method not found": msg,
							   (data.isValid())? data: "The method does not exist / is not available.");
		}
		static Error createInvalidParamsError(const Value::String &msg = Value::String(), const Value &data = Value()) {
			return createError(InvalidParams,
							   (msg.isEmpty())? "Invalid params": msg,
							   (data.isValid())? data: "Invalid method parameter(s).");
		}
		*/
		static Error createMethodCallExceptionError(const RpcValue::String &msg = RpcValue::String()) {
			return create(MethodCallException, (msg.empty())? "Method call exception": msg);
		}
		static Error createInternalError(const RpcValue::String &msg = RpcValue::String()) {
			return create(InternalError, (msg.empty())? "Internal error": msg);
		}
		static Error createSyncMethodCallTimeout(const RpcValue::String &msg = RpcValue::String()) {
			return create(MethodCallTimeout, (msg.empty())? "Sync method call timeout": msg);
		}
	};
public:
	//RpcResponse(const Json &json = Json()) : Super(json) {}
	//RpcResponse(const Value &request_id) : Super(Json()) { setId(request_id); }
	RpcResponse() : Super() {}
	RpcResponse(const RpcMessage &msg) : Super(msg) {}
	RpcResponse(const RpcResponse &msg) = default;
	~RpcResponse() override;

	static RpcResponse forRequest(const RpcValue::MetaData &meta);
	static RpcResponse forRequest(const RpcRequest &rq) {return forRequest(rq.metaData());}
public:
	/// hasRetVal() is deprecated, use hasResult() instead
	bool hasRetVal() const {return !error().empty() || result().isValid();}

	bool hasResult() const {return !error().empty() || result().isValid();}
	bool isSuccess() const {return result().isValid() && !isError();}
	bool isError() const {return !error().empty();}
	std::string errorString() const;
	RpcResponse& setError(Error err);
	Error error() const;
	RpcResponse& setResult(const RpcValue &res);
	RpcValue result() const;
	RpcResponse& setRequestId(const RpcValue &id) {Super::setRequestId(id); return *this;}
};

} // namespace chainpackrpc
} // namespace shv
