OBJECTS=ft-thread.o ft-server.o composite-flaschen-taschen.o ppm-reader.o pixel-pusher-client.o bjk-pixel-pusher.o
CFLAGS=-Wall -O3 $(INCLUDES) $(DEFINES)
CXXFLAGS=$(CFLAGS)
LDFLAGS+=-lpthread

all : ft-server

ft-server: main.o $(OBJECTS)
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o : %.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f ft-server main.o $(OBJECTS)
