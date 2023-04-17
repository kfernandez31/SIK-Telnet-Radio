sox -S "Muzyczka.mp3" -r 44100 -b 16 -e signed-integer -c 2 -t raw -|\
pv -q -L 176400 |\
./sikradio-sender -a 0
