# Part 3: builds elevator.ko from the files in src/
obj-m += elevator.o
elevator-objs := src/elevator_main.o src/pets.o src/elevator_proc.o src/elevator_thread.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all: ; $(MAKE) -C $(KDIR) M=$(PWD) modules
clean: ; $(MAKE) -C $(KDIR) M=$(PWD) clean

.PHONY: all clean
