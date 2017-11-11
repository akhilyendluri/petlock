CC = gcc
AR = ar
CFLAGS = -g
LFLAGS = -lpthread

TGT = driver

OBJS =  driver.o \
	locking.o

build = \
	@if [ -z "$V" ]; then \
		echo '	[$1]	$@'; \
		$2; \
	else \
		echo '$2'; \
		$2; \
	fi

% : %.o
	$(call build,LINK,$(CC) $(CFLAGS) $(OBJS)  -o $@ $(LFLAGS))

%.o : %.c 
	$(call build,CC,$(CC) $(CFLAGS) -c $< -o $@)

%.a : %.o
	$(call build,AR,$(AR) rcs $@ $^)


all: $(OBJS) $(TGT)

locking.o: locking.c locking.h

driver.o: locking.o

clean: 
	rm -f *.o $(OBJS)
