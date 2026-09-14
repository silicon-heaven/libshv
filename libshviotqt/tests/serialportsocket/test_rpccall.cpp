// NOLINTBEGIN

#include "mockserialport.h"

#include <shv/iotqt/rpc/clientconnection.h>
#include <shv/iotqt/rpc/rpccall.h>
#include <shv/iotqt/rpc/serialportsocket.h>

#include <QBuffer>
#include <QCoreApplication>
#include <QEventLoop>
#include <QTimer>
#include <QUrl>

#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

using namespace shv::chainpack;
using namespace shv::iotqt::rpc;

int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);
	return doctest::Context(argc, argv).run();
}

namespace {
class TestConnection : public ClientConnection
{
public:
	using SocketRpcConnection::socket;

	TestConnection() : ClientConnection("CRC regression test")
	{
		serial = new MockSerialPort("RpcCall");
		auto *socket = new SerialPortSocket(serial);
		socket->setReceiveTimeout(0);
		// Open before attaching the socket to avoid starting the login handshake.
		socket->connectToHost(QUrl());
		setSocket(socket);
		m_connectionState.state = State::BrokerConnected;
	}

	MockSerialPort *serial = nullptr;
};

struct CallResult : QObject
{
	CallResult(TestConnection &conn, int request_id, int timeout = 60000)
	{
		call = RpcCall::create(&conn);
		call->setParent(this);
		connect(call, &RpcCall::maybeResult, this, [this](const RpcValue &value, const RpcError &error) {
			++maybeCount;
			result = value;
			maybeError = error;
		});
		connect(call, &RpcCall::error, this, [this](const RpcError &error) {
			++errorCount;
			signalError = error;
		});
		connect(call, &RpcCall::result, this, [this](const RpcValue &) { ++resultCount; });
		REQUIRE(call->setRequestId(request_id)->setShvPath("test")->setMethod("get")->setTimeout(timeout)->start() == request_id);
	}

	QPointer<RpcCall> call;
	int maybeCount = 0;
	int errorCount = 0;
	int resultCount = 0;
	RpcValue result;
	RpcError maybeError;
	RpcError signalError;
};

RpcResponse response(int request_id)
{
	RpcResponse resp;
	resp.setRequestId(request_id);
	resp.setResult("crc-payload");
	return resp;
}

QByteArray serial_frame(const RpcMessage &message, Rpc::ProtocolType protocol = Rpc::ProtocolType::ChainPack)
{
	SerialFrameWriter writer(SerialFrameWriter::CrcCheck::Yes);
	writer.addFrame(message.toRpcFrame(protocol));
	QByteArray data;
	QBuffer buffer(&data);
	REQUIRE(buffer.open(QIODevice::WriteOnly));
	writer.flushToDevice(&buffer);
	return data;
}

QByteArray corrupt_payload(QByteArray data)
{
	const auto pos = data.indexOf("crc-payload");
	REQUIRE(pos >= 0);
	// Change only ASCII payload bytes, not metadata or serial delimiters/escapes.
	data[pos] = 'C';
	return data;
}
}

DOCTEST_TEST_CASE("Serial CRC error fails only the matching RpcCall once")
{
	bool fragmented = false;
	bool same_batch = false;
	DOCTEST_SUBCASE("Complete corrupt response, then valid concurrent response") {}
	DOCTEST_SUBCASE("Corrupt response fragmented after metadata") { fragmented = true; }
	DOCTEST_SUBCASE("Corrupt and valid concurrent responses in one read") { same_batch = true; }

	TestConnection conn;
	CallResult failed(conn, 101);
	CallResult other(conn, 102);
	QPointer<RpcResponseCallBack> callback = failed.call->findChild<RpcResponseCallBack*>();
	REQUIRE(!callback.isNull());
	std::vector<std::pair<int, QString>> socket_errors;
	std::vector<std::pair<int, QString>> connection_errors;
	QList<int> meta_ids;
	QList<int> delivered_ids;
	QObject context;
	QObject::connect(conn.socket(), &Socket::responseReceiveError, &context, [&](int id, const QString &error) {
		socket_errors.emplace_back(id, error);
	});
	QObject::connect(&conn, &ClientConnection::responseReceiveError, &context, [&](int id, const QString &error) {
		connection_errors.emplace_back(id, error);
	});
	QObject::connect(&conn, &ClientConnection::responseMetaReceived, &context, [&](int id) { meta_ids << id; });
	QObject::connect(&conn, &ClientConnection::rpcMessageReceived, &context, [&](const RpcMessage &message) {
		delivered_ids << message.requestId().toInt();
	});

	const auto valid = serial_frame(response(101));
	const auto corrupt = corrupt_payload(valid);
	auto remaining = corrupt;
	if (fragmented) {
		const auto split = valid.indexOf("crc-payload");
		REQUIRE(split > 0);
		conn.serial->setDataToReceive(corrupt.left(split));
		QCoreApplication::processEvents();
		REQUIRE(meta_ids == QList<int>{101});
		REQUIRE(socket_errors.empty());
		REQUIRE(failed.maybeCount == 0);
		REQUIRE(other.maybeCount == 0);
		remaining = corrupt.mid(split);
	}
	if (same_batch) {
		remaining += serial_frame(response(102));
	}
	conn.serial->setDataToReceive(remaining);
	REQUIRE(failed.maybeCount == 1);
	REQUIRE(failed.errorCount == 1);
	REQUIRE(failed.resultCount == 0);
	REQUIRE_FALSE(failed.result.isValid());
	REQUIRE(failed.maybeError.code() == RpcError::Unknown);
	REQUIRE(failed.signalError == failed.maybeError);
	REQUIRE_FALSE(callback->findChild<QTimer*>()->isActive());
	REQUIRE(socket_errors.size() == 1);
	REQUIRE(socket_errors.front().first == 101);
	REQUIRE(socket_errors.front().second.contains("CRC"));
	REQUIRE(failed.maybeError.message() == socket_errors.front().second.toStdString());
	REQUIRE(connection_errors == socket_errors);
	REQUIRE(other.maybeCount == 0);

	// Deliver queued frames, but keep deferred deletions pending to exercise the guard.
	if (!same_batch) {
		conn.serial->setDataToReceive(serial_frame(response(102)));
	}
	QCoreApplication::sendPostedEvents(&conn, QEvent::MetaCall);
	REQUIRE(other.maybeCount == 1);
	REQUIRE(other.resultCount == 1);
	REQUIRE(other.errorCount == 0);
	REQUIRE(other.result == RpcValue("crc-payload"));
	REQUIRE_FALSE(other.maybeError.isValid());
	REQUIRE(!callback.isNull());
	conn.serial->setDataToReceive(corrupt + corrupt + valid);
	QCoreApplication::sendPostedEvents(&conn, QEvent::MetaCall);
	REQUIRE(!callback.isNull());
	REQUIRE(socket_errors.size() == 3);
	REQUIRE(connection_errors == socket_errors);
	REQUIRE(failed.maybeCount == 1);
	REQUIRE(failed.errorCount == 1);
	REQUIRE(failed.resultCount == 0);
	REQUIRE(other.maybeCount == 1);
	REQUIRE(delivered_ids == QList<int>{102, 101});
	REQUIRE(conn.serial->isOpen());
	REQUIRE(conn.socket()->isOpen());
	REQUIRE(conn.isBrokerConnected());
	QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
	REQUIRE(callback.isNull());
	REQUIRE(failed.call.isNull());
	REQUIRE(other.call.isNull());
}

DOCTEST_TEST_CASE("Unidentifiable serial CRC errors leave RpcCall to time out")
{
	auto message = RpcMessage(response(101));
	bool unreadable_meta = false;
	DOCTEST_SUBCASE("Unreadable response metadata") { unreadable_meta = true; }
	DOCTEST_SUBCASE("Request") {
		RpcRequest request;
		request.setRequestId(101);
		request.setMethod("get");
		request.setParams("crc-payload");
		message = request;
	}
	DOCTEST_SUBCASE("Signal") {
		RpcSignal signal;
		signal.setMethod("changed");
		signal.setParams("crc-payload");
		message = signal;
	}
	DOCTEST_SUBCASE("Routed response") { message.setCallerIds(RpcList{7}); }
	DOCTEST_SUBCASE("Zero request ID") { message.setRequestId(0); }
	DOCTEST_SUBCASE("Negative request ID") { message.setRequestId(-1); }
	DOCTEST_SUBCASE("Request ID would narrow to a pending call ID") { message.setRequestId(int64_t{4294967397}); }
	DOCTEST_SUBCASE("Fractional request ID") { message.setRequestId(101.5); }
	DOCTEST_SUBCASE("String request ID") { message.setRequestId("101"); }

	auto data = serial_frame(message, Rpc::ProtocolType::Cpon);
	if (unreadable_meta) {
		const auto end = data.indexOf('>');
		REQUIRE(end > 0);
		// Leave the request ID readable, but make the metadata incomplete.
		data[end] = ',';
	}
	data = corrupt_payload(data);
	SerialFrameReader reader(SerialFrameReader::CrcCheck::Yes);
	reader.addData(data.toStdString());
	REQUIRE(reader.takeResponseErrors().empty());
	REQUIRE(reader.takeFrames().empty());

	TestConnection conn;
	CallResult pending(conn, 101, 20);
	int receive_errors = 0;
	QEventLoop loop;
	QObject::connect(&conn, &ClientConnection::responseReceiveError, &loop, [&](int, const QString &) { ++receive_errors; });
	QObject::connect(pending.call, &RpcCall::maybeResult, &loop, &QEventLoop::quit);
	conn.serial->setDataToReceive(data);
	REQUIRE(pending.maybeCount == 0);
	REQUIRE(receive_errors == 0);
	QTimer::singleShot(1000, &loop, &QEventLoop::quit);
	loop.exec();
	REQUIRE(pending.maybeCount == 1);
	REQUIRE(pending.errorCount == 1);
	REQUIRE(pending.resultCount == 0);
	REQUIRE(pending.maybeError.code() == RpcError::MethodCallTimeout);
	REQUIRE(pending.signalError == pending.maybeError);
	REQUIRE(receive_errors == 0);
	REQUIRE(conn.serial->isOpen());
	REQUIRE(conn.isBrokerConnected());
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
DOCTEST_TEST_CASE("Serial CRC error propagates through RpcCall future")
{
	TestConnection conn;
	QObject context;
	auto *call = RpcCall::create(&conn);
	call->setParent(&context);
	call->setRequestId(101)->setShvPath("test")->setMethod("get");
	auto future = call->intoFuture();
	conn.serial->setDataToReceive(corrupt_payload(serial_frame(response(101))));
	QCoreApplication::processEvents();
	REQUIRE(future.isFinished());
	REQUIRE_THROWS_AS(future.result(), RpcException);
}
#endif

DOCTEST_TEST_CASE("Serial response CRC error queue drains and resets")
{
	SerialFrameReader reader(SerialFrameReader::CrcCheck::Yes);
	const auto valid = serial_frame(response(101));
	const auto corrupt = corrupt_payload(valid);
	reader.addData((corrupt + corrupt_payload(serial_frame(response(102))) + valid).toStdString());
	const auto errors = reader.takeResponseErrors();
	REQUIRE(errors.size() == 2);
	REQUIRE(errors[0].first == 101);
	REQUIRE(errors[1].first == 102);
	REQUIRE(errors[0].second.contains("CRC"));
	REQUIRE(errors[1].second.contains("CRC"));
	REQUIRE(reader.takeResponseErrors().empty());
	auto frames = reader.takeFrames();
	REQUIRE(frames.size() == 1);
	REQUIRE(frames.front().toRpcMessage().value() == response(101).value());

	reader.addData(corrupt.toStdString());
	reader.resetCommunication();
	REQUIRE(reader.takeResponseErrors().empty());
	REQUIRE(reader.takeFrames().empty());
	const auto split = valid.indexOf("crc-payload");
	REQUIRE(split > 0);
	REQUIRE(reader.addData(corrupt.left(split).toStdString()) == QList<int>{101});
	reader.resetCommunication();
	reader.addData(corrupt.mid(split).toStdString());
	REQUIRE(reader.takeResponseErrors().empty());
	REQUIRE(reader.takeFrames().empty());
	reader.addData(valid.toStdString());
	REQUIRE(reader.takeResponseErrors().empty());
	REQUIRE(reader.takeFrames().size() == 1);
}

// NOLINTEND
