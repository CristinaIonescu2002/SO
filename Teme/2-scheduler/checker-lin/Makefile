CC = gcc
CFLAGS = -fPIC -Wall


.PHONY: build
build: libscheduler.so

libscheduler.so: so_scheduler.o
	$(CC) -shared -o $@ $^

so_scheduler.o: ../util/so_scheduler.c
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: clean
clean:
	-rm -f so_scheduler.o libscheduler.so
