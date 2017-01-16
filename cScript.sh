#!/bin/bash

# for loop

for (( i=0;i<150;i++ ))
do
	echo doom$i
	./client Doom$(i) cs itchy.cs.umu.se 10057 &
	sleep .5
done	

