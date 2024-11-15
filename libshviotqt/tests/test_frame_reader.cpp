#include <shv/coreqt/rpc.h>
#include <shv/coreqt/log.h>
#include <shv/coreqt/utils.h>
#include <shv/iotqt/rpc/serialportsocket.h>
#include <shv/iotqt/rpc/socket.h>

#include <QBuffer>
#include <QVariant>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

using namespace shv::coreqt::rpc;
using namespace shv::iotqt::rpc;
using namespace shv::chainpack;
using namespace std;

doctest::String toString(const QString& str)
{
	return str.toLatin1().data();
}

doctest::String toString(const QList<int>& lst)
{
	QStringList sl;
	for(auto i : lst) {
		sl << QString::number(i);
	}
	return ('[' + sl.join(',') + ']').toLatin1().data();
}

vector<string> msg_to_raw_data(const vector<string> &cpons)
{
	vector<string> ret;
	for (const auto &cpon : cpons) {
		auto rv = RpcValue::fromCpon(cpon);
		auto msg = RpcMessage(rv);
		StreamFrameWriter wr;
		wr.addFrame(msg.toRpcFrame());
		QByteArray ba;
		{
			QBuffer buffer(&ba);
			buffer.open(QIODevice::WriteOnly);
			wr.flushToDevice(&buffer);
		}
		ret.emplace_back(ba.constData(), ba.size());
	}
	return ret;
}
vector<string> msg_to_raw_data_serial(const vector<string> &cpons, SerialFrameWriter::CrcCheck crc_check)
{
	vector<string> ret;
	for (const auto &cpon : cpons) {
		auto rv = RpcValue::fromCpon(cpon);
		auto msg = RpcMessage(rv);
		SerialFrameWriter wr(crc_check);
		wr.addFrame(msg.toRpcFrame());
		QByteArray ba;
		{
			QBuffer buffer(&ba);
			buffer.open(QIODevice::WriteOnly);
			wr.flushToDevice(&buffer);
		}
		ret.emplace_back(ba.constData(), ba.size());
	}
	return ret;
}

void test_valid_data(FrameReader *rd, const vector<string> &data)
{
	auto rq1 = data[0];
	auto rs1 = data[1];
	auto rs2 = data[2];
	auto sig1 = data[3];
	{
		auto ret = rd->addData(rq1);
		REQUIRE(ret.isEmpty());
		REQUIRE(rd->takeFrames().size() == 1);
	}
	{
		auto ret = rd->addData(rs1);
		REQUIRE(ret == QList<int>{3,});
		REQUIRE(rd->takeFrames().size() == 1);
	}
	{
		auto ret = rd->addData(rs1 + rs2);
		REQUIRE(ret == QList<int>{3,2});
		REQUIRE(rd->takeFrames().size() == 2);
	}
	{
		auto ret = rd->addData(rs1 + sig1 + rs2 + sig1);
		REQUIRE(ret == QList<int>{3,2});
		REQUIRE(rd->takeFrames().size() == 4);
	}
}

void test_incomplete_data(FrameReader *rd, const vector<string> &data)
{
	auto rq1 = data[0];
	auto rs1 = data[1];
	auto rs2 = data[2];
	auto sig1 = data[3];
	vector<string> chunks;
	vector<size_t> ixs = {0, 4, rs1.size() - 2, rs1.size()};
	for (size_t i = 1; i < ixs.size(); i++) {
		chunks.push_back(rs1.substr(ixs[i - 1], ixs[i] - ixs[i - 1]));
	}
	{
		auto ret = rd->addData(chunks[0]);
		REQUIRE(ret == QList<int>{});
		REQUIRE(rd->takeFrames().size() == 0);
	}
	{
		auto ret = rd->addData(chunks[1]);
		REQUIRE(ret == QList<int>{3,});
		REQUIRE(rd->takeFrames().size() == 0);
	}
	{
		auto ret = rd->addData(chunks[2]);
		REQUIRE(ret == QList<int>{});
		REQUIRE(rd->takeFrames().size() == 1);
	}
}
const vector<string> cpons = {
	R"(<1:1,8:3,9:"shv",10:"ls">i{1:"cze"})",
	R"(<1:1,8:3>i{2:true})",
	R"(<1:1,8:2>i{2:true})",
	R"(<1:1,9:"shv",10:"lsmod">i{1:{"cze":true}})",
};

DOCTEST_TEST_CASE("Stream FrameReader")
{
	auto data = msg_to_raw_data(cpons);

	DOCTEST_SUBCASE("Valid data")
	{
		StreamFrameReader rd;
		test_valid_data(&rd, data);
	}
	DOCTEST_SUBCASE("Incomplete data")
	{
		StreamFrameReader rd;
		test_incomplete_data(&rd, data);
	}
}

DOCTEST_TEST_CASE("Serial FrameReader with CRC check")
{
	auto crc_check_wr = SerialFrameWriter::CrcCheck::Yes;
	auto crc_check_rd = SerialFrameReader::CrcCheck::Yes;
	auto data = msg_to_raw_data_serial(cpons, crc_check_wr);

	DOCTEST_SUBCASE("Valid data")
	{
		SerialFrameReader rd(crc_check_rd);
		test_valid_data(&rd, data);
	}
	DOCTEST_SUBCASE("Incomplete data")
	{
		SerialFrameReader rd(crc_check_rd);
		test_incomplete_data(&rd, data);
	}
}

DOCTEST_TEST_CASE("Serial FrameReader without CRC check")
{
	auto crc_check_wr = SerialFrameWriter::CrcCheck::No;
	auto crc_check_rd = SerialFrameReader::CrcCheck::No;
	auto data = msg_to_raw_data_serial(cpons, crc_check_wr);

	DOCTEST_SUBCASE("Valid data")
	{
		SerialFrameReader rd(crc_check_rd);
		test_valid_data(&rd, data);
	}
	DOCTEST_SUBCASE("Incomplete data")
	{
		SerialFrameReader rd(crc_check_rd);
		test_incomplete_data(&rd, data);
	}
}

