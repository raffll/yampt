#pragma once

#include <QObject>
#include <functional>
#include <string>
#include <vector>

class QNetworkAccessManager;
class QNetworkReply;

class model_downloader_t : public QObject
{
	Q_OBJECT

public:
	explicit model_downloader_t(QObject * parent = nullptr);

	void download(const std::string & target_dir, std::function<void(const std::string &)> on_progress);
	bool is_model_present(const std::string & target_dir) const;

signals:
	void finished(bool success, const std::string & error);

private:
	void download_next();
	void on_file_finished(QNetworkReply * reply);

	struct download_file_t
	{
		std::string url;
		std::string local_path;
	};

	QNetworkAccessManager * network_ = nullptr;
	std::vector<download_file_t> files_;
	size_t current_index_ = 0;
	std::string target_dir_;
	std::function<void(const std::string &)> on_progress_;
};
