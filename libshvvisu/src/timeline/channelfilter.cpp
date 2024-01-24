#include <shv/visu/timeline/channelfilter.h>

#include <shv/core/log.h>

namespace shv::visu::timeline {

ChannelFilter::ChannelFilter() = default;

ChannelFilter::ChannelFilter(const QSet<QString> &permitted_paths, const QString &name)
{
	m_permittedPaths = permitted_paths;
	m_name = name;
}

QSet<QString> ChannelFilter::permittedPaths() const
{
	return m_permittedPaths;
}

void ChannelFilter::addPermittedPath(const QString &path)
{
	m_permittedPaths.insert(path);
}

void ChannelFilter::removePermittedPath(const QString &path)
{
	m_permittedPaths.remove(path);
}

void ChannelFilter::setPermittedPaths(const QSet<QString> &paths)
{
	m_permittedPaths = paths;
}

bool ChannelFilter::isPathPermitted(const QString &path) const
{
	return m_permittedPaths.contains(path);
}

QString ChannelFilter::name()
{
	return m_name;
}

}
