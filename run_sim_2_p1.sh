#!/bin/sh

EXE="./run_sim"
RES="result1.txt"

echo > $RES

#$EXE  -m c -R 1.05 -C 8 -r 1.25 -t ssdsim/osdb_comp_trace_8k.txt --skip_ssd >> $RES
#$EXE  -m c -R 1.10 -C 8 -r 1.25 -t ssdsim/osdb_comp_trace_8k.txt --skip_ssd >> $RES
#$EXE  -m c -R 1.15 -C 8 -r 1.25 -t ssdsim/osdb_comp_trace_8k.txt --skip_ssd >> $RES
#$EXE  -m c -R 1.20 -C 8 -r 1.25 -t ssdsim/osdb_comp_trace_8k.txt --skip_ssd >> $RES
#$EXE  -m c -R 1.25 -C 8 -r 1.25 -t ssdsim/osdb_comp_trace_8k.txt --skip_ssd >> $RES
#$EXE  -m c -R 1.30 -C 8 -r 1.25 -t ssdsim/osdb_comp_trace_8k.txt --skip_ssd >> $RES

#$EXE  -m c -R 1.05 -C 16 -r 1.25 -t ssdsim/osdb_comp_trace_8k.txt --skip_ssd >> $RES
#$EXE  -m c -R 1.10 -C 16 -r 1.25 -t ssdsim/osdb_comp_trace_8k.txt --skip_ssd >> $RES
#$EXE  -m c -R 1.15 -C 16 -r 1.25 -t ssdsim/osdb_comp_trace_8k.txt --skip_ssd >> $RES
#$EXE  -m c -R 1.20 -C 16 -r 1.25 -t ssdsim/osdb_comp_trace_8k.txt --skip_ssd >> $RES
#$EXE  -m c -R 1.25 -C 16 -r 1.25 -t ssdsim/osdb_comp_trace_8k.txt --skip_ssd >> $RES
#$EXE  -m c -R 1.30 -C 16 -r 1.25 -t ssdsim/osdb_comp_trace_8k.txt --skip_ssd >> $RES

$EXE  -m c -R 1.05 -C 128 -r 1.25 -t ssdsim/osdb_comp_trace_64k.txt --skip_ssd >> $RES
$EXE  -m c -R 1.10 -C 128 -r 1.25 -t ssdsim/osdb_comp_trace_64k.txt --skip_ssd >> $RES
$EXE  -m c -R 1.15 -C 128 -r 1.25 -t ssdsim/osdb_comp_trace_64k.txt --skip_ssd >> $RES
$EXE  -m c -R 1.20 -C 128 -r 1.25 -t ssdsim/osdb_comp_trace_64k.txt --skip_ssd >> $RES
$EXE  -m c -R 1.25 -C 128 -r 1.25 -t ssdsim/osdb_comp_trace_64k.txt --skip_ssd >> $RES
$EXE  -m c -R 1.30 -C 128 -r 1.25 -t ssdsim/osdb_comp_trace_64k.txt --skip_ssd >> $RES

#$EXE  -m c -R 1.05 -C 32 -r 1.25 -t ssdsim/osdb_comp_trace_16k.txt --skip_ssd >> $RES
#$EXE  -m c -R 1.10 -C 32 -r 1.25 -t ssdsim/osdb_comp_trace_16k.txt --skip_ssd >> $RES
#$EXE  -m c -R 1.15 -C 32 -r 1.25 -t ssdsim/osdb_comp_trace_16k.txt --skip_ssd >> $RES
#$EXE  -m c -R 1.20 -C 32 -r 1.25 -t ssdsim/osdb_comp_trace_16k.txt --skip_ssd >> $RES
#$EXE  -m c -R 1.25 -C 32 -r 1.25 -t ssdsim/osdb_comp_trace_16k.txt --skip_ssd >> $RES
#$EXE  -m c -R 1.30 -C 32 -r 1.25 -t ssdsim/osdb_comp_trace_16k.txt --skip_ssd >> $RES
#$EXE  -m c -R 1.35 -C 32 -r 1.25 -t ssdsim/osdb_comp_trace_16k.txt --skip_ssd >> $RES
#$EXE  -m c -R 1.40 -C 32 -r 1.25 -t ssdsim/osdb_comp_trace_16k.txt --skip_ssd >> $RES
#$EXE  -m c -R 1.45 -C 32 -r 1.25 -t ssdsim/osdb_comp_trace_16k.txt --skip_ssd >> $RES
#$EXE  -m c -R 1.50 -C 32 -r 1.25 -t ssdsim/osdb_comp_trace_16k.txt --skip_ssd >> $RES
#
#$EXE  -m c -R 1.05 -C 64 -r 1.25 -t ssdsim/osdb_comp_trace_32k.txt --skip_ssd >> $RES
#$EXE  -m c -R 1.10 -C 64 -r 1.25 -t ssdsim/osdb_comp_trace_32k.txt --skip_ssd >> $RES
#$EXE  -m c -R 1.15 -C 64 -r 1.25 -t ssdsim/osdb_comp_trace_32k.txt --skip_ssd >> $RES
#$EXE  -m c -R 1.20 -C 64 -r 1.25 -t ssdsim/osdb_comp_trace_32k.txt --skip_ssd >> $RES
#$EXE  -m c -R 1.25 -C 64 -r 1.25 -t ssdsim/osdb_comp_trace_32k.txt --skip_ssd >> $RES
#$EXE  -m c -R 1.30 -C 64 -r 1.25 -t ssdsim/osdb_comp_trace_32k.txt --skip_ssd >> $RES
#$EXE  -m c -R 1.35 -C 64 -r 1.25 -t ssdsim/osdb_comp_trace_32k.txt --skip_ssd >> $RES
#$EXE  -m c -R 1.40 -C 64 -r 1.25 -t ssdsim/osdb_comp_trace_32k.txt --skip_ssd >> $RES
#$EXE  -m c -R 1.45 -C 64 -r 1.25 -t ssdsim/osdb_comp_trace_32k.txt --skip_ssd >> $RES
#$EXE  -m c -R 1.50 -C 64 -r 1.25 -t ssdsim/osdb_comp_trace_32k.txt --skip_ssd >> $RES
#$EXE  -m c -R 1.55 -C 64 -r 1.25 -t ssdsim/osdb_comp_trace_32k.txt --skip_ssd >> $RES
#$EXE  -m c -R 1.60 -C 64 -r 1.25 -t ssdsim/osdb_comp_trace_32k.txt --skip_ssd >> $RES

#$EXE  -m c -R 1.20 -C 256 -r 1.25 -t ssdsim/osdb_comp_trace_128k.txt >> $RES
#$EXE  -m c -R 1.20 -C 256 -r 1.25 -t ssdsim/osdb_comp_trace_128k.txt >> $RES
#$EXE  -m c -R 1.20 -C 256 -r 1.25 -t ssdsim/osdb_comp_trace_128k.txt >> $RES
#$EXE  -m c -R 1.20 -C 256 -r 1.25 -t ssdsim/osdb_comp_trace_128k.txt >> $RES
#$EXE  -m c -R 1.20 -C 256 -r 1.25 -t ssdsim/osdb_comp_trace_128k.txt >> $RES
#
#$EXE  -m s -r 1.10 -t ssdsim/osdb_comp_trace_16k.txt >> $RES
#$EXE  -m s -r 1.15 -t ssdsim/osdb_comp_trace_16k.txt >> $RES
#$EXE  -m s -r 1.20 -t ssdsim/osdb_comp_trace_16k.txt >> $RES
#$EXE  -m s -r 1.25 -t ssdsim/osdb_comp_trace_16k.txt >> $RES
#$EXE  -m s -r 1.30 -t ssdsim/osdb_comp_trace_16k.txt >> $RES
#
