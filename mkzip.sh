#!/bin/bash

NAME="daisyff"

if [ -d ./${NAME} ] ; then
	echo "error: ./${NAME} already exist."
	exit
fi
git clone . ./${NAME}

zip -r9 ${NAME}_$(date '+%Y%m%d_%Hh%Mm').zip ./${NAME} > /dev/null

rm -rf ./${NAME}


