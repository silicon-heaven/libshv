#include <shv/iotqt/node/localfsnode.h>

#include <shv/chainpack/rpcmessage.h>
#include <shv/chainpack/metamethod.h>
#include <shv/core/exception.h>
#include <shv/core/stringview.h>
#include <shv/coreqt/log.h>

#include <QDateTime>

using namespace shv::chainpack;

namespace shv::iotqt::node {
namespace {
const auto M_WRITE = "write";
const auto M_DELETE = "delete";
const auto M_MKFILE = "mkfile";
const auto M_MKDIR = "mkdir";
const auto M_RMDIR = "rmdir";
const auto M_LS_FILES = "lsfiles";

const std::vector<MetaMethod> meta_methods_dir {
	shv::chainpack::methods::DIR,
	shv::chainpack::methods::LS,
	{M_LS_FILES, MetaMethod::Flag::None, "Map|Null", "List", MetaMethod::AccessLevel::Read},
	{M_MKFILE, MetaMethod::Flag::None, "String|List", "Bool", MetaMethod::AccessLevel::Write},
	{M_MKDIR, MetaMethod::Flag::None, "String", "Bool", MetaMethod::AccessLevel::Write},
	{M_RMDIR, MetaMethod::Flag::None, "Bool", "Bool", MetaMethod::AccessLevel::Service}
};

const std::vector<MetaMethod> meta_methods_dir_write_file {
	{M_WRITE, MetaMethod::Flag::None, "String|List", "Bool", MetaMethod::AccessLevel::Write},
};

const std::vector<MetaMethod> meta_methods_file_modificable {
	{M_WRITE, MetaMethod::Flag::None, "String|List", "Bool", MetaMethod::AccessLevel::Write},
	{M_DELETE, MetaMethod::Flag::None, "", "Bool", MetaMethod::AccessLevel::Service}
};

const std::vector<MetaMethod> & get_meta_methods_file()
{
	static std::vector<MetaMethod> meta_methods;
	if (meta_methods.empty()) {
		meta_methods = FileNode::meta_methods_file_base;
		meta_methods.insert(meta_methods.end(), meta_methods_file_modificable.begin(), meta_methods_file_modificable.end());
	}
	return meta_methods;

}
}

LocalFSNode::LocalFSNode(const QString &root_path, ShvNode *parent)
	: Super({}, parent)
	, m_rootDir(root_path)
{
}

LocalFSNode::LocalFSNode(const QString &root_path, const std::string &node_id, ShvNode *parent)
	: Super(node_id, parent)
	, m_rootDir(root_path)
{
}

chainpack::RpcValue LocalFSNode::callMethod(const ShvNode::StringViewList &shv_path, const std::string &method, const chainpack::RpcValue &params, const shv::chainpack::RpcValue &user_id)
{
	if(method == M_WRITE) {
		return ndWrite(QString::fromStdString(shv_path.join('/')), params);
	}
	if(method == M_DELETE) {
		return ndDelete(QString::fromStdString(shv_path.join('/')));
	}
	if(method == M_MKFILE) {
		return ndMkfile(QString::fromStdString(shv_path.join('/')), params);
	}
	if(method == M_MKDIR) {
		return ndMkdir(QString::fromStdString(shv_path.join('/')), params);
	}
	if(method == M_RMDIR) {
		bool recursively = (params.isBool()) ? params.toBool() : false;
		return ndRmdir(QString::fromStdString(shv_path.join('/')), recursively);
	}
	if(method == M_LS_FILES) {
		return ndLsDir(QString::fromStdString(shv_path.join('/')), params);
	}

	return Super::callMethod(shv_path, method, params, user_id);
}

ShvNode::StringList LocalFSNode::childNames(const ShvNode::StringViewList &shv_path)
{
	QString qpath = QString::fromStdString(shv_path.join('/'));
	ShvNode::StringList ret;
	RpcValue dirs = ndLsDir(qpath, {});
	for(const auto &rv : dirs.asList()) {
		ret.push_back(rv.asList().value(0).asString());
	}
	return ret;
}

chainpack::RpcValue LocalFSNode::hasChildren(const ShvNode::StringViewList &shv_path)
{
	return isDir(shv_path);
}

size_t LocalFSNode::methodCount(const ShvNode::StringViewList &shv_path)
{
	QFileInfo fi = ndFileInfo(QString::fromStdString(shv_path.join('/')));
	if(fi.exists())
		return isDir(shv_path) ? meta_methods_dir.size() : get_meta_methods_file().size();
	if(!shv_path.empty()) {
		StringViewList dir_path = shv_path.mid(0, shv_path.size() - 1);
		fi = ndFileInfo(QString::fromStdString(dir_path.join('/')));
		if(fi.isDir())
			return meta_methods_dir_write_file.size();
	}
	return 0;
}

const chainpack::MetaMethod *LocalFSNode::metaMethod(const StringViewList &shv_path, size_t ix)
{
	QFileInfo fi = ndFileInfo(QString::fromStdString(shv_path.join('/')));
	if(fi.exists()) {
		size_t meta_methods_size = (fi.isDir()) ? meta_methods_dir.size() : get_meta_methods_file().size();

		if(meta_methods_size <= ix)
			SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(meta_methods_size));

		return (fi.isDir()) ? &(meta_methods_dir[ix]) : &(get_meta_methods_file()[ix]);
	}
	if(!shv_path.empty()) {
		StringViewList dir_path = shv_path.mid(0, shv_path.size() - 1);
		fi = ndFileInfo(QString::fromStdString(dir_path.join('/')));
		if(fi.isDir() && ix < meta_methods_dir_write_file.size())
			return &(meta_methods_dir_write_file[ix]);
	}
	SHV_EXCEPTION("Invalid method index: " + std::to_string(ix));
}

QString LocalFSNode::makeAbsolutePath(const QString &relative_path) const
{
	return m_rootDir.absolutePath() + '/' + relative_path;
}

std::string LocalFSNode::fileName(const ShvNode::StringViewList &shv_path) const
{
	return !shv_path.empty() ? std::string{shv_path.back()} : "";
}

chainpack::RpcValue LocalFSNode::readContent(const ShvNode::StringViewList &shv_path, int64_t offset, int64_t size) const
{
	return ndRead(QString::fromStdString(shv_path.join('/')), offset, size);
}

chainpack::RpcValue LocalFSNode::size(const ShvNode::StringViewList &shv_path) const
{
	return ndSize(QString::fromStdString(shv_path.join('/')));
}

bool LocalFSNode::isDir(const ShvNode::StringViewList &shv_path) const
{
	QString qpath = QString::fromStdString(shv_path.join('/'));
	QFileInfo fi(makeAbsolutePath(qpath));
	if(!fi.exists())
		shvError() << "Invalid path:" << fi.absoluteFilePath();
	shvDebug() << __FUNCTION__ << "shv path:" << qpath << "file info:" << fi.absoluteFilePath() << "is dir:" << fi.isDir();
	return fi.isDir();
}

void LocalFSNode::checkPathIsBoundedToFsRoot(const QString &path) const
{
	if (!QDir::cleanPath(path).startsWith(m_rootDir.absolutePath())) {
		SHV_EXCEPTION("Path:" + path.toStdString() + " is out of fs root directory:" + m_rootDir.path().toStdString());
	}
}

QFileInfo LocalFSNode::ndFileInfo(const QString &path) const
{
	QFileInfo fi(makeAbsolutePath(path));
	return fi;
}

RpcValue LocalFSNode::ndSize(const QString &path) const
{
	return static_cast<unsigned>(ndFileInfo(path).size());
}

chainpack::RpcValue LocalFSNode::ndRead(const QString &path, qint64 offset, qint64 size) const
{
	QString file_path = makeAbsolutePath(path);
	checkPathIsBoundedToFsRoot(file_path);

	QFile f(file_path);
	if(f.open(QFile::ReadOnly)) {
		qint64 sz = f.size() - offset;
		if(sz <= 0)
			return {};
		sz = std::min(sz, size);
		f.seek(offset);
		RpcValue::Blob blob;
		blob.resize(static_cast<size_t>(sz));
		f.read(reinterpret_cast<char*>(blob.data()), sz);
		return RpcValue(std::move(blob));
	}
	SHV_EXCEPTION("Cannot open file " + f.fileName().toStdString() + " for reading.");
}

chainpack::RpcValue LocalFSNode::ndWrite(const QString &path, const chainpack::RpcValue &methods_params)
{
	QFile f(makeAbsolutePath(path));

	if (methods_params.isString()){
		if(f.open(QFile::WriteOnly)) {
			const chainpack::RpcValue::String &content = methods_params.asString();
			f.write(content.data(), static_cast<qint64>(content.size()));
			return true;
		}
		SHV_EXCEPTION("Cannot open file " + f.fileName().toStdString() + " for writing.");
	}
	if (methods_params.isList()){
		chainpack::RpcValue::List params = methods_params.asList();

		if (params.size() != 2){
			SHV_EXCEPTION("Cannot write to file " + f.fileName().toStdString() + ". Invalid parameters count.");
		}
		chainpack::RpcValue::Map flags = params[1].asMap();
		QFile::OpenMode open_mode = (flags.value("append").toBool()) ? QFile::Append : QFile::WriteOnly;

		if(f.open(open_mode)) {
			const chainpack::RpcValue::String &content = params[0].toString();
			f.write(content.data(), static_cast<qint64>(content.size()));
			return true;
		}
		SHV_EXCEPTION("Cannot open file " + f.fileName().toStdString() + " for writing.");
	}

	SHV_EXCEPTION("Unsupported param type.");
	return false;
}

chainpack::RpcValue LocalFSNode::ndDelete(const QString &path)
{
	QString file_path = makeAbsolutePath(path);
	checkPathIsBoundedToFsRoot(file_path);

	QFile file (file_path);
	return file.remove();
}

chainpack::RpcValue LocalFSNode::ndMkfile(const QString &path, const chainpack::RpcValue &methods_params)
{
	std::string error;

	if (methods_params.isString()){
		QString file_path = makeAbsolutePath(path + '/' + QString::fromStdString(methods_params.asString()));
		checkPathIsBoundedToFsRoot(file_path);

		QFile f(file_path);
		if(f.open(QFile::WriteOnly)) {
			return true;
		}
		error = "Cannot open file " + file_path.toStdString() + " for writing.";
	}
	else if (methods_params.isList()){
		const chainpack::RpcValue::List &param_lst = methods_params.asList();

		if (param_lst.size() != 2) {
			throw shv::core::Exception(R"(Invalid params, ["name", "content"] expected.)");
		}

		QString file_path = makeAbsolutePath(path + '/' + QString::fromStdString(param_lst[0].asString()));
		QDir d(QFileInfo(file_path).dir());

		if (!d.mkpath(d.absolutePath())){
			SHV_EXCEPTION("Cannot create path " + file_path.toStdString() + ".");
		}

		QFile f(file_path);

		if (f.exists()){
			SHV_EXCEPTION("File " + f.fileName().toStdString() + " already exists.");
		}

		if(f.open(QFile::WriteOnly)) {
			const std::string &data = param_lst[1].asString();
			f.write(data.data(), static_cast<qint64>(data.size()));
			return true;
		}
		SHV_EXCEPTION("Cannot open file " + f.fileName().toStdString() + " for writing.");
	}
	else{
		SHV_EXCEPTION("Unsupported param type.");
	}

	return false;
}

chainpack::RpcValue LocalFSNode::ndMkdir(const QString &path, const chainpack::RpcValue &methods_params)
{
	QString dir_path = makeAbsolutePath(path);
	checkPathIsBoundedToFsRoot(dir_path);

	QDir d(dir_path);

	if (!methods_params.isString())
		SHV_EXCEPTION("Cannot create directory in directory " + d.absolutePath().toStdString() + ". Invalid parameter: " + methods_params.toCpon());

	return d.mkpath(makeAbsolutePath(path + '/' + QString::fromStdString(methods_params.asString())));
}

chainpack::RpcValue LocalFSNode::ndRmdir(const QString &path, bool recursively)
{
	QString file_path = makeAbsolutePath(path);
	checkPathIsBoundedToFsRoot(file_path);

	QDir d(file_path);

	if (path.isEmpty())
		SHV_EXCEPTION("Cannot remove root directory " + d.absolutePath().toStdString());

	if (recursively)
		return d.removeRecursively();

	return d.rmdir(makeAbsolutePath(path));
}

RpcValue LocalFSNode::ndLsDir(const QString &path, const chainpack::RpcValue &methods_params)
{
	bool with_size = methods_params.asMap().value("size", true).toBool();
	bool with_ctime = methods_params.asMap().value("ctime").toBool();
	bool with_dirs = methods_params.asMap().value("dirs", true).toBool();
	bool with_files = methods_params.asMap().value("files", true).toBool();
	QFileInfo fi_path(makeAbsolutePath(path));
	if(fi_path.isDir()) {
		QDir d2(fi_path.absoluteFilePath());
		if(!d2.exists())
			SHV_EXCEPTION("Path " + d2.absolutePath().toStdString() + " do not exists.");
		RpcValue::List lst;
		QDir::Filters filters = QDir::NoDotAndDotDot;
		if(with_dirs)
			filters |= QDir::Dirs;
		if(with_files)
			filters |= QDir::Files;
		for(const QFileInfo &fi : d2.entryInfoList(filters, QDir::Name | QDir::IgnoreCase | QDir::DirsFirst)) {
			RpcValue::List lst2;
			lst2.push_back(fi.fileName().toStdString());
			lst2.push_back(fi.isDir()? "d": "f");
			if(with_size) {
				if(fi.isDir())
					lst2.push_back(0);
				else
					lst2.push_back(static_cast<int64_t>(fi.size()));
			}
			if(with_ctime)
				lst2.push_back(RpcValue::DateTime::fromMSecsSinceEpoch(fi.birthTime().toMSecsSinceEpoch()));

			lst.push_back(lst2);
		}
		return lst;
	}

	return RpcValue::List{};
}

} // namespace shv
