#!/bin/bash

NAME=yampt
BASE="$NAME/dict_base"
USER="$NAME/dict_user"
NEW="$NAME/dict_new"
IN="$NAME/input"
OUT="$NAME/output"
SCPT="$NAME"

DIR="../"
MASTER="../../../master"
PL="$MASTER/pl"
DE="$MASTER/de"
EN="$MASTER/en"
FR="$MASTER/fr"

if [ -e "$NAME" ]; then
	rm -rv "$NAME"
	rm -v "$NAME.zip"
fi

mkdir "$NAME"
mkdir "$BASE"
mkdir "$USER"
mkdir "$NEW"
mkdir "$IN"
mkdir "$OUT"
#mkdir "$SCPT"

cp "$DIR/x64/Release/yampt.exe" "$NAME"
cp "$DIR/CHANGELOG.txt" "$NAME"
cp "$DIR/README.md" "$NAME"
cp "$DIR/scripts/yampt-convert-XXtoXX.bat" "$SCPT"
cp "$DIR/scripts/yampt-make-base-XXtoXX.bat" "$SCPT"
cp "$DIR/scripts/yampt-make-user-XXtoXX.bat" "$SCPT"

# PL
DICT=ENtoPL
cp "$SCPT/yampt-convert-XXtoXX.bat" "$NAME/yampt-convert-${DICT}.bat"
sed -i "s|XXtoXX|$DICT|g" "$NAME/yampt-convert-${DICT}.bat"
cp "$SCPT/yampt-make-user-XXtoXX.bat" "$NAME/yampt-make-user-${DICT}.bat"
sed -i "s|XXtoXX|$DICT|g" "$NAME/yampt-make-user-${DICT}.bat"

"$NAME/yampt.exe" --make-base -f "$PL/Morrowind.esm" "$EN/Morrowind.esm"
"$NAME/yampt.exe" --make-base -f "$PL/Tribunal.esm" "$EN/Tribunal.esm"
"$NAME/yampt.exe" --make-base -f "$PL/Bloodmoon.esm" "$EN/Bloodmoon.esm"
"$NAME/yampt.exe" --merge -d "Morrowind.BASE.xml" "Tribunal.BASE.xml" "Bloodmoon.BASE.xml" -o "${DICT}.xml"
rm "Morrowind.BASE.xml" "Tribunal.BASE.xml" "Bloodmoon.BASE.xml"
mv "${DICT}.xml" "$BASE"

"$NAME/yampt.exe" --make-all --disable-annotations -f "$EN/Morrowind.esm" -d "$BASE/${DICT}.xml"
"$NAME/yampt.exe" --make-all --disable-annotations -f "$EN/Tribunal.esm" -d "$BASE/${DICT}.xml"
"$NAME/yampt.exe" --make-all --disable-annotations -f "$EN/Bloodmoon.esm" -d "$BASE/${DICT}.xml"
"$NAME/yampt.exe" --merge -d "Morrowind.ALL.xml" "Tribunal.ALL.xml" "Bloodmoon.ALL.xml" -o "${DICT}_H.xml"
rm "Morrowind.ALL.xml" "Tribunal.ALL.xml" "Bloodmoon.ALL.xml"
mv "${DICT}_H.xml" "$BASE"

###
rm "yampt.log"
zip -r "$NAME.zip" "$NAME"
