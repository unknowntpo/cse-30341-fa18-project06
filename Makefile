# Configuration

CC		= gcc
LD		= gcc
AR		= ar
CFLAGS		= -g -std=gnu99 -Wall -Iinclude -fPIC
LDFLAGS		= -Llib
LIBS		= -lm
ARFLAGS		= rcs

# Variables

SFS_LIB_HDRS	= $(wildcard include/sfs/*.h)
SFS_LIB_SRCS	= $(wildcard src/library/*.c)
SFS_LIB_OBJS	= $(SFS_LIB_SRCS:.c=.o)
SFS_LIBRARY	= lib/libsfs.a

SFS_SHL_SRCS	= $(wildcard src/shell/*.c)
SFS_SHL_OBJS	= $(SFS_SHL_SRCS:.c=.o)
SFS_SHELL	= bin/sfssh

SFS_TEST_SRCS   = $(wildcard src/tests/*.c)
SFS_TEST_OBJS   = $(SFS_TEST_SRCS:.c=.o)
SFS_UNIT_TESTS	= $(patsubst src/tests/%,bin/%,$(patsubst %.c,%,$(wildcard src/tests/unit_*.c)))

# Rules

all:		$(SFS_LIBRARY) $(SFS_UNIT_TESTS) $(SFS_SHELL)

%.o:		%.c $(SFS_LIB_HDRS)
	@echo "Compiling $@"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(SFS_LIBRARY):	$(SFS_LIB_OBJS)
	@echo "Linking   $@"
	@$(AR) $(ARFLAGS) $@ $^

$(SFS_SHELL):	$(SFS_SHL_OBJS) $(SFS_LIBRARY)
	@echo "Linking   $@"
	@$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)

bin/unit_%:	src/tests/unit_%.o $(SFS_LIBRARY)
	@echo "Linking   $@"
	@$(LD) $(LDFLAGS) -o $@ $^

test-unit:	$(SFS_UNIT_TESTS)
	@for test in bin/unit_*; do 		\
	    for i in $$(seq 0 $$($$test 2>&1 | tail -n 1 | awk '{print $$1}')); do \
		echo "Running   $$(basename $$test) $$i";		\
		valgrind --leak-check=full $$test $$i > test.log 2>&1;	\
		grep -q 'ERROR SUMMARY: 0' test.log || cat test.log;	\
		! grep -q 'Assertion' test.log || cat test.log; 	\
	    done				\
	done

test-shell:	$(SFS_SHELL)
	@for test in bin/test_*.sh; do		\
	    $$test;				\
	done

test:	test-unit test-shell

clean:
	@echo "Removing  objects"
	@rm -f $(SFS_LIB_OBJS) $(SFS_SHL_OBJS) $(SFS_TEST_OBJS)

	@echo "Removing  libraries"
	@rm -f $(SFS_LIBRARY)

	@echo "Removing  programs"
	@rm -f $(SFS_SHELL)

	@echo "Removing  tests"
	@rm -f $(SFS_UNIT_TESTS) test.log

.PRECIOUS: %.o
