# accelerometer
A program for STM32 DISCOVERY that reads data from the built-in accelerometer and displays acceleration along 3 axes in real time using Python

# microcontroller
The stm32f407vg tx microcontroller accesses the LIS302Dl accelerometer built into the Discovery board via the SPI interface.
It is configured as standard for +-2g. CRC-8 is also calculated on the controller. 
Everything is transmitted via USB in packets of 10 bytes, in which the first two bytes are the starting ones, then 6 coordinates, a transmission counter and CRC

# interface on the computer
Everything is written in python using the serial library and matplotlib. charts are updated in real time. 
There is also a CRC-8 check and calculation of the average acceleration value along 3 axes and sigma
