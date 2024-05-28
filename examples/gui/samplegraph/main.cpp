#include "mainwindow.h"

#include <necrolog.h>
#include <shv/core/utils.h>
#include <shv/coreqt/utils.h>
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

	NecroLog::setCLIOptions(argc, argv);

	QApplication a(argc, argv);
	MainWindow w;
	w.show();
	return QApplication::exec();
}
