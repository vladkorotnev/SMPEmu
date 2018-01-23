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
    // shift right 1 bit
    current_byte = current_byte >> 1;
    // set output high if last bit is 1
    if( current_byte & 0x1 > 0x0 ) 
      digitalWrite(DATA_PIN, HIGH);
    else
      digitalWrite(DATA_PIN, LOW);

    do_wait_clock_rise();
      
  }
}

extern const PROGMEM byte cart_image[];

// Gets executed when SELECT is pulled low
void recvCommand() {
  // Read command
  current_command = do_get_byte();
  // Lower 4 bits of opcode should be zero
  if( (current_command & 0x0F) != 0x0 ) return; 
  // Filter invalid commands
  if ( (current_command > 0x20 && current_command < 0x80) || (current_command > 0xE0) ) return; 

  // Now accept the data
  switch(current_command) {
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
          // If i suppose right, we should be giving out data until we have been unselected
          while( digitalRead(SELECT_PIN) ) {
            byte current_byte = pgm_read_byte(cart_image[current_address]);
            do_send_byte(current_byte);
            ++current_address;
          }
      break;

    // TODO: implement more commands!
    
    default: // unknown command
      break;
  }
}

void wake_me_up_inside() {
  // this happens once the device is awake (SELECT is LOW)
   // just go ahead and do what it wants from us
  recvCommand();
  // and then go to sleep again
  sleep_now();
}

void sleep_now() {
    // Configure power saving
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable(); // enable sleep in MCUCR register
  byte interrupt_no = 0;
#if SELECT_PIN == 2
    interrupt_no = 0; // pin 2, interrupt 0
#elif SELECT_PIN == 3
    interrupt_no = 1;
#else
    #error SELECT can only be pin 2 or 3.
#endif
    // SELECT pin acts as a wake up pin, when it goes LOW (see time diagrams at http://www.pisi.com.pl/piotr433/mk90cahe.htm )
  attachInterrupt(interrupt_no, wake_me_up_inside, LOW); 

  sleep_mode();            // here the device is actually put to sleep!!
                             // THE PROGRAM CONTINUES FROM HERE AFTER WAKING UP
 
 sleep_disable();         // first thing after waking from sleep:
                             // disable sleep...
  detachInterrupt(0);      // disables interrupt 0 on pin 2 so the
                             // wake up code will not be executed
                             // during normal running time.
}

void setup() {

  
  // SELECT pin always listens
  pinMode(SELECT_PIN, INPUT);
  // So does the CLOCK, as the calculator is the clock master
  pinMode(CLOCK_PIN, INPUT);
  // DATA pin listen by default
  pinMode(DATA_PIN, INPUT);
}

void loop() {
  // nothing to do here, really
  delayMicroseconds(1000);
}

#include "datafile.h" // this needs to be on the bottom, to avoid code entering the Far address space
