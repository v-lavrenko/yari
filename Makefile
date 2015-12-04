f64=-D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE 
LIB=-lm -lpthread

opt ?= -g # make opt=-O3 (optimised) or opt=-pg (profile)

CC=gcc -m64 $(opt) -W -Wall -Wno-unused-result -I . -o $@ $(f64) -fopenmp

exe = testmmap testvec testcoll testhash mtx stem plug hl bio

all: $(exe) libyari.a
	etags *.c *.h

clean: 
	rm -rf $(exe) *.o *.dSYM libyari.a TAGS

publish:
	scp mtx www@ir:/data/www/yari/$(OSTYPE)/

%.o: %.c
	$(CC) -c $<

libyari.a: mmap.o vector.o coll.o hash.o matrix.o netutil.o timeutil.o stemmer_porter.o stemmer_krovetz.o textutil.o synq.o svm.o
	ar -r libyari.a $^

%::
	$(CC) $^ $(LIB)

testmmap: testmmap.c mmap.c

testvec: testvec.c mmap.c vector.c

testcoll: testcoll.c mmap.c vector.c coll.c

testhash: testhash.c mmap.c vector.c coll.c hash.c

mtx: mtx.c mmap.c vector.c coll.c hash.c matrix.c svm.c \
	textutil.c stemmer_porter.c stemmer_krovetz.c \
	maxent.c

stem: stem.c stemmer_krovetz.c stemmer_porter.c

plug: plug.c libyari.a

hl: hl.c

bio: bio.c libyari.a
