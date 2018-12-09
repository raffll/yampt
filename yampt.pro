TEMPLATE = subdirs

DISTFILES += \
    CHANGELOG.md \
    README.md \
    LICENSE.txt \
    resources/HOW-TO-START.md \
    resources/yampt-make-base-template.sh \
    resources/yampt-convert-template.cmd \
    resources/yampt-make-base-template.cmd \
    resources/yampt-make-user-template.cmd \
    resources/yampt-generate-report.cmd

SUBDIRS += \
    src/yampt_cli.pro \
    test/yampt_test.pro \
    gui/yampt_gui.pro
