/*
 * Copyright (c) 2020 Andreas Signer <asigner@gmail.com>
 *
 * This file is part of amiga-ps2-mouse-adapter.
 * 
 * amiga-ps2-mouse-adapter is free software: you can redistribute it 
 * and/or modify it under the terms of the GNU General Public License 
 * as published by the Free Software Foundation, either version 3 of 
 * the License, or (at your option) any later version.
 * 
 * amiga-ps2-mouse-adapter is distributed in the hope that it will be 
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ps2.h"
#include "Arduino.h"

static int clkPin = 2; 
static int dataPin = 3;

const unsigned char oddParityTable[256] = {
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1  
};

static void waitPin(int pin, int val) {
  while(digitalRead(pin) != val);
}

void ps2SetPins(int clk, int data) {
  clkPin = clk;
  dataPin = data;
}

void ps2Send(byte val) {
  #ifdef DEBUG_PS2
  Serial.print("PS/2 send: ");
  Serial.println(val, HEX);
  #endif

  pinMode(clkPin, OUTPUT);  
  pinMode(dataPin, OUTPUT);
 
  byte parityBit = oddParityTable[val];
  
  // From https://web.archive.org/web/20180126072045if_/http://www.computer-engineering.org:80/ps2mouse/

  // 1)   Bring the Clock line low for at least 100 microseconds. 
  digitalWrite(clkPin, LOW);
  delayMicroseconds(150);
  
  // 2)   Bring the Data line low. 
  digitalWrite(dataPin, LOW);

  // 3)   Release the Clock line. 
  digitalWrite(clkPin, HIGH);
  pinMode(clkPin, INPUT_PULLUP);  

  // 4)   Wait for the device to bring the Clock line low. 
  waitPin(clkPin, LOW);

  for (int i = 0; i < 8; i++) {
    // 5)   Set/reset the Data line to send the first data bit 
    digitalWrite(dataPin, val & 1);
    val = val >> 1;

    // 6)   Wait for the device to bring Clock high. 
    waitPin(clkPin, HIGH);
    
    // 7)   Wait for the device to bring Clock low. 
    waitPin(clkPin, LOW);

    // 8)   Repeat steps 5-7 for the other seven data bits and the parity bit     
  }
  digitalWrite(dataPin, parityBit);
  waitPin(clkPin, HIGH);
  waitPin(clkPin, LOW);

  // 9)   Release the Data line. 
  digitalWrite(dataPin, HIGH);
  pinMode(dataPin, INPUT_PULLUP);

  // 10) Wait for the device to bring Data low. 
  waitPin(dataPin, LOW);

  // 11) Wait for the device to bring Clock  low. 
  waitPin(clkPin, LOW);

  // 12) Wait for the device to release Data and Clock
  waitPin(dataPin, HIGH);
  waitPin(clkPin, HIGH);


  #ifdef DEBUG_PS2
  Serial.println("PS/2 send done");
  #endif
}

byte ps2Receive() {  
  byte val = 0;
  byte read = 0;

  waitPin(clkPin, LOW);

  read = digitalRead(dataPin);
  if (read != 0) {
    // Start bit not 0??? Can't happen... What should we do now?
    #ifdef DEBUG_PS2
    Serial.println("Wrong start bit!");
    #endif
  }

  val = 0;
  byte mask = 1;
  for (int i = 0; i < 8; i++) {
    waitPin(clkPin, HIGH);
    waitPin(clkPin, LOW);    
    read = digitalRead(dataPin);
    val |= (read ? mask : 0);
    mask <<= 1;
  }
  waitPin(clkPin, HIGH);
  waitPin(clkPin, LOW);
  byte parity = digitalRead(dataPin);

  if (parity != oddParityTable[val]) {
    // Parity bit wrong? WTF?
    #ifdef DEBUG_PS2
    Serial.println("Wrong parity!");
    #endif
  }

  waitPin(clkPin, HIGH);
  waitPin(clkPin, LOW);
  read = digitalRead(dataPin);
  if (read != 1) {
    // Stop bit not 1??? Can't happen... What should we do now?
    #ifdef DEBUG_PS2
    Serial.println("Wrong stop bit!");
    #endif
  }

  waitPin(clkPin, HIGH);

#ifdef DEBUG_PS2
  Serial.print("PS/2 received: 0x");  
  Serial.println(val, HEX);
#endif

  return val;
}

void ps2SendCommand(byte cmd) {
  ps2Send(cmd);  
  byte read = ps2Receive(); 
  if (read != PS2_ACK) {
    #ifdef DEBUG_PS2
    Serial.print("Mouse didn't ack! Expected: 0x"); Serial.print(PS2_ACK, HEX);
    Serial.print(", received: 0x"); Serial.println(read, HEX);
    #endif
  }
}
