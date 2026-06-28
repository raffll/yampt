#include "model_downloader.hpp"

#include "../../yampt/utility/tools.hpp"

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

	if (!fs::is_directory(target_dir))
		return false;

	for (const auto & entry : fs::directory_iterator(target_dir))
	{
		if (!entry.is_directory())
			continue;

		auto dir = entry.path();
		if (fs::exists(dir / "sentencepiece.bpe.model") && fs::is_directory(dir / "model"))
			return true;
	}

	return false;
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

void model_downloader_t::finalize_download()
{
	namespace fs = std::filesystem;

	const auto & model_dir = fs::path(target_dir_) / "nllb-200" / "model";
	const auto & spm_path = (model_dir / "sentencepiece.bpe.model").string();

	for (const auto & lang_pair : LANG_PAIRS)
	{
		const auto & pair_dir = fs::path(target_dir_) / lang_pair.dir_name;
		const auto & destination = (pair_dir / "sentencepiece.bpe.model").string();

		if (!fs::exists(destination))
			fs::copy_file(spm_path, destination);

		const auto & model_link = pair_dir / "model";
		if (!fs::exists(model_link))
		{
			std::error_code error_code;
			fs::create_directory_symlink(fs::absolute(model_dir), model_link, error_code);
			if (error_code)
				fs::copy(model_dir, model_link, fs::copy_options::recursive, error_code);
		}
	}

	tools_t::add_log("[info] all files downloaded\r\n");
	emit finished(true, "");
}

void model_downloader_t::connect_reply_signals(QNetworkReply * reply)
{
	connect(
	    reply,
	    &QNetworkReply::downloadProgress,
	    this,
	    [this](qint64 received, qint64 total)
	{
		if (total <= 0)
			return;

		const auto & percent = static_cast<int>(received * 100 / total);
		auto filename = files_[current_index_].url;
		const auto & separator = filename.find_last_of('/');
		if (separator != std::string::npos)
			filename = filename.substr(separator + 1);

		if (on_progress_)
			on_progress_(
			    "[info] " + filename + " " + std::to_string(percent) + "% (" + std::to_string(received / 1024) + "/" +
			    std::to_string(total / 1024) + " KB)\r\n");
	});

	connect(
	    reply,
	    &QNetworkReply::errorOccurred,
	    this,
	    [this, reply](QNetworkReply::NetworkError code)
	{
		const auto & error_message = reply->errorString().toStdString();
		if (on_progress_)
			on_progress_(
			    "[error] network error " + std::to_string(static_cast<int>(code)) + ": " + error_message + "\r\n");
	});

	connect(
	    reply,
	    &QNetworkReply::sslErrors,
	    this,
	    [this, reply](const QList<QSslError> & errors)
	{
		for (const auto & error : errors)
			if (on_progress_)
				on_progress_("[warning] SSL: " + error.errorString().toStdString() + "\r\n");

		reply->ignoreSslErrors();
	});

	connect(reply, &QNetworkReply::finished, this, [this, reply]() { on_file_finished(reply); });
}

void model_downloader_t::download_next()
{
	if (current_index_ >= files_.size())
	{
		finalize_download();
		return;
	}

	tools_t::add_log("[info] downloading \"" + files_[current_index_].url + "\"\r\n");
	if (on_progress_)
		on_progress_("[info] downloading \"" + files_[current_index_].url + "\"\r\n");

	QNetworkRequest request(QUrl(QString::fromStdString(files_[current_index_].url)));
	request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
	request.setRawHeader("User-Agent", "yampt/1.0");

	auto * reply = network_->get(request);

	if (reply->error() != QNetworkReply::NoError)
	{
		const auto & error_message = reply->errorString().toStdString();
		tools_t::add_log("[error] immediate network error: " + error_message + "\r\n");
		emit finished(false, error_message);
		reply->deleteLater();
		return;
	}

	connect_reply_signals(reply);
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
	if (on_progress_)
		on_progress_("[info] saved \"" + local_path + "\"\r\n");

	++current_index_;
	download_next();
}
