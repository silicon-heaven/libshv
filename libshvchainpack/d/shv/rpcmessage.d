module shv.rpcmessage;

import shv.cpon;
import shv.rpcvalue;
import shv.rpctype;

struct MetaType
{
public:
	enum {ID = GlobalNS.MetaTypeID.ChainPackRpcMessage}
	enum Tag {
		RequestId = cast(int)shv.rpctype.Tag.USER, // 8
		ShvPath, // 9
		Method,  // 10
		CallerIds, // 11
		ProtocolType, //needed when dest client is using different version than source one to translate raw message data to correct format
		RevCallerIds,
		AccessGrant,
		TunnelCtl,
		UserId,
		MAX
	}
	enum Key {Params = 1, Result, Error, ErrorCode, ErrorMessage, MAX }
}

struct RpcMessage
{
@safe:
public:
	this(ref return scope const RpcMessage o) {}
	this(ref return scope const RpcValue val) {}
	//virtual ~RpcMessage();
	ref inout(RpcValue) value() inout @safe { return m_value; }
	void value(RpcValue v) @safe { m_value = v; }
protected:
	//bool hasKey(int key) const;
	//RpcValue value(int key) const;
	//void setValue(int key, const ref RpcValue val);
public:
	bool isValid() const {
		return m_value.isValid();
	}
	bool isRequest() const {
			return hasRequestId() && hasMethod();
	}
	bool isResponse() const {
		return hasRequestId() && !hasMethod();
	}
	bool isSignal() const {
		return !hasRequestId() && hasMethod();
	}

	static bool isRequest(const ref RpcValue.Meta meta) {
		return hasRequestId(meta) && hasMethod(meta);
	}
	static bool isResponse(const ref RpcValue.Meta meta) {
		return hasRequestId(meta) && !hasMethod(meta);
	}
	static bool isSignal(const ref RpcValue.Meta meta) {
		return !hasRequestId(meta) && hasMethod(meta);
	}

	static bool hasRequestId(const ref RpcValue.Meta meta) {
		return (MetaType.Tag.RequestId in meta) != null;
	}
	static const(RpcValue) requestId(const ref RpcValue.Meta meta) {
		return meta.value(MetaType.Tag.RequestId);
	}
	static void setRequestId(ref RpcValue.Meta meta, RpcValue request_id) {
		meta[MetaType.Tag.RequestId] = request_id;
	}
	bool hasRequestId() const {
		return hasRequestId(m_value.meta);
	}
	const(RpcValue) requestId() const {
		return requestId(m_value.meta);
	}
	void setRequestId(RpcValue request_id) {
		setRequestId(m_value.meta, request_id);
	}
	static bool hasMethod(const ref RpcValue.Meta meta) {
		return (MetaType.Tag.Method in meta) != null;
	}
	bool hasMethod() const {
		return hasMethod(m_value.meta);
	}
	/+
	static RpcValue method(const ref RpcValue.Meta meta) {
		return meta.value(MetaType::Tag::Method);

	}
	static void setMethod(const ref RpcValue.Meta meta, string method) {

	}
	RpcValue method() const {

	}
	void setMethod(string method) {

	}

	static RpcValue shvPath(const ref RpcValue.Meta meta);
	static void setShvPath(const ref RpcValue.Meta meta, string path);
	RpcValue shvPath() const;
	void setShvPath(string path);

	static RpcValue accessGrant(const ref RpcValue.Meta meta);
	static void setAccessGrant(const ref RpcValue.Meta meta, const ref RpcValue ag);
	RpcValue accessGrant() const;
	void setAccessGrant(const ref RpcValue ag);

	static TunnelCtl tunnelCtl(const ref RpcValue.Meta meta);
	static void setTunnelCtl(const ref RpcValue.Meta meta, const TunnelCtl &tc);
	TunnelCtl tunnelCtl() const;
	void setTunnelCtl(const TunnelCtl &tc);

	static RpcValue callerIds(const ref RpcValue.Meta meta);
	static void setCallerIds(const ref RpcValue.Meta meta, const ref RpcValue caller_id);
	static void pushCallerId(const ref RpcValue.Meta meta, int caller_id);
	static RpcValue popCallerId(const ref RpcValue caller_ids, int &id);
	static int popCallerId(const ref RpcValue.Meta meta);
	int popCallerId();
	int peekCallerId() const;
	RpcValue callerIds() const;
	void setCallerIds(const ref RpcValue callerIds);

	static RpcValue revCallerIds(const ref RpcValue.Meta meta);
	static void setRevCallerIds(const ref RpcValue.Meta meta, const ref RpcValue caller_ids);
	static void pushRevCallerId(const ref RpcValue.Meta meta, int caller_id);
	RpcValue revCallerIds() const;

	void setRegisterRevCallerIds();
	static bool isRegisterRevCallerIds(const ref RpcValue.Meta meta);

	RpcValue userId() const;
	void setUserId(const ref RpcValue user_id);

	static ProtocolType protocolType(const ref RpcValue.Meta meta);
	static void setProtocolType(const ref RpcValue.Meta meta, shv::chainpack::ProtocolType ver);
	ProtocolType protocolType() const;
	void setProtocolType(ProtocolType ver);

	string toPrettyString() const;
	string toCpon() const;

	const RpcValue.Meta metaData() const {return m_value.metaData();}
	RpcValue metaValue(int key) const;
	void setMetaValue(int key, const ref RpcValue val);

	void write(R)(ref R out_range) const;
protected:
	void checkMetaValues();
	+/
@system:
	string toCpon() const
	{
		return m_value.toCpon();
	}
private:
	RpcValue m_value;
};
/+
struct RpcResponse;

struct LIBSHVCHAINPACK_CPP_EXPORT RpcRequest : public RpcMessage
{
private:
	using Super = RpcMessage;
public:
	RpcRequest() : Super() {}
	RpcRequest(const RpcMessage &msg) : Super(msg) {}
	~RpcRequest() override;
public:
	RpcRequest& setMethod(string met);
	RpcRequest& setMethod(RpcValue::String &&met);
	//RpcValue::String method() const;
	RpcRequest& setParams(const ref RpcValue p);
	RpcValue params() const;
	RpcRequest& setRequestId(const int id) {Super::setRequestId(id); return *this;}

	RpcResponse makeResponse() const;

	//size_t write(AbstractStreamWriter &wr) const override;
};

struct LIBSHVCHAINPACK_CPP_EXPORT RpcSignal : public RpcRequest
{
private:
	using Super = RpcRequest;
public:
	RpcSignal() : Super() {}
	RpcSignal(const RpcMessage &msg) : Super(msg) {}
	~RpcSignal() override;
public:
	RpcRequest& setRequestId(const int requestId) = delete;

	void write(AbstractStreamWriter &wr, const std::string &method, std::function<void (AbstractStreamWriter &)> write_params_callback);
};

struct LIBSHVCHAINPACK_CPP_EXPORT RpcException : public Exception
{
	using Super = Exception;
public:
	ception(int err_code, const std::string& _msg, const std::string& _where = std::string());
	~RpcException() override {}

	int errorCode() const { return m_errorCode; }
protected:
	int m_errorCode;
};

struct LIBSHVCHAINPACK_CPP_EXPORT RpcResponse : public RpcMessage
{
private:
	using Super = RpcMessage;
public:
	struct LIBSHVCHAINPACK_CPP_EXPORT Error : public RpcValue::IMap
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
		std::string errorCodeToString(int code);
		lue::String toString() const {return std::string("RPC ERROR ") + errorCodeToString(code()) + ": " + message();}
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
				{KeyMessage, json.value(Rpc::JSONRPC_ERROR_MESSAGE).toString()},
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
		static Error createMethodCallExceptionError(string msg = RpcValue::String()) {
			return create(MethodCallException, (msg.empty())? "Method call exception": msg);
		}
		static Error createInternalError(string msg = RpcValue::String()) {
			return create(InternalError, (msg.empty())? "Internal error": msg);
		}
		static Error createSyncMethodCallTimeout(string msg = RpcValue::String()) {
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

	static RpcResponse forRequest(const ref RpcValue.Meta meta);
	static RpcResponse forRequest(const RpcRequest &rq) {return forRequest(rq.metaData());}
public:
	bool hasRetVal() const {return isError() || result().isValid();}
	bool isError() const {return !error().empty();}
	RpcResponse& setError(Error err);
	Error error() const;
	RpcResponse& setResult(const ref RpcValue res);
	RpcValue result() const;
	RpcResponse& setRequestId(const ref RpcValue id) {Super::setRequestId(id); return *this;}
};
+/
