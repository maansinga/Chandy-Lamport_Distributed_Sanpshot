#!/bin/bash

# Change this to your netid
netid=sxg177330

#
# Root directory of your project
PROJDIR=$HOME/DistributedSystems

#
# This assumes your config file is named "config.txt"
# and is located in your project directory
#
CONFIG=$PROJDIR/config.txt

#
# Directory your java classes are in
#
BINDIR=$PROJDIR
#/bin

DUMPDIR=$PROJDIR/dumps

PIDS=$PROJDIR/pids

#
# Your main project class
#
PROG=ChandyLamport

i=1

while [ "$i" -ne "50" ]
do
	printf -v host "dc%02d" $i
	echo $host
	
	ssh -o StrictHostkeyChecking=no $netid@$host.utdallas.edu "pkill -f $PROG"
    # ./$PROG $i > config_$i.txt&


    i=$(( i + 1 ))
done