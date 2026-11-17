"""Write README.md and docs/README.bbcode from one feature list so they stay mirrored.

Edit the lists below (groups and items in order of importance, no bold), then run:
  py scripts/write_readme.py
"""

title = "Yet Another Morrowind Plugin Toolkit"
intro = ("A suite of tools for working with Morrowind plugins: a plugin conflict editor and merger, "
         "a translation workbench, and a command-line tool for batch translation work.")

# (heading, description, [(subheading, [text, ...]), ...]); groups and items in order of importance
apps = [
    ("yEditor.exe — Plugin Editor",
     "Views, compares, edits, and merges plugins, in the spirit of xEdit.",
     [
         ("Merging", [
             "Automatic merged patch: leveled lists plus three-way merging of individual fields",
             "Locks records or fields so later merges leave your manual changes alone",
             "Guard patches: records a guard plugin overrides are not merged from plugins loaded before it",
             "One exclusion list for plugins, record IDs, record types, and sub-records, plus switchable fixes",
             "Automatic fixes for fog density, summon persistence, and reverted cell names",
             "Shows a topic's dialogue responses in their final in-game order, resolved the way OpenMW does it",
         ]),
         ("Viewing", [
             "Compares any number of plugins at once, with no plugin limit",
             "Loads plugins and OpenMW content files from a folder, a Mod Organizer 2 profile, or an OpenMW configuration",
             "Shows each record's decoded fields side by side, lined up by content across plugins",
             "Navigation tree with xEdit-style conflict colors that carry up to each plugin",
             "Combinable filters: conflicts only, ID and name search, record type, deleted, locked, and excluded records",
             "Finds conflicting OpenMW Lua handler registrations across mods",
             "Side-by-side text comparison with character-level difference highlighting",
         ]),
         ("Editing", [
             "Edits decoded fields directly",
             "Copies or removes whole records, groups, or single fields between plugins",
             "Works on the merged patch or on a plugin you create yourself",
         ]),
         ("Maintenance", [
             "Plugin cleaning: removes evil GMSTs and junk cells in bulk",
             "Header repair: corrects master file sizes and updates the plugin version to 1.3",
         ]),
     ]),
    ("yTranslator.exe — Translation Workbench",
     "An interactive workbench for translating plugins.",
     [
         ("Translation", [
             "Built-in offline translation engine, no API key required",
             "Web translation providers: DeepL, Google Translate, ChatGPT, Claude",
             "Curated examples, up to twenty records, that guide the AI's translation style",
         ]),
         ("Editing", [
             "Three-panel editor: original text, adapted text, and your translation",
             "Record table filtered by type, sub-type, and translation status",
             "Entry validation, status tracking, and history with undo and revert",
             "Find and replace across the dictionary",
         ]),
         ("Workflows", [
             "Topic tags: marks dialogue topics, including inflected forms, as clickable links in translated responses",
             "Inserts dialogue hyperlinks while converting plugins",
             "EET import: turns an ESP-ESM Translator file into a complete, applicable dictionary using its source plugin, or into a plain reference dictionary",
             "Lua localization: edits YAML localization files with automatic pairing and scaffold generation",
             "Generates localization files (.cel, .mrk, .top) from a dictionary",
             "Removes topic tags from a dictionary's dialogue responses",
         ]),
         ("Analysis", [
             "Spell checking per language",
             "Syntax highlighting for MWScript keywords, hyperlinks, glossary terms, and forbidden characters, each switchable",
             "Annotations: hyperlinks, speaker gender, glossary matches, and inflected topic forms",
             "Full script preview with synchronized scrolling between original and translation",
             "Live book preview",
         ]),
     ]),
    ("yampt.exe — Command Line",
     "Batch dictionary and conversion work for scripted workflows.",
     [
         ("Conversion", [
             "Converts plugins using dictionary translations",
             "Converts compiled script data without recompiling in the Construction Set",
             "Creates patch plugins that contain only the changed records",
         ]),
         ("Dictionary", [
             "Creates base dictionaries by pairing two language versions of a master file",
             "Creates plugin dictionaries from a base dictionary",
             "Merges dictionaries in priority order",
             "Adds dialogue topic tags to translated text",
         ]),
     ]),
]

general = [
    "Windows and Linux",
    "Dark mode",
    "Remembers window layout and session between runs",
]
changelog_url = "https://github.com/raffll/yampt/blob/master/CHANGELOG.md"
issues_url = "https://github.com/raffll/yampt/issues"


def markdown():
    out = [f"# {title}", "", intro, ""]
    for heading, description, groups in apps:
        out += [f"## {heading}", "", description, ""]
        for sub, items in groups:
            out.append(f"### {sub}")
            out += [f"- {text}" for text in items]
            out.append("")
    out += ["## General", ""] + [f"- {item}" for item in general] + [""]
    out += ["## Changelog", "", f"See the [full changelog]({changelog_url}) for version history.", ""]
    out += ["## Bug Reports", "", f"Report bugs and feature requests on [GitHub]({issues_url}) or in the Nexus Mods comments section.", ""]
    return "\n".join(out)


def bbcode():
    out = [f"[center][size=5][b]{title}[/b][/size][/center]", "", intro, ""]
    for heading, description, groups in apps:
        out += [f"[size=4][b]{heading}[/b][/size]", "", description, ""]
        for sub, items in groups:
            out += [f"[b]{sub}[/b]", "", "[list]"]
            out += [f"[*]{text}" for text in items]
            out += ["[/list]", ""]
    out += ["[size=4][b]General[/b][/size]", "", "[list]"] + [f"[*]{item}" for item in general] + ["[/list]", ""]
    out += ["[size=4][b]Changelog[/b][/size]", "", f"See the [url={changelog_url}]full changelog[/url] for version history.", ""]
    out += ["[size=4][b]Bug Reports[/b][/size]", "", f"Report bugs and feature requests on [url={issues_url}]GitHub[/url] or in the Nexus Mods comments section.", ""]
    return "\n".join(out)


for path, content in ((r"c:\OMEN\Morrowind\yampt\README.md", markdown()),
                      (r"c:\OMEN\Morrowind\yampt\docs\README.bbcode", bbcode())):
    with open(path, encoding="utf-8", newline="") as handle:
        newline = "\r\n" if "\r\n" in handle.read() else "\n"
    with open(path, "w", encoding="utf-8", newline="") as handle:
        handle.write(content.replace("\n", newline))
    print("wrote", path)
