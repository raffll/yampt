## Version history

0.1 alpha
- initial release

0.2 alpha
- add merger log
- add option to write raw script text log
- remove ENCH records from dictionary (they haven't got friendly names)
- add aiescortcell to script keywords
- add REPLACE_BROKEN_CHARS option
- minor bug fixes

0.3 alpha
- fix exception when try to convert non existent subrecord

0.4 alpha
- fix exception when RNAM record has a variable length
- fix limit of INFO string to 512 and FNAM to 32 (instead of 511 and 31)
- rewrite --make-all command
- add --convert-safe command
- remove yampt.cfg

0.5 alpha
- redesign interface
	- remove --convert-with-dial
	- remove --convert-safe
	- remove --scripts
	- remove -r
	- remove -a
	- add --add-dial
	- add --safe
	- add --compare
- add detailed log
- add limit of RNAM string to 32
- creator and converter now ommit FNAM "player" record
- many bug fixes

0.6 beta
- minor bug fixes
- change dictionary sorting to case insensitive
- add suffix to newly created dictionaries
- add --make-changed command
- remove --compare command
- add -o option to --merge command

0.7 beta
- add GMDT convertion
- add --binary-dump option
- add --find-diff option
- revert length limit FNAM back to 31 and INFO to 511
- add length limit CELL to 63
- fix crash when script keyword is a part of longer string

0.8 beta
- log fixes
- add --word-list option
- add --swap-records option

0.9 beta
- many improvements in script parser

0.10 beta
- minor script parser improvement
- add compiled script data converter

0.11 beta
- log fixes
