#!/bin/bash

for file in configs/*; do
    cat $file
    ./run.sh $file
done
