#!/bin/bash

NAME="yampt"
BASE="dict_base"
USER="dict_user"
NEW="dict_new"
IN="input"
OUT="output"
SCPT="scripts"

DEV="../"
MST="../../../../!master"
PL="$MST/pl"
PL_PATCH="$MST/pl_plpatch"
DE="$MST/de"
EN="$MST/en"
FR="$MST/fr"

if [ -e "$NAME" ]; then
	rm -rv "$NAME"
	rm -v "$NAME.zip"
fi

mkdir "$NAME"
mkdir "$NAME/$BASE"
mkdir "$NAME/$USER"
mkdir "$NAME/$NEW"
mkdir "$NAME/$IN"
mkdir "$NAME/$OUT"
mkdir "$NAME/$SCPT"

cp "$DEV/x64/Release/yampt.exe" "$NAME"
cp "$DEV/CHANGELOG.txt" "$NAME"
cp "$DEV/README.md" "$NAME"
cp "$DEV/scripts/"*.bat "$NAME/$SCPT"

### PL
DICT=ENtoPL

cp "$NAME/$SCPT/yampt-make-base-XXtoXX.bat" "$NAME/yampt-make-base-${DICT}.bat"
sed -i "s|NATIVE=|NATIVE=$PL|g" "$NAME/yampt-make-base-${DICT}.bat"
sed -i "s|FOREIGN=|FOREIGN=$EN|g" "$NAME/yampt-make-base-${DICT}.bat"
sed -i "s|XXtoXX|$DICT|g" "$NAME/yampt-make-base-${DICT}.bat"

cp "$NAME/$SCPT/yampt-convert-XXtoXX.bat" "$NAME/yampt-convert-${DICT}.bat"
sed -i "s|XXtoXX|$DICT|g" "$NAME/yampt-convert-${DICT}.bat"

cp "$NAME/$SCPT/yampt-make-user-XXtoXX.bat" "$NAME/yampt-make-user-${DICT}.bat"
sed -i "s|XXtoXX|$DICT|g" "$NAME/yampt-make-user-${DICT}.bat"
sed -i "s|COMMANDS=|COMMANDS=--add-annotations --add-hyperlinks|g" "$NAME/yampt-make-user-${DICT}.bat"

cd "$NAME"
cmd.exe /c "yampt-make-base-${DICT}.bat" nopause
rm "yampt-make-base-${DICT}.bat"
cd ..

### PL_plpatch
DICT=ENtoPL_plpatch

cp "$NAME/$SCPT/yampt-make-base-XXtoXX.bat" "$NAME/yampt-make-base-${DICT}.bat"
sed -i "s|NATIVE=|NATIVE=$PL_PATCH|g" "$NAME/yampt-make-base-${DICT}.bat"
sed -i "s|FOREIGN=|FOREIGN=$EN|g" "$NAME/yampt-make-base-${DICT}.bat"
sed -i "s|XXtoXX|$DICT|g" "$NAME/yampt-make-base-${DICT}.bat"

cp "$NAME/$SCPT/yampt-convert-XXtoXX.bat" "$NAME/yampt-convert-${DICT}.bat"
sed -i "s|XXtoXX|$DICT|g" "$NAME/yampt-convert-${DICT}.bat"

cp "$NAME/$SCPT/yampt-make-user-XXtoXX.bat" "$NAME/yampt-make-user-${DICT}.bat"
sed -i "s|XXtoXX|$DICT|g" "$NAME/yampt-make-user-${DICT}.bat"
sed -i "s|COMMANDS=|COMMANDS=--add-annotations --add-hyperlinks|g" "$NAME/yampt-make-user-${DICT}.bat"

cd "$NAME"
cmd.exe /c "yampt-make-base-${DICT}.bat" nopause
rm "yampt-make-base-${DICT}.bat"
cd ..

###
rm "$NAME/yampt.log"
zip -r "$NAME.zip" "$NAME"
