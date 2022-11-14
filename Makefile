

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
warn=-W -Wall -Wno-unused-result -Wno-implicit-fallthrough -Wno-null-pointer-arithmetic

#CC=gcc -m64 $(opt) $(warn) -I . -o $@ $(f64) -fopenmp
CC=gcc -m64 $(opt) $(warn) -I . -o $@ $(f64) 

exe = testmmap testvec testcoll dict mtx stem kvs hl bio pdb shard pval ts ptail xcut xtime spell query nutil hash2 

all: libyari.a $(exe)
	etags *.c *.h

clean: 
	rm -rf $(exe) *.o *.dSYM libyari.a TAGS

publish:
	scp mtx www@ir:/data/www/yari/$(OSTYPE)/

%.o: %.c
	$(CC) -c $<

libyari.a: mmap.o vector.o coll.o hash.o matrix.o netutil.o timeutil.o stemmer_krovetz.o textutil.o synq.o svm.o spell.o query.o
	ar -r libyari.a $^

%::
	$(CC) $^ $(LIB)

testmmap: testmmap.c mmap.c 

testvec: testvec.c mmap.c vector.c

testcoll: testcoll.c mmap.c vector.c coll.c

dict: dict.c mmap.c vector.c coll.c hash.c timeutil.c 

mtx: mtx.c mmap.c vector.c coll.c hash.c matrix.c svm.c \
	textutil.c stemmer_krovetz.c maxent.c synq.c timeutil.c

stem: stem.c stemmer_krovetz.c synq.c mmap.c

kvs: kvs.c libyari.a

hl: hl.c

ptail: ptail.c

xcut: xcut.c libyari.a

xtime: xtime.c libyari.a

bio: bio.c libyari.a

pdb: pdb.c libyari.a

shard: shard.c libyari.a

pval: pval.c libyari.a

ts: ts.c libyari.a

spell: spell.c libyari.a
	$(CC) -DMAIN $^ $(LIB)

query: query.c libyari.a
	$(CC) -DMAIN $^ $(LIB)

nutil: netutil.c libyari.a
	$(CC) -DMAIN $^ $(LIB)

compress: compress.c libyari.a libzstd.a
	$(CC) -DMAIN $^ $(LIB)

hash2: hash2.c hashf.c libyari.a
	$(CC) -DMAIN $^ $(LIB)

bpe: bpe.c libyari.a
