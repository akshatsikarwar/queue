CPPFLAGS+=-DNDEBUG -I. -Icomdb2/net -Ireaderwriterqueue
CFLAGS+=-g -O3
CXXFLAGS+=-g -O3
CXXFLAGS+=-fno-exceptions
LDFLAGS+=-pthread
LINK.o=$(LINK.cc)
run:bench
	time ./bench a
	time ./bench b
bench:
clean:
	rm bench
