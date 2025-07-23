#ifndef __MISC_DEF_H__
#define __MISC_DEF_H__


#define __AVR_ATmega48__
#define F_CPU 8000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/cpufunc.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <stdio.h>
#include <avr/eeprom.h>
#include <stdlib.h>

#define stable_version  1
#define beta_version    0

#define PIN_clear(port, pin)                port &= ~( 1 << pin )
#define PIN_set(port, pin)                  port |= ( 1 << pin )
#define PIN_toggle(port, pin)               port ^= ( 1 << pin )
#define PIN_control(port, pin, state)       port = ( port & ~( 1 << pin ) ) | ( state << pin )

#define PIN_is_high(pinport, pin)           (pinport & ( 1 << pin )) > 0
#define PIN_is_low(pinport, pin)            (pinport & ( 1 << pin )) == 0

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)


#endif // __MISC_DEF_H__