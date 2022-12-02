#!/usr/bin/env bash

# Test that included tables do not have metadata

has_metadata() {
    file=$1
    while read -r line; do
        if [ -z "$line" ]; then
            :
        elif [[ "$line" =~ ^# ]]; then
            if [[ "$line" =~ ^#\+ ]]; then
                return 0
            fi
        else
            break
        fi
    done < $file
    return 1
}

ret=0
while read -r table; do
    if has_metadata $table; then
        echo "Table $table has metadata and is also included somewhere" >&2
        ret=1
    fi
done <<< $(
    for table in $(dirname $0)/../tables/*; do
        cat "$table" | grep '^include ' | sed "s|^include  *\([^ ]*\).*$|$(dirname $table)/\1|g"
    done | sort | uniq
)
exit $ret

# TODO: Test that all tables without metadata are included at least once
