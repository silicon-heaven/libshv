#pragma once

#include <shv/iotqt/shviotqtglobal.h>

#include <shv/chainpack/metamethod.h>
#include <shv/chainpack/rpcvalue.h>
#include <shv/chainpack/rpcmessage.h>
#include <shv/core/stringview.h>
#include <shv/core/utils.h>
#include <shv/core/utils/shvpath.h>

#include <QObject>
#include <QMetaProperty>

#include <cstddef>

namespace shv::chainpack { struct AccessGrant; }
namespace shv::core::utils { class ShvJournalEntry; }
namespace shv::iotqt {
namespace utils { class ShvPath; }

namespace node {

class ShvRootNode;

class SHVIOTQT_DECL_EXPORT ShvNode : public QObject
{
	Q_OBJECT
public:
	using String = std::string;
	using StringList = std::vector<String>;
	using StringView = shv::core::StringView;
	using StringViewList = shv::core::StringViewList;
public:
	static const std::string ADD_LOCAL_TO_LS_RESULT_HACK_META_KEY;
	static const std::string LOCAL_NODE_HACK;
public:
	explicit ShvNode(ShvNode *parent);
	explicit ShvNode(const std::string &node_id, ShvNode *parent = nullptr);
	~ShvNode() override;

	ShvNode* parentNode() const;
	QList<ShvNode*> ownChildren() const;
	virtual ShvNode* childNode(const String &name, bool throw_exc = true) const;
	virtual void setParentNode(ShvNode *parent);
	virtual String nodeId() const;
	void setNodeId(String &&n);
	void setNodeId(const String &n);

	shv::core::utils::ShvPath shvPath() const;

	ShvNode* rootNode();
	virtual void emitSendRpcMessage(const shv::chainpack::RpcMessage &msg);
	void emitLogUserCommand(const shv::core::utils::ShvJournalEntry &e);

	void setSortedChildren(bool b);

	void deleteIfEmptyWithParents();

	bool isRootNode() const;

	virtual void handleRpcFrame(chainpack::RpcFrame &&frame);
	virtual void handleRpcRequest(const chainpack::RpcRequest &rq);
	virtual chainpack::RpcValue handleRpcRequestImpl(const chainpack::RpcRequest &rq);
	virtual chainpack::RpcValue processRpcRequest(const shv::chainpack::RpcRequest &rq);

	virtual shv::chainpack::RpcValue dir(const StringViewList &shv_path, const shv::chainpack::RpcValue &methods_params);

	virtual shv::chainpack::RpcValue ls(const StringViewList &shv_path, const shv::chainpack::RpcValue &methods_params);
	// returns null if does not know
	virtual chainpack::RpcValue hasChildren(const StringViewList &shv_path);

	static chainpack::AccessLevel basicGrantToAccessLevel(const chainpack::RpcValue &acces_grant);
	virtual chainpack::AccessLevel grantToAccessLevel(const chainpack::RpcValue &acces_grant) const;

	void treeWalk(std::function<void (ShvNode *parent_nd, const StringViewList &shv_path)> callback);
private:
	static void treeWalk_helper(std::function<void (ShvNode *parent_nd, const StringViewList &shv_path)> callback, ShvNode *parent_nd, const StringViewList &shv_path);
public:
	virtual size_t methodCount(const StringViewList &shv_path);
	virtual const shv::chainpack::MetaMethod* metaMethod(const StringViewList &shv_path, size_t ix);
	const shv::chainpack::MetaMethod* metaMethod(const StringViewList &shv_path, const std::string &name);

	virtual StringList childNames(const StringViewList &shv_path);
	StringList childNames();

	virtual shv::chainpack::RpcValue callMethodRq(const chainpack::RpcRequest &rq);
	virtual shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params, const shv::chainpack::RpcValue &user_id);
public:
	Q_SIGNAL void sendRpcMessage(const shv::chainpack::RpcMessage &msg);
	Q_SIGNAL void logUserCommand(const shv::core::utils::ShvJournalEntry &e);
protected:
	bool m_isRootNode = false;
private:
	String m_nodeId;
	bool m_isSortedChildren = true;
};

/// helper class to save lines when creating root node
/// any ShvNode descendant with m_isRootNode = true may be RootNode
class SHVIOTQT_DECL_EXPORT ShvRootNode : public ShvNode
{
	using Super = ShvNode;
public:
	explicit ShvRootNode(QObject *parent);
	~ShvRootNode() override;
};

class SHVIOTQT_DECL_EXPORT MethodsTableNode : public shv::iotqt::node::ShvNode
{
	using Super = shv::iotqt::node::ShvNode;
public:
	explicit MethodsTableNode(const std::string &node_id, const std::vector<shv::chainpack::MetaMethod> *methods, shv::iotqt::node::ShvNode *parent = nullptr);

	size_t methodCount(const StringViewList &shv_path) override;
	const shv::chainpack::MetaMethod* metaMethod(const StringViewList &shv_path, size_t ix) override;
protected:
	const std::vector<shv::chainpack::MetaMethod> *m_methods = nullptr;
};


class SHVIOTQT_DECL_EXPORT RpcValueMapNode : public shv::iotqt::node::ShvNode
{
	Q_OBJECT
	using Super = shv::iotqt::node::ShvNode;

	SHV_FIELD_BOOL_IMPL(r, R, eadOnly)
public:
	static constexpr auto M_LOAD = "loadFile";
	static constexpr auto M_SAVE = "saveFile";
	static constexpr auto M_COMMIT = "commitChanges";
public:
	RpcValueMapNode(const std::string &node_id, shv::iotqt::node::ShvNode *parent = nullptr);
	RpcValueMapNode(const std::string &node_id, const shv::chainpack::RpcValue &values, shv::iotqt::node::ShvNode *parent = nullptr);

	size_t methodCount(const StringViewList &shv_path) override;
	const shv::chainpack::MetaMethod* metaMethod(const StringViewList &shv_path, size_t ix) override;

	StringList childNames(const ShvNode::StringViewList &shv_path) override;
	shv::chainpack::RpcValue hasChildren(const StringViewList &shv_path) override;

	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params, const shv::chainpack::RpcValue &user_id) override;

	shv::chainpack::RpcValue valueOnPath(const std::string &shv_path, bool throw_exc = true);
	virtual shv::chainpack::RpcValue valueOnPath(const StringViewList &shv_path, bool throw_exc = true);
	void setValueOnPath(const std::string &shv_path, const shv::chainpack::RpcValue &val);
	virtual void setValueOnPath(const StringViewList &shv_path, const shv::chainpack::RpcValue &val);

	void commitChanges();

	void clearValuesCache();

	Q_SIGNAL void configSaved();
protected:
	static shv::chainpack::RpcValue valueOnPath(const shv::chainpack::RpcValue &val, const StringViewList &shv_path, bool throw_exc);
	virtual const shv::chainpack::RpcValue &values();
	virtual void loadValues();
	virtual void saveValues();
	bool isDir(const StringViewList &shv_path);
protected:
	bool m_valuesLoaded = false;
	shv::chainpack::RpcValue m_values;
};

class SHVIOTQT_DECL_EXPORT RpcValueConfigNode : public RpcValueMapNode
{
	Q_OBJECT

	using Super = RpcValueMapNode;

	SHV_FIELD_IMPL2(std::string, c, C, onfigName, "config")
	SHV_FIELD_IMPL2(std::string, c, C, onfigDir, ".")
	SHV_FIELD_IMPL(std::string, u, U, serConfigName)
	SHV_FIELD_IMPL(std::string, u, U, serConfigDir)
	SHV_FIELD_IMPL(std::string, t, T, emplateConfigName)
	SHV_FIELD_IMPL(std::string, t, T, emplateDir)
public:
	RpcValueConfigNode(const std::string &node_id, shv::iotqt::node::ShvNode *parent);

protected:

	shv::chainpack::RpcValue loadConfigTemplate(const std::string &file_name);
	std::string resolvedUserConfigName() const;
	std::string resolvedUserConfigDir() const;

	void loadValues() override;
	void saveValues() override;

	size_t methodCount(const StringViewList &shv_path) override;
	const shv::chainpack::MetaMethod *metaMethod(const StringViewList &shv_path, size_t ix) override;
	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params, const chainpack::RpcValue &user_id) override;
protected:
	shv::chainpack::RpcValue m_templateValues;
};

class SHVIOTQT_DECL_EXPORT ValueProxyShvNode : public shv::iotqt::node::ShvNode
{
	Q_OBJECT

	using Super = shv::iotqt::node::ShvNode;
public:
	enum class Type {
		Invalid = 0,
		Read = 1,
		Write = 2,
		ReadWrite = 3,
		Signal = 4,
		ReadSignal = 5,
		WriteSignal = 6,
		ReadWriteSignal = 7,
	};
	class SHVIOTQT_DECL_EXPORT Handle
	{
		friend class ValueProxyShvNode;
	public:
		Handle();
		virtual ~Handle();

		virtual shv::chainpack::RpcValue shvValue(int value_id) = 0;
		virtual void setShvValue(int value_id, const shv::chainpack::RpcValue &val) = 0;
		virtual std::string shvValueIdToName(int value_id) = 0;
		const shv::chainpack::RpcRequest& servedRpcRequest() const;
	protected:
		shv::chainpack::RpcRequest m_servedRpcRequest;
	};
public:
	explicit ValueProxyShvNode(const std::string &node_id, int value_id, Type type, Handle *handled_obj, shv::iotqt::node::ShvNode *parent = nullptr);

	void addMetaMethod(shv::chainpack::MetaMethod &&mm);

	size_t methodCount(const StringViewList &shv_path) override;
	const shv::chainpack::MetaMethod* metaMethod(const StringViewList &shv_path, size_t ix) override;

	shv::chainpack::RpcValue callMethodRq(const shv::chainpack::RpcRequest &rq) override;
	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params, const shv::chainpack::RpcValue &user_id) override;
protected:
	bool isWriteable();
	bool isReadable();
	bool isSignal();

	Q_SLOT void onShvValueChanged(int value_id, shv::chainpack::RpcValue val);
protected:
	int m_valueId;
	Type m_type;
	Handle *m_handledObject = nullptr;
	std::vector<shv::chainpack::MetaMethod> m_extraMetaMethods;
};

}}
