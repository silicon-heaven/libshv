#pragma once

#include <shv/broker/appclioptions.h>

#include <QSslKey>
#include <QSslCertificate>
#include <QSslConfiguration>

QSslConfiguration load_ssl_configuration(shv::broker::AppCliOptions *opts);
