#include "mockserialport.h"
#include "mockrpcconnection.h"

#include <shv/chainpack/utils.h>
#include <shv/iotqt/rpc/serialportsocket.h>
#include <shv/iotqt/rpc/socket.h>

#include <shv/chainpack/rpcmessage.h>

#include <necrolog.h>

#include <QUrl>

#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#define logSerialPortSocketD() nCDebug("SerialPortSocket")
#define logSerialPortSocketM() nCMessage("SerialPortSocket")
#define logSerialPortSocketW() nCWarning("SerialPortSocket")

using namespace shv::iotqt::rpc;
using namespace shv::chainpack;
using namespace std;

int main(int argc, char** argv)
{
	NecroLog::setTopicsLogThresholds("SerialPortSocket");
	Exception::setAbortOnException(true);
	return doctest::Context(argc, argv).run();
}

std::tuple<SerialPortSocket*, MockSerialPort*> init_connection(MockRpcConnection &conn)
{
	auto *serial = new MockSerialPort("TestSend", &conn);
	auto *socket = new SerialPortSocket(serial);
	socket->setReceiveTimeout(0);
	conn.setSocket(socket);
	conn.connectToHost(QUrl());
	return make_tuple(socket, serial);
}

DOCTEST_TEST_CASE("Send")
{
	MockRpcConnection conn;
	auto [socket, serial] = init_connection(conn);

	RpcMessage rec_msg;
	QObject::connect(&conn, &MockRpcConnection::rpcMessageReceived, [&rec_msg](const shv::chainpack::RpcMessage &msg) {
		rec_msg = msg;
	});

	RpcRequest rq;
	rq.setRequestId(1);
	rq.setShvPath("foo/bar");
	rq.setMethod("baz");
	RpcValue::List params = {
		"hello",
		"\xaa",
		"\xa2\xa3\xa4\xa5\xaa",
		"aa\xaa""aa",
		RpcValue::List{0xa2, 0xa3, 0xa4, 0xa5, 0xaa, "\xa2\xa3\xa4\xa5\xaa"},
	};
	for(const auto &p : params) {
		rq.setParams(p);
		serial->clearWrittenData();
		conn.sendRpcMessage(rq);
		auto data = serial->writtenData();
		//logSerialPortSocketD() << "msg:" << rq.toCpon();
		//logSerialPortSocketD() << "data:" << utils::hexArray(data.constData(), data.size());

		vector<string> rubbish1 = {
			"",
			"#$%^&",
			"\xa2",
			"\xa2\xaa\xa4\xa2\xa4\xaa",
			"%^\xa2""rr",
		};
		for(const auto &leading_rubbish : rubbish1) {
			// add some rubbish
			auto data1 = QByteArray::fromStdString(leading_rubbish) + data;
			vector<string> rubbish2 = {
				"",
				"/*-+",
				"\xa3",
				"\xa2\xa3\xa4\xa5\xaa",
				"%^\xaa""rr",
			};
			for(const auto &extra_rubbish : rubbish2) {
				// add some rubbish
				auto data2 = data1 +  QByteArray::fromStdString(extra_rubbish);
				rec_msg = {};
				//logSerialPortSocketD().nospace() << "data to send:\n" << utils::hexDump(data2.constData(), data2.size());
				serial->setDataToReceive(data2);
				//REQUIRE(socket->readMessageError() == SerialPortSocket::ReadMessageError::Ok);
				//logSerialPortSocketD() << "rec msg:" << rec_msg.value().toCpon();
				//logSerialPortSocketD() << "rq  msg:" << rq.value().toCpon();
				REQUIRE(rec_msg.value() == rq.value());
			}
		}
	}
}

DOCTEST_TEST_CASE("Test CRC error")
{
	MockRpcConnection conn;
	auto [socket, serial] = init_connection(conn);

	RpcRequest rq;
	rq.setRequestId(1);
	rq.setShvPath("some/path");
	rq.setMethod("method");
	rq.setParams(123);

	serial->clearWrittenData();
	conn.sendRpcMessage(rq);
	auto data = serial->writtenData();
	data[5] = ~data[5];

	serial->setDataToReceive(data);
	REQUIRE(socket->readFrameData().empty());
}
/*
DOCTEST_TEST_CASE("Test RESET message")
{
	MockRpcConnection conn;
	auto [socket, serial] = init_connection(conn);

	bool reset_received = false;
	QObject::connect(socket, &SerialPortSocket::socketReset, [&reset_received]() { // clazy:exclude=lambda-in-connect - the local won't go out of scope here
		reset_received = true;
	});

	auto data = serial->writtenData();
	serial->setDataToReceive(data);
	REQUIRE(reset_received == true);
}
*/
