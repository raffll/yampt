#pragma once

#include <string>
#include <functional>

#pragma warning(push, 0)
#include <yyjson.h>
#pragma warning(pop)

class json_reader_t
{
public:
	json_reader_t() = default;

	~json_reader_t()
	{
		if (doc_)
			yyjson_doc_free(doc_);
	}

	json_reader_t(const json_reader_t &) = delete;
	json_reader_t & operator=(const json_reader_t &) = delete;

	json_reader_t(json_reader_t && other) noexcept
	    : doc_(other.doc_)
	{
		other.doc_ = nullptr;
	}

	json_reader_t & operator=(json_reader_t && other) noexcept
	{
		if (this != &other)
		{
			if (doc_)
				yyjson_doc_free(doc_);
			doc_ = other.doc_;
			other.doc_ = nullptr;
		}
		return *this;
	}

	bool parse(const char * data, size_t length, bool allow_invalid_unicode)
	{
		if (doc_)
		{
			yyjson_doc_free(doc_);
			doc_ = nullptr;
		}

		yyjson_read_flag flags = YYJSON_READ_ALLOW_BOM;
		if (allow_invalid_unicode)
			flags |= YYJSON_READ_ALLOW_INVALID_UNICODE;

		doc_ = yyjson_read(data, length, flags);
		return doc_ != nullptr;
	}

	std::string get_error() const
	{
		if (doc_)
			return {};
		return "JSON parse error";
	}

	yyjson_val * root() const
	{
		if (!doc_)
			return nullptr;
		return yyjson_doc_get_root(doc_);
	}

	using obj_callback_t = std::function<void(const char * key, size_t key_len, yyjson_val * val)>;
	using arr_callback_t = std::function<void(size_t idx, yyjson_val * val)>;

	static void foreach_obj(yyjson_val * obj, const obj_callback_t & callback)
	{
		if (!obj || !yyjson_is_obj(obj))
			return;

		size_t idx, max;
		yyjson_val * key;
		yyjson_val * val;
		yyjson_obj_foreach(obj, idx, max, key, val)
		{
			callback(yyjson_get_str(key), yyjson_get_len(key), val);
		}
	}

	static void foreach_arr(yyjson_val * arr, const arr_callback_t & callback)
	{
		if (!arr || !yyjson_is_arr(arr))
			return;

		size_t idx, max;
		yyjson_val * val;
		yyjson_arr_foreach(arr, idx, max, val)
		{
			callback(idx, val);
		}
	}

	static const char * get_str(yyjson_val * val)
	{
		return yyjson_get_str(val);
	}

	static size_t get_str_len(yyjson_val * val)
	{
		return yyjson_get_len(val);
	}

	static std::string get_string(yyjson_val * obj, const char * key, const char * fallback = "")
	{
		yyjson_val * val = yyjson_obj_get(obj, key);
		if (!val || !yyjson_is_str(val))
			return fallback;
		const char * str = yyjson_get_str(val);
		size_t len = yyjson_get_len(val);
		return std::string(str, len);
	}

private:
	yyjson_doc * doc_ = nullptr;
};
