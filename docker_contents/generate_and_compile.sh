#!/bin/bash

./out.sh

pushd out > /dev/null

make -j`nproc`

popd > /dev/null
