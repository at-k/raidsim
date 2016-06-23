CC	= g++
CFLAGS	= -Wall -g -c -std=c++11
LDFLAGS	=
INCLUDES = -I.
LIBS	=
HEADERS =
TARGET	= run_sim
OBJDIR	= ./obj

SOURCES	=	$(wildcard *.cpp) \
			$(wildcard inilib/*.cpp) \
			$(wildcard spc1_gen/*.cpp)

OBJS	=	$(addprefix $(OBJDIR)/, $(SOURCES:.cpp=.o))

all:	$(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

clean:
	-rm -f $(TARGET) $(OBJS)

$(OBJDIR)/%.o: %.cpp
	mkdir -p $(dir $@); \
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ -c $<

.PHONY: files
files:
	@echo $(SOURCES) $(OBJS) $(HEADERS) | tr ' ' '\n'
