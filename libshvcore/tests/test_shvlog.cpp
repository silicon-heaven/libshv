#include <shv/core/utils/shvfilejournal.h>
#include <shv/core/utils/shvlogheader.h>
#include <shv/core/utils/shvtypeinfo.h>
#include <shv/core/utils/shvjournalentry.h>
#include <shv/core/utils/shvlogfilereader.h>
#include <shv/core/utils/shvjournalfilewriter.h>
#include <shv/core/utils/shvjournalfilereader.h>
#include <shv/core/utils/shvmemoryjournal.h>
#include <shv/core/utils/shvlogfilter.h>
#include <shv/core/utils/shvlogrpcvaluereader.h>

#include <shv/core/log.h>

#include <shv/chainpack/chainpackwriter.h>
#include <shv/chainpack/cponwriter.h>

#include <filesystem>
#include <random>

#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

using namespace shv::core::utils;
using namespace shv::core;
using namespace shv::chainpack;
using namespace std;

namespace shv::chainpack {

doctest::String toString(const RpcValue& value) {
	return value.toCpon().c_str();
}

}

namespace {

void write_cpon_file(const string &fn, const RpcValue &log)
{
	ofstream out(fn, std::ios::binary | std::ios::out);
	CponWriterOptions opts;
	opts.setIndent("\t");
	CponWriter wr(out, opts);
	wr << log;
}

struct Channel
{
	ShvTypeInfo typeInfo;
	string domain;
	DataChange::ValueFlags valueFlags = 0;
	int minVal = 0;
	int maxVal = 0;
	uint16_t period = 1;
	uint16_t fasttime = 0;
	RpcValue value;
};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::map<string, Channel> channels;

const ShvTypeInfo typeInfo = ShvTypeInfo::fromVersion2(
	// types
	{
		{
			"Temperature",
			{
				ShvTypeDescr::Type::Decimal,
				ShvTypeDescr::SampleType::Continuous,
			}
		},
		{
			"Voltage",
			{
				ShvTypeDescr::Type::Int,
				ShvTypeDescr::SampleType::Continuous,
				{
					{"minVal", 0},
					{"maxVal", 100},
				},
			}
		},
		{
			"VetraStatus",
			{
				ShvTypeDescr::Type::BitField,
				{
					{"Active", "Bool", 0},
					{"LastDir", "UInt", RpcValue::List{1,2}, {{"description", "N,L,R,S"}}},
					{"CommError", "Bool", 3},
				},
			}
		},
		{
			"VehicleData",
			{
				ShvTypeDescr::Type::Map,
				{
					{"vehicleId", "UInt"},
					{"dir", "String", {}, {{"description", "N,L,R,S"}}},
					{"CommError", "Bool"},
				},
				ShvTypeDescr::SampleType::Discrete
			}
		},
	},
	// paths
	{
		ShvPropertyDescr{ "temperature", "Temperature" },
		ShvPropertyDescr{ "voltage", "Voltage" },
		ShvPropertyDescr{ "doorOpen", "Bool" },
		ShvPropertyDescr{ "someError", "Bool" },
		ShvPropertyDescr{ "vetra/status", "VetraStatus" },
		ShvPropertyDescr{ "vetra/vehicleDetected", "VehicleData" },
	}
);

void init_channels()
{
	nDebug() << "Registered topics:" << NecroLog::registeredTopicsInfo();
	{
		Channel &c = channels["temperature"];
		c.value = RpcValue::Decimal(2200, -2);
		c.period = 50;
		c.minVal = 0;
		c.maxVal = 5000;
		c.domain = ShvJournalEntry::DOMAIN_VAL_CHANGE;
	}
	{
		Channel &c = channels["voltage"];
		c.value = 0;
		c.minVal = 0;
		c.maxVal = 1500;
		c.domain = ShvJournalEntry::DOMAIN_VAL_FASTCHANGE;
	}
	{
		Channel &c = channels["doorOpen"];
		c.value = false;
		c.period = 500;
		c.minVal = 0;
		c.maxVal = 1;
		c.domain = ShvJournalEntry::DOMAIN_VAL_CHANGE;
	}
	{
		Channel &c = channels["someError"];
		c.value = false;
		c.period = 5;
		c.minVal = 0;
		c.maxVal = 1;
		c.domain = ShvJournalEntry::DOMAIN_VAL_CHANGE;
	}
	{
		Channel &c = channels["vetra/vehicleDetected"];
		c.value = RpcValue::Map{{"vehicleId", 1234}, {"direction", "R"}};
		c.period = 100;
		c.minVal = 6000;
		c.maxVal = 6999;
		c.domain = ShvJournalEntry::DOMAIN_VAL_CHANGE;
		c.valueFlags = 1 << DataChange::ValueFlag::Spontaneous;
	}
	{
		Channel &c = channels["vetra/status"];
		c.value = 100;
		c.minVal = 0;
		c.maxVal = 0xFF;
		c.domain = ShvJournalEntry::DOMAIN_VAL_CHANGE;
	}
}

const string TEST_DIR = filesystem::temp_directory_path().string() + "/test_ShvLog";
const string JOURNAL_DIR = TEST_DIR + "/journal";
const int64_t JOURNAL_FILES_CNT = 10;

void init_journal_dir()
{
	filesystem::remove_all(TEST_DIR);
	filesystem::create_directories(JOURNAL_DIR);
}

void init_file_journal(ShvFileJournal &file_journal)
{
	file_journal.setDeviceId("testdev");
	file_journal.setJournalDir(JOURNAL_DIR);
	file_journal.setFileSizeLimit(1024*64*10);
	file_journal.setJournalSizeLimit(file_journal.fileSizeLimit() * JOURNAL_FILES_CNT);
}

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
int64_t device_start1_msec = 0;
int64_t device_stop1_msec = 0;
int64_t device_start2_msec = 0;
int64_t device_stop2_msec = 0;
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

class TimeScope
{
public:
	TimeScope(const string &msg)
		: m_start(std::chrono::steady_clock::now())
		, m_message(msg) {  }
	~TimeScope()
	{
		nInfo() << m_message << "time elapsed:" << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_start).count() << "msec";
	}
private:
	std::chrono::time_point<std::chrono::steady_clock> m_start;
	string m_message;
};

}

int main(int argc, char** argv)
{
	init_channels();
	init_journal_dir();

	return doctest::Context(argc, argv).run();
}

DOCTEST_TEST_CASE("ShvLog")
{
	DOCTEST_SUBCASE("Create log files")
	{
		nDebug() << "------------- Journal init:" << JOURNAL_DIR;
		auto ts = TimeScope("Generating log files");
		auto msec = RpcValue::DateTime::now().msecsSinceEpoch();
		device_start1_msec = msec;
		std::random_device randev;
		std::mt19937 mt(randev());
		std::uniform_int_distribution<int> rndmsec(0, 2000);
		std::uniform_int_distribution<int> rndval(0, 1000);
		bool some_error = false;
		constexpr int CNT = 30000;
		for(int j=0; j<2; ++j) {
			if(j == 1) {
				static constexpr int64_t OFF_TIME_MSEC = 1000 * 60 * 123;
				device_stop1_msec = msec;
				msec += OFF_TIME_MSEC;
				device_start2_msec = msec;
				some_error = false;
			}

			ShvFileJournal file_journal;
			init_file_journal(file_journal);

			for (int i = 0; i < CNT; ++i) {
				msec += rndmsec(mt);
				for(auto &kv : channels) {
					Channel &c = kv.second;
					if(i % c.period == 0) {
						ShvJournalEntry e;
						e.epochMsec = msec;
						e.path = kv.first;
						e.domain = c.domain;
						e.value = RpcValue((c.maxVal - c.minVal) * rndval(mt) / 1000 + c.minVal);
						if(e.path == "temperature") {
							e.value = RpcValue::Decimal(e.value.toInt(), -2);
						}
						else if(e.path == "voltage") {
							e.shortTime = static_cast<int>(msec % 0x100);
						}
						else if(e.path == "doorOpen") {
							e.value = (rndval(mt) > 500);
						}
						else if(e.path == "someError") {
							if(!some_error) {
								// write some error only once
								some_error = true;
								e.value = true;
							}
							else {
								e.value = {};
							}
						}
						else if(e.path == "vetra/vehicleDetected") {
							e.value = RpcValue::Map{{"id", e.value}, {"direction", (i % 2)? "R": "L"}};
							e.setSpontaneous(true);
						}
						if(e.value.isValid())
							file_journal.append(e);
					}
				}
			}
		}
		device_stop2_msec = msec;
	}
	DOCTEST_SUBCASE("getlog()")
	{
		DOCTEST_SUBCASE("simple")
		{
			auto ts = TimeScope("getLog simple");
			ShvFileJournal file_journal;
			init_file_journal(file_journal);

			ShvGetLogParams params;
			params.withSnapshot = true;
			params.withPathsDict = false;
			params.since = RpcValue::DateTime::fromMSecsSinceEpoch(device_start1_msec + (device_stop2_msec - device_start1_msec) / 4);
			params.until = RpcValue::DateTime::fromMSecsSinceEpoch(device_stop2_msec - (device_stop2_msec - device_start1_msec) / 4);
			params.recordCountLimit = 10000;
			nDebug() << "\t params:" << params.toRpcValue().toCpon();
			RpcValue log1 = file_journal.getLog(params);
			string fn = TEST_DIR + "/log1.chpk";
			{
				ofstream out(fn, std::ios::binary | std::ios::out);
				ChainPackWriter wr(out);
				wr << log1;
			}
			{
				string fn2 = TEST_DIR + "/log1.cpon";
				nDebug() << "\t file:" << fn2;
				write_cpon_file(fn2, log1);
			}
			size_t cnt = 0;
			ShvJournalEntry first_entry;
			ShvJournalEntry last_entry;
			ShvLogFileReader rd(fn);
			while(rd.next()) {
				last_entry = rd.entry();
				if(cnt == 0)
					first_entry = last_entry;
				cnt++;
			}
			REQUIRE(rd.logHeader().since().toDateTime().msecsSinceEpoch() == first_entry.epochMsec);
			REQUIRE(cnt == rd.logHeader().recordCount());
			REQUIRE(cnt == log1.asList().size());
			shvInfo() << cnt << "records read";
		}
		DOCTEST_SUBCASE("filtered")
		{
			auto ts = TimeScope("getLog filtered");
			ShvFileJournal file_journal;
			init_file_journal(file_journal);

			ShvGetLogParams params;
			params.withSnapshot = true;
			params.withPathsDict = false;
			params.pathPattern = "doorOpen";
			params.since = RpcValue::DateTime::fromMSecsSinceEpoch(device_start1_msec + (device_stop2_msec - device_start2_msec) / 4);
			nDebug() << "\t params:" << params.toRpcValue().toCpon();
			RpcValue log1 = file_journal.getLog(params);
			string fn = TEST_DIR + "/log1-filtered.cpon";
			nDebug() << "\t file:" << fn;
			write_cpon_file(fn, log1);
			ShvLogRpcValueReader rd(log1);
			while(rd.next()) {
				auto e = rd.entry();
				REQUIRE(e.path == "doorOpen");
			}
		}
		DOCTEST_SUBCASE("testing SINCE_LAST")
		{
			ShvFileJournal file_journal;
			init_file_journal(file_journal);

			ShvGetLogParams params;
			params.withSnapshot = true;
			params.withPathsDict = false;
			params.since = ShvGetLogParams::SINCE_LAST;
			RpcValue log1 = file_journal.getLog(params);
			write_cpon_file(TEST_DIR + "/log1-since-now.cpon", log1);
			ShvLogRpcValueReader rd(log1);
			while(rd.next()) {
				const ShvJournalEntry &e = rd.entry();
				REQUIRE(e.isSnapshotValue());
				REQUIRE(!e.isSpontaneous());
				REQUIRE(rd.logHeader().since().toDateTime().msecsSinceEpoch() == e.epochMsec);
				REQUIRE(rd.logHeader().until().toDateTime().msecsSinceEpoch() == e.epochMsec);
				REQUIRE(device_stop2_msec == e.epochMsec);
			}
		}
		DOCTEST_SUBCASE("testing SINCE_LAST filtered")
		{
			ShvFileJournal file_journal;
			init_file_journal(file_journal);

			ShvGetLogParams params;
			params.withSnapshot = true;
			params.withPathsDict = false;
			params.since = ShvGetLogParams::SINCE_LAST;
			params.pathPattern = "someError";
			RpcValue log1 = file_journal.getLog(params);
			//even if someError is set only once, it should be in last snapshot
			REQUIRE(log1.asList().empty() == false);
			ShvLogRpcValueReader rd(log1);
			REQUIRE(rd.logHeader().recordCount() == 1);
			while(rd.next()) {
				const ShvJournalEntry &e = rd.entry();
				REQUIRE(e.isSnapshotValue());
				REQUIRE(!e.isSpontaneous());
				REQUIRE(rd.logHeader().since().toDateTime().msecsSinceEpoch() == e.epochMsec);
				REQUIRE(rd.logHeader().until().toDateTime().msecsSinceEpoch() == e.epochMsec);
				REQUIRE(e.path == "someError");
				REQUIRE(e.value == true);
			}
		}
	}
	DOCTEST_SUBCASE("dirty log + memory log")
	{
		std::random_device randev;
		std::mt19937 mt(randev());
		std::uniform_int_distribution<int> rndmsec(0, 2000);
		std::uniform_int_distribution<int> rndval(0, 1000);

		unsigned dirty_cnt = 0;
		string dirty_fn = TEST_DIR + "/dirty.log2";
		ShvJournalFileWriter dirty_log(dirty_fn);
		ShvMemoryJournal memory_jurnal;
		memory_jurnal.setTypeInfo(typeInfo);
		int64_t msec = device_stop2_msec;
		for (int i = 0; i < 10000; ++i) {
			msec += rndmsec(mt);
			for(auto &kv : channels) {
				Channel &c = kv.second;
				if(i % c.period == 0) {
					ShvJournalEntry e;
					e.epochMsec = msec;
					e.path = kv.first;
					RpcValue rv((c.maxVal - c.minVal) * rndval(mt) / 1000 + c.minVal);
					if(e.path == "temperature") {
						e.value = RpcValue::Decimal(rv.toInt(), -2);
					}
					else if(e.path == "vetra/vehicleDetected") {
						e.value = RpcValue::List{rv, i %2? "R": "L"};
					}
					else {
						e.value = rv;
					}
					e.domain = c.domain;
					e.valueFlags = c.valueFlags;
					e.shortTime = static_cast<int>(msec % 0x100);
					dirty_log.append(e);
					memory_jurnal.append(e);
					dirty_cnt++;
				}
			}
		}
		{
			ShvJournalFileReader rd2(dirty_fn);
			unsigned cnt = 0;
			while (rd2.next()) {
				const ShvJournalEntry &e1 = rd2.entry();
				REQUIRE(cnt < dirty_cnt);
				const ShvJournalEntry &e2 = memory_jurnal.entries()[cnt++];
				REQUIRE(e1 == e2);
			}
			REQUIRE(cnt == dirty_cnt);
			{
				cnt--;
				rd2.last();
				const ShvJournalEntry &e1 = rd2.entry();
				const ShvJournalEntry &e2 = memory_jurnal.entries()[cnt];
				REQUIRE(e1 == e2);
			}
		}
	}
	DOCTEST_SUBCASE("Read chainpack to memory log")
	{
		nDebug() << "------------- Read chainpack to memory log";
		string fn1 = TEST_DIR + "/log1.chpk";
		string fn2 = TEST_DIR + "/log2.chpk";
		ShvMemoryJournal memory_jurnal;
		memory_jurnal.loadLog(fn1);
		{
			ofstream out(fn2, std::ios::binary | std::ios::out);
			ChainPackWriter wr(out);
			wr << memory_jurnal.getLog(ShvGetLogParams());
		}
		unsigned cnt = 0;
		ShvLogFileReader rd2(fn2);
		while (rd2.next()) {
			const ShvJournalEntry &e1 = rd2.entry();
			REQUIRE(cnt < memory_jurnal.entries().size());
			const ShvJournalEntry &e2 = memory_jurnal.entries()[cnt++];
			REQUIRE(e1 == e2);
		}
	}
	DOCTEST_SUBCASE("Filtering chainpack appends to memory log 1")
	{
		nDebug() << "------------- Filtering chainpack appends to memory log 1";
		ShvGetLogParams params;
		params.pathPattern = "**/vetra/**";
		ShvLogFilter filter(params);
		ShvMemoryJournal mmj;
		string fn1 = TEST_DIR + "/log1.chpk";
		ShvLogFileReader rd1(fn1);
		while (rd1.next()) {
			const ShvJournalEntry &e1 = rd1.entry();
			if (filter.match(e1)) {
				mmj.append(e1);
			}
		}
		for(const auto &e : mmj.entries()) {
			REQUIRE(e.path.find("vetra") != string::npos);
		}
	}
	DOCTEST_SUBCASE("Filtering chainpack appends to memory log 2")
	{
		string fn1 = TEST_DIR + "/log1.chpk";
		ShvLogFileReader rd1(fn1);
		ShvGetLogParams params;
		params.pathPattern = "((temp.*)|(volt.+))";
		params.pathPatternType = ShvGetLogParams::PatternType::RegEx;
		int64_t dt1 = rd1.logHeader().since().toDateTime().msecsSinceEpoch();
		REQUIRE(dt1 > 0);
		int64_t dt2 = rd1.logHeader().until().toDateTime().msecsSinceEpoch();
		REQUIRE(dt2 > 0);
		auto dt_since = dt1 + (dt2 - dt1) / 3;
		auto dt_until = dt2 - (dt2 - dt1) / 3;
		params.since = RpcValue::DateTime::fromMSecsSinceEpoch(dt_since);
		params.until = RpcValue::DateTime::fromMSecsSinceEpoch(dt_until);
		ShvLogFilter filter(params);
		ShvMemoryJournal mmj;
		while (rd1.next()) {
			const ShvJournalEntry &e1 = rd1.entry();
			if (filter.match(e1)) {
				mmj.append(e1);
			}
		}
		for(const auto &e : mmj.entries()) {
			REQUIRE((e.path.find("temp") == 0 || e.path.find("volt") == 0) == true);
			REQUIRE(e.epochMsec >= dt_since);
			REQUIRE(e.epochMsec < dt_until);
		}
		{
			ShvGetLogParams params2;
			params2.withPathsDict = false;
			params2.withSnapshot = true;
			write_cpon_file(TEST_DIR + "/memlog.cpon", mmj.getLog(params2));
		}
	}
	DOCTEST_SUBCASE("Test log rotate")
	{
		ShvFileJournal file_journal;
		init_file_journal(file_journal);

		int64_t msec = device_stop2_msec + 1000 * 60 * 24;

		for (int i = 0; i < 30000; ++i) {
			msec += 1234;
			ShvJournalEntry e;
			e.epochMsec = msec;
			e.path = "log/rotate/test";
			e.domain = "chng";
			e.value = true;
			e.setSpontaneous(true);
			file_journal.append(e);
		}
		const auto &ctx = file_journal.checkJournalContext(true);
		REQUIRE(ctx.files.size() == JOURNAL_FILES_CNT);
	}
}
