

  # Copyright (c) 1997-2016 Victor Lavrenko (v.lavrenko@gmail.com)

  # This file is part of YARI.

  # YARI is free software: you can redistribute it and/or modify it
  # under the terms of the GNU General Public License as published by
  # the Free Software Foundation, either version 3 of the License, or
  # (at your option) any later version.

  # YARI is distributed in the hope that it will be useful, but WITHOUT
  # ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  # or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
  # License for more details.

  # You should have received a copy of the GNU General Public License
  # along with YARI. If not, see <http://www.gnu.org/licenses/>.


f64=-D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE 
LIB=-lm -lpthread

opt ?= -g # make opt=-O3 (optimised) or opt=-pg (profile)

CC=gcc -m64 $(opt) -W -Wall -Wno-unused-result -I . -o $@ $(f64) -fopenmp

exe = testmmap testvec testcoll dict mtx stem kvs hl bio shard

all: $(exe) libyari.a
	etags *.c *.h

clean: 
	rm -rf $(exe) *.o *.dSYM libyari.a TAGS

publish:
	scp mtx www@ir:/data/www/yari/$(OSTYPE)/

%.o: %.c
	$(CC) -c $<

libyari.a: mmap.o vector.o coll.o hash.o matrix.o netutil.o timeutil.o stemmer_krovetz.o textutil.o synq.o svm.o
	ar -r libyari.a $^

%::
	$(CC) $^ $(LIB)

testmmap: testmmap.c mmap.c

testvec: testvec.c mmap.c vector.c

testcoll: testcoll.c mmap.c vector.c coll.c

dict: dict.c mmap.c vector.c coll.c hash.c

mtx: mtx.c mmap.c vector.c coll.c hash.c matrix.c svm.c \
	textutil.c stemmer_krovetz.c maxent.c synq.c

stem: stem.c stemmer_krovetz.c synq.c mmap.c

kvs: kvs.c libyari.a

hl: hl.c

bio: bio.c libyari.a

shard: shard.c libyari.a
