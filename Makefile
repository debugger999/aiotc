all:
	make libs
	make app
	make test

app:
	make -C src
libs:
	make -C work
test:
	make -C test

clean:
	cd src;make clean
	cd work;make clean
	cd test;make clean

