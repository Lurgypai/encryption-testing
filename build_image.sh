#!/bin/bash

docker image rm testing-encryption

docker build -t testing-encryption .
