#include "ssl_common.h"

#include <shv/coreqt/log.h>

#include <QFile>
#include <QDir>

namespace {
QList<QSslCertificate> load_ssl_certificate_chain(const QString &cert_file_name)
{
	QFile cert_file(cert_file_name);
	if(!cert_file.open(QIODevice::ReadOnly)) {
		shvError() << "Cannot open SSL certificate file:" << cert_file.fileName() << "for reading";
		return {};
	}
	const QList<QSslCertificate> certificate_chain = QSslCertificate::fromDevice(&cert_file, QSsl::Pem);
	for (const auto &cert : certificate_chain) {
		shvDebug() << "CERT:" << cert.toText();
	}
	return certificate_chain;
}
}

QSslConfiguration load_ssl_configuration(shv::broker::AppCliOptions *opts)
{
	QList<QSslCertificate> certificate_chain;
	QString cert_files_string = QString::fromStdString(opts->serverSslCertFiles());
	QList<QString> cert_files = cert_files_string.split(',');

	for (QString cert_file : cert_files) {
		if(QDir::isRelativePath(cert_file))
			cert_file = QString::fromStdString(opts->configDir()) + '/' + cert_file;

		const QList<QSslCertificate> certs = load_ssl_certificate_chain(cert_file);
		if (certs.empty()) {
			shvWarning() << "No certificates loaded from" << cert_file;
		}
		certificate_chain.append(certs);
	}

	QString key_fn = QString::fromStdString(opts->serverSslKeyFile());
	if(QDir::isRelativePath(key_fn))
		key_fn = QString::fromStdString(opts->configDir()) + '/' + key_fn;
	QFile key_file(key_fn);
	if(!key_file.open(QIODevice::ReadOnly)) {
		shvError() << "Cannot open SSL key file:" << key_file.fileName() << "for reading";
		return QSslConfiguration();
	}
	QSslKey ssl_key(&key_file, QSsl::Rsa, QSsl::Pem);
	key_file.close();

	QSslConfiguration ssl_configuration;
	ssl_configuration.setPeerVerifyMode(QSslSocket::VerifyNone);
	ssl_configuration.setLocalCertificateChain(certificate_chain);
	ssl_configuration.setPrivateKey(ssl_key);
	//ssl_configuration.setProtocol(QSsl::TlsV1SslV3);
	return ssl_configuration;
}
