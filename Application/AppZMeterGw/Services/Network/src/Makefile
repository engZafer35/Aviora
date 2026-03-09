CC := gcc
AR := ar
CFLAGS := -I../../inc -I../../../inc -Wall -Wextra -O2

# Build artifacts
SRCS := $(filter-out AppNetworkService_Config.c, $(wildcard *.c))
OBJS := $(SRCS:.c=.o)
TARGET := libnetwork.a

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

# Notes:
# - This Makefile intentionally excludes AppNetworkService_Config.c from
#   compilation because the generated file is intended to be included via
#   AppNetworkService_Config.inc. Adjust include paths in CFLAGS as needed.
