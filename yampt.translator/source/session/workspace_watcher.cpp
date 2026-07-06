#include "workspace_watcher.hpp"
#include <QDir>
#include <QDirIterator>
#include <QFileSystemWatcher>
#include <QTimer>

workspace_watcher_t::workspace_watcher_t(QObject * parent)
    : QObject(parent)
    , m_watcher(new QFileSystemWatcher(this))
    , m_debounce_timer(new QTimer(this))
{
	m_debounce_timer->setSingleShot(true);
	m_debounce_timer->setInterval(200);

	connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, &workspace_watcher_t::on_file_changed);
	connect(m_debounce_timer, &QTimer::timeout, this, &workspace_watcher_t::workspace_changed);
}

void workspace_watcher_t::set_watch_roots(const QStringList & roots)
{
	const auto current = m_watcher->directories();
	if (!current.isEmpty())
		m_watcher->removePaths(current);

	QStringList paths;
	for (const auto & root : roots)
		add_directory_recursive(paths, root);

	if (!paths.isEmpty())
		m_watcher->addPaths(paths);
}

void workspace_watcher_t::clear()
{
	m_debounce_timer->stop();

	const auto current = m_watcher->directories();
	if (!current.isEmpty())
		m_watcher->removePaths(current);
}

void workspace_watcher_t::on_file_changed()
{
	m_debounce_timer->start();
}

void workspace_watcher_t::add_directory_recursive(QStringList & paths, const QString & directory)
{
	QDir qdir(directory);
	if (!qdir.exists())
		return;

	paths.append(directory);

	QDirIterator it(directory, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
	while (it.hasNext())
		paths.append(it.next());
}
