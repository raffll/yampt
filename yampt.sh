#!/bin/bash

./yampt --make -b ../esm/Morrowind.PL.esm ../esm/Morrowind.ENG.esm
./yampt --make -b ../esm/Tribunal.PL.esm ../esm/Tribunal.ENG.esm
./yampt --make -b ../esm/Bloodmoon.PL.esm ../esm/Bloodmoon.ENG.esm

./yampt --merge Morrowind.PL.dic Tribunal.PL.dic Bloodmoon.PL.dic
rm Morrowind.PL.dic Tribunal.PL.dic Bloodmoon.PL.dic
mv Morrowind.PL-Tribunal.PL-Bloodmoon.PL-Merged.dic PLtoEN.dic

./yampt --convert ../esm/Morrowind.ENG.esm PLtoEN.dic
./yampt --convert ../esm/Tribunal.ENG.esm PLtoEN.dic
./yampt --convert ../esm/Bloodmoon.ENG.esm PLtoEN.dic
