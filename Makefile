build:
	gcc -O0 -Wall -Wextra -lX11 src/*.c -o newwm

xephyr:
	export DISPLAY=:0
	Xephyr -br -ac -noreset -screen 800x600 :1 &
	export DISPLAY=:1

run:
	./newwm

clean:
	rm newwm
