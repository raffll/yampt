#!/bin/bash

# Set path to folders where you keep native and foreign master files (Morrowind.esm, Tribunal.esm and Bloodmoon.esm)
PATH_NATIVE=
PATH_FOREIGN=

# Skip
BASE=dict_base

############### DON'T EDIT ###############

mkdir $BASE

# Create new native dictionary
yampt --make-base -f "$PATH_NATIVE\Morrowind.esm" "$PATH_FOREIGN\Morrowind.esm"
yampt --make-base -f "$PATH_NATIVE\Tribunal.esm" "$PATH_FOREIGN\Tribunal.esm"
yampt --make-base -f "$PATH_NATIVE\Bloodmoon.esm" "$PATH_FOREIGN\Bloodmoon.esm"
yampt --merge -d "Morrowind.dic" "Tribunal.dic" "Bloodmoon.dic" -o "NATIVE.dic"

rm "Morrowind.dic" "Tribunal.dic" "Bloodmoon.dic"
mv "NATIVE.dic" $BASE

# Create new foreign dictionary
yampt --make-all -f "$PATH_FOREIGN\Morrowind.esm" -d "$BASE\NATIVE.dic"
yampt --make-all -f "$PATH_FOREIGN\Tribunal.esm" -d "$BASE\NATIVE.dic"
yampt --make-all -f "$PATH_FOREIGN\Bloodmoon.esm" -d "$BASE\NATIVE.dic"
yampt --merge -d "Morrowind.ALL.dic" "Tribunal.ALL.dic" "Bloodmoon.ALL.dic" -o "FOREIGN.dic"

rm "Morrowind.ALL.dic" "Tribunal.ALL.dic" "Bloodmoon.ALL.dic"
mv "FOREIGN.dic" $BASE
