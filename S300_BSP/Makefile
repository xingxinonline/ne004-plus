## Aggregated Makefile for S300_BSP

.PHONY: all clean examples uart_helloworld

all: examples

examples: uart_helloworld

uart_helloworld:
	$(MAKE) -C Projects/Examples/UART_HelloWorld/GCC all

clean:
	$(MAKE) -C Projects/Examples/UART_HelloWorld/GCC clean
	rm -rf build

