#!/bin/sh
set -eu

if ping -c 1 -W 1 8.8.8.8 >/dev/null 2>&1; then
    printf '\n\n'
    printf '%s\n' '============================================================'
    printf '%s\n' '!!! EXTERNAL CONNECTIONS ALLOWED !!!'
    printf '%s\n' 'Container has reached 8.8.8.8 and will shut down.'
    printf '%s\n' '============================================================'
    printf '\n'
    exit 1
fi

printf '%s\n' 'Network check passed: 8.8.8.8 is unreachable; starting application.'
exec python app.py
