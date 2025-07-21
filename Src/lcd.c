#include "lcd.h"

uint8_t currentCol, currentRow, lcdRowStart [4];

uint8_t disp_buffers_dirty = 0;        // buffer "dirty" bits, one means buffer updated and ready to display
// bits: 7-4: disp_linear_buff[79:159] = 7 - 4th line .. 4 - 1st line, 3-0: disp_linear_buff[0:79] = 3 - 4th line .. 0 - 1st line

uint8_t disp_operation = DISP_STATE_NOP;

uint8_t disp_active_buffer = DISP_FRONTBUFFER;
void *disp_buffer_pointer = NULL;

unsigned char disp_linear_buff [DISP_BUFFER_SIZE];

static void __wait_us( uint16_t us );

unsigned char *get_buffer_address( void ){
    return &disp_linear_buff;
}

void process_lcd_FSM( void ){
    static uint8_t disp_state = DISP_STATE_NOP, disp_last_state = DISP_STATE_NOP;
    static uint8_t disp_delay = 0, disp_temp_data = 0, disp_column_counter = 0;

    if ( disp_state == DISP_STATE_NOP ){
        if ( ( disp_buffers_dirty || disp_column_counter ) && disp_operation == DISP_STATE_NOP ){
            if ( disp_column_counter == 0 ){
                unsigned buff_switch_offset = ( disp_active_buffer == DISP_FRONTBUFFER )? 0 : 4;
                for (unsigned char buff_idx = buff_switch_offset; buff_idx < ( 4 + buff_switch_offset ); buff_idx++ ){
                    if ( ( disp_buffers_dirty >> buff_idx ) & 1 ){
                        disp_buffer_pointer = (unsigned char *)disp_linear_buff + (unsigned char)(buff_idx * 20);
                        disp_column_counter = 20;
                        disp_buffers_dirty &= ~( 1 << buff_idx );
                        disp_state = DISP_STATE_MCURSOR;
                        currentCol = 0;
                        currentRow = buff_idx % 4;
                        disp_temp_data = 0x80 | lcdRowStart[currentRow];
                        break;
                    }
                }
            } else {
                PIN_set(RS_PORT, RS_PIN);
                
                disp_state = DISP_STATE_PUTHDATA;
                disp_temp_data = *((uint8_t *)disp_buffer_pointer++);
                disp_column_counter--;
            }
        } else {
            disp_state = disp_operation;
        }
    }

    if ( disp_state == DISP_STATE_MCURSOR ){
        disp_state = DISP_STATE_PUTHDATA;
        PIN_clear(RS_PORT, RS_PIN);
        disp_temp_data = 0x80 | ( lcdRowStart[currentRow] + currentCol);      // lcd home command
    }

    if ( disp_state == DISP_STATE_HOME ){
        disp_state = DISP_STATE_PUTHDATA;
        currentCol = 0;
        currentRow = 0;
        PIN_clear(RS_PORT, RS_PIN);
        disp_temp_data = 0x02;      // lcd home command
    }

    if ( disp_state == DISP_STATE_CLEAR ){
        disp_state = DISP_STATE_PUTHDATA;
        PIN_clear(RS_PORT, RS_PIN);
        disp_temp_data = 0x01;      // lcd clear command
    }

    if ( disp_state == DISP_STATE_PUTHDATA || disp_state == DISP_STATE_PUTLDATA ){
        disp_last_state = disp_state;

        PIN_clear(D4_PORT, D4_PIN);
        PIN_clear(D5_PORT, D5_PIN);
        PIN_clear(D6_PORT, D6_PIN);
        PIN_clear(D7_PORT, D7_PIN);

        if ( disp_state == DISP_STATE_PUTLDATA )
            disp_temp_data = ( disp_temp_data << 4 ) | ( disp_temp_data >> 4 );

        if ( disp_temp_data & 16 )
            PIN_set(D4_PORT, D4_PIN);
        if ( disp_temp_data & 32 )
            PIN_set(D5_PORT, D5_PIN);
        if ( disp_temp_data & 64 )
            PIN_set(D6_PORT, D6_PIN);
        if ( disp_temp_data & 128 )
            PIN_set(D7_PORT, D7_PIN);

        disp_state = DISP_STATE_WAIT;
        disp_delay = 2;
    }

    if ( disp_state == DISP_STATE_WAIT ){
        if ( --disp_delay == 0 ){
            disp_state = DISP_STATE_ENTOGGLE;
        }
    }

    if ( disp_state == DISP_STATE_ENWAIT ){
        if ( --disp_delay == 0 ){
            if ( disp_last_state == DISP_STATE_PUTHDATA )
                disp_state = DISP_STATE_PUTLDATA;
            else {
                disp_state = DISP_STATE_NOP;
                disp_operation = DISP_STATE_NOP;
            }
        }
    }

    if ( disp_state == DISP_STATE_ENTOGGLE ){
        disp_state = DISP_STATE_ENWAIT;

        PIN_clear(EN_PORT, EN_PIN);
        PIN_set(EN_PORT, EN_PIN);
        PIN_clear(EN_PORT, EN_PIN);

        disp_delay = 2;
    }
}


void put_data_to_lcd_buffer( void * data, uint8_t length, uint8_t row, uint8_t col, uint8_t buffer, uint8_t from_flash){
    unsigned char position = (unsigned char)(buffer + (row * 20) + col), temp;
    for (unsigned char offset = 0; length > 0 ; length--, offset++ ){
        if ( from_flash )
            temp = pgm_read_byte(data + offset);
        else
            temp = *((unsigned char *)data + offset);
        if ( temp < 32)
            break;
        *( disp_linear_buff + position ) = temp;
        position++;
    }
    disp_buffers_dirty |= 1 << ( ( buffer / 20 ) + row );
}

uint8_t disp_swap_buffers( void ){
    if ( disp_active_buffer == DISP_FRONTBUFFER ){
        disp_active_buffer = DISP_BACKBUFFER;
        disp_buffers_dirty = 0xF0;
    } else {
        disp_active_buffer = DISP_FRONTBUFFER;
        disp_buffers_dirty = 0x0F;
    }
    return disp_active_buffer;
}

uint8_t disp_active_buffer_get( void ){
    return disp_active_buffer;
}

void disp_clear_buffer(uint8_t buffer){
    for (unsigned char offset = (unsigned char)buffer; offset < (80 + buffer); offset++)
        *( disp_linear_buff + offset ) = (unsigned char)' ';
    if ( buffer == DISP_FRONTBUFFER )
        disp_buffers_dirty = 0x0F;
    else
        disp_buffers_dirty = 0xF0;
}

void put_one_char(unsigned char character, uint8_t length, uint8_t row, uint8_t col, uint8_t buffer){
    unsigned char temp[20];
    if ( col + length > 20 )
        length -= col;
    for ( uint8_t chr = 0; chr < length; chr++)
        temp[chr] = character;
    put_data_to_lcd_buffer(&temp, length, row, col, buffer, 0);
}


void lcd_command(uint8_t command){
    PIN_clear(RS_PORT, RS_PIN);
    lcd_write_nibble(command >> 4);
    lcd_write_nibble(command);
}

void lcd_write_nibble(uint8_t data){
    PIN_clear(D4_PORT, D4_PIN);
    PIN_clear(D5_PORT, D5_PIN);
    PIN_clear(D6_PORT, D6_PIN);
    PIN_clear(D7_PORT, D7_PIN);

    if ( data & 1 )
        PIN_set(D4_PORT, D4_PIN);
    if ( data & 2 )
        PIN_set(D5_PORT, D5_PIN);
    if ( data & 4 )
        PIN_set(D6_PORT, D6_PIN);
    if ( data & 8 )
        PIN_set(D7_PORT, D7_PIN);

    __wait_us(200);

    PIN_clear(EN_PORT, EN_PIN);
    PIN_set(EN_PORT, EN_PIN);
    PIN_clear(EN_PORT, EN_PIN);

    __wait_us(200);
}

void lcd_init(void){
    for ( uint8_t enable_4b_mode = 0; enable_4b_mode < 3; enable_4b_mode++){
        lcd_write_nibble(0x03);
        __wait_us(5000);
    }
    lcd_write_nibble(0x02);
    __wait_us(1000);
    
    lcd_command(0x28);
    
    lcd_command(0x01);     // lcd clear
    __wait_us(2000);

    lcd_command(0x0C);
    lcd_command(0x06);
    
    lcdRowStart[0] = 0x00;
    lcdRowStart[1] = 0x40;
    lcdRowStart[2] = 20;            // Number of columns
    lcdRowStart[3] = 0x50 + 4;        // plus number of rows
    
    lcd_command(0x02);      // lcd home
    __wait_us(2000);

    PIN_set(RS_PORT, RS_PIN);
    lcd_write_nibble('O' >> 4);
    lcd_write_nibble('O');
    lcd_write_nibble('K' >> 4);
    lcd_write_nibble('K');
    lcd_command(0x02);      // lcd home
    __wait_us(60000);
    __wait_us(60000);
}

static void __wait_us( uint16_t us ){
    while(us--)
        _delay_us(1);
}