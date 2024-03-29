#include "application.h"

#include <shv/iotqt/node/propertynode.h>
#include <shv/iotqt/node/shvnodetree.h>
#include <shv/coreqt/log.h>
#include <shv/iotqt/acl/aclroleaccessrules.h>

#include <QTimer>

namespace cp = shv::chainpack;

namespace {
constexpr auto METH_RESET = "reset";
}

class TestNode : public shv::iotqt::node::MethodsTableNode
{
	using Super = shv::iotqt::node::MethodsTableNode;
public:
	TestNode(shv::iotqt::node::ShvNode *parent = nullptr)
		: Super(std::string(), &m_metaMethods, parent)
		, m_metaMethods{
			shv::chainpack::methods::DIR,
			shv::chainpack::methods::LS,
		}
	{ }

private:
	std::vector<cp::MetaMethod> m_metaMethods;
};

namespace {
const std::vector<shv::chainpack::MetaMethod> methods{
	{METH_RESET, cp::MetaMethod::Flag::None, "param", "ret", shv::chainpack::AccessLevel::Command},
};
}

class UptimeNode : public shv::iotqt::node::ReadOnlyPropertyNode<int>
{
	using Super = shv::iotqt::node::ReadOnlyPropertyNode<int>;
public:
	UptimeNode(shv::iotqt::node::ShvNode *parent = nullptr)
		: Super("uptime", parent)
	{
		auto *t = new QTimer(this);
		connect(t, &QTimer::timeout, this, &UptimeNode::incUptime);
		t->start(1000);
	}

	size_t methodCount(const StringViewList &shv_path) override
	{
		if (shv_path.empty()) {
			return Super::methodCount(shv_path) + 1;
		}
		return Super::methodCount(shv_path);
	}

	const shv::chainpack::MetaMethod* metaMethod(const StringViewList &shv_path, size_t ix) override
	{
		static auto super_method_count = Super::methodCount(shv_path);

		if (ix >= super_method_count) {
			return &methods.at(ix - super_method_count);
		}

		return Super::metaMethod(shv_path, ix);
	}

	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params, const shv::chainpack::RpcValue &user_id) override
	{
		if(shv_path.empty()) {
			if(method == METH_RESET) {
				setValue(0);
				return true;
			}
		}
		return Super::callMethod(shv_path, method, params, user_id);
	}
private:
	void incUptime()
	{
		setValue(getValue() + 1);
	}
};

class UptimeNodeDuration : public shv::iotqt::node::ReadOnlyPropertyNode<std::chrono::milliseconds>
{
	using Super = shv::iotqt::node::ReadOnlyPropertyNode<std::chrono::milliseconds>;

	static constexpr auto MSEC_INCREMENT = 10;
public:
	UptimeNodeDuration(shv::iotqt::node::ShvNode *parent = nullptr)
		: Super("uptime-duration", parent, [] (const std::chrono::milliseconds& duration) {
		return shv::chainpack::RpcValue{duration_cast<std::chrono::duration<double>>(duration).count()};
	})
	{
		auto *t = new QTimer(this);
		connect(t, &QTimer::timeout, this, &UptimeNodeDuration::incUptime);
		t->start(MSEC_INCREMENT);
	}

	size_t methodCount(const StringViewList &shv_path) override
	{
		if (shv_path.empty()) {
			return Super::methodCount(shv_path) + 1;
		}
		return Super::methodCount(shv_path);
	}

	const shv::chainpack::MetaMethod* metaMethod(const StringViewList &shv_path, size_t ix) override
	{
		static auto super_method_count = Super::methodCount(shv_path);

		if (ix >= super_method_count) {
			return &methods.at(ix - super_method_count);
		}

		return Super::metaMethod(shv_path, ix);
	}

	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params, const shv::chainpack::RpcValue &user_id) override
	{
		if(shv_path.empty()) {
			if(method == METH_RESET) {
				setValue(std::chrono::milliseconds{0});
				return true;
			}
		}
		return Super::callMethod(shv_path, method, params, user_id);
	}
private:
	void incUptime()
	{
		setValue(getValue() + std::chrono::milliseconds{MSEC_INCREMENT});
	}
};

Application::Application(int &argc, char **argv, shv::broker::AppCliOptions *cli_opts)
	: Super(argc, argv, cli_opts)
{
	auto *m = new shv::broker::AclManagerConfigFiles(this);
	setAclManager(m);
	auto nd1 = new TestNode();
	m_nodesTree->mount("test", nd1);
	new UptimeNode(nd1);
	new UptimeNodeDuration(nd1);
	{
		auto *nd = new shv::iotqt::node::PropertyNode<std::string>("stringProperty", nd1);
		nd->setValue("Some text");
	}
	{
		auto *nd = new shv::iotqt::node::PropertyNode<int>("intProperty", nd1);
		nd->setValue(42);
	}
	{
		auto *nd = new shv::iotqt::node::PropertyNode<shv::chainpack::RpcValue>("rpcValueProperty", nd1);
		nd->setValue(shv::chainpack::RpcValue::Map{
			{"key1", "value1"},
			{"key2", "value2"},
		});
	}

	struct MyStruct {
		int x;
		std::string y;
		bool z;
		bool operator==(const MyStruct&) const = default;
	};

	auto from_my_struct = [] (const MyStruct& my_struct) {
		return shv::chainpack::RpcValue::Map{
			{"x", my_struct.x},
			{"y", my_struct.y},
			{"z", my_struct.z},
		};
	};
	auto to_my_struct  =[] (const shv::chainpack::RpcValue& val) {
			return MyStruct{
				.x = val.asMap().value("x").toInt(),
				.y = val.asMap().value("y").asString(),
				.z = val.asMap().value("z").toBool(),
			};
		};
	{
		auto *nd = new shv::iotqt::node::PropertyNode<MyStruct>("myStructProperty1", nd1, from_my_struct, to_my_struct);
		nd->setValue(MyStruct{
			.x = 42,
			.y = "some string",
			.z = false,
		});
	}
	{
		auto *nd = new shv::iotqt::node::PropertyNode<MyStruct>("myStructProperty2", nd1, from_my_struct, to_my_struct);
		nd->setValue(MyStruct{
			.x = 12,
			.y = "some other string",
			.z = true,
		});
	}
}


