DEVICE_CC = atmega328
DEVICE_DUDE = m328p

PROGRAMMER_DUDE = -Pusb -c dragon_isp -B8.0 -v

AVRDUDE=avrdude
OBJCOPY=avr-objcopy
OBJDUMP=avr-objdump
SIZE=avr-size
CC=avr-gcc
LD=avr-gcc

LDFLAGS=-Wall -mmcu=$(DEVICE_CC)
CPPFLAGS=
CFLAGS=-mmcu=$(DEVICE_CC) -Os -Wall -Wextra -g3 -ggdb -DF_CPU=16000000

MYNAME=avr-hupe


OBJS=$(MYNAME).o serial.o

all : $(MYNAME).hex $(MYNAME).lst

$(MYNAME).bin : $(OBJS)

%.hex : %.bin
	$(OBJCOPY) -j .text -j .data -O ihex $^ $@ || (rm -f $@ ; false )

%.lst : %.bin
	$(OBJDUMP) -S $^ >$@ || (rm -f $@ ; false )
	$(SIZE) $^

%.bin : %.o
	$(LD) $(LDFLAGS) -o $@ $^

ifneq "$(MAKECMDGOALS)" "clean" 
include $(OBJS:.o=.d)
endif

%.d : %.c
	$(CC) -o $@ -MM $^

.PHONY : clean burn
burn : $(MYNAME).hex
	$(AVRDUDE) $(PROGRAMMER_DUDE) -p $(DEVICE_DUDE) -U flash:w:$^
clean :
	rm -f *.bak *~ *.bin *.hex *.lst *.o *.d
