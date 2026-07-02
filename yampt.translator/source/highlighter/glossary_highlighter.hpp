#pragma once

#include "../editor/glossary.hpp"
#include <theme_system.hpp>
#include <utility/tools.hpp>
#include <vector>
#include <QSyntaxHighlighter>

class glossary_t;

class glossary_highlighter_t : public QSyntaxHighlighter
{
	Q_OBJECT

public:
	explicit glossary_highlighter_t(QTextDocument * parent = nullptr);

	void set_annotation_manager(glossary_t * manager);
	void set_record_type(tools_t::rec_type_t type);
	void set_annotations(const std::vector<annotation_t> & annotations);

protected:
	void highlightBlock(const QString & text) override;

private slots:
	void on_theme_changed();

private:
	glossary_t * m_manager = nullptr;
	tools_t::rec_type_t m_record_type = tools_t::rec_type_t::unknown;
	std::vector<annotation_t> m_annotations;
};
