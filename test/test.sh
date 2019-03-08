#!/bin/bash
# @brief command line arguments test.
#
set -ue
set -x
set -o pipefail

trap 'echo "$0(${LINENO}) ${BASH_COMMAND}"' ERR

# -t(table)
./daisydump.exe DaisyMini.otf -t cmap > /dev/null

# --strict mode
./daisydump.exe "example/DaisyMiniFF_A.ttf" --strict > /dev/null

set +e
./daisydump.exe "test/data/DaisyMini_CmapTableInvalid.otf" --strict > /dev/null
RET=$?
set -e
[ 0 -ne $RET ]

