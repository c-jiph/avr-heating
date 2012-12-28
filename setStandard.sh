#!/bin/bash


./ctrl.rb clearAllEntries


# Turn off the heating everyday at 2.30am
y=0
for x in mon tue wed thu fri sat sun ; do
	./ctrl.rb setEntry $y $x 2 30 off
	y=$[$y + 1]
done


# Turn on the heating every weekday at 7.15am

#y=12
#for x in mon tue wed thu fri ; do
#	./ctrl.rb setEntry $y $x 7 15 on
#	y=$[$y + 1]
#done


# Turn off the heating every weekday at 9.45am

#y=7
#for x in mon tue wed thu fri ; do
#	./ctrl.rb setEntry $y $x 9 45 off
#	y=$[$y + 1]
#done


./ctrl.rb dumpEntries

