#pragma once

#include <utility/tools.hpp>
#include "../controller/glossary.hpp"
#include <QSyntaxHighlighter>
#include <vector>

class glossary_t;

class annotation_highlighter_t : public QSyntaxHighlighter
{
	Q_OBJECT

public:
	explicit annotation_highlighter_t(QTextDocument * parent = nullptr);

	void set_annotation_manager(glossary_t * manager);
	void set_record_type(tools_t::rec_type_t type);
	void set_annotations(const std::vector<annotation_t> & annotations);

protected:
	void highlightBlock(const QString & text) override;

private:
	glossary_t * manager_ = nullptr;
	tools_t::rec_type_t record_type_ = tools_t::rec_type_t::unknown;
	std::vector<annotation_t> annotations_;
};
