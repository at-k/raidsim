#!/bin/sh

EXEA="./run_ssdsimL4P8"
EXEB="./run_ssdsimL8P8"
RES="result.txt"

echo > $RES

for fn in 1.15 1.2 1.25 1.3
do
	$EXEA -o $fn -f pa >> $RES
	$EXEA -o $fn -f ca >> $RES
	$EXEA -o $fn -f ca -b 8 -t osdb_comp_trace_2.txt >> $RES
	$EXEA -o $fn -f cc >> $RES
	$EXEA -o $fn -f cc-nocomp >> $RES

	$EXEB -o $fn -f pa >> $RES
	$EXEB -o $fn -f ca >> $RES
	$EXEB -o $fn -f cc >> $RES
	$EXEB -o $fn -f cc-nocomp >> $RES
done

