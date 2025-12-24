build:
	gcc -O0 -Wall -Wextra -lX11 src/*.c -o newwm

xephyr:
	startx ./newwm -- /usr/bin/Xephyr -screen 1290x720 :1

run:
	./newwm

clean:
	rm newwm
