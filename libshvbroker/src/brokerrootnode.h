#pragma once

#include <shv/iotqt/node/shvnode.h>
#include <shv/chainpack/rpcvalue.h>

namespace shv::broker {

class BrokerRootNode : public shv::iotqt::node::ShvRootNode
{
	Q_OBJECT
	using Super = shv::iotqt::node::ShvRootNode;

public:
	BrokerRootNode(shv::iotqt::node::ShvNode *parent = nullptr);
};
}

