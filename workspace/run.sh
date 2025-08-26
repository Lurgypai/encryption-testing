#!/bin/bash

pushd /workspace >> /dev/null

if [[ ! -f $1 ]]; then
    echo "Config \"$1\" not found"
    exit 1
fi
   

. $1

out/test $mode $count

popd >> /dev/null
