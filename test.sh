i=0
while [ "$i" -le "32" ]
do
	
	echo $i, `cat dumps/dump_0.txt |grep "CCAST"|grep "Snapshot($1)"|grep "node#$i:"|wc -l`

	i=$(( i + 1))

done
