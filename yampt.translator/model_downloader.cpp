#include "model_downloader.hpp"

#include "../yampt/tools.hpp"

#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslError>

#include <filesystem>
#include <fstream>

static const char * HF_REPO = "OpenNMT/nllb-200-distilled-600M-ct2-int8";

static const std::vector<std::string> MODEL_FILES = {
	"model.bin",
	"config.json",
	"vocabulary.json",
	"sentencepiece.bpe.model",
};

struct lang_pair_t
{
	const char * dir_name;
	const char * source_lang;
	const char * target_lang;
};

static const std::vector<lang_pair_t> LANG_PAIRS = {
	{ "en-de", "eng_Latn", "deu_Latn" },
	{ "en-pl", "eng_Latn", "pol_Latn" },
	{ "en-fr", "eng_Latn", "fra_Latn" },
	{ "en-ru", "eng_Latn", "rus_Cyrl" },
};

model_downloader_t::model_downloader_t(QObject * parent)
    : QObject(parent)
    , network_(new QNetworkAccessManager(this))
{}

bool model_downloader_t::is_model_present(const std::string & target_dir) const
{
	namespace fs = std::filesystem;

	auto model_dir = fs::path(target_dir) / "nllb-200" / "model";
	if (!fs::is_directory(model_dir))
		return false;

	for (const auto & f : MODEL_FILES)
	{
		if (!fs::exists(model_dir / f))
			return false;
	}

	return true;
}

void model_downloader_t::download(const std::string & target_dir, std::function<void(const std::string &)> on_progress)
{
	namespace fs = std::filesystem;

	target_dir_ = target_dir;
	on_progress_ = std::move(on_progress);
	files_.clear();
	current_index_ = 0;

	auto base_dir = fs::path(target_dir) / "nllb-200";
	auto model_dir = base_dir / "model";
	fs::create_directories(model_dir);

	std::string base_url = std::string("https://huggingface.co/") + HF_REPO + "/resolve/main/";

	for (const auto & f : MODEL_FILES)
	{
		download_file_t df;
		df.url = base_url + f + "?download=true";
		df.local_path = (model_dir / f).string();
		files_.push_back(df);
	}

	for (const auto & lp : LANG_PAIRS)
	{
		auto pair_dir = fs::path(target_dir) / lp.dir_name;
		fs::create_directories(pair_dir);

		auto lang_file = (pair_dir / "languages.txt").string();
		{
			std::ofstream f(lang_file);
			f << lp.source_lang << "\n" << lp.target_lang << "\n";
		}
	}

	tools_t::add_log("[info] starting download of " + std::to_string(files_.size()) + " files\r\n");
	download_next();
}

void model_downloader_t::download_next()
{
	if (current_index_ >= files_.size())
	{
		namespace fs = std::filesystem;

		auto model_dir = fs::path(target_dir_) / "nllb-200" / "model";
		auto spm_path = (model_dir / "sentencepiece.bpe.model").string();

		for (const auto & lp : LANG_PAIRS)
		{
			auto pair_dir = fs::path(target_dir_) / lp.dir_name;
			auto dst = (pair_dir / "sentencepiece.bpe.model").string();

			if (!fs::exists(dst))
				fs::copy_file(spm_path, dst);

			auto model_link = pair_dir / "model";
			if (!fs::exists(model_link))
			{
				std::error_code ec;
				fs::create_directory_symlink(fs::absolute(model_dir), model_link, ec);
				if (ec)
					fs::copy(model_dir, model_link, fs::copy_options::recursive, ec);
			}
		}

		tools_t::add_log("[info] all files downloaded\r\n");
		emit finished(true, "");
		return;
	}

	tools_t::add_log("[info] downloading \"" + files_[current_index_].url + "\"\r\n");
	if (on_progress_) on_progress_("[info] downloading \"" + files_[current_index_].url + "\"\r\n");

	QNetworkRequest request(QUrl(QString::fromStdString(files_[current_index_].url)));
	request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
	request.setRawHeader("User-Agent", "yampt/1.0");

	auto * reply = network_->get(request);

	if (reply->error() != QNetworkReply::NoError)
	{
		auto err = reply->errorString().toStdString();
		tools_t::add_log("[error] immediate network error: " + err + "\r\n");
		emit finished(false, err);
		reply->deleteLater();
		return;
	}

	connect(reply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
		if (total > 0)
		{
			int pct = static_cast<int>(received * 100 / total);
			auto filename = files_[current_index_].url;
			auto sep = filename.find_last_of('/');
			if (sep != std::string::npos)
				filename = filename.substr(sep + 1);

			if (on_progress_) on_progress_("[info] " + filename + " " + std::to_string(pct) + "% (" +
			    std::to_string(received / 1024) + "/" + std::to_string(total / 1024) + " KB)\r\n");
		}
	});

	connect(reply, &QNetworkReply::errorOccurred, this, [this, reply](QNetworkReply::NetworkError code) {
		auto err = reply->errorString().toStdString();
		if (on_progress_) on_progress_("[error] network error " + std::to_string(static_cast<int>(code)) + ": " + err + "\r\n");
	});

	connect(reply, &QNetworkReply::sslErrors, this, [this, reply](const QList<QSslError> & errors) {
		for (const auto & e : errors)
			if (on_progress_) on_progress_("[warning] SSL: " + e.errorString().toStdString() + "\r\n");

		reply->ignoreSslErrors();
	});

	connect(reply, &QNetworkReply::finished, this, [this, reply]() { on_file_finished(reply); });
}

void model_downloader_t::on_file_finished(QNetworkReply * reply)
{
	reply->deleteLater();

	if (reply->error() != QNetworkReply::NoError)
	{
		auto err = reply->errorString().toStdString();
		auto url = reply->url().toString().toStdString();
		tools_t::add_log("[error] download failed: " + err + " (" + url + ")\r\n");
		emit finished(false, err + " (" + url + ")");
		return;
	}

	auto local_path = files_[current_index_].local_path;
	QFile file(QString::fromStdString(local_path));
	if (!file.open(QIODevice::WriteOnly))
	{
		tools_t::add_log("[error] failed to write \"" + local_path + "\"\r\n");
		emit finished(false, "failed to write " + local_path);
		return;
	}

	file.write(reply->readAll());
	file.close();

	tools_t::add_log("[info] saved \"" + local_path + "\"\r\n");
	if (on_progress_) on_progress_("[info] saved \"" + local_path + "\"\r\n");

	++current_index_;
	download_next();
}
