#pragma once

#include <QMetaType>
#include <string>

struct translation_suggestion_t
{
    std::string text;
    bool success = false;
    std::string error;
};

Q_DECLARE_METATYPE(translation_suggestion_t)

class translation_provider_t
{
public:
    virtual ~translation_provider_t() = default;

    virtual std::string name() const = 0;
    virtual bool is_available() const = 0;
    virtual bool is_async() const = 0;
    virtual bool has_quota() const = 0;
    virtual int remaining_quota() const = 0;

    virtual void translate(const std::string & text, const std::string & target_lang) = 0;
};
