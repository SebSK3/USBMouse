obj-m += mouse.o

PWD := $(CURDIR)

all:
	intercept-build make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules V=1

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
