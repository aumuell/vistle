#! /bin/bash

while [ -n "$*" ]; do
    f="$1"
    n=$f.new

    echo 'import os' > $n
    echo 'covisedir = os.getenv("COVISEDIR")' >> $n
    echo >> $n

    cat $f | grep -v '^#' | sed -e "s:'${COVISEDIR}:covisedir + ':" >> $n && mv $n $f

    shift
done
