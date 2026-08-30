#include <catch2/catch_all.hpp>
#include <translator/model_list_utils.hpp>
#include <QByteArray>
#include <QJsonDocument>

namespace {

QJsonDocument document_from(const char * json)
{
	return QJsonDocument::fromJson(QByteArray(json));
}

} // namespace

TEST_CASE("model_list_utils::extract_model_list, openai anthropic data array", "[u]")
{
	const auto document = document_from(R"({"data":[{"id":"x"},{"id":"y"}]})");
	const auto models = model_list_utils::extract_model_list(document, {"data", "id"});

	REQUIRE(models.size() == 2);
	REQUIRE(models[0] == "x");
	REQUIRE(models[1] == "y");
}

TEST_CASE("model_list_utils::extract_model_list, missing array yields empty", "[u]")
{
	const auto document = document_from(R"({"other":[{"id":"x"}]})");
	const auto models = model_list_utils::extract_model_list(document, {"data", "id"});

	REQUIRE(models.empty());
}

TEST_CASE("model_list_utils::extract_model_list, empty array yields empty", "[u]")
{
	const auto document = document_from(R"({"data":[]})");
	const auto models = model_list_utils::extract_model_list(document, {"data", "id"});

	REQUIRE(models.empty());
}

TEST_CASE("model_list_utils::extract_model_list, element missing id key skipped", "[u]")
{
	const auto document = document_from(R"({"data":[{"id":"x"},{"name":"y"},{"id":"z"}]})");
	const auto models = model_list_utils::extract_model_list(document, {"data", "id"});

	REQUIRE(models.size() == 2);
	REQUIRE(models[0] == "x");
	REQUIRE(models[1] == "z");
}

TEST_CASE("model_list_utils::extract_model_list, non-array node yields empty", "[u]")
{
	const auto document = document_from(R"({"data":{"id":"x"}})");
	const auto models = model_list_utils::extract_model_list(document, {"data", "id"});

	REQUIRE(models.empty());
}

TEST_CASE("model_list_utils::extract_model_list, custom id key", "[u]")
{
	const auto document = document_from(R"({"data":[{"model_id":"a"},{"model_id":"b"}]})");
	const auto models = model_list_utils::extract_model_list(document, {"data", "model_id"});

	REQUIRE(models.size() == 2);
	REQUIRE(models[0] == "a");
	REQUIRE(models[1] == "b");
}

TEST_CASE("model_list_utils::choose_selected_model, previous present keeps previous", "[u]")
{
	const std::vector<std::string> new_list{"one", "two", "three"};
	const auto selected = model_list_utils::choose_selected_model("two", new_list, "one");

	REQUIRE(selected == "two");
}

TEST_CASE("model_list_utils::choose_selected_model, previous absent uses default", "[u]")
{
	const std::vector<std::string> new_list{"one", "two", "three"};
	const auto selected = model_list_utils::choose_selected_model("gone", new_list, "one");

	REQUIRE(selected == "one");
}
