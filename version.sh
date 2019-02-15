#!/bin/bash

set -ue

trap 'echo "$0(${LINENO}) ${BASH_COMMAND}"' ERR

[ 1 -eq $# ]



OBJECT_DIR=$1

# repository
GIT_HASH=$(git log --pretty=format:'%h' -n 1)
GIT_BRANCH=$(git branch | awk '/^\*/ { printf $2 }')
GIT_DATETIME=$(git log --date=iso --pretty=format:"%ad" -n 1)
## @todo can't check to unstaging new file.
GIT_STATUS_SHORT=$(git diff --stat | tail -1)

# version
## show version: "yy.mm" (ex. ubuntu)
GIT_TIMESTAMP=$(git log -n1 --format="%at")
SHOW_VERSION=$(date -u -d @${GIT_TIMESTAMP} +0.%y.%m | sed -e 's/^[0-9][0-9]\(.*\)/\1/')
SHOW_VERSION="${SHOW_VERSION}-${GIT_HASH}"
## Major.Miner.Micro-git_hash_short : YYYY.mmddHHMMSS.PatchNumber
MICRO_VERSION=0
MAJOR_MINER_VERSION=$(date -u -d @${GIT_TIMESTAMP} +0.%y.%m-%d%H%M%S)
FULL_VERSION="${MAJOR_MINER_VERSION}.${MICRO_VERSION}-${GIT_HASH}"


#BUILD_DATETIME=$(date '+%Y-%m-%d %H:%M:%S')
#BUILD_DATETIME=$(date --iso-8601=seconds)


VERSION_C=${OBJECT_DIR}/version.c
mkdir -p $(dirname ${VERSION_C})

cat << __EOT__  > ${VERSION_C}

#include "version.h"

const char *GIT_HASH		= "${GIT_HASH}";
const char *GIT_BRANCH		= "${GIT_BRANCH}";
const char *GIT_DATETIME	= "${GIT_DATETIME}";
const char *GIT_STATUS_SHORT	= "${GIT_STATUS_SHORT}";

const char *SHOW_VERSION	= "${SHOW_VERSION}";
const char *FULL_VERSION	= "${FULL_VERSION}";


const char *get_vecterion_build()
{
	return ""
			"git_hash		: ${GIT_HASH}\n"
			"git_branch		: ${GIT_BRANCH}\n"
			"git_datetime		: ${GIT_DATETIME}\n"
			"git_status_short	: ${GIT_STATUS_SHORT}\n"
			"\n"
			"show_version		: ${SHOW_VERSION}\n"
			"full_version		: ${FULL_VERSION}\n"
			"\n"
			;
}


__EOT__



