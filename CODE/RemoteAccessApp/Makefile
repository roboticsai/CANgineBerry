MAJORVERSION := 1
MINORVERSION := 20
NAME := ra_app
#OUTDIR := "./Release"
OUTDIR := "./"

BASENAME := $(NAME)
EXECUTABLE := $(BASENAME)

LIBS := dl pthread

CFLAGS := -g -Wall -fPIC -pthread -std=c++11 -Wno-write-strings -DTARGET_LINUX -D_POSIX_C_SOURCE=200112 -D_DEFAULT_SOURCE=1 -D_ISOC99_SOURCE=1 -DPF_CAN=29 -DAF_CAN=PF_CAN -I./dropt -I./Serial -I.
CXXFLAGS:= $(CFLAGS)

RM-F := rm -f
RM-O := find . -path ./.vs -prune -o -name '*.o' -print | xargs rm -f
RM-D := find . -path ./.vs -prune -o -name '*.d' -print | xargs rm -f

#SOURCE := $(wildcard ./*.c)
SOURCE += $(wildcard ./*.cpp)

OBJS := $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCE)))
DEPS := $(patsubst %.o,%.d,$(OBJS))
MISSING_DEPS := $(filter-out $(wildcard $(DEPS)),$(DEPS))
MISSING_DEPS_SOURCES := $(wildcard $(patsubst %.d,%.c,$(MISSING_DEPS)) \
                                   $(patsubst %.d,%.cpp,$(MISSING_DEPS)))
CPPFLAGS += -MD

.PHONY : everything deps objs clean veryclean rebuild

everything : $(EXECUTABLE)

deps : $(DEPS)

objs : $(OBJS)

clean :
	@$(RM-O)
	@$(RM-D)

veryclean: clean
	@$(RM-F) $(EXECUTABLE)

rebuild: veryclean everything

ifneq ($(MISSING_DEPS),)
$(MISSING_DEPS) :
	@$(RM-F) $(patsubst %.d,%.o,$@)
endif

-include $(DEPS)

$(EXECUTABLE) : $(OBJS)
	g++ -o $(OUTDIR)/$(EXECUTABLE) -lstdc++ $(OBJS) $(addprefix -l,$(LIBS))

