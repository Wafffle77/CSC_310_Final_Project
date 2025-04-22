#!/usr/bin/env bash

if [ -z "$1" ]; then
    echo "USAGE: $0 DISK_PATH" >&2
    exit 1
fi

if [ -f "$1" ]; then
    echo "Press ENTER to overwrite '$1'. Press Ctrl+C to abort." >&2
    read;
fi

dd if=/dev/zero of="$1" count=1024 bs=4096