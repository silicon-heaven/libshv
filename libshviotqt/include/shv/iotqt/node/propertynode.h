#pragma once

#include <shv/core/exception.h>
#include <shv/iotqt/node/shvnode.h>

namespace shv::iotqt::node {
#define PROPERTY_TYPE_NEEDS_CONVERTORS "PropertyType: This type is not supported natively by RpcValue, please supply custom convertors."
enum class PropertyNodeAccessType {
	ReadOnly,
	ReadWrite
};

extern LIBSHVIOTQT_EXPORT const std::vector<shv::chainpack::MetaMethod> READONLY_PROPERTY_NODE_METHODS;
extern LIBSHVIOTQT_EXPORT const std::vector<shv::chainpack::MetaMethod> PROPERTY_NODE_METHODS;
template <typename PropertyType, auto ACCESS_TYPE>
class BasePropertyNode : public shv::iotqt::node::MethodsTableNode
{
	using Super = shv::iotqt::node::MethodsTableNode;
	using ToRpcValueArgs = const chainpack::RpcValue(const PropertyType&);
	using FromRpcValueArgs = PropertyType(const chainpack::RpcValue&);
public:
	BasePropertyNode(
		const std::string& prop_name,
		shv::iotqt::node::ShvNode* parent = nullptr,
		const std::function<ToRpcValueArgs>& to_rpc_value = [] (const PropertyType& x) {
			static_assert(std::is_constructible<chainpack::RpcValue, PropertyType>(), PROPERTY_TYPE_NEEDS_CONVERTORS);
			return chainpack::RpcValue{x}; \
		},
		const std::function<FromRpcValueArgs>& from_rpc_value = [] {
			if constexpr (ACCESS_TYPE == PropertyNodeAccessType::ReadWrite) {
				return [] (const chainpack::RpcValue& x) {
					static_assert(std::is_constructible<chainpack::RpcValue, PropertyType>(), PROPERTY_TYPE_NEEDS_CONVERTORS);
					if constexpr (!std::is_same<PropertyType, chainpack::RpcValue>()) {
						if (!x.is<PropertyType>()) {
							SHV_EXCEPTION(std::string{"Invalid type: "} + x.typeName() + ", expected type: " + chainpack::RpcValue::typeToName(chainpack::RpcValue::guessType<PropertyType>()));
						}
					}
					return x.to<PropertyType>();
				};
			} else { // NOLINT(readability-misleading-indentation)
				return [] (const chainpack::RpcValue&) {
					// stub for ReadOnlyPropertyNode, PropertyType must be default constructible anyway
					return PropertyType{};
				};
			}

		}()
	)
		: Super(prop_name, ACCESS_TYPE == PropertyNodeAccessType::ReadWrite ? &PROPERTY_NODE_METHODS : &READONLY_PROPERTY_NODE_METHODS, parent)
		, m_toRpcValue(to_rpc_value)
		, m_fromRpcValue(from_rpc_value)
	{
	}

	shv::chainpack::RpcValue callMethod(const StringViewList& shv_path, const std::string& method, const shv::chainpack::RpcValue& params, const shv::chainpack::RpcValue& user_id) override
	{
		if(shv_path.empty()) {
			if(method == shv::chainpack::Rpc::METH_GET) {
				return m_toRpcValue(getValue());
			}

			if constexpr (ACCESS_TYPE == PropertyNodeAccessType::ReadWrite) {

				if(method == shv::chainpack::Rpc::METH_SET) {
					setValue(m_fromRpcValue(params));
					return true;
				}
			}
		}
		return Super::callMethod(shv_path, method, params, user_id);
	}

	PropertyType getValue() const
	{
		return m_value;
	}

	void setValue(const PropertyType& val)
	{
		if(val == m_value) {
			return;
		}
		m_value = val;
		shv::chainpack::RpcSignal sig;
		sig.setMethod(shv::chainpack::Rpc::SIG_VAL_CHANGED);
		sig.setShvPath(shvPath().asString());
		sig.setParams(m_toRpcValue(m_value));
		emitSendRpcMessage(sig);
	}

private:
	PropertyType m_value;
	std::function<ToRpcValueArgs> m_toRpcValue;
	std::function<FromRpcValueArgs> m_fromRpcValue;
};

template <typename PropertyType = shv::chainpack::RpcValue>
class PropertyNode : public BasePropertyNode<PropertyType, PropertyNodeAccessType::ReadWrite> {
public:
	using BasePropertyNode<PropertyType, PropertyNodeAccessType::ReadWrite>::BasePropertyNode;
};

template <typename PropertyType = shv::chainpack::RpcValue>
class ReadOnlyPropertyNode : public BasePropertyNode<PropertyType, PropertyNodeAccessType::ReadOnly> {
public:
	using BasePropertyNode<PropertyType, PropertyNodeAccessType::ReadOnly>::BasePropertyNode;
};
}
