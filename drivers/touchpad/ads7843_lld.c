/*
    ChibiOS/RT - Copyright (C) 2012
                 Joel Bodenmann aka Tectu <joel@unormal.org>

    This file is part of ChibiOS-LCD-Driver.

    ChibiOS-LCD-Driver is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    ChibiOS-LCD-Driver is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ads7843_lld.h"

#ifdef TOUCHPAD_USE_ADS7843

__inline uint16_t lld_tpReadX(void) {
    uint8_t txbuf[1];
    uint8_t rxbuf[2];
    uint16_t x;

    txbuf[0] = 0xd0;
    TP_CS_LOW;
	spiSend(&SPID1, 1, txbuf);
    spiReceive(&SPID1, 2, rxbuf);
    TP_CS_HIGH;

    x = rxbuf[0] << 4;
    x |= rxbuf[1] >> 4;
    
    return x;
}

__inline uint16_t lld_tpReadY(void) {
    uint8_t txbuf[1];
    uint8_t rxbuf[2];
    uint16_t y;

    txbuf[0] = 0x90;
    TP_CS_LOW;
    spiSend(&SPID1, 1, txbuf);
    spiReceive(&SPID1, 2, rxbuf);
    TP_CS_HIGH;

    y = rxbuf[0] << 4;
    y |= rxbuf[1] >> 4;

    return y;
}

__inline uint16_t lld_tpReadZ(void) {
	return 0;
}

#endif
