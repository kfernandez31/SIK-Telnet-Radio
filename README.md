# What is this?

This is the repository of my project of the [Computer Networks (pol. Sieci Komputerowe, SIK)](https://usosweb.mimuw.edu.pl/kontroler.php?_action=katalog2%2Fprzedmioty%2FpokazPrzedmiot&prz_kod=1000-214bSIK&lang=en) course offered by the Faculty of Mathematics, Informatics and Mechanics at the University of Warsaw (further referred to as "MIM UW") in the 2022/2023 summer semester.

# Brief description
The task was to write an audio streaming service, a "radio". The service consisted of a transmitter and a receiver. The transmitter corresponded to one "radio station" and send audio data through UDP to the receiver. The receiver could listen audio from multiple radio stations and the currently playing one could be toggled through a TCP/Telnet connection, of which there could be many.

My solution is intensely multi-threaded, on both the transmitter and the receiver ends. In order to handle multiple TCP/Telnet connections, the receiver uses `poll`. In order for threads to communicate with each other, they do so through pipes. Additionally, the solution has complete signal handling and a proper cleanup (graceful shutdown).

# Full description
The task was divided into two iterations. The iterations' descriptions are available in Polish here:
- [part 1](https://github.com/kfernandez31/SIK-Telnet-Radio/blob/main/task_description_pt1.md)
- [part 2](https://github.com/kfernandez31/SIK-Telnet-Radio/blob/main/task_description_pt2.md)

---
Copyright of the task's description and resources: MIM UW.
