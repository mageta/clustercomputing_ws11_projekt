#!/bin/bash

echo pixelpattern
time ./pixelpattern 512 512 2>patternx.txt | grep '^[0-9]' | sort -fn > .comp.1.txt
echo "components: `wc -l .comp.1.txt`"
echo find_components
time ./find_components patternx.txt 1>/dev/null 2>.comp.2.txt
echo compare
sort -fn .comp.2.txt > .comp.2.txt.sort
mv .comp.2.txt.sort .comp.2.txt

if ! (diff .comp.1.txt .comp.2.txt &>/dev/null)
then
	echo "the components doesn't match! something bad happened" &>/dev/stderr
	exit 1
fi

rm -f .comp.1.txt 2>/dev/null
rm -f .comp.2.txt 2>/dev/null
rm -f patternx.txt 2>/dev/null

echo "finished successfull"

exit 0
