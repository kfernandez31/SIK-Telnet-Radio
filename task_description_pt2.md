Zadanie polega na na rozszerzeniu funkcjonalności nadajnika i odbiornika radia internetowego z zadania 1.

"Zmienne" użyte w treści

- `MCAST_ADDR` - adres rozgłaszania ukierunkowanego, ustawiany obowiązkowym parametrem -a nadajnika

- `DISCOVER_ADDR` - adres używany przez odbiornik do wykrywania aktywnych nadajników, ustawiany parametrem -d odbiornika, domyślnie 255.255.255.255

- `DATA_PORT` - port UDP używany do przesyłania danych, ustawiany parametrem -P nadajnika, domyślnie 20000 + (numer_albumu % 10000)

- `CTRL_PORT` - port UDP używany do transmisji pakietów kontrolnych, ustawiany parametrem -C nadajnika i odbiornika, domyślnie 30000 + (numer_albumu % 10000)

- `UI_PORT` - port TCP, na którym udostępniany jest prosty interfejs tekstowy do przełączania się między stacjami, domyślnie 10000 + (numer_albumu % 10000); ustawiany parametrem -U odbiornika

- `PSIZE` - rozmiar w bajtach pola audio_data paczki, ustawiany parametrem -p nadajnika, domyślnie 512B

- `BSIZE` - rozmiar w bajtach bufora, ustawiany parametrem -b odbiornika, domyślnie 64kB (65536B)

- `FSIZE` - rozmiar w bajtach kolejki FIFO nadajnika, ustawiany parametrem -f nadajnika, domyślnie 128kB

- `RTIME` - czas (w milisekundach) pomiędzy wysłaniem kolejnych raportów o brakujących paczkach (dla odbiorników) oraz czas pomiędzy kolejnymi retransmisjami paczek, ustawiany parametrem -R, domyślnie 250

- `NAME` - nazwa to nazwa nadajnika, ustawiana parametrem -n, domyślnie "Nienazwany Nadajnik"

# Cześć A (nadajnik)

Nadajnik służy do wysyłania strumienia danych otrzymanego na standardowe wejście do odbiorników. Nadajnik powinien otrzymywać na standardowe wejście strumień danych z taką prędkością, z jaką odbiorcy są w stanie dane przetwarzać, a następnie wysyłać te dane zapakowane w datagramy UDP na port `DATA_PORT` na wskazany w linii poleceń adres ukierunkowanego rozgłaszania `MCAST_ADDR`. Dane powinny być przesyłane w paczkach po `PSIZE` bajtów, zgodnie z protokołem opisanym poniżej. Prędkość wysyłania jest taka jak podawania danych na wejście odbiornika, nadajnik nie ingeruje w nią w żaden sposób.

Nadajnik powinien przechowywać w kolejce FIFO ostatnich `FSIZE` bajtów przeczytanych z wejścia tak, żeby mógł ponownie wysłać te paczki, o których retransmisję poprosiły odbiorniki.

Nadajnik cały czas zbiera od odbiorników prośby o retransmisje paczek. Gromadzi je przez czas `RTIME`, następnie wysyła serię retransmisji (podczas ich wysyłania nadal zbiera prośby, do wysłania w kolejnej serii), następnie znów gromadzi je przez czas `RTIME`, itd.

Nadajnik nasłuchuje na UDP na porcie `CTRL_PORT`, przyjmując także pakiety rozgłoszeniowe. Powinien rozpoznawać dwa rodzaje komunikatów:

- **LOOKUP** (prośby o identyfikację): na takie natychmiast odpowiada komunikatem REPLY zgodnie ze specyfikacją protokołu poniżej.

- **REXMIT** (prośby o retransmisję paczek): na takie nie odpowiada bezpośrednio; raz na jakiś czas ponownie wysyła paczki, według opisu powyżej.

Po wysłaniu całej zawartości standardowego wejścia nadajnik się kończy z kodem wyjścia 0. Jeśli rozmiar odczytanych danych nie jest podzielny przez `PSIZE`, ostatnia (niekompletna) paczka jest porzucana; nie jest wysyłana.

## Uruchomienie nadajnika

Na przykład, żeby wysłać ulubioną MP-trójkę w jakości płyty CD, można użyć takiego polecenia:
```
sox -S "05 Muzyczka.mp3" -r 44100 -b 16 -e signed-integer -c 2 -t raw - | pv -q -L \$((44100\*4)) | ./sikradio-sender -a 239.10.11.12 -n "Radio Muzyczka"
```
Pierwsza część polecenia konwertuje plik MP3 na strumień surowych danych (44100 4-bajtowych sampli na każdą sekundę pliku wejściowego), druga część ogranicza prędkość przekazywania danych do nadajnika tak, żeby odbiorniki wyrabiały się z pobieraniem danych.

Żeby wysyłać dane z mikrofonu (w formacie jak powyżej), można użyć takiego polecenia:
```
arecord -t raw -f cd | ./sikradio-sender -a 239.10.11.12 -n "Radio Podcast"
```
Polecenie `arecord` można znaleźć w pakiecie alsa-utils.

# Część B (odbiornik)

Odbiornik odbiera dane wysyłane przez nadajnik i wyprowadza je na standardowe wyjście.

Odbiornik co ok. 5s wysyła na adres `DISCOVER_ADDR` na port `CTRL_PORT` prośbę o identyfikację (komunikat **LOOKUP**). Na podstawie otrzymanych odpowiedzi (komunikatów **REPLY**) tworzy listę dostępnych stacji radiowych. Stacja, od której przez 20 sekund odbiornik nie otrzymał komunikatu **REPLY**, jest usuwana z listy. Jeśli to była stacja aktualnie odtwarzana, rozpoczyna się odtwarzanie innej stacji. Jeśli podano parametr -n, odbiornik rozpoczyna odtwarzanie stacji o zadanej nazwie, gdy tylko ją wykryje. Jeśli nie podano argumentu -n, odbiornik rozpoczyna odtwarzanie pierwszej wykrytej stacji.

Odbiornik posiada bufor o rozmiarze `BSIZE` bajtów, przeznaczony do przechowywania danych z maksymalnie ⌊`BSIZE`/`PSIZE`⌋ kolejnych paczek.

Rozpoczynając odtwarzanie, odbiornik:
- Czyści bufor, w szczególności porzucając dane w nim się znajdujące, a jeszcze nie wyprowadzone na standardowe wyjście.

- Jeśli potrzeba, wypisuje się z poprzedniego adresu grupowego, a zapisuje się na nowy.

- Po otrzymaniu pierwszej paczki audio, zapisuje z niej wartość pola session_id oraz numer pierwszego odebranego bajtu (nazwijmy go BYTE0; patrz specyfikacja protokołu poniżej), oraz rozpoczyna wysyłanie próśb o retransmisję zgodnie z opisem poniżej.

- Aż do momentu odebrania bajtu o numerze BYTE0 + ⌊`BSIZE`*3/4⌋ lub większym, odbiornik nie przekazuje danych na standardowe wyjście. Gdy jednak to nastąpi, przekazuje dane na standardowe wyjście tak szybko, jak tylko standardowe wyjście na to pozwala.

Powyższą procedurę należy zastosować wszędzie tam, gdzie w treści zadania mowa jest o rozpoczynaniu odtwarzania.

Jeśli odbiornik miałby wyprowadzić na standardowe wyjście dane, których jednakże brakuje w buforze, choćby to była tylko jedna paczka, rozpoczyna odtwarzanie od nowa. **UWAGA: to wymaganie nie występowało w zadaniu 1.**

Jeśli odbiornik odbierze nową paczkę, o numerze większym niż dotychczas odebrane, umieszcza ją w buforze i w razie potrzeby rezerwuje miejsce na brakujące paczki, których miejsce jest przed nią. Jeśli do wykonania tego potrzeba usunąć stare dane, które nie zostały jeszcze wyprowadzone na standardowe wyjście, należy to zrobić.

Odbiornik wysyła prośby o retransmisję brakujących paczek. Prośbę o retransmisję paczki o numerze `n` powinien wysłać w momentach `t + k * RTIME`, gdzie `t` oznacza moment odebrania pierwszej paczki o numerze większym niż `n`, dla `k = 1, 2, …` (w nieskończoność, póki dana stacja znajduje się na liście dostępnych stacji). Odbiornik nie wysyła próśb o retransmisję paczek zawierających bajty wcześniejsze niż `BYTE0`, ani tak starych, że i tak nie będzie na nie miejsca w buforze.

Odbiornik oczekuje połączeń TCP na porcie `UI_PORT`. Jeśli użytkownik podłączy się tam np. programem telnet, powinien zobaczyć prosty tekstowy interfejs, w którym za pomocą strzałek góra/dół można zmieniać stacje (bez konieczności wciskania Enter). Oczywiście, jeśli jest kilka połączeń, wszystkie powinny wyświetlać to samo i zmiany w jednym z nich powinny być widoczne w drugim. Podobnie, wyświetlana lista stacji powinna się aktualizować w przypadku wykrycia nowych stacji lub porzucenia już niedostępnych. Powinno to wyglądać dokładnie tak:
```
------------------------------------------------------------------------

 SIK Radio

------------------------------------------------------------------------

PR1

Radio "357"

 > Radio "Disco Pruszkow"

------------------------------------------------------------------------
```
Stacje powinny być posortowane alfabetycznie po nazwie. Przy każdorazowej zmianie stanu (aktywnej stacji, listy dostępnych stacji itp.) listę należy ponownie wyświetlić w całości; w ten sposób będzie można w sposób automatyczny przetestować działanie programów.

## Uruchomienie odbiornika
```
./sikradio-receiver | play -t raw -c 2 -r 44100 -b 16 -e signed-integer --buffer 32768 -
```
Polecenia `play` należy szukać w pakiecie z programem sox.

# Protokół przesyłania danych audio

- Wymiana danych: wymiana danych odbywa się po UDP. Komunikacja jest jednostronna - nadajnik wysyła paczki audio, a odbiornik je odbiera.

- Format datagramów: w datagramach przesyłane są dane binarne, zgodne z poniżej zdefiniowanym formatem komunikatów.

- Porządek bajtów: w komunikatach wszystkie liczby przesyłane są w sieciowej kolejności bajtów (big-endian).

- Paczka audio
```c
struct __attribute__((packed)) audio_packet {
    uint64_t session_id;
    uint64_t first_byte_num;
    char     audio_data[];
}
```
- Pole `session_id` jest stałe przez cały czas uruchomienia nadajnika. Na początku jego działania inicjowane jest datą wyrażoną w sekundach od początku epoki.

- Odbiornik zaś zapamiętuje wartość `session_id` z pierwszej paczki, jaką otrzymał po rozpoczęciu odtwarzania. W przypadku odebrania paczki z:
    - mniejszym `session_id`, ignoruje ją,
    - z większym `session_id`, rozpoczyna odtwarzanie od nowa.

- Bajty odczytywane przez nadajnik ze standardowego wejścia numerowane są od zera. Nadajnik w polu `first_byte_num` umieszcza numer pierwszego spośród bajtów zawartych w audio_data.
- Nadajnik wysyła paczki, w których pole `audio_data` ma dokładnie `PSIZE` bajtów (a `first_byte_num` jest podzielne przez `PSIZE`).

# Protokół kontrolny
- Wymiana danych odbywa się po UDP.

- W datagramach przesyłane są dane tekstowe, zgodne z formatem opisanym poniżej.

- Każdy komunikat to pojedyncza linia tekstu zakończona uniksowym znakiem końca linii. Poza znakiem końca linii dopuszcza się jedynie znaki o numerach od 32 do 127 według kodowania ASCII.

- Poszczególne pola komunikatów oddzielone są pojedynczymi spacjami. Ostatnie pole (np. nazwa stacji w **REPLY**) może zawierać spacje.

- Komunikat **LOOKUP** wygląda następująco:
```
ZERO_SEVEN_COME_IN
```
- Komunikat **REPLY** wygląda następująco:
```
BOREWICZ_HERE [`MCAST_ADDR`] [`DATA_PORT`] [nazwa stacji]
```
Maksymalna długość nazwy stacji wynosi 64 znaki.
- Komunikat **REXMIT** wygląda następująco:
```
LOUDER_PLEASE [lista numerów paczek oddzielonych przecinkami]
```
gdzie numer paczki to wartość jej pola first_byte_num, np.
```
LOUDER_PLEASE 512,1024,1536,5632,3584
```

# Ustalenia dodatkowe

- Programy powinny umożliwiać komunikację przy użyciu IPv4. Obsługa IPv6 nie jest konieczna.

- W implementacji programów duże kolejki komunikatów, zdarzeń itp. powinny być alokowane dynamicznie.

- Programy muszą być odporne na sytuacje błędne, które dają szansę na kontynuowanie działania. Intencja jest taka, że programy powinny móc być uruchomione na stałe bez konieczności ich restartowania, np. w przypadku kłopotów komunikacyjnych, czasowej niedostępności sieci, zwykłych zmian jej konfiguracji itp.

- Programy powinny być napisane zrozumiale. Tu można znaleźć wartościowe wskazówki w tej kwestii: https://www.kernel.org/doc/html/v5.6/process/coding-style.html

- W obydwu aplikacjach opóźnienia w komunikacji z podzbiorem klientów nie mogą wpływać na jakość komunikacji z pozostałymi klientami. Patrz: https://stackoverflow.com/questions/4165174/when-does-a-udp-sendto-block

- Odtwarzany dźwięk musi być płynny, bez częstych lub niewyjaśnionych trzasków.

- Polecam stosować zasadę niezawodności Postela: https://en.wikipedia.org/wiki/Robustness_principle

- Przy przetwarzaniu sieciowych danych binarnych należy używać typów o ustalonej szerokości: http://en.cppreference.com/w/c/types/integer

- W przypadku otrzymania niepoprawnych argumentów linii komend, programy powinny wypisywać stosowny komunikat na standardowe wyjście błędów i zwracać kod 1.

# Oddawanie rozwiązania

Można oddać rozwiązanie tylko części A lub tylko części B, albo obu części.

Rozwiązanie ma:
- działać w środowisku Linux w LK

- być napisane w języku C lub C++ z wykorzystaniem interfejsu gniazd (nie wolno korzystać z `boost::asio`)

- kompilować się za pomocą GCC (polecenie `gcc` lub `g++`) – wśród parametrów należy użyć `-Wall` i `-O2`, można korzystać ze standardów `-std=c2x`, `-std=c++20` (w zakresie wspieranym przez kompilator na maszynie students)

Można korzystać z powszechnie znanych bibliotek pomocniczych (np. `boost::program_options`), o ile są zainstalowane na maszynie students.

Jako rozwiązanie należy dostarczyć pliki źródłowe oraz plik makefile, które należy umieścić na moodle w archiwum `ab123456.tgz` gdzie ab123456 to standardowy login osoby oddającej rozwiązanie, używany na maszynach wydziału, wg schematu: inicjały, nr indeksu. Nie wolno umieszczać tam plików binarnych ani pośrednich powstających podczas kompilacji.

W wyniku wykonania polecenia make dla części A zadania ma powstać plik wykonywalny sikradio-sender, a dla części B zadania – plik wykonywalny sikradio-receiver.

Ponadto makefile powinien obsługiwać cel 'clean', który po wywołaniu kasuje wszystkie pliki powstałe podczas kompilacji.
