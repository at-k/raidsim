#!/bin/sh

EXE="./run_sim"
RES="result2.txt"

echo > $RES

#$EXE  -m c -R 1.10 -C 16 -r 1.25 -t ssdsim/osdb_comp_trace_8k.txt >> $RES
#$EXE  -m c -R 1.10 -C 16 -r 1.25 -t ssdsim/osdb_comp_trace_8k.txt >> $RES
#$EXE  -m c -R 1.10 -C 16 -r 1.25 -t ssdsim/osdb_comp_trace_8k.txt >> $RES
#$EXE  -m c -R 1.10 -C 16 -r 1.25 -t ssdsim/osdb_comp_trace_8k.txt >> $RES
#$EXE  -m c -R 1.10 -C 16 -r 1.25 -t ssdsim/osdb_comp_trace_8k.txt >> $RES
#
#$EXE  -m c -R 1.15 -C 32 -r 1.25 -t ssdsim/osdb_comp_trace_16k.txt >> $RES
#$EXE  -m c -R 1.15 -C 32 -r 1.25 -t ssdsim/osdb_comp_trace_16k.txt >> $RES
#$EXE  -m c -R 1.15 -C 32 -r 1.25 -t ssdsim/osdb_comp_trace_16k.txt >> $RES
#$EXE  -m c -R 1.15 -C 32 -r 1.25 -t ssdsim/osdb_comp_trace_16k.txt >> $RES
#$EXE  -m c -R 1.15 -C 32 -r 1.25 -t ssdsim/osdb_comp_trace_16k.txt >> $RES

$EXE  -m c -R 1.05 -C 32 -r 1.05 -t ssdsim/osdb_comp_trace_16k.txt >> $RES
$EXE  -m c -R 1.05 -C 32 -r 1.15 -t ssdsim/osdb_comp_trace_16k.txt >> $RES
$EXE  -m c -R 1.05 -C 32 -r 1.25 -t ssdsim/osdb_comp_trace_16k.txt >> $RES
$EXE  -m c -R 1.15 -C 32 -r 1.05 -t ssdsim/osdb_comp_trace_16k.txt >> $RES
$EXE  -m c -R 1.15 -C 32 -r 1.15 -t ssdsim/osdb_comp_trace_16k.txt >> $RES
$EXE  -m c -R 1.15 -C 32 -r 1.25 -t ssdsim/osdb_comp_trace_16k.txt >> $RES
$EXE  -m c -R 1.25 -C 32 -r 1.05 -t ssdsim/osdb_comp_trace_16k.txt >> $RES
$EXE  -m c -R 1.25 -C 32 -r 1.15 -t ssdsim/osdb_comp_trace_16k.txt >> $RES
$EXE  -m c -R 1.25 -C 32 -r 1.25 -t ssdsim/osdb_comp_trace_16k.txt >> $RES

#$EXE  -m s -r 1.10 -t ssdsim/osdb_comp_trace_16k.txt >> $RES
#$EXE  -m s -r 1.15 -t ssdsim/osdb_comp_trace_16k.txt >> $RES
#$EXE  -m s -r 1.20 -t ssdsim/osdb_comp_trace_16k.txt >> $RES
#$EXE  -m s -r 1.25 -t ssdsim/osdb_comp_trace_16k.txt >> $RES
#$EXE  -m s -r 1.30 -t ssdsim/osdb_comp_trace_16k.txt >> $RES
#$EXE  -m s -r 1.35 -t ssdsim/osdb_comp_trace_16k.txt >> $RES
#$EXE  -m s -r 1.40 -t ssdsim/osdb_comp_trace_16k.txt >> $RES
#$EXE  -m s -r 1.45 -t ssdsim/osdb_comp_trace_16k.txt >> $RES
#$EXE  -m s -r 1.50 -t ssdsim/osdb_comp_trace_16k.txt >> $RES

