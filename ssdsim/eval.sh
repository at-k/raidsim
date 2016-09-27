#!/bin/sh

EXEA="./run_ssdsimL4P8"
EXEB="./run_ssdsimL8P8"
RES="result.txt"

echo > $RES

for typ in pa cc-nocomp
do
	for fn in 1.15 1.2 1.25 1.3
	do
		$EXEA -o $fn -f $typ >> $RES
	done
done

for typ in ca cc
do
	for fn in 1.15 1.2 1.25 1.3
	do
		$EXEA -o $fn -f $typ -b 4 -t osdb_comp_trace_16k.txt >> $RES
	done
done

for typ in ca cc
do
	for fn in 1.15 1.2 1.25 1.3
	do
		$EXEA -o $fn -f $typ -b 8 -t osdb_comp_trace_32k.txt >> $RES
	done
done

