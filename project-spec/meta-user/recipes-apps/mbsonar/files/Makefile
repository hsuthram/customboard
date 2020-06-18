APP = mbsonar

# Add any other object files to this list below
src = $(wildcard src/*.c)\
      $(wildcard common/*.c) \
      $(wildcard LSVC/*.c)

APP_OBJS = $(src:.c=.o)

ROOT = `pwd`

INCLUDE_DIRS = $(ROOT)/src \
	       $(ROOT)/LSVC \
	       $(ROOT)/common 

C_INCLUDE = $(foreach INCLUDE_PATH, $(INCLUDE_DIRS), -I$(INCLUDE_PATH))

all: build

build: $(APP)

LDFLAGS = -pthread -lrt -lm -lslp

CFLAGS = -D__RTXC__ \
	 -D_GNU_SOURCE \
	 $(C_INCLUDE)

$(APP): $(APP_OBJS)
	$(CC) $(CFLAGS) -o $@ $(APP_OBJS) $(LDFLAGS) $(LDLIBS)
clean:
	rm -f $(APP) *.o

