/*
    ChibiOS/GFX - Copyright (C) 2012, 2013
                 Joel Bodenmann aka Tectu <joel@unormal.org>

    This file is part of ChibiOS/GFX.

    ChibiOS/GFX is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    ChibiOS/GFX is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * @file    drivers/gdisp/SSD1963/gdisp_lld_board_example.h
 * @brief   GDISP Graphic Driver subsystem board interface for the SSD1963 display.
 *
 * @addtogroup GDISP
 * @{
 */

#ifndef _GDISP_LLD_BOARD_H
#define _GDISP_LLD_BOARD_H

#if defined(GDISP_USE_GPIO)
	#define Set_CS		palSetPad(GDISP_CMD_PORT, GDISP_CS);
	#define Clr_CS		palClearPad(GDISP_CMD_PORT, GDISP_CS);
	#define Set_RS		palSetPad(GDISP_CMD_PORT, GDISP_RS);
	#define Clr_RS		palClearPad(GDISP_CMD_PORT, GDISP_RS);
	#define Set_WR		palSetPad(GDISP_CMD_PORT, GDISP_WR);
	#define Clr_WR		palClearPad(GDISP_CMD_PORT, GDISP_WR);
	#define Set_RD		palSetPad(GDISP_CMD_PORT, GDISP_RD);
	#define Clr_RD		palClearPad(GDISP_CMD_PORT, GDISP_RD);
#endif

#if defined(GDISP_USE_FSMC)
	/* Using FSMC A16 as RS */
	#define GDISP_REG              (*((volatile uint16_t *) 0x60000000)) /* RS = 0 */
	#define GDISP_RAM              (*((volatile uint16_t *) 0x60020000)) /* RS = 1 */
#endif

/**
 * @brief   Send data to the index register.
 *
 * @param[in] index		The index register to set
 *
 * @notapi
 */
static inline void write_index(uint16_t index) {
	Set_CS; Set_RS; Set_WR; Clr_RD;
	palWritePort(GDISP_DATA_PORT, index);
	Clr_CS;
}

/**
 * @brief   Send data to the lcd.
 *
 * @param[in] data		The data to send
 * 
 * @notapi
 */
static inline void write_data(uint16_t data) {
	Set_CS; Clr_RS; Set_WR; Clr_RD;
	palWritePort(GDISP_DATA_PORT, data);
	Clr_CS;
}

/**
 * @brief   Initialise the board for the display.
 *
 * @notapi
 */
static inline void init_board(void) {
	
	IOBus busCMD = {GDISP_CMD_PORT, (1 << GDISP_CS) | (1 << GDISP_RS) | (1 << GDISP_WR) | (1 << GDISP_RD), 0};
	IOBus busDATA = {GDISP_CMD_PORT, 0xFFFFF, 0};
	palSetBusMode(&busCMD, PAL_MODE_OUTPUT_PUSHPULL);
	palSetBusMode(&busDATA, PAL_MODE_OUTPUT_PUSHPULL);
}

static inline void post_init_board(void) {
	/* Nothing to do here */
}

/**
 * @brief   Set or clear the lcd reset pin.
 *
 * @param[in] state		TRUE = lcd in reset, FALSE = normal operation
 * 
 * @notapi
 */
static inline void setpin_reset(bool_t state) {

}

/**
 * @brief   Set the lcd back-light level.
 *
 * @param[in] percent		0 to 100%
 * 
 * @notapi
 */
static inline void set_backlight(uint8_t percent) {
	//duty_cycle is 00..FF
	//Work in progress: the SSD1963 has a built-in PWM, its output can
	//be used by a Dynamic Background Control or by a host (user)
	//Check your LCD's hardware, the PWM connection is default left open and instead
	//connected to a LED connection on the breakout board
	write_index(SSD1963_SET_PWM_CONF);//set PWM for BackLight
	write_data(0x0001);
	write_data(percent & 0x00FF);
	write_data(0x0001);//controlled by host (not DBC), enabled
	write_data(0x00FF);
	write_data(0x0060);//don't let it go too dark, avoid a useless LCD
	write_data(0x000F);//prescaler ???
}

/**
 * @brief   Take exclusive control of the bus
 *
 * @notapi
 */
static inline void acquire_bus(void) {
	/* Nothing to do here */
}

/**
 * @brief   Release exclusive control of the bus
 *
 * @notapi
 */
static inline void release_bus(void) {
	/* Nothing to do here */
}

__inline void write_stream(uint16_t *buffer, uint16_t size) {
	uint16_t i;
	Set_CS; Clr_RS; Set_WR; Clr_RD;
	for(i = 0; i < size; i++) {
		Set_WR;
		palWritePort(GDISP_DATA_PORT, buffer[i]);
		Clr_WR;
	}
	Clr_CS;
}

__inline void read_stream(uint16_t *buffer, size_t size) {
	uint16_t i;
	Set_CS; Clr_RS; Clr_WR; Set_RD;
	for(i = 0; i < size; i++) {
		Set_RD;
		buffer[i] = palReadPort(GDISP_DATA_PORT);
		Clr_RD;
	}
}

#if GDISP_HARDWARE_READPIXEL || GDISP_HARDWARE_SCROLL || defined(__DOXYGEN__)
/**
 * @brief   Read data from the lcd.
 *
 * @return	The data from the lcd
 * @note	The chip select may need to be asserted/de-asserted
 * 			around the actual spi read
 * 
 * @notapi
 */
static inline uint16_t read_data(void) {
	Set_CS; Clr_RS; Clr_WR; Set_RD;
	uint16_t data = palReadPort(GDISP_DATA_PORT); 
	Clr_CS;
	return data;
}
#endif

#endif /* _GDISP_LLD_BOARD_H */
/** @} */
