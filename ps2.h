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

#pragma once

#include "config.h"
#include "Arduino.h"

// PS/2 commands
#define PS2_CMD_RESET                 0xff
#define PS2_CMD_ENABLE_DATA_REPORTING 0xf4
#define PS2_CMD_SET_SAMPLE_RATE       0xf3
#define PS2_CMD_SET_REMOTE_MODE       0xf0
#define PS2_CMD_SET_WRAP_MODE         0xee
#define PS2_CMD_RESET_WRAP_MODE       0xec
#define PS2_CMD_READ_DATA             0xeb
#define PS2_CMD_SET_RESOLUTION        0xe8

// PS/2 responses
#define PS2_ACK    0xfa
#define PS2_BAT_OK 0xaa

// PS/2 Data masks
#define PS2_MASK_Y_SIGN (1<<5)
#define PS2_MASK_X_SIGN (1<<4)
#define PS2_MASK_MMB (1<<2)
#define PS2_MASK_RMB (1<<1)
#define PS2_MASK_LMB (1<<0)

// PS/2 Resolutions
#define PS2_RES_1_CNT_PER_MM 0
#define PS2_RES_2_CNT_PER_MM 1
#define PS2_RES_4_CNT_PER_MM 2
#define PS2_RES_8_CNT_PER_MM 3

void ps2SetPins(int clkPin, int dataPin);
void ps2Send(byte val);
byte ps2Receive();
void ps2SendCommand(byte cmd);
