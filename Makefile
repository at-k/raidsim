CC	= g++
#CFLAGS	= -Wall -g -c -std=c++11
CFLAGS	= -Wall -O2 -c -std=c++11
LDFLAGS	=
INCLUDES = -I. -I./util -I./ssdsim
LIBS	=
HEADERS =
TARGET	= run_sim
OBJDIR	= ./obj

# source file without main
SOURCES	=	$(filter-out test_%,  $(wildcard *.cpp)) \
			$(wildcard inilib/*.cpp) \
			$(wildcard util/*.cpp) \
			$(wildcard spc1_gen/*.cpp) \
			$(filter-out ssdsim/test_ssdsim.cpp, $(wildcard ssdsim/*.cpp)) \
			$(wildcard ssdsim/ftl/*.cpp) \
			$(wildcard ssdsim/phy/*.cpp)

# objs without main
OBJS	=	$(addprefix $(OBJDIR)/, $(SOURCES:.cpp=.o))

# main and target, the format of file name including main func is test*main.cpp
MAINFILES	=	$(wildcard test*main.cpp)
TARGETS 	=	$(patsubst %.cpp, %, $(MAINFILES))
#TARGETS	=	$(patsubst $, run_%, $(MAINS))

all:	$(TARGETS)

clean:
	-rm -f $(TARGET) $(OBJS)

$(OBJDIR)/%.o: %.cpp
	mkdir -p $(dir $@); \
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ -c $<

define PROGRAM_TEMPLATE
  $1 : $(addprefix $$(OBJDIR)/, $1.o) $$(OBJS)
	$$(CC) $$(LDFLAGS) -o $$@ $$(OBJS) $(addprefix $$(OBJDIR)/,$1.o) $$(LIBS)
endef

$(foreach prog, $(TARGETS), $(eval $(call PROGRAM_TEMPLATE, $(prog))))

.PHONY: files
files:
	@echo $(SOURCES) $(OBJS) $(HEADERS) | tr ' ' '\n'

#define TEST
#	MAINFILE	:=	$(patsubst run_%.out, %.o, $1)
#	$(MAINFILE)
#endef
#
#define TEST2
#	@echo $1
#endef
#
#VAR := hoge.cpp hogera.cpp
.PHONY: test
test:
	@echo $(OBJS)
	@echo $(SOURCES)
#	@echo $(patsubst run_%.out, %.o, run_hoge.out)
#	@echo "$(patsubst %.cpp,%.o,$(VAR))"
#	@echo $(foreach prog, $(VAR), $(call TEST, $(prog)))
#	$(call TEST2, hoge)
##	$(foreach prog, $(TARGETS), $(eval $(call TEST2, $(prog))))
