# Part 2: builds my_timer.ko from src/my_timer.c
obj-m += my_timer.o
my_timer-objs := src/my_timer.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all: ; $(MAKE) -C $(KDIR) M=$(PWD) modules
clean: ; $(MAKE) -C $(KDIR) M=$(PWD) clean

.PHONY: all clean
