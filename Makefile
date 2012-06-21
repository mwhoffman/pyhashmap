PYVERSION = 2.7
PYBASE = /opt/local/Library/Frameworks/Python.framework/Versions/Current
PYSITE = $(PYBASE)/lib/python$(PYVERSION)/site-packages
PYINCDIR = $(PYBASE)/include/python$(PYVERSION)
PYINCLUDE = -I$(PYINCDIR)

CXX = clang++
CXXFLAGS = -fPIC -fno-common $(INCLUDE)
INCLUDE = $(PYINCLUDE)
LDFLAGS = -lboost_python

SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:.cpp=.o)
SOBJS = $(SRCS:.cpp=.so)

.PHONY: all clean

all: $(SOBJS)

clean:
	@rm -fv $(OBJS) $(SOBJS)

%.so: %.o
	$(CXX) -bundle -undefined dynamic_lookup -o $@ $+ $(LDFLAGS)

