build:
	gcc -O0 -Wall -Wextra -lX11 src/*.c -o newwm

xephyr:
	# doesn't work btw
	export DISPLAY=:0
	Xephyr -br -ac -noreset -screen 1290x720 :1 &
	export DISPLAY=:1

run:
	./newwm

clean:
	rm newwm
