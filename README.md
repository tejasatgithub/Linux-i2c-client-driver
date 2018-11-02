# Linux-i2c-client-driver
i2c client driver to probe i2c devices and perform read/writes to the device.

The driver code shall be responsible for below functions:
1) Registers itself as i2c client driver.
2) Provides device probe() and removal() routines.
3) Creates a list of interested i2c clients.
4) Registers with sysfs to provide user access to device attributes.
5) Provides exported API for any other kernel module to use.

The code is compiled and tested for 3.16 Linux kernel for x86 platform. Compiling for other Linux versions, shall be a matter of changing the Makefile appropriately.

Usage:

In your project, if you are interested in accessing i2c device and registers, but do not have time to implement the probe, registration and read/write routines, this module could easily be re-used. It needs modification depending on the i2c device and functionalities supported by i2c device.
