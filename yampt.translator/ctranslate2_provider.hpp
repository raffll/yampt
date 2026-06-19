#pragma once

#include "translation_provider.hpp"
#include "../yampt/translation_engine.hpp"

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

	bool load_model(const std::string & model_path);

signals:
	void translation_finished(translation_suggestion_t result);

private:
	translation_engine_t engine_;
};
