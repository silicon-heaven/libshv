#include "application.h"
#include "appclioptions.h"

#include <shv/core/utils.h>
#include <shv/coreqt/log.h>

#include <iostream>

int main(int argc, char *argv[])
{
	QCoreApplication::setOrganizationName("Elektroline");
	QCoreApplication::setOrganizationDomain("elektroline.cz");
	QCoreApplication::setApplicationName("shvcall");
	QCoreApplication::setApplicationVersion("0.1.0");

	std::vector<std::string> shv_args = NecroLog::setCLIOptions(argc, argv);

	AppCliOptions cli_opts;
	cli_opts.parse(shv_args);
	if (cli_opts.isParseError()) {
		for(const std::string &err : cli_opts.parseErrors()) {
			shvError() << err;
		}
		return EXIT_FAILURE;
	}
	if (cli_opts.isAppBreak()) {
		if (cli_opts.isHelp()) {
			cli_opts.printHelp(std::cout);
		}
		return EXIT_SUCCESS;
	}

	Application a(argc, argv, &cli_opts);
	return Application::exec();
}
