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

0.6
- minor bug fixes with counters
- change sort to case insensitive
- add suffix to newly created dictionary
- add --make-changed
- remove --compare
- add -o option to --merge
