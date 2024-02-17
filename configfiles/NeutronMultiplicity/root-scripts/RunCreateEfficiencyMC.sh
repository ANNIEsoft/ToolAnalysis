#/bin/bash

portlist="./portlist.txt"
heightlist="./heightlist.txt"

while read -r port
do
	echo "Port: ${port}"
	while read -r height
	do
		echo "Z: ${height}"
		root -b -l -q "create_efficiency_mc.C(${port},${height})"
	done < $heightlist
done < $portlist

