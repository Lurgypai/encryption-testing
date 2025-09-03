#!/bin/bash

if [[ ! -f $1 ]]; then
    echo "Config \"$1\" not found"
    exit 1
fi
   
cat $1

. $1

out/encryption-benchmark $lib $mode $count

rm plainfile.txt
rm cipherfile
