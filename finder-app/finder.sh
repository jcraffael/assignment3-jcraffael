#!/bin/sh
#

if [ $# -lt 2 ]; then
	echo "The searching path and the searching string are needed"
      	exit 1
elif [ ! -d "$1" ]; then
       echo "The searching path is not existing"
	exit 1
fi	

NUM_FILES=$(find "$1" -type f | wc -l)
NUM_MATCH_LINES=$(grep -rnw "$1" -e "$2" | wc -l)

echo "The number of files are $NUM_FILES and the number of matching lines are $NUM_MATCH_LINES"
