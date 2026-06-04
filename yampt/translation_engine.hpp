#pragma once

#include <memory>
#include <string>

struct translation_result_t
{
	std::string text;
	bool success = false;
	std::string error;
};

class translation_engine_t
{
public:
	translation_engine_t();
	~translation_engine_t();

	translation_engine_t(translation_engine_t &&) noexcept;
	translation_engine_t & operator=(translation_engine_t &&) noexcept;

	bool load(const std::string & model_pack_path);
	void unload();
	bool is_loaded() const;

	std::string source_language() const;
	std::string target_language() const;

	translation_result_t translate(const std::string & text) const;

private:
	struct impl_t;
	std::unique_ptr<impl_t> impl_;
};
