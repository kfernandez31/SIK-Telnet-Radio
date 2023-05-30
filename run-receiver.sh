#!/bin/bash

cd ~/repos/radio-take-two/SIK-1-Radio
cmake --build build
./build/sikradio-receiver

# ./build/sikradio-receiver | play -t raw -c 2 -r 44100 -b 16 -e signed-integer --buffer 32768 -