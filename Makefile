.PHONY: debug, clean

test: lyapunov_2D_image.cpp jpeg.cpp mux.cpp
	g++ -O3 -o test lyapunov_2D_image.cpp -ljpeg -lm -lavcodec -lavformat -lswscale -lavutil

debug: gen.cpp
	g++ -g -O0 -std=c++14 -Wall -Wextra -Wpedantic -Werror -o test lyapunov_2D_image.cpp -ljpeg -lm -lavcodec -lavformat -lswscale -lavutil

clean:
	rm -f test
