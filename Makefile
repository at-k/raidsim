CC	= g++
#CFLAGS	= -Wall -g -c -std=c++11
CFLAGS	= -Wall -O2 -c -std=c++11
LDFLAGS	=
INCLUDES = -I. -I./util -I./ssdsim
LIBS	=
HEADERS =
TARGET	= run_sim
OBJDIR	= ./obj

SOURCES	=	$(filter-out test_main.cpp,  $(wildcard *.cpp)) \
			$(wildcard inilib/*.cpp) \
			$(wildcard util/*.cpp) \
			$(wildcard spc1_gen/*.cpp) \
			$(filter-out ssdsim/test_ssdsim.cpp, $(wildcard ssdsim/*.cpp)) \
			$(wildcard ssdsim/ftl/*.cpp) \
			$(wildcard ssdsim/phy/*.cpp)

OBJS	=	$(addprefix $(OBJDIR)/, $(SOURCES:.cpp=.o))

all:	$(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)  $(LIBS)

clean:
	-rm -f $(TARGET) $(OBJS)

$(OBJDIR)/%.o: %.cpp
	mkdir -p $(dir $@); \
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ -c $<

.PHONY: files
files:
	@echo $(SOURCES) $(OBJS) $(HEADERS) | tr ' ' '\n'
