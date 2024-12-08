#!/bin/sh

set -e

DIRS=$(find . -maxdepth 1 -type d -exec test -f {}/CMakeCache.txt \; -exec basename {} \;)
DIRCOUNT=$(echo $DIRS | wc -w)

if [ -z "$DIRS" ]; then
  exit 0
fi
ninja -f /dev/stdin -j ${DIRCOUNT} <<EOF
rule test
  command = cmake --build \$in && ctest --output-on-failure --test-dir \$in
  description = Testing \$in

build all: phony | $(echo $(for i in $DIRS; do echo "test-$i ";done))

$(for i in $DIRS; do echo "build test-$i: test $i";done)
EOF
