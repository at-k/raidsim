#!/bin/sh

EXE="./run_sim"
RES="result.txt"

echo > $RES

$EXE  -d 5 -b 34359738368 -c 0.5 -m c -R 1.10 -C 16 -r 1.25 -i 33554320 >> $RES
#$EXE  -d 5 -b 34359738368 -c 0.5 -m c -R 1.15 -C 16 -r 1.25 -i 33554320 >> $RES
$EXE  -d 5 -b 34359738368 -c 0.5 -m c -R 1.20 -C 16 -r 1.25 -i 33554320 >> $RES
#$EXE  -d 5 -b 34359738368 -c 0.5 -m c -R 1.25 -C 16 -r 1.25 -i 33554320 >> $RES
$EXE  -d 5 -b 34359738368 -c 0.5 -m c -R 1.30 -C 16 -r 1.25 -i 33554320 >> $RES
#$EXE  -d 5 -b 34359738368 -c 0.5 -m c -R 1.35 -C 16 -r 1.25 -i 33554320 >> $RES
$EXE  -d 5 -b 34359738368 -c 0.5 -m c -R 1.40 -C 16 -r 1.25 -i 33554320 >> $RES
#$EXE  -d 5 -b 34359738368 -c 0.5 -m c -R 1.45 -C 16 -r 1.25 -i 33554320 >> $RES
$EXE  -d 5 -b 34359738368 -c 0.5 -m c -R 1.50 -C 16 -r 1.25 -i 33554320 >> $RES

$EXE  -d 5 -b 34359738368 -c 0.5 -m c -R 1.10 -C 16 -r 1.1 -i 33554320 >> $RES
#$EXE  -d 5 -b 34359738368 -c 0.5 -m c -R 1.15 -C 16 -r 1.1 -i 33554320 >> $RES
$EXE  -d 5 -b 34359738368 -c 0.5 -m c -R 1.20 -C 16 -r 1.1 -i 33554320 >> $RES
#$EXE  -d 5 -b 34359738368 -c 0.5 -m c -R 1.25 -C 16 -r 1.1 -i 33554320 >> $RES
$EXE  -d 5 -b 34359738368 -c 0.5 -m c -R 1.30 -C 16 -r 1.1 -i 33554320 >> $RES
#$EXE  -d 5 -b 34359738368 -c 0.5 -m c -R 1.35 -C 16 -r 1.1 -i 33554320 >> $RES
$EXE  -d 5 -b 34359738368 -c 0.5 -m c -R 1.40 -C 16 -r 1.1 -i 33554320 >> $RES
#$EXE  -d 5 -b 34359738368 -c 0.5 -m c -R 1.45 -C 16 -r 1.1 -i 33554320 >> $RES
$EXE  -d 5 -b 34359738368 -c 0.5 -m c -R 1.50 -C 16 -r 1.1 -i 33554320 >> $RES


$EXE  -d 5 -b 34359738368 -c 0.5 -m s -r 1.10 -i 33554320 >> $RES
#$EXE  -d 5 -b 34359738368 -c 0.5 -m s -r 1.15 -i 33554320 >> $RES
$EXE  -d 5 -b 34359738368 -c 0.5 -m s -r 1.20 -i 33554320 >> $RES
#$EXE  -d 5 -b 34359738368 -c 0.5 -m s -r 1.25 -i 33554320 >> $RES
$EXE  -d 5 -b 34359738368 -c 0.5 -m s -r 1.30 -i 33554320 >> $RES
#$EXE  -d 5 -b 34359738368 -c 0.5 -m s -r 1.35 -i 33554320 >> $RES
$EXE  -d 5 -b 34359738368 -c 0.5 -m s -r 1.40 -i 33554320 >> $RES
#$EXE  -d 5 -b 34359738368 -c 0.5 -m s -r 1.45 -i 33554320 >> $RES
$EXE  -d 5 -b 34359738368 -c 0.5 -m s -r 1.50 -i 33554320 >> $RES

