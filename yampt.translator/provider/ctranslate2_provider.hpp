#pragma once

#include "translation_provider.hpp"
#include "../../yampt/model/translation_engine.hpp"

#include <QObject>
#include <functional>
#include <string>

class ctranslate2_provider_t : public QObject, public translation_provider_t
{
	Q_OBJECT

public:
	explicit ctranslate2_provider_t(QObject * parent = nullptr);

	std::string name() const override;
	bool is_available() const override;
	bool is_async() const override;
	bool has_quota() const override;
	int remaining_quota() const override;

	void translate(const std::string & text, const std::string & target_lang) override;
	translation_result_t translate_sync(const std::string & text);

	bool load_model(const std::string & model_path);

	translation_engine_t * engine_ptr()
	{
		return &engine_;
	}

signals:
	void translation_finished(translation_suggestion_t result);

private:
	translation_engine_t engine_;
};
