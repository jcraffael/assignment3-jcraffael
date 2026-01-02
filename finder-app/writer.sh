#!/bin/bash
#
create_path(){
	local DIR=$1
	local BASE=$2
	if [ ! -e $DIR ]; then
		create_path $(dirname $DIR) $(basename $DIR)
	fi	
	mkdir "$DIR/$BASE"
}


if [[ $# < 2 ]];then
	echo "The file path and the string to write are needed"
      	exit 1
elif [ -d $1 ] || [ -L $1 ]; then
       echo "The path should be to a file"
	exit 1
fi	

writefile=$1
writestr=$2
echo $writestr > $writefile 2>/dev/null 
ret=$(echo $?)
if [[ ret -ne 0 ]]; then
	directory=$(dirname $writefile)
	if [ ! -e $directory ]; then
		create_path $(dirname $directory) $(basename $directory)
	fi
	touch $writefile

	ret2=$(echo $?)
	if [[ ret2 -ne 0 ]]; then
		echo "File creation failed"
		exit 1
	fi
	echo $writestr > $writefile
fi
