CXX=g++
CXXFLAGS=-fPIC -std=c++11 -Wall -pedantic -I/usr/include/python3.4m
LDFLAGS=-lpython3.4m -lboost_python3 -lboost_system
OBJ=scan/native/capture.o

all: scan/native/capture.so

scan/native/capture.so: $(OBJ)
	$(CXX) $(CXXFLAGS) -shared -o scan/native/capture.so $(OBJ) $(LDFLAGS)

%.o: %.cc
	$(CXX) $(CXXFLAGS) -o $@ -c $<

clean:
	rm -f scan/native/capture.so $(OBJ)

