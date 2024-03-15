#include <shv/coreqt/utils/clioptions.h>
#include <shv/coreqt/utils.h>
#include <shv/coreqt/log.h>

#include <shv/chainpack/rpcvalue.h>
#include <shv/chainpack/utils.h>

#include <shv/core/log.h>
#include <shv/core/utils.h>
#include <shv/core/assert.h>
#include <shv/core/exception.h>

#include <QStringBuilder>
#include <QStringList>
#include <QDir>
#include <QJsonParseError>

#ifdef Q_OS_WIN
#include <qt_windows.h> // needed by CLIOptions::applicationDirAndName()
#endif

#include <limits>
#include <iostream>

namespace shv::coreqt::utils {

const CLIOptions::Option & CLIOptions::Option::sharedNull()
{
	static Option n = Option(NullConstructor());
	return n;
}

CLIOptions::Option::Option(CLIOptions::Option::NullConstructor)
{
	d = new Data();
}

CLIOptions::Option::Option()
{
	*this = sharedNull();
}

CLIOptions::Option::Option(QMetaType::Type type)
{
	d = new Data(type);
}

CLIOptions::Option& CLIOptions::Option::setValueString(const QString& val_str)
{
	QMetaType::Type t = type();
	switch(t) {
	case(QMetaType::UnknownType):
		shvWarning() << "Setting value:" << val_str << "to an invalid type option.";
		break;
	case(QMetaType::Bool):
	{
		if(val_str.isEmpty()) {
			setValue(true);
		}
		else {
			bool ok;
			int n = val_str.toInt(&ok);
			if(ok) {
				setValue(n != 0);
			}
			else {
				bool is_true = true;
				for(const char * const s : {"n", "no", "false"}) {
					if(val_str.compare(QLatin1String(s), Qt::CaseInsensitive)) {
						is_true = false;
						break;
					}
				}
				setValue(is_true);
			}
		}
		break;
	}
	case(QMetaType::Int):
	{
		bool ok;
		setValue(val_str.toInt(&ok));
		if(!ok)
			shvWarning() << "Value:" << val_str << "cannot be converted to Int.";
		break;
	}
	case(QMetaType::Double):
	{
		bool ok;
		setValue(val_str.toDouble(&ok));
		if(!ok)
			shvWarning() << "Value:" << val_str << "cannot be converted to Double.";
		break;
	}
	default:
		setValue(val_str);
	}
	return *this;
}

CLIOptions::CLIOptions(QObject *parent)
	: QObject(parent), m_parsedArgIndex(), m_isAppBreak()
{
	addOption("abortOnException").setType(QMetaType::Bool).setNames("--abort-on-exception").setComment(tr("Abort application on exception"));
	addOption("help").setType(QMetaType::Bool).setNames("-h", "--help").setComment(tr("Print help"));
}

CLIOptions::~CLIOptions() = default;

CLIOptions::Option& CLIOptions::addOption(const QString key, const CLIOptions::Option& opt)
{
	m_options[key] = opt;
	return m_options[key];
}

bool CLIOptions::removeOption(const QString key)
{
	return m_options.remove(key) > 0;
}

CLIOptions::Option CLIOptions::option(const QString& name, bool throw_exc) const
{
	Option ret = m_options.value(name);
	if(ret.isNull() && throw_exc) {
		std::string msg = "Key '" + name.toStdString() + "' not found.";
		shvWarning() << msg;
		SHV_EXCEPTION(msg);
	}
	return ret;
}

CLIOptions::Option& CLIOptions::optionRef(const QString& name)
{
	if(!m_options.contains(name)) {
		std::string msg = "Key '" + name.toStdString() + "' not found.";
		shvWarning() << msg;
		SHV_EXCEPTION(msg);
	}
	return m_options[name];
}

QVariantMap CLIOptions::values() const
{
	QVariantMap ret;
	QMapIterator<QString, Option> it(m_options);
	while(it.hasNext()) {
		it.next();
		ret[it.key()] = value(it.key());
	}
	return ret;
}

QVariant CLIOptions::value(const QString &name) const
{
	QVariant ret = value_helper(name, shv::core::Exception::Throw);
	return ret;
}

QVariant CLIOptions::value(const QString& name, const QVariant default_value) const
{
	QVariant ret = value_helper(name, !shv::core::Exception::Throw);
	if(!ret.isValid())
		ret = default_value;
	return ret;
}

bool CLIOptions::isValueSet(const QString &name) const
{
	return option(name, !shv::core::Exception::Throw).isSet();
}

QVariant CLIOptions::value_helper(const QString &name, bool throw_exception) const
{
	Option opt = option(name, throw_exception);
	if(opt.isNull())
		return QVariant();
	QVariant ret = opt.value();
	if(!ret.isValid())
		ret = opt.defaultValue();
	return ret;
}

bool CLIOptions::optionExists(const QString &name) const
{
	return !option(name, !shv::core::Exception::Throw).isNull();
}

bool CLIOptions::setValue(const QString& name, const QVariant val, bool throw_exc)
{
	Option o = option(name, false);
	if(optionExists(name)) {
		Option &orf = optionRef(name);
		orf.setValue(val);
		return true;
	}

	QString msg = "setValue():"%val.toString()%" Key '"%name%"' not found.";
	shvWarning() << msg.toStdString();
	if(throw_exc) {
		SHV_EXCEPTION(msg.toStdString());
	}
	return false;
}

QString CLIOptions::takeArg(bool &ok)
{
	ok = m_parsedArgIndex < m_arguments.count();
	QString ret = m_arguments.value(m_parsedArgIndex++);
	return ret;
}

QString CLIOptions::peekArg(bool &ok) const
{
	ok = m_parsedArgIndex < m_arguments.count();
	QString ret = m_arguments.value(m_parsedArgIndex);
	return ret;
}

// NOLINTNEXTLINE(modernize-avoid-c-arrays)
void CLIOptions::parse(int argc, char* argv[])
{
	QStringList args;
	for(int i=0; i<argc; i++)
		args << QString::fromUtf8(argv[i]);
	parse(args);
}

void CLIOptions::parse(const QStringList& cmd_line_args)
{
	m_isAppBreak = false;
	m_parsedArgIndex = 0;
	m_arguments = cmd_line_args.mid(1);
	m_unusedArguments = QStringList();
	m_parseErrors = QStringList();
	m_allArgs = cmd_line_args;

	bool ok;
	while(true) {
		QString arg = takeArg(ok);
		if(!ok) {
			break;
		}
		if(arg == QStringLiteral("--help") || arg == QStringLiteral("-h")) {
			setHelp(true);
			m_isAppBreak = true;
			return;
		}
		bool found = false;
		QMutableMapIterator<QString, Option> it(m_options);
		while(it.hasNext()) {
			it.next();
			Option &opt = it.value();
			QStringList names = opt.names();
			if(names.contains(arg)) {
				found = true;
				arg = peekArg(ok);
				if(arg.startsWith('-') || !ok) {
					// switch has no value entered
					arg = QString();
				}
				else {
					arg = takeArg(ok);
				}
				opt.setValueString(arg);
				break;
			}
		}
		if(!found) {
			if(arg.startsWith("-"))
				m_unusedArguments << arg;
		}

	}
	{
		QMapIterator<QString, Option> it(m_options);
		while(it.hasNext()) {
			it.next();
			Option opt = it.value();
			if(opt.isMandatory() && !opt.value().isValid()) {
				addParseError(QString("Mandatory option '%1' not set.").arg(opt.names().value(0)));
			}
		}
	}
	shv::core::Exception::setAbortOnException(isAbortOnException());
}

QPair<QString, QString> CLIOptions::applicationDirAndName() const
{
	static QString app_dir;
	static QString app_name;
	if(app_name.isEmpty()) {
		if(!m_allArgs.empty()) {
	#ifdef Q_OS_WIN
			QString app_file_path;
			wchar_t buffer[MAX_PATH + 2];
			DWORD v = GetModuleFileName(0, buffer, MAX_PATH + 1);
			buffer[MAX_PATH + 1] = 0;
			if (v <= MAX_PATH)
				app_file_path = QString::fromWCharArray(buffer);
			QChar sep = '\\';
	#else
			QString app_file_path = m_allArgs[0];
			QChar sep = '/';
	#endif
			app_dir = app_file_path.section(sep, 0, -2);
			app_name = app_file_path.section(sep, -1);
	#ifdef Q_OS_WIN
			if(app_name.endsWith(QLatin1String(".exe"), Qt::CaseInsensitive))
				app_name = app_name.mid(0, app_name.length() - 4);
	#else
			if(app_name.endsWith(QLatin1String(".so"), Qt::CaseInsensitive)) {
				// for example zygotized Android application
				app_name = app_name.mid(0, app_name.length() - 3);
			}
	#endif
		}
	}
	return QPair<QString, QString>(app_dir, app_name);
}

QString CLIOptions::applicationDir() const
{
	return QDir::fromNativeSeparators(applicationDirAndName().first);
}

QString CLIOptions::applicationName() const
{
	return applicationDirAndName().second;
}

void CLIOptions::printHelp(std::ostream &os) const
{
	using namespace std;
	os << applicationName().toStdString() << " [OPTIONS]" << std::endl << std::endl;
	os << "OPTIONS:" << std::endl << std::endl;
	QMapIterator<QString, Option> it(m_options);
	while(it.hasNext()) {
		it.next();
		Option opt = it.value();
		os << opt.names().join(", ").toStdString();
		if(opt.type() != QMetaType::Bool) {
			if(opt.type() == QMetaType::Int || opt.type() == QMetaType::Double) os << " " << "number";
			else os << " " << "'string'";
		}
		QVariant def_val = opt.defaultValue();
		if(def_val.isValid()) os << " [default(" << def_val.toString().toStdString() << ")]";
		if(opt.isMandatory()) os << " [MANDATORY]";
		os << std::endl;
		QString oc = opt.comment();
		if(!oc.isEmpty()) os << "\t" << opt.comment().toStdString() << std::endl;
	}
	os << NecroLog::cliHelp() << std::endl;
}

void CLIOptions::printHelp() const
{
	printHelp(std::cout);
}

void CLIOptions::dump(std::ostream &os) const
{
	QMapIterator<QString, Option> it(m_options);
	while(it.hasNext()) {
		it.next();
		Option opt = it.value();
		os << it.key().toStdString() << '(' << opt.names().join(", ").toStdString() << ')' << ": " << opt.value().toString().toStdString() << std::endl;
	}
}

void CLIOptions::dump() const
{
	std::cout << "=============== options values dump ==============" << std::endl;
	dump(std::cout);
	std::cout << "-------------------------------------------------" << std::endl;
}

void CLIOptions::addParseError(const QString& err)
{
	m_parseErrors << err;
}

QVariant CLIOptions::rpcValueToQVariant(const chainpack::RpcValue &v)
{
	return shv::coreqt::Utils::rpcValueToQVariant(v, nullptr);
}

chainpack::RpcValue CLIOptions::qVariantToRpcValue(const QVariant &v)
{
	return shv::coreqt::Utils::qVariantToRpcValue(v, nullptr);
}

ConfigCLIOptions::ConfigCLIOptions(QObject *parent)
	: Super(parent)
{
	addOption("config").setType(QMetaType::Type::QString).setNames("--config").setComment("Application config name, it is loaded from {config}[.conf] if file exists in {config-dir}, default value is {app-name}.conf");
	addOption("configDir").setType(QMetaType::QString).setNames("--config-dir").setComment("Directory where application config fields are searched, default value: {app-dir-path}.");
}

void ConfigCLIOptions::parse(const QStringList &cmd_line_args)
{
	Super::parse(cmd_line_args);
}

bool ConfigCLIOptions::loadConfigFile()
{
	QString config_file = configFile();
	QFile f(config_file);
	shvInfo() << "Checking presence of config file:" << f.fileName().toStdString();
	if(f.open(QFile::ReadOnly)) {
		shvInfo() << "Reading config file:" << f.fileName();
		QByteArray ba = f.readAll();
		std::string cpon(ba.constData(), static_cast<size_t>(ba.size()));
		std::string str = shv::chainpack::Utils::removeJsonComments(std::string(ba.constData()));
		std::string err;
		chainpack::RpcValue rv = shv::chainpack::RpcValue::fromCpon(cpon, &err);
		if(err.empty()) {
			mergeConfig(rv);
		}
		else {
			shvError() << "Error parsing config file:" << f.fileName() << err;
			return false;
		}
	}
	else {
		shvInfo() << "Config file:" << f.fileName() << "not found.";
	}
	return true;
}

QString ConfigCLIOptions::configFile()
{
	auto config = QStringLiteral("config");
	auto conf_ext = QStringLiteral(".conf");
	QString config_file;
	if(isValueSet(config)) {
		config_file = value(config).toString();
		if(config_file.isEmpty()) {
			/// explicitly set empty config means DO NOT load config from any file
			return QString();
		}
	}
	else {
		config_file = applicationName() + conf_ext;
	}
	if(!QDir::isAbsolutePath(config_file)) {
		QString config_dir = configDir();
		if(config_dir.isEmpty())
			config_dir = applicationDir();
		config_file = config_dir + '/' + config_file;
	}
	if(!config_file.endsWith(conf_ext)) {
		if(QFile::exists(config_file + conf_ext))
			config_file += conf_ext;
	}
	return config_file;
}

void ConfigCLIOptions::mergeConfig(const QVariant &config_map)
{
	mergeConfig_helper(QString(), qVariantToRpcValue(config_map));
}

void ConfigCLIOptions::mergeConfig_helper(const QString &key_prefix, const shv::chainpack::RpcValue &config_map)
{
	const chainpack::RpcValue::Map &cm = config_map.asMap();
	for(const auto &kv : cm) {
		QString key = QString::fromStdString(kv.first);
		SHV_ASSERT(!key.isEmpty(), "Empty key!", continue);
		if(!key_prefix.isEmpty()) {
			key = key_prefix + '.' + key;
		}
		chainpack::RpcValue v = kv.second;
		if(options().contains(key)) {
			Option &opt = optionRef(key);
			if(!opt.isSet()) {
				opt.setValue(rpcValueToQVariant(v));
			}
		}
		else if(v.isMap()) {
			mergeConfig_helper(key, v);
		}
		else {
			shvWarning() << "Cannot merge nonexisting option key:" << key;
		}
	}
}

}
