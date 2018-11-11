#Makefile to compile i2c client driver
#Compiling our driver as Module, we need kernel headers and compiler path here
#makefile assumes, compiler path added to PATH env variable and
#kernel headers to be present in /lib/modules/<kernel version>/build path.
#If successfully compiled, it generates i2c_client_driver.ko file in src directory.
obj-m += i2c_client_driver.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
