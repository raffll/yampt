#!/bin/bash

DIC=.
PATH_N=../esm/PL
PATH_F=../esm/EN

./yampt --make-base -f $PATH_N/Morrowind.esm $PATH_F/Morrowind.esm
./yampt --make-base -f $PATH_N/Tribunal.esm $PATH_F/Tribunal.esm
./yampt --make-base -f $PATH_N/Bloodmoon.esm $PATH_F/Bloodmoon.esm

./yampt --merge -d $DIC/Morrowind.dic $DIC/Tribunal.dic $DIC/Bloodmoon.dic
