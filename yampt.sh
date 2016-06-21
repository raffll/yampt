#!/bin/bash

LANG_1=PL
LANG_2=EN

DIR_1=../esm/PL
DIR_2=../esm/EN

ESM_1=Morrowind
ESM_2=Tribunal
ESM_3=Bloodmoon

./yampt --make -b $DIR_1/$ESM_1.esm $DIR_2/$ESM_1.esm
./yampt --make -b $DIR_1/$ESM_2.esm $DIR_2/$ESM_2.esm
./yampt --make -b $DIR_1/$ESM_3.esm $DIR_2/$ESM_3.esm

./yampt --merge $ESM_1.dic $ESM_2.dic $ESM_3.dic
rm $ESM_1.dic $ESM_2.dic $ESM_3.dic
mv $ESM_1-$ESM_2-$ESM_3-Merged.dic $LANG_2-to-$LANG_1.dic

./yampt --make -b $DIR_2/$ESM_1.esm $DIR_1/$ESM_1.esm
./yampt --make -b $DIR_2/$ESM_2.esm $DIR_1/$ESM_2.esm
./yampt --make -b $DIR_2/$ESM_3.esm $DIR_1/$ESM_3.esm

./yampt --merge $ESM_1.dic $ESM_2.dic $ESM_3.dic
rm $ESM_1.dic $ESM_2.dic $ESM_3.dic
mv $ESM_1-$ESM_2-$ESM_3-Merged.dic $LANG_1-to-$LANG_2.dic

./yampt --make -r $DIR_1/$ESM_1.esm
./yampt --make -r $DIR_1/$ESM_2.esm
./yampt --make -r $DIR_1/$ESM_3.esm

./yampt --merge $ESM_1.dic $ESM_2.dic $ESM_3.dic
rm $ESM_1.dic $ESM_2.dic $ESM_3.dic
mv $ESM_1-$ESM_2-$ESM_3-Merged.dic $LANG_1-to-$LANG_1.dic

./yampt --make -r $DIR_2/$ESM_1.esm
./yampt --make -r $DIR_2/$ESM_2.esm
./yampt --make -r $DIR_2/$ESM_3.esm

./yampt --merge $ESM_1.dic $ESM_2.dic $ESM_3.dic
rm $ESM_1.dic $ESM_2.dic $ESM_3.dic
mv $ESM_1-$ESM_2-$ESM_3-Merged.dic $LANG_2-to-$LANG_2.dic
