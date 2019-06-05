#!/bin/bash

# Change this to your netid
netid=sxg177330

#
# Root directory of your project
PROJDIR=$HOME/ChandyLamport

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

DUMPDIR=$PROJDIR/dump

PIDS=$PROJDIR/pids

#
# Your main project class
#
PROG=ChandyLamport

i=0

cat $CONFIG | sed -e "s/#.*//" | sed -e "/^\s*$/d" |
(
	read n
	n=$( echo $n| awk '{ print $1}' )
	echo $n

	while [ "$i" -ne "$n" ]
	do
		read line 

		host=$( echo $line | awk '{ print $2 }' )

		hosts_list[$i]=$host
		i=$(( i + 1 ))
	done

	i=0

	while [ "$i" -ne "$n" ]
	do
		host=${hosts_list[$i]}
		echo host=$host, i=$i
		
		ssh -o StrictHostkeyChecking=no $netid@$host.utdallas.edu "ps aux|grep $PROG";
        # ./$PROG $i > config_$i.txt&


        i=$(( i + 1 ))
    done

)
