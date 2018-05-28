all: clean
	mkdir build
	gcc main.c -g -Wall -o build/reconstruct -lavformat -lavcodec -lavutil

clean:
	@rm -rf ./build
