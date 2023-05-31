#!/bin/bash

cd ~/repos/radio-take-two/SIK-1-Radio
cmake --build build

# sox -S "./samples/boogie_wonderland.mp3" -r 44100 -b 16 -e signed-integer -c 2 -t raw - | \
# pv -q -L 176400 | ./build/sikradio-sender \
# -a 239.10.11.12 -n "Boogie Wonderland"

# sox -S "samples/magnolia.wav" -r 44100 -b 16 -e signed-integer -c 2 -t raw - | \
# pv -q -L 176400 | ./build/sikradio-sender \
# -a 239.10.11.11 -n "Magnolia" -P 29627

sox -S "samples/latitude.mp3" -r 44100 -b 16 -e signed-integer -c 2 -t raw - | \
pv -q -L 176400 | ./build/sikradio-sender \
-a 239.10.11.12 -n "Latitude"

# sox -S "samples/here_comes_the_sun.mp3" -r 44100 -b 16 -e signed-integer -c 2 -t raw - | \
# pv -q -L 176400 | ./build/sikradio-sender \
# -a 239.10.11.12 -n "Here Comes The Sun"
