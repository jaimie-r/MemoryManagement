#!/bin/sh
if [ "$#" -ne 1 ] || ! [ -f "$1" ]; then
  echo "Usage: $0 trace" >&2
  exit 1
fi

trace="$1"
./gprof_performance "$trace"
gprof gprof_performance