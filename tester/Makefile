O = api_core.o api_utils.o attribute.o autobuffer.o bus.o cmdparser.o console.o edcl.o elfloader.o main.o memsim.o udp.o
CPPFLAGS=-g

tester: $O
	g++ -o $@ $O -lpthread -ldl

clean:
	rm -f tester $O
