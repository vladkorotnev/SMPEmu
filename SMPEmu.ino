// Elektronika SMP emulator
// by Genjitsu Labs, 2018
// version 0.0.1

#include <avr/pgmspace.h>
#include <avr/sleep.h>

#define SELECT_PIN 3 /* NB: can only be pin 2 or 3 */
#define CLOCK_PIN 4
#define DATA_PIN 5

/* Connect as follows:
 *  (see http://www.pisi.com.pl/piotr433/mk90cahe.htm for reference)
 *  
 *  Pin 2 Vcc of calculator into +5V of Arduino (when PC not connected to arduino!)
 *  Pin 3 CLK of calculator into CLOCK_PIN defined above via R=270 Ohm 
 *  Pin 4 DAT of calculator into DATA_PIN  defined above via R=270 Ohm
 *                                                      ^^^^^^^ this helps in case there is a time when the calculator is listening but
 *                                                      arduino is still in input mode, otherwise there is a risk of high current thru
 *                                                      the ICs and it can burn your computer or arduino
 *  Pin 5 SEL of calculator into pin 2 or 3 of Arduino (see SELECT_PIN above!!!)
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

void do_wait_clock_fall() {
  while( digitalRead(CLOCK_PIN) ) {  
      // while clock is high, waiting
      delayMicroseconds(5); // NB: Maybe remove?
    }
}

void do_wait_clock_rise() {
  while( !digitalRead(CLOCK_PIN) ) {  
      // while clock is high, waiting
      delayMicroseconds(5); // NB: Maybe remove?
    }
}

// Read a byte from input
byte do_get_byte() {
  pinMode(DATA_PIN, INPUT);
  byte current_byte = 0x0;
  // Data written to the cartridge change at rising edges of the CLOCK pulses, and are shifted-in on falling edges of the CLOCK pulses beginning with the most significant bit. 
  for(int i=0;i<8;i++) {
    // Reading 8 times

    do_wait_clock_fall();
    
    // shift left 1 bit
    current_byte = current_byte << 1;
    if( digitalRead(DATA_PIN) ) 
      current_byte += 1; // set last bit to 1 if DATA high

    do_wait_clock_rise();
  }
  return current_byte;
}

// Send a byte to data pin
void do_send_byte(byte toSend) {
  pinMode(DATA_PIN, OUTPUT);
  byte current_byte = toSend;
  // Data read from the cartridge change at falling edges of the CLOCK pulses, and are sampled at rising edges of the CLOCK pulses.
  for(int i=0; i<8; i++) {
    // Sending 8 times
    do_wait_clock_fall();
    // set output high if last bit is 1
    if( (current_byte & 0x80) > 0x0 )
      digitalWrite(DATA_PIN, HIGH);
    else
      digitalWrite(DATA_PIN, LOW);

    // shift left 1 bit
    current_byte = current_byte << 1;
    do_wait_clock_rise();
      
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
          while( !digitalRead(SELECT_PIN) ) {
            byte current_byte = pgm_read_byte(cart_image + current_address);
            do_send_byte(current_byte);
            ++current_address;
          }
      break;
      
     case 0xC0: // write postincrement
          if(is_locked) return;
          while( !digitalRead(SELECT_PIN) ) {
           byte current_byte = do_get_byte();
           // TODO: actual write
           ++current_address; 
          }
          
        break;
      
    
    case 0x10: // read postdecrement
          if( is_locked ) return;
          // If i suppose right, we should be giving out data until we have been unselected
          while( !digitalRead(SELECT_PIN) ) {
            byte current_byte = pgm_read_byte(cart_image +current_address);
            do_send_byte(current_byte);
            --current_address;
          }
      break;
      
    case 0xE0: // write postdecrement
        if( is_locked ) return;
    case 0x20: // erase postdecrement
        if( current_address == 0xFFFF || current_command == 0xE0 ) {
             while( !digitalRead(SELECT_PIN) ) {
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
          while( !digitalRead(SELECT_PIN) && current_address <= 0x0007 ) {
            byte current_byte = do_get_byte();
            byte reference_byte = pgm_read_byte(cart_image[current_address]);
            if ( current_byte != reference_byte ) {
              return; // if password won't match, why bother processing further?
            } 
            ++current_address;
          }
          // OK so we've made it up to here without mismatching a single byte in the password
          if ( !digitalRead(SELECT_PIN) ) {
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
  
  // SELECT pin always listens
  pinMode(SELECT_PIN, INPUT);
  // So does the CLOCK, as the calculator is the clock master
  pinMode(CLOCK_PIN, INPUT);
  // DATA pin listen by default
  pinMode(DATA_PIN, INPUT);
}

void loop() {
  recvCommand();
}

#include "pdpboot.h" // this needs to be on the bottom, to avoid code entering the Far address space
