#!/bin/bash

directories=( "piping" "readable-byte-streams" "readable-streams" "transform-streams" "writable-streams" )
files=( "./streams/queuing-strategies.any.js" )

for directory in ${directories[@]}; do
	for file in ./streams/$directory/*; do
		if [ -f $file ]; then
			files+=( $file )
		fi
	done
done

for file in ./encoding/streams/*; do
	if [ -f $file ]; then
		files+=( $file )
	fi
done

for file in ${files[@]}; do
	echo "# $file"
	$MODDABLE/build/bin/mac/debug/xst -m test.js $file
done
