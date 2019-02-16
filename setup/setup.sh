#!/bin/bash
#

set -eu
set -o pipefail

trap 'echo "error:$0($LINENO) \"$BASH_COMMAND\" \"$@\""' ERR


# install depend package
#sudo apt-get install gtk3-dev libxml2-dev -y
SCRIPT_DIR=$(cd $(dirname $0); pwd)
ROOT_DIR=${SCRIPT_DIR}/..

# install git hook
cp ${SCRIPT_DIR}/pre-commit ${ROOT_DIR}/.git/hooks/pre-commit
chmod +x ${ROOT_DIR}/.git/hooks/pre-commit

