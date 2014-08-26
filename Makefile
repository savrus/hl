CXXFLAGS = -fopenmp -O3 -I./lib
LIB=lib/*.hpp
PROGRAMS= hhl akiba degree lcheck ghl

all: $(PROGRAMS)

$(PROGRAMS):%: %.cpp $(LIB) Makefile
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm $(PROGRAMS)
