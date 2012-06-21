PYVERSION = 2.7
PYBASE = /opt/local/Library/Frameworks/Python.framework/Versions/Current
PYSITE = $(PYBASE)/lib/python$(PYVERSION)/site-packages
PYINCDIR = $(PYBASE)/include/python$(PYVERSION)
PYINCLUDE = -I$(PYINCDIR)

CXX = g++
CXXFLAGS = -fPIC -fno-common -g -O3 $(INCLUDE)
INCLUDE = $(PYINCLUDE)
LDFLAGS = -lboost_python -lcityhash

SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:.cpp=.o)
SOBJS = $(SRCS:.cpp=.so)

.PHONY: all clean check

all: $(SOBJS)

check: $(SOBJS)
	nosetests

clean:
	@rm -fv $(OBJS) $(SOBJS)

%.so: %.o
	$(CXX) -bundle -undefined dynamic_lookup -o $@ $+ $(LDFLAGS)

