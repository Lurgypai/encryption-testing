#!/bin/bash

rm -r out
mkdir out
pushd out

CC=gcc-14 CXX=g++-14 cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=On
mv compile_commands.json ..

popd
