SOURCES = $(wildcard src/*.c)
OBJECTS = $(SOURCES:src/%.c=out/%.o)
DEPENDS = $(OBJECTS:.o=.d)

CC = gcc
CCFLAGS = -Wall -Wextra -g -std=c99 -fsanitize=address
CCLINKS = -lm -lcurl

.PHONY: all run clean

all: out/asciify

clean:
	rm -rf out

out:
	mkdir -p out

out/%.o: src/%.c | out
	$(CC) $(CCFLAGS) -MMD -MP -c $< -o $@

-include $(DEPENDS)

out/asciify: $(OBJECTS)
	$(CC) $(CCFLAGS) $(CCLINKS) $(OBJECTS) -o out/asciify
