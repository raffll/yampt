#pragma once

#include <functional>
#include <string>
#include <vector>

namespace phrase_form_builder {

using phrase_validity_predicate_t = std::function<bool(const std::vector<std::string> & words)>;

std::vector<std::string> build_phrase_forms(
    const std::vector<std::vector<std::string>> & per_word_candidate_forms,
    const phrase_validity_predicate_t & is_valid_phrase,
    int max_phrase_forms);

} // namespace phrase_form_builder
