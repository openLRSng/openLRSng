#!/bin/bash
PORT="$1"
test -f "$PORT" || PORT="/dev/$PORT"
test -f "$PORT" || PORT="/dev/ttyUSB0"
declare -i PID="$(fuser $PORT 2>/dev/null)"
if [ -e /proc/$PID/fd ] ; then
    for N in /proc/$PID/fd/* ; do
	[ "$PORT" == "$(readlink $N)" ] && FILENO=${N##*/}
    done
    echo monitor file $FILENO of process $PID
    strace -s 512 -xx -e read,write -f -p $PID 2>&1 | fgrep "($FILENO,"

fi
