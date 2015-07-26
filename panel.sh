#!/bin/bash

pkill -0 panel || {
	echo "panel is already running" >&2
	exit 1
}

trap 'trap - TERM; kill 0' INT TERM EXIT QUIT

for pipe in /tmp/cwm-*.fifo
do
	clock -s -i 1 -f "clock:%a %H:%M:%S" > "$pipe" &
done

./read_status.pl | ./barpanel | lemonbar -g x16 -p -d -Bblue -u2
