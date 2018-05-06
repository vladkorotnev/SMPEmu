// Elektronika SMP emulator
// by Genjitsu Labs, 2018
// version 0.0.2

#include <avr/pgmspace.h>
#include <avr/sleep.h>

/* Connect as follows:
 *  (see http://www.pisi.com.pl/piotr433/mk90cahe.htm for reference)
 *  
 *  Pin 2 Vcc of calculator into +5V of Arduino (when PC not connected to arduino!)
 *  Pin 3 CLK of calculator into pin 4 via R=220 Ohm 
 *  Pin 4 DAT of calculator into pin 5 via R=270 Ohm
 *                                         ^^^^^^^ this helps in case there is a time when the calculator is listening but
 *                                         arduino is still in input mode, otherwise there is a risk of high current thru
 *                                         the ICs and it can burn your computer or arduino
 *  Pin 5 SEL of calculator into pin 3 of Arduino 
 *  Pin 6 GND of calculator into GND of Arduino
 */

word current_address = 0x0; // 1-word address register of the SMP
boolean is_locked = false; // when the cartridge is locked, r/w instructions are ignored

byte current_command = 0x0; 
/* Allowed commands:
 *  0x00  Read status
 *  0x10  Read postdecrement (unused in MK90)
 *  0x20  Erase postdecrement (used in INIT of MK90)
 *  0x80  Lock
 *  0x90  Unlock
 *  0xA0  Write address
 *  0xB0  Read address (unused in MK90)
 *  0xC0  Write postincrement
 *  0xD0  Read postincrement
 *  0xE0  Write postdecrement (unused in MK90)
 */

#define WAIT_CLOCK_FALL  while( (PIND & B00010000) != 0x0 )
#define WAIT_CLOCK_RISE  while( (PIND & B00010000) == 0x0 )
#define IS_SELECTED     ( (PIND & B00001000) == 0x0 )
#define IS_NOT_SELECTED ( (PIND & B00001000) != 0x0 )

// Read a byte from input
byte do_get_byte() {
  // Switch DATA pin 5 to INPUT
  DDRD = B11000111;
  
  byte current_byte = 0x0;
  // Data written to the cartridge change at rising edges of the CLOCK pulses, and are shifted-in on falling edges of the CLOCK pulses beginning with the most significant bit. 
  for(byte i=8; i!=0; --i) {
    // Reading 8 times

    WAIT_CLOCK_FALL;
    
    // shift left 1 bit
    current_byte <<= 1;
    if( (PIND & B00100000) > 0x0 ) 
      current_byte += 1; // set last bit to 1 if DATA high

    WAIT_CLOCK_RISE;
  }
  return current_byte;
}

// Send a byte to data pin
void do_send_byte(byte toSend) {
  // Switch DATA pin 5 to OUTPUT
  DDRD = B11100111;
  
  byte current_byte = toSend;
  // Data read from the cartridge change at falling edges of the CLOCK pulses, and are sampled at rising edges of the CLOCK pulses.
  for(byte i=8; i!=0; --i) {
    // Sending 8 times
    WAIT_CLOCK_FALL;
    
    // set output high if last bit is 1
    if( (current_byte & 0x80) > 0x0 )
      PORTD |= B00100000;
    else
      PORTD &= B11011111; 

    // shift left 1 bit
    current_byte <<= 1;
    WAIT_CLOCK_RISE;
      
  }
}

extern const unsigned char cart_image[];

// Gets executed when SELECT is pulled low
void recvCommand() {
  // Read command
  current_command = do_get_byte();
  // Now accept the data
  switch(current_command) {
    
    case 0x00: // query status
      // TODO: Implement password errors counter?
      do_send_byte( is_locked ? 0x01 : 0x00 );
      break;
    
    case 0xA0: // set address
        {
          byte hi = do_get_byte(); 
         byte lo = do_get_byte();
         current_address = (hi << 8) + lo;
        }
      break;

    case 0xB0: // read address
        {
          byte hi = (byte) (current_address >> 8) & 0xFF;
          byte lo = (byte) current_address & 0xFF;
          do_send_byte(hi);
          do_send_byte(lo);
        }
      break;

    case 0xD0: // read postincrement
          if( is_locked ) return;
          // If i suppose right, we should be giving out data until we have been unselected
          while( IS_SELECTED ) {
            byte current_byte = pgm_read_byte(cart_image + current_address);
            do_send_byte(current_byte);
            ++current_address;
          }
     break;
      
     case 0xC0: // write postincrement
          if(is_locked) return;
          while( IS_SELECTED ) {
           byte current_byte = do_get_byte();
           // TODO: actual write
           ++current_address; 
          }
          
        break;
      
    
    case 0x10: // read postdecrement
          if( is_locked ) return;
          // If i suppose right, we should be giving out data until we have been unselected
          while( IS_SELECTED ) {
            byte current_byte = pgm_read_byte(cart_image +current_address);
            do_send_byte(current_byte);
            --current_address;
          }
      break;
      
    case 0xE0: // write postdecrement
        if( is_locked ) return;
    case 0x20: // erase postdecrement
        if( current_address == 0xFFFF || current_command == 0xE0 ) {
             while( IS_SELECTED ) {
                 byte current_byte = do_get_byte();
                 // TODO: actual write
                 --current_address; 
              }
        }
        // the 0x20 command is ignored if address is not FFFFh, tho.
        break;
        
    case 0x80: // Lock
        // Well you asked for it
        is_locked = true;
        break;
        
    case 0x90: // Unlock
        if( ! is_locked ) return; // Ignore attempts to unlock if not locked
        if( current_address == 0x0000 ) {
          // Unlocking basically takes 7 bytes of data input, and if they match bytes at 0000h...0007h
          while( IS_SELECTED && current_address <= 0x0007 ) {
            byte current_byte = do_get_byte();
            byte reference_byte = pgm_read_byte(cart_image[current_address]);
            if ( current_byte != reference_byte ) {
              return; // if password won't match, why bother processing further?
            } 
            ++current_address;
          }
          // OK so we've made it up to here without mismatching a single byte in the password
          if ( IS_SELECTED ) {
            // Let's not unlock if password entry was just canceled right :p
            is_locked = false; 
          }
        }
        break;
    
    // ---------------------------------------
    // Genjitsu SMP Commandset Extensions 0.2
    // ---------------------------------------    
    case 0xF0: // List files
      {
        // TODO: build a string of file names and send them
         do_send_byte('T');
         do_send_byte('O');
         do_send_byte('D');
         do_send_byte('O');
         do_send_byte(0x0);
         do_send_byte(0xFF); 
      }
      break;
      
    case 0xF1: // Mount image
      {
        // TODO: receive file name 
      }
      break;
    
    case 0xF2:
     {
     
     }
     break;  
     
    

    
    default: // unknown command
      break;
  }
}




void setup() {
  pinMode(13, OUTPUT);
  
  // SELECT pin 3 always listens
  // So does the CLOCK 4, as the calculator is the clock master
  // DATA pin 5 listen by default
  PORTD = B11111111;
  DDRD = B11000111;
  
  // remove interrupts
  cli();
  
}

void loop() {
  recvCommand();
}

#include "pdpboot.h" // this needs to be on the bottom, to avoid code entering the Far address space
