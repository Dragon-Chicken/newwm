build:
	gcc -O2 -Wall -Wextra -lm -lX11 src/*.c -o nwm

xephyr:
	startx ./nwm -- /usr/bin/Xephyr -screen 1000x1000 :1

clean:
	rm nwm
