#include "main.h"

uint8_t leds = 0;

ISR(TIMER0_COMPA_vect){
    // update 74173 (D-FF) inputs each tme there is data ready
    // This 4b register is used as Outputs expander on SHARP N00^GC-1 LCD Board
    // This particular board comes with 4 color leds, and alphanumeric display with HD44780 driver (1x16)
    if ( leds & 0xF0){
        PORTB ^= (1 << PB4 );

        PORTC &= 0xC3;
        PORTC |= (~leds & 0x0F) << 2; //~leds
        leds &= 0x0F;

        PIN_clear(PORTD, PD3);
        PIN_clear(PORTB, PB0);
        PIN_set(PORTB, PB0);
        PIN_clear(PORTB, PB0);

        PIN_set(PORTD, PD3);
        PORTC &= 0xC3;
    }

    process_lcd_FSM();
}

ISR(BADISR_vect){}

void lcd_write(char data){
    PIN_set(RS_PORT, RS_PIN);
    lcd_write_nibble(data >> 4);
    lcd_write_nibble(data);
}

void lcd_addr(uint8_t pos){
    lcd_command(0x80 | pos);
}

static void setup( void ){
    DDRC = 0x3C;
    PORTC = 0x00;
    wait_us(100);
    PORTC = 0x3C;
    wait_us(100);
    PORTC = 0x00;

    DDRB = 0x13;
    PORTB = 0x00;


    DDRD = 0x18;
    PORTD = 0;

    TCCR0A = ( 1 << WGM01 );                     // Timer0 in CTC mode, clk/8 = 6444.8Hz
    TCCR0B =  ( 1 << CS01 );
    OCR0A = 76;//90; // 76 = 6400Hz

    TIMSK0 =  ( 1 << OCIE0A );

    lcd_init();
    sei();
}

void wait_ms( uint16_t ms ){
    while(ms--)
        _delay_ms(1);
}

void wait_us( uint8_t us ){
    while(us--)
        _delay_us(1);
}

int main( void ){
    setup();

    //disp_clear_buffer(DISP_FRONTBUFFER);

    //put_data_to_lcd_buffer("Hello World!", 12, 0, 0, DISP_BUFFER_SIZE, 0);

    lcd_command(0x01);

    wait_ms(2);
    
    // testing DDRAM memory organization
    // for (uint8_t i = 0; i < 16; i++){
    //     wait_ms(10);
    //     if(i == 8)
    //         lcd_addr(0x40);
    //     lcd_write('0' + i);
    // }

    for(;;){
        wait_ms(300);
        if (leds++ > 16)
            leds = 0;
        leds |= 0xF0;
    }
}