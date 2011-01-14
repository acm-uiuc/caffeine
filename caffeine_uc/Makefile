CC=avr-gcc
DEVICE=atmega644p
CFLAGS=-g -Os -Wall -mcall-prologues -mmcu=$(DEVICE)
OBJ2HEX=avr-objcopy 
AVRDUDE=avrdude
TARGET=caffeine

program : $(TARGET).hex
		$(AVRDUDE) -p m48 -P /dev/ttyS0 -C avrdude.conf erase
		$(AVRDUDE) -p m48 -P /dev/ttyS0 -C avrdude.conf 

%.obj : %.o
		$(CC) $(CFLAGS) $< -o $@

%.hex : %.obj
		$(OBJ2HEX) -R .eeprom -O ihex $< $@

clean :
		rm -f *.hex *.obj *.o

