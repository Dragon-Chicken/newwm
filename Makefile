build:
	gcc -O0 -Wall -Wextra -lX11 src/*.c -o newwm

run:
	./newwm

clean:
	rm newwm
