#include "mainwindow.h"

#include <necrolog.h>
#include <shv/core/utils.h>
#include <shv/coreqt/utils.h>
#include <shv/coreqt/log.h>
#include <shv/iotqt/utils/network.h>
#include <shv/chainpack/chainpack.h>

#include <QApplication>

int main(int argc, char *argv[])
{
	// call something from shv::coreqt to avoid linker error:
	// error while loading shared libraries: libshvcoreqt.so.1: cannot open shared object file: No such file or directory
	shv::chainpack::ChainPack::PackingSchema::name(shv::chainpack::ChainPack::PackingSchema::Decimal);
	shv::core::Utils::intToVersionString(0);
	shv::coreqt::Utils::isValueNotAvailable(QVariant());
	shv::iotqt::utils::Network::isGlobalIPv4Address(0);

	QCoreApplication::setOrganizationName("Elektroline");
	QCoreApplication::setOrganizationDomain("elektroline.cz");
	QCoreApplication::setApplicationName("samplegraph");
	QCoreApplication::setApplicationVersion("0.0.2");

	auto opts = NecroLog::setCLIOptions(argc, argv);
	QString csv_file;
	if (opts.size() == 2) {
		const auto &file_name = opts[1];
		if (!file_name.ends_with(".csv")) {
			shvError() << "Only CSV data file is supported currently";
			return -1;
		}
		csv_file = QString::fromStdString(file_name);
	}

	QApplication a(argc, argv);
	MainWindow w(csv_file);
	w.show();
	return QApplication::exec();
}
