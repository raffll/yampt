#!/bin/bash

# Set path to folders where you keep native and foreign master files (Morrowind.esm, Tribunal.esm and Bloodmoon.esm)
_NATIVE=
FOREIGN=
NAME=NATIVE
BASE=dict_base

############### DON'T EDIT ###############

# Prepare
mkdir "$BASE" 2> /dev/null

# Create new native dictionaries
./yampt --make-base -f "$_NATIVE/Morrowind.esm" "$FOREIGN/Morrowind.esm"
./yampt --make-base -f "$_NATIVE/Tribunal.esm" "$FOREIGN/Tribunal.esm"
./yampt --make-base -f "$_NATIVE/Bloodmoon.esm" "$FOREIGN/Bloodmoon.esm"
./yampt --merge -d "Morrowind.BASE.xml" "Tribunal.BASE.xml" "Bloodmoon.BASE.xml" -o "$NAME.xml"

rm "Morrowind.BASE.xml" "Tribunal.BASE.xml" "Bloodmoon.BASE.xml"
mv "$NAME.xml" "$BASE"
mv "yampt.log" "yampt.$NAME.log"

./yampt --make-all -f "$FOREIGN/Morrowind.esm" -d "$BASE/$NAME.xml"
./yampt --make-all -f "$FOREIGN/Tribunal.esm" -d "$BASE/$NAME.xml"
./yampt --make-all -f "$FOREIGN/Bloodmoon.esm" -d "$BASE/$NAME.xml"
./yampt --merge -d "Morrowind.ALL.xml" "Tribunal.ALL.xml" "Bloodmoon.ALL.xml" -o "${NAME}_for_find_changed_only.xml"

rm "Morrowind.ALL.xml" "Tribunal.ALL.xml" "Bloodmoon.ALL.xml"
mv "${NAME}_for_find_changed_only.xml" "$BASE"
mv "yampt.log" "yampt.${NAME}_for_find_changed_only.log"