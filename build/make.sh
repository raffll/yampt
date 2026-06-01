#!/bin/bash

cd "$(dirname "$0")"

NAME="yampt"
BASE="dict_base"
USER="dict_user"
NEW="dict_new"
IN="input"
OUT="output"

DEV=".."
MST="$DEV/master"
PL="$MST/pl"
PL_PATCH="$MST/pl_plpatch"
EN="$MST/en"

YAMPT="$NAME/yampt.exe"

if [ -e "$NAME" ]; then
	rm -rf "$NAME"/*
	rm -f "$NAME.zip"
else
	mkdir "$NAME"
fi

mkdir -p "$NAME/$BASE"
mkdir -p "$NAME/$USER"
mkdir -p "$NAME/$NEW"
mkdir -p "$NAME/$IN"
mkdir -p "$NAME/$OUT"

cp "$DEV/x64/Release/yampt.exe" "$NAME"
cp "$DEV/CHANGELOG.txt" "$NAME"
cp "$DEV/README.md" "$NAME"

### PL — ENtoPL
DICT=ENtoPL

"$YAMPT" --make-base -f "$PL/Morrowind.esm" "$EN/Morrowind.esm"
"$YAMPT" --make-base -f "$PL/Tribunal.esm" "$EN/Tribunal.esm"
"$YAMPT" --make-base -f "$PL/Bloodmoon.esm" "$EN/Bloodmoon.esm"
"$YAMPT" --merge -d "Morrowind.BASE.json" "Tribunal.BASE.json" "Bloodmoon.BASE.json" -o "$NAME/$BASE/${DICT}.json"

mv -f "Morrowind.BASE.json" "$NAME/$BASE/Morrowind.BASE.json"
mv -f "Tribunal.BASE.json" "$NAME/$BASE/Tribunal.BASE.json"
mv -f "Bloodmoon.BASE.json" "$NAME/$BASE/Bloodmoon.BASE.json"

### PL_plpatch — ENtoPL_plpatch
DICT=ENtoPL_plpatch

"$YAMPT" --make-base -f "$PL_PATCH/Morrowind.esm" "$EN/Morrowind.esm"
"$YAMPT" --make-base -f "$PL_PATCH/Tribunal.esm" "$EN/Tribunal.esm"
"$YAMPT" --make-base -f "$PL_PATCH/Bloodmoon.esm" "$EN/Bloodmoon.esm"
"$YAMPT" --merge -d "Morrowind.BASE.json" "Tribunal.BASE.json" "Bloodmoon.BASE.json" -o "$NAME/$BASE/${DICT}.json"

###
rm -f "$NAME/yampt.log"
powershell.exe -NoProfile -Command "Compress-Archive -Path '$NAME' -DestinationPath '$NAME.zip' -Force"
