#!/bin/bash

rm -r out
mkdir out
pushd out >> /dev/null

CC=gcc-14 CXX=g++-14 cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=On -DCMAKE_BUILD_TYPE=Debug
mv compile_commands.json ..

popd >> /dev/null
