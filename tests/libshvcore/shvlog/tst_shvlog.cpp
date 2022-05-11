#include <shv/core/utils/shvfilejournal.h>
#include <shv/core/utils/shvlogheader.h>
#include <shv/core/utils/shvlogtypeinfo.h>
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

#include <QtTest/QtTest>
#include <QDebug>
#include <QDir>

#include <fstream>

using namespace std;
using namespace shv::core::utils;
using namespace shv::chainpack;

namespace {

QDebug operator<<(QDebug debug, const std::string &s)
{
	debug << s.c_str();
	return debug;
}

void write_cpon_file(const string &fn, const RpcValue &log)
{
	ofstream out(fn, std::ios::binary | std::ios::out);
	CponWriterOptions opts;
	opts.setIndent("\t");
	CponWriter wr(out, opts);
	wr << log;
}

const string TEST_DIR = "/tmp/TestShvLog";
const string JOURNAL_DIR = TEST_DIR + "/journal";

struct Channel
{
	ShvLogTypeInfo typeInfo;
	string domain;
	DataChange::ValueFlags valueFlags = 0;
	int minVal = 0;
	int maxVal = 0;
	uint16_t period = 1;
	uint16_t fasttime = 0;
	RpcValue value;
};

std::map<string, Channel> channels;
/*
void snapshot_fn(std::vector<ShvJournalEntry> &ev)
{
	for(const auto &kv : channels) {
		ShvJournalEntry e;
		e.path = kv.first;
		e.value = kv.second.value;
		e.domain = kv.second.domain;
		e.valueFlags = kv.second.valueFlags;
		ev.push_back(std::move(e));
	}
}
*/
ShvLogTypeInfo typeInfo
{
	// types
	{
		{
			"Temperature",
			{
				ShvLogTypeDescr::Type::Decimal,
				ShvLogTypeDescr::SampleType::Continuous,
				{
					{ShvLogTypeDescr::OPT_MIN_VAL, RpcValue::Decimal(0, -2)},
					{ShvLogTypeDescr::OPT_MIN_VAL, RpcValue::Decimal(10000, -2)},
				},
			}
		},
		{
			"Voltage",
			{
				ShvLogTypeDescr::Type::Int,
				ShvLogTypeDescr::SampleType::Continuous,
				{
					{ShvLogTypeDescr::OPT_MIN_VAL, RpcValue{0}},
					{ShvLogTypeDescr::OPT_MIN_VAL, RpcValue{100}},
				},
			}
		},
		{
			"VetraStatus",
			{
				ShvLogTypeDescr::Type::BitField,
				{
					{"Active", 0},
					{"LastDir", "UInt", RpcValue::List{1,2}, "N,L,R,S"},
					{"CommError", 3},
				},
			}
		},
		{
			"VehicleData",
			{
				ShvLogTypeDescr::Type::Map,
				{
					{"vehicleId", "UInt"},
					{"dir", "String", RpcValue::List{"", "L", "R", "S"}},
					{"CommError", "Bool"},
				},
				ShvLogTypeDescr::SampleType::Discrete
			}
		},
	},
	// paths
	{
		{ "temperature", {"Temperature"} },
		{ "voltage", {"Voltage"} },
		{ "doorOpen", {"Bool"} },
		{ "someError", {"Bool"} },
		{ "vetra/status", {"VetraStatus"} },
		{ "vetra/vehicleDetected", {"VehicleData"} },
	},
};
}

class TestShvLog: public QObject
{
	Q_OBJECT
private:
	void test1()
	{
		qDebug() << "============= TestShvFileReader ============\n";
		{
			qDebug() << "------------- Journal init:" << JOURNAL_DIR;
			if(!QDir(QString::fromStdString(JOURNAL_DIR)).removeRecursively())
				qWarning() << "Cannot delete journal dir:" << JOURNAL_DIR;
			//if(!QDir().mkpath(QString::fromStdString(JOURNAL_DIR)))
			//	qWarning() << "Cannot create journal dir:" << JOURNAL_DIR;
			ShvFileJournal file_journal("testdev");
			file_journal.setJournalDir(JOURNAL_DIR);
			file_journal.setFileSizeLimit(1024*64*20);
			file_journal.setJournalSizeLimit(file_journal.fileSizeLimit() * 10);
			qDebug() << "------------- Generating log files";
			auto msec = RpcValue::DateTime::now().msecsSinceEpoch();
			int64_t msec_start = msec;
			int64_t msec_middle;
			std::random_device randev;
			std::mt19937 mt(randev());
			std::uniform_int_distribution<int> rndmsec(0, 2000);
			std::uniform_int_distribution<int> rndval(0, 1000);
			bool some_error = false;
			constexpr int CNT = 30000;
			for(int j=0; j<2; ++j) {
				msec_middle = msec;
				file_journal.clearSnapshot();
				file_journal.createNewLogFile(msec);
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
								e.shortTime = msec % 0x100;
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
			qDebug() << "\t" << CNT << "records written, recent timestamp:" << RpcValue::DateTime::fromMSecsSinceEpoch(file_journal.recentlyWrittenEntryDateTime()).toIsoString();
			int64_t msec_end = msec;
			qDebug() << "------------- testing getlog()";
			{
				ShvGetLogParams params;
				params.withSnapshot = true;
				params.withPathsDict = false;
				params.since = RpcValue::DateTime::fromMSecsSinceEpoch(msec_start + (msec_end - msec_start) / 4);
				params.until = RpcValue::DateTime::fromMSecsSinceEpoch(msec_end - (msec_end - msec_start) / 4);
				qDebug() << "\t params:" << params.toRpcValue().toCpon();
				RpcValue log1 = file_journal.getLog(params);
				string fn = TEST_DIR + "/log1.chpk";
				{
					ofstream out(fn, std::ios::binary | std::ios::out);
					ChainPackWriter wr(out);
					wr << log1;
				}
				{
					string fn2 = TEST_DIR + "/log1.cpon";
					qDebug() << "\t file:" << fn2;
					write_cpon_file(fn2, log1);
				}
				int cnt = 0;
				ShvLogFileReader rd(fn);
				while(rd.next()) {
					//rd.entry();
					cnt++;
				}
				QVERIFY(cnt == rd.logHeader().recordCount());
				QVERIFY(cnt == (int)log1.toList().size());
			}
			qDebug() << "------------- testing getlog() file merge snapshot erasure";
			{
				ShvGetLogParams params;
				params.withSnapshot = true;
				params.withPathsDict = false;
				params.since = RpcValue::DateTime::fromMSecsSinceEpoch(msec_middle - 100);
				params.until = RpcValue::DateTime::fromMSecsSinceEpoch(msec_middle + 100);
				qDebug() << "\t params:" << params.toRpcValue().toCpon();
				RpcValue log1 = file_journal.getLog(params);
				{
					string fn2 = TEST_DIR + "/log-merged.cpon";
					qDebug() << "\t file:" << fn2;
					write_cpon_file(fn2, log1);
				}
				std::vector<bool> some_errors;
				ShvLogRpcValueReader rd(log1);
				while(rd.next()) {
					const ShvJournalEntry &e = rd.entry();
					if(e.path == "someError") {
						qDebug() << "\t e:" << e.toRpcValue().toCpon();
						some_errors.push_back(e.value.toBool());
					}
				}
				std::vector<bool> v = {true, false};
				QCOMPARE(some_errors, v);
			}
			qDebug() << "------------- testing getlog() filtered";
			{
				ShvGetLogParams params;
				params.withSnapshot = true;
				params.withPathsDict = false;
				params.pathPattern = "doorOpen";
				params.since = RpcValue::DateTime::fromMSecsSinceEpoch(msec_start + (msec_end - msec_start) / 4);
				//params.until = RpcValue::DateTime::fromMSecsSinceEpoch(msec2 - (msec2 - msec1) / 4);
				qDebug() << "\t params:" << params.toRpcValue().toCpon();
				RpcValue log1 = file_journal.getLog(params);
				string fn = TEST_DIR + "/log1-filtered.cpon";
				qDebug() << "\t file:" << fn;
				write_cpon_file(fn, log1);
				ShvLogRpcValueReader rd(log1);
				while(rd.next()) {
					auto e = rd.entry();
					QVERIFY(e.path == "doorOpen");
				}
			}
			qDebug() << "------------- testing SINCE_LAST";
			{
				ShvGetLogParams params;
				params.withSnapshot = true;
				params.withPathsDict = false;
				params.since = ShvGetLogParams::SINCE_LAST;
				RpcValue log1 = file_journal.getLog(params);
				write_cpon_file(TEST_DIR + "/log1-since-now.cpon", log1);
				ShvLogRpcValueReader rd(log1);
				while(rd.next()) {
					const ShvJournalEntry &e = rd.entry();
					//shvWarning() << "entry:" << e.toRpcValueMap().toCpon();
					QVERIFY(e.isSnapshotValue());
					QVERIFY(!e.isSpontaneous());
				}
			}
			qDebug() << "------------- testing SINCE_LAST filtered";
			{
				ShvGetLogParams params;
				params.withSnapshot = true;
				params.withPathsDict = false;
				params.since = ShvGetLogParams::SINCE_LAST;
				params.pathPattern = "someError";
				RpcValue log1 = file_journal.getLog(params);
				//someError is false in last log file, so it should not be in snapshot
				QVERIFY(log1.asList().empty());
			}
			qDebug() << "------------- testing since middle msec";
			NecroLog::setTopicsLogTresholds("ShvJournal:D");
			{
				ShvGetLogParams params;
				params.withSnapshot = true;
				params.withPathsDict = false;
				params.since = RpcValue::DateTime::fromMSecsSinceEpoch(msec_middle - 1);
				params.pathPattern = "someError";
				RpcValue log1 = file_journal.getLog(params);
				write_cpon_file(TEST_DIR + "/log1-since-middle.cpon", log1);
				int cnt = 0;
				ShvLogRpcValueReader rd(log1);
				while(rd.next()) {
					const ShvJournalEntry &e = rd.entry();
					//shvWarning() << "entry:" << e.toRpcValueMap().toCpon();
					QVERIFY(!e.isSpontaneous());
					QVERIFY(e.path == "someError");
					QVERIFY(e.value == (cnt == 0)? true: false);
					cnt++;
				}
				QVERIFY(cnt == 2);
			}

			qDebug() << "------------- Generating dirty log + memory log";
			unsigned dirty_cnt = 0;
			string dirty_fn = JOURNAL_DIR + "/dirty.log2";
			ShvJournalFileWriter dirty_log(dirty_fn);
			ShvMemoryJournal memory_jurnal;
			memory_jurnal.setTypeInfo(typeInfo);
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
							//e.sampleType = ShvLogTypeDescr::SampleType::Discrete;
						}
						else {
							e.value = rv;
						}
						e.domain = c.domain;
						e.valueFlags = c.valueFlags;
						e.shortTime = msec % 0x100;
						dirty_log.append(e);
						memory_jurnal.append(e);
						dirty_cnt++;
						//if(dirty_cnt < 10)
						//	qDebug() << dirty_cnt << "INS:" << e.toRpcValueMap().toCpon();
					}
				}
			}
			{
				ShvJournalFileReader rd2(dirty_fn);
				unsigned cnt = 0;
				while (rd2.next()) {
					const ShvJournalEntry &e1 = rd2.entry();
					QVERIFY(cnt < dirty_cnt);
					const ShvJournalEntry &e2 = memory_jurnal.entries()[cnt++];
					//qDebug() << cnt << e1.toRpcValueMap().toCpon() << "vs" << e2.toRpcValueMap().toCpon();
					QVERIFY(e1 == e2);
				}
				QVERIFY(cnt == dirty_cnt);
				{
					cnt--;
					rd2.last();
					const ShvJournalEntry &e1 = rd2.entry();
					const ShvJournalEntry &e2 = memory_jurnal.entries()[cnt++];
					//qDebug() << cnt << e1.toRpcValueMap().toCpon() << "vs" << e2.toRpcValueMap().toCpon();
					QVERIFY(e1 == e2);
				}
			}
			{
				qDebug() << "------------- Read chainpack to memory log";
				string fn1 = TEST_DIR + "/log1.chpk";
				string fn2 = TEST_DIR + "/log2.chpk";
				//ShvMemoryJournal memory_jurnal;
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
					QVERIFY(cnt < memory_jurnal.entries().size());
					const ShvJournalEntry &e2 = memory_jurnal.entries()[cnt++];
					//qDebug() << cnt << e1.toRpcValueMap().toCpon() << "vs" << e2.toRpcValueMap().toCpon();
					QVERIFY(e1 == e2);
				}
			}
			{
				qDebug() << "------------- Filtering chainpack appends to memory log 1";
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
				for(auto e : mmj.entries()) {
					QVERIFY(e.path.find("vetra") != string::npos);
				}
			}
			{
				qDebug() << "------------- Filtering chainpack appends to memory log 2";
				string fn1 = TEST_DIR + "/log1.chpk";
				ShvLogFileReader rd1(fn1);
				ShvGetLogParams params;
				params.pathPattern = "((temp.*)|(volt.+))";
				params.pathPatternType = ShvGetLogParams::PatternType::RegEx;
				int64_t dt1 = rd1.logHeader().since().toDateTime().msecsSinceEpoch();
				QVERIFY(dt1 > 0);
				int64_t dt2 = rd1.logHeader().until().toDateTime().msecsSinceEpoch();
				QVERIFY(dt2 > 0);
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
					//qDebug() << e.toRpcValueMap().toCpon();
					QVERIFY(e.path.find("temp") == 0 || e.path.find("volt") == 0);
					QVERIFY(e.epochMsec >= dt_since);
					QVERIFY(e.epochMsec < dt_until);
				}
				{
					ShvGetLogParams params;
					params.withPathsDict = false;
					params.withSnapshot = true;
					write_cpon_file(TEST_DIR + "/memlog.cpon", mmj.getLog(params));
				}
			}
		}
	}
private slots:
	void initTestCase()
	{
		//shv::chainpack::Exception::setAbortOnException(true);
		//NecroLog::setTopicsLogTresholds("ShvJournal:D");
		qDebug() << "Registered topics:" << NecroLog::registeredTopicsInfo();
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
	void tests()
	{
		test1();
	}

	void cleanupTestCase()
	{
		//qDebug("called after firstTest and secondTest");
	}
};

QTEST_MAIN(TestShvLog)
#include "tst_shvlog.moc"
