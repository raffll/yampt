#pragma once

#include <QObject>
#include <QStringList>

class QFileSystemWatcher;
class QTimer;

class workspace_watcher_t : public QObject
{
	Q_OBJECT

public:
	explicit workspace_watcher_t(QObject * parent = nullptr);

	void set_watch_roots(const QStringList & roots);
	void clear();

signals:
	void workspace_changed();

private:
	void on_file_changed();
	void add_directory_recursive(QStringList & paths, const QString & directory);

	QFileSystemWatcher * m_watcher = nullptr;
	QTimer * m_debounce_timer = nullptr;
};
