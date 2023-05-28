#!/bin/bash

sox -S "samples/boogie_wonderland.mp3" -r 44100 -b 16 -e signed-integer -c 2 -t raw - | \
pv -q -L 176400 | ./sikradio-sender \
-a 239.10.11.12 -n "Boogie Wonderland"

# sox -S "samples/magnolia.mp3" -r 44100 -b 16 -e signed-integer -c 2 -t raw - | \
# pv -q -L 176400 | ./sikradio-sender \
# -a 239.10.11.12 -n "Magnolia"


# sox -S "samples/latitude.mp3" -r 44100 -b 16 -e signed-integer -c 2 -t raw - | \
# pv -q -L 176400 | ./sikradio-sender \
# -a 239.10.11.12 -n "Latitude"

# sox -S "samples/here_comes_the_sun.mp3" -r 44100 -b 16 -e signed-integer -c 2 -t raw - | \
# pv -q -L 176400 | ./sikradio-sender \
# -a 239.10.11.12 -n "Here Comes The Sun"
