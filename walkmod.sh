#!/usr/bin/env bash

set -eo pipefail

[ "$1" = '' ] && {
    echo "Usage: $0 <modlist>" >&2
    exit 1
}

walkdeps()
{
    mod="$1"
    deps="$(modinfo -F depends "$mod" | sed 's/,/ /g')"
    deps="$deps $(modinfo -F softdep "$mod" | sed 's/pre:\|post:\|platform:/ /g')"
    for dep in $deps
    do
        echo "$dep"
        walkdeps "$dep"
    done
}

modlist="$1"

while read -r mod
do
    if [ "$mod" = "" ] || [ "${mod###}" != "${mod}" ]
    then
        continue
    fi
    mod="${mod%% *}"
    echo "$mod"
    walkdeps "$mod"
done < "$modlist" | sort | uniq |
while read -r mod
do
    modinfo -n "$mod"
done | grep -v '(builtin)' | sort | uniq
