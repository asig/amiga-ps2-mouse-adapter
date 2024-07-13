/*
 * Convert PS/2 mouse movements to Amiga mouse movements.
 * 
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

#include "config.h"
#include "ps2.h"

#define LED 13

// Input pins
#define P_PS2_CLK  3
#define P_PS2_DATA 2

// Output pins              // Cable color to D-sub 9
#define P_AMIGA_V_PULSE  12 // Purple
#define P_AMIGA_H_PULSE  11 // Brown
#define P_AMIGA_VQ_PULSE 10 // Blue
#define P_AMIGA_HQ_PULSE  9 // White
#define P_AMIGA_LMB       8 // Yellow
#define P_AMIGA_RMB       7 // Green
#define P_AMIGA_MMB       6 // Orange

static inline int sgn(int val) {
  return (val < 0) ? -1 : ((val > 0) ? 1 : 0);
}

/*
 * From "Amiga Intern", page 258:
 * 
 * Right/Down: H/V:      ______        _____
 *                      |      |      |
 *                      |      |      |
 *                     _|      |______|
 * 
 *             HQ/VQ:        ______        _
 *                          |      |      |
 *                          |      |      |
 *                     _____|      |______|
 *         
 *         
 * Left/Up:    H/V:          ______        _
 *                          |      |      |
 *                          |      |      |
 *                     _____|      |______|
 * 
 *             HQ/VQ:    ______        _____
 *                      |      |      |
 *                      |      |      |
 *                     _|      |______|
 *         
 */   

// pulse pattern for moving right/down.
int p[4]  = { 1, 1, 0, 0 };
int pq[4] = { 0, 1, 1, 0 };

// position in pattern for x and y movement
int posX = 0;
int posY = 0;

void setupMouse() {
  ps2SendCommand(PS2_CMD_RESET);
  ps2Receive(); // read and ignore status response
  ps2Receive(); // read and ignore device ID

  // Amiga generates 200 impulses/inch, which is  
  // 7.87 impulses/mm, so let's go with a resolution
  // of 8 count per mm.
  ps2SendCommand(PS2_CMD_SET_RESOLUTION);
  ps2SendCommand(PS2_RES_8_CNT_PER_MM);

  // Switch to polling.
  ps2SendCommand(PS2_CMD_SET_REMOTE_MODE);
}

void blink() {
  for (int i = 0; i < 2; i++) {
    delay(200);
    digitalWrite(LED, HIGH);
    delay(200);
    digitalWrite(LED, LOW);    
  }
}

void setup() {
  ps2SetPins(P_PS2_CLK, P_PS2_DATA);

#if defined(DEBUG) || defined(DEBUG_PS2)
  Serial.begin(115200);
#endif  

  // Blink a little to let people know that we entered setup()
  pinMode(LED, OUTPUT);
  blink();

  // Set up pin modes
  pinMode(P_AMIGA_V_PULSE,  OUTPUT);
  pinMode(P_AMIGA_H_PULSE,  OUTPUT);
  pinMode(P_AMIGA_VQ_PULSE, OUTPUT);
  pinMode(P_AMIGA_HQ_PULSE, OUTPUT);
  pinMode(P_AMIGA_LMB,      OUTPUT);
  pinMode(P_AMIGA_RMB,      OUTPUT);
  pinMode(P_AMIGA_MMB,      OUTPUT);

  setupMouse();  

  // Blink some more to signal that setup is done.
  blink();
}

int led = 0;
void loop() {
  ps2SendCommand(PS2_CMD_READ_DATA);
  byte b1 = ps2Receive();
  short dx = ps2Receive();
  short dy = ps2Receive();

  if ((b1 &  PS2_MASK_Y_SIGN) > 0) {
    dy |= 0xff00;
  } 
  if ((b1 &  PS2_MASK_X_SIGN) > 0) {
    dx |= 0xff00;
  }
  dy = -dy;

  bool lmb = (b1 & PS2_MASK_LMB) > 0;
  bool rmb = (b1 & PS2_MASK_RMB) > 0;
  bool mmb = (b1 & PS2_MASK_MMB) > 0;
  
  digitalWrite(P_AMIGA_LMB, lmb ? 0 : 1);
  digitalWrite(P_AMIGA_MMB, mmb ? 0 : 1);
  digitalWrite(P_AMIGA_RMB, rmb ? 0 : 1);

  // compute direction and steps, both horizontally and vertically.
  // For each pixel, we need to send 4 signals, therefore we multiple
  // the distances by 4.
  int dirX = sgn(dx);
  int stepsX = 4*abs(dx);
  int dirY = sgn(dy);
  int stepsY = 4*abs(dy);

  // We need to send stepsX horizontal and stepsY vertical signals
  // as quickly as we can. 
  int steps = max(stepsX, stepsY);
  while (steps-- > 0) {
      // Show some activity...
      if (steps % 4 == 0) {
        digitalWrite(LED, led);
        led = 1 - led;
      }
      
      if (stepsY > 0) {
        digitalWrite(P_AMIGA_V_PULSE, p[posY]);
        digitalWrite(P_AMIGA_VQ_PULSE, pq[posY]);
        posY = (posY + dirY + 4) % 4;
        stepsY--;
      }

      if (stepsX > 0) {
        digitalWrite(P_AMIGA_H_PULSE, p[posX]);
        digitalWrite(P_AMIGA_HQ_PULSE, pq[posX]);
        posX = (posX + dirX + 4) % 4;
        stepsX--;
      }

      // 100 micros is just a guess, seems to work out fine.
      delayMicroseconds(100);
  }
}
