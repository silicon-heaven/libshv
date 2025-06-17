#include "utils.h"
#include "log.h"
#include "stringview.h"

#include <regex>
#include <sstream>

#include <string.h>
#include <unistd.h>

using namespace std;
using namespace shv::chainpack;

namespace shv {
namespace core {

namespace {
template<typename Out>
void split(const std::string &s, char delim, Out result)
{
	std::stringstream ss;
	ss.str(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		*(result++) = item;
	}
}

std::vector<std::string> split(const std::string &s, char delim)
{
	std::vector<std::string> elems;
	split(s, delim, std::back_inserter(elems));
	return elems;
}
}

int Utils::versionStringToInt(const std::string &version_string)
{
	int ret = 0;
	for(auto s : split(version_string, '.')) {
		int i = ::atoi(s.c_str());
		ret = 100 * ret + i;
	}
	return ret;
}

std::string Utils::intToVersionString(int ver)
{
	std::string ret;
	while(ver) {
		int i = ver % 100;
		ver /= 100;
		std::string s = shv::core::Utils::toString(i);
		//if(i < 10 && ver > 0)
		//	s = '0' + s;
		if(ret.empty())
			ret = s;
		else
			ret = s + '.' + ret;
	}
	return ret;
}

std::string Utils::removeJsonComments(const std::string &json_str)
{
	// http://blog.ostermiller.org/find-comment
	const std::regex re_block_comment("/\\*(?:.|[\\n])*?\\*/");
	const std::regex re_line_comment("//.*[\\n]");
	std::string result1 = std::regex_replace(json_str, re_block_comment, std::string());
	std::string ret = std::regex_replace(result1, re_line_comment, std::string());
	return ret;
}

std::string Utils::binaryDump(const std::string &bytes)
{
	std::string ret;
	for (size_t i = 0; i < bytes.size(); ++i) {
		uint8_t u = bytes[i];
		if(i > 0)
			ret += '|';
		for (size_t j = 0; j < 8*sizeof(u); ++j) {
			ret += (u & (((uint8_t)128) >> j))? '1': '0';
		}
	}
	return ret;
}

static inline char hex_nibble(char i)
{
	if(i < 10)
		return '0' + i;
	return 'A' + (i - 10);
}

std::string Utils::toHex(const std::string &bytes)
{
	std::string ret;
	for (size_t i = 0; i < bytes.size(); ++i) {
		unsigned char b = bytes[i];
		char h = b / 16;
		char l = b % 16;
		ret += hex_nibble(h);
		ret += hex_nibble(l);
	}
	return ret;
}

std::string Utils::toHex(const std::basic_string<uint8_t> &bytes)
{
	std::string ret;
	for (size_t i = 0; i < bytes.size(); ++i) {
		unsigned char b = bytes[i];
		char h = b / 16;
		char l = b % 16;
		ret += hex_nibble(h);
		ret += hex_nibble(l);
	}
	return ret;
}

static inline char unhex_char(char c)
{
	if(c >= '0' && c <= '9')
		return c - '0';
	if(c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if(c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return char(0);
}

std::string Utils::fromHex(const std::string &bytes)
{
	std::string ret;
	for (size_t i = 0; i < bytes.size(); ) {
		unsigned char u = unhex_char(bytes[i++]);
		u = 16 * u;
		if(i < bytes.size())
			u += unhex_char(bytes[i++]);
		ret.push_back(u);
	}
	return ret;
}

static void create_key_val(RpcValue &map, const StringViewList &path, const RpcValue &val)
{
	if(path.empty())
		return;
	if(path.size() == 1) {
		map.set(path[path.length() - 1].toString(), val);
	}
	else {
		string key = path[0].toString();
		RpcValue mval = map.at(key);
		if(!mval.isMap())
			mval = RpcValue::Map();
		create_key_val(mval, path.mid(1), val);
		map.set(key, mval);
	}
}

RpcValue Utils::foldMap(const chainpack::RpcValue::Map &plain_map, char key_delimiter)
{
	RpcValue ret = RpcValue::Map();
	for(const auto &kv : plain_map) {
		const string &key = kv.first;
		StringViewList lst = StringView(key).split(key_delimiter);
		create_key_val(ret, lst, kv.second);
	}
	return ret;
}

std::string Utils::joinPath(const StringView &p1, const StringView &p2)
{
	StringView sv1(p1);
	while(sv1.endsWith('/'))
		sv1 = sv1.mid(0, sv1.size() - 1);
	StringView sv2(p2);
	while(sv2.startsWith('/'))
		sv2 = sv2.mid(1);
	if(sv2.empty())
		return sv1.toString();
	if(sv1.empty())
		return sv2.toString();
	return sv1.toString() + '/' + sv2.toString();
}

std::string Utils::simplifyPath(const std::string &p)
{
	StringViewList ret;
	StringViewList lst = StringView(p).split('/');
	for (size_t i = 0; i < lst.size(); ++i) {
		const StringView &s = lst[i];
		if(s == ".")
			continue;
		if(s == "..") {
			if(!ret.empty())
				ret.pop_back();
			continue;
		}
		ret.push_back(s);
	}
	return ret.join('/');
}

std::vector<char> Utils::readAllFd(int fd)
{
	/// will not work with blockong read !!!
	/// one possible solution for the blocking sockets, pipes, FIFOs, and tty's is
	/// ioctl(fd, FIONREAD, &n);
	static constexpr ssize_t CHUNK_SIZE = 1024;
	std::vector<char> ret;
	while(true) {
		size_t prev_size = ret.size();
		ret.resize(prev_size + CHUNK_SIZE);
		ssize_t n = ::read(fd, ret.data() + prev_size, CHUNK_SIZE);
		if(n < 0) {
			if(errno == EAGAIN) {
				if(prev_size && (prev_size % CHUNK_SIZE)) {
					shvError() << "no data available, returning so far read bytes:" << prev_size << "number of chunks:" << (prev_size/CHUNK_SIZE);
				}
				else {
					/// can happen if previous read returned exactly CHUNK_SIZE
				}
				ret.resize(prev_size);
				return ret;
			}
			else {
				shvError() << "error read fd:" << fd << ::strerror(errno);
				return std::vector<char>();
			}
		}
		if(n < CHUNK_SIZE) {
			ret.resize(prev_size + n);
			break;
		}
#ifdef USE_IOCTL_FIONREAD
		else if(n == CHUNK_SIZE) {
			if(S_ISFIFO(mode) || S_ISSOCK(mode)) {
				if(::ioctl(fd, FIONREAD, &n) < 0) {
					shvError() << "error ioctl(FIONREAD) fd:" << fd << ::strerror(errno);
					return ret;
				}
				if(n == 0)
					return ret;
			}
		}
#endif
	}
	return ret;
}

#if 0
const QString& Utils::nullValueString()
{
	static QString n = QStringLiteral("null");
	return n;
}

void qf::core::Utils::parseFieldName(const QString &full_field_name, QString *pfield_name, QString *ptable_name, QString *pdb_name)
{
	QString s = full_field_name;
	QString field_name, table_name, db_name;
	field_name = s;

	int ix = s.lastIndexOf('.');
	if(ix >= 0) {
		field_name = s.mid(ix+1);
		s = s.mid(0, ix);
		table_name = s;

		ix = s.lastIndexOf('.');
		if(ix >= 0) {
			table_name = s.mid(ix+1);
			s = s.mid(0, ix);
			db_name = s;
		}
	}

	if(pfield_name) *pfield_name = field_name;
	if(ptable_name) *ptable_name = table_name;
	if(pdb_name) *pdb_name = db_name;
}

QString Utils::composeFieldName(const QString &field_name, const QString &table_name, const QString &db_name)
{
	QString ret;
	if(!field_name.isEmpty()) {
		ret = field_name;
		if(!table_name.isEmpty()) {
			ret = table_name + '.' + ret;
			if(!db_name.isEmpty()) {
				ret = db_name + '.' + ret;
			}
		}
	}
	return ret;
}

bool Utils::fieldNameEndsWith(const QString &field_name1, const QString &field_name2)
{
	/// psql (podle SQL92) predelava vsechny nazvy sloupcu, pokud nejsou v "" do lowercase, ale mixedcase se lip cte, tak at se to sparuje.
	int l1 = field_name1.length();
	int l2 = field_name2.length();
	if(l2 > l1)
		return false;
	if(field_name1.endsWith(field_name2, Qt::CaseInsensitive)) {
		if(l1 == l2)
			return true; /// same length, must be same
		if(field_name1[l1 - l2 - 1] == '.')
			return true; /// dot '.' is on position l1 - l2 - 1
	}
	return false;
}

bool Utils::fieldNameCmp(const QString &fld_name1, const QString &fld_name2)
{
	if(fld_name1.isEmpty() || fld_name2.isEmpty()) return false;
	if(fieldNameEndsWith(fld_name1, fld_name2)) return true;
	if(fieldNameEndsWith(fld_name2, fld_name1)) return true;
	return false;
}

QVariant Utils::retypeVariant(const QVariant &val, int meta_type_id)
{
	if(meta_type_id == QVariant::Invalid) {
		//qfWarning() << "Cannot convert" << val << "to QVariant::Invalid type!";
		// retype whatever to invalid variant
		return QVariant();
	}
	if(val.userType() == meta_type_id)
		return val;
	if(val.canConvert(meta_type_id)) {
		QVariant ret = val;
		ret.convert(meta_type_id);
		return ret;
	}
	if(meta_type_id < QMetaType::User) {
		if(val.isNull()) {
			QVariant::Type t = (QVariant::Type)meta_type_id;
			return QVariant(t);
		}
	}
	//if(meta_type_id >= QVariant::UserType) {
	//	if(val.userType() >= QVariant::UserType) {
	//		if()
	//	}
	//}
	qfWarning() << "Don't know, how to convert variant type" << val.typeName() << "to:" << meta_type_id << QMetaType::typeName(meta_type_id);
	return val;
}

QVariant Utils::retypeStringValue(const QString &str_val, const QString &type_name)
{
	QByteArray ba = type_name.toLatin1();
	QVariant::Type type = QVariant::nameToType(ba.constData());
	QVariant ret = qf::core::Utils::retypeVariant(str_val, type);
	return ret;
}

int Utils::findCaption(const QString &caption_format, int from_ix, QString *caption)
{
	int ix1 = caption_format.indexOf(QLatin1String("{{"), from_ix);
	if(ix1 >= 0) {
		int ix2 = caption_format.indexOf(QLatin1String("}}"), ix1+2);
		if(ix2 > ix1) {
			if(caption)
				*caption = caption_format.mid(ix1+2, ix2-ix1-2);
			return ix1;
		}
	}
	return -1;
}

QSet<QString> Utils::findCaptions(const QString caption_format)
{
	QSet<QString> ret;
	QRegExp rx;
	rx.setPattern("\\{\\{([A-Za-z][A-Za-z0-9]*(\\.[A-Za-z][A-Za-z0-9]*)*)\\}\\}");
	rx.setPatternSyntax(QRegExp::RegExp);
	int ix = 0;
	while((ix = rx.indexIn(caption_format, ix)) != -1) {
		ret << rx.cap(1);
		ix += rx.matchedLength();
	}
	return ret;
}

QString Utils::replaceCaptions(const QString format_str, const QString &caption_name, const QVariant &caption_value)
{
	QString ret = format_str;
	QString placeholder = QLatin1String("{{") + caption_name + QLatin1String("}}");
	//shvInfo() << placeholder << "->" << caption_value.toString();
	ret.replace(placeholder, caption_value.toString());
	return ret;
}

QString Utils::replaceCaptions(const QString format_str, const QVariantMap &replacements)
{
	QString ret = format_str;
	QMapIterator<QString, QVariant> it(replacements);
	while(it.hasNext()) {
		it.next();
		ret = replaceCaptions(ret, it.key(), it.value());
	}
	return ret;
}

bool Utils::invokeMethod_B_V(QObject *obj, const char *method_name)
{
	QVariant ret = false;
	bool ok = QMetaObject::invokeMethod(obj, method_name, Qt::DirectConnection, Q_RETURN_ARG(QVariant, ret));
	if(!ok)
		qfWarning() << obj << "Method" << method_name << "invocation failed!";
	return ret.toBool();
}
#endif
}}
