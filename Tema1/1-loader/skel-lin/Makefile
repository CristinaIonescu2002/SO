CC = gcc
CFLAGS = -fPIC -m32 -Wall
LDFLAGS = -m32

.PHONY: build
build: libso_loader.so

libso_loader.so: loader.o exec_parser.o
	$(CC) $(LDFLAGS) -shared -o $@ $^

exec_parser.o: loader/exec_parser.c loader/exec_parser.h
	$(CC) $(CFLAGS) -o $@ -c $<

loader.o: loader/loader.c
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: clean
clean:
	-rm -f exec_parser.o loader.o libso_loader.so


# CC = gcc
# CFLAGS = -m32 -Wall -fno-pic
# LDFLAGS = -m32 -no-pie
# LDLIBS = -lso_loader

# .PHONY: build
# build: so_exec so_test_prog

# so_exec: exec.o
# 	$(CC) $(LDFLAGS) -L. -Wl,-Ttext-segment=0x20000000 -o $@ $< $(LDLIBS)

# exec.o: exec/exec.c
# 	$(CC) $(CFLAGS) -Iloader -o $@ -c $<

# so_test_prog: test_prog.o
# 	$(CC) $(LDFLAGS) -nostdlib -o $@ $<

# test_prog.o: test_prog/hello.S
# 	$(CC) $(CFLAGS) -o $@ -c $<

# .PHONY: clean
# clean:
# 	-rm -f exec.o so_exec so_test_prog test_prog.o
