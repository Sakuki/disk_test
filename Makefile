TAGET = cdevdemo
KDIR = /lib/modules/$(shell uname -r)/build
PWD = $(shell pwd)
obj-m := $(TAGET).o
default:
	make -C $(KDIR) M=$(PWD) modules
clean:
	$(RM) *.o *.ko *.mod.c Module.symvers