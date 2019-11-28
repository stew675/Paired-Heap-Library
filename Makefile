all:	phtest pht

phtest:	phtest.c ph.h ph.c
	gcc -O3 -o phtest ph.c phtest.c

pht:	pht.c ph.h ph.c
	gcc -O3 -o pht ph.c pht.c

clean:
	rm -f phtest pht
