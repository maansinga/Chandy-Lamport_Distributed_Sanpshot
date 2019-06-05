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
BINDIR=$PROJDIR/bin
#/bin

#DUMPDIR
DUMPDIR=$PROJDIR/dumps

#
# Your main project class
#
PROG=ChandyLamport

i=0

cat $CONFIG | sed -e "s/#.*//" | sed -e "/^\s*$/d" |
(
	read n
	n=$( echo $n| awk '{ print $1}' )
	# echo $n

	while [ "$i" -ne "$n" ]
	do
		read line 

		i=$( echo $line | awk '{ print $1 }' )
		host=$( echo $line | awk '{ print $2 }' )
		port=$( echo $line | awk '{ print $3 }' )
		# echo $i $host $port

		hosts_list[$i]=$host
		i=$(( i + 1 ))
	done

	i=$n

	while [ "$i" -ge "0" ]
	do
		host=${hosts_list[$i]}
		echo host=$host, i=$i
		# echo "cd $BINDIR; ./$PROG $i > dump_$i.txt &"
        # ssh -o StrictHostkeyChecking=no $netid@$host "stdbuf --output=L $BINDIR/$PROG $i >$DUMPDIR/dump_$i.txt 2>$DUMPDIR/error_$i.txt" &
        ssh -o StrictHostkeyChecking=no $netid@$host "stdbuf --output=L gdb -batch -ex \"run $i\" -ex \"bt\" $BINDIR/$PROG > $DUMPDIR/dump_$i.txt 2>$DUMPDIR/error_$i.txt" &
        #xterm -e "ssh -o StrictHostkeyChecking=no $netid@$host \"stdbuf --output=L gdb -batch -ex \"run $i\" -ex \"bt\" $BINDIR/$PROG > $DUMPDIR/dump_$i.txt 2>$DUMPDIR/error_$i.txt\"" &
        # ./$PROG $i > config_$i.txt&


        i=$(( i - 1 ))
    done

)


