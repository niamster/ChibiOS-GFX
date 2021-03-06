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
 * @file    drivers/ginput/touch/MCU/ginput_lld_mouse.c
 * @brief   GINPUT Touch low level driver source for the MCU.
 *
 * @defgroup Mouse Mouse
 * @ingroup GINPUT
 *
 * @{
 */

#include "ch.h"
#include "hal.h"
#include "gfx.h"

#if (GFX_USE_GINPUT && GINPUT_NEED_MOUSE) /*|| defined(__DOXYGEN__)*/

#include "ginput/lld/mouse.h"

#if defined(GINPUT_MOUSE_USE_CUSTOM_BOARD) && GINPUT_MOUSE_USE_CUSTOM_BOARD
	#include "ginput_lld_mouse_board.h"
#elif defined(BOARD_OLIMEX_STM32_LCD)
	#include "ginput_lld_mouse_board_olimex_stm32_lcd.h"
#elif defined(BOARD_OLIMEX_PIC32MX_LCD)
	#include "ginput_lld_mouse_board_olimex_pic32mx_lcd.h"
#else
	#include "ginput_lld_mouse_board.h"
#endif

static uint16_t sampleBuf[7];
static coord_t	lastx, lasty;

/**
 * @brief   7-point median filtering code for touchscreen samples
 *
 * @note    This is an internally used routine only.
 *
 * @notapi
 */
static void filter(void) {
	uint16_t temp;
	int i,j;

	for(i = 0; i < 4; i++) {
		for(j = i; j < 7; j++) {
			if(sampleBuf[i] > sampleBuf[j]) {
				/* Swap the values */
				temp = sampleBuf[i];
				sampleBuf[i] = sampleBuf[j];
				sampleBuf[j] = temp;
			}
		}
	}
}

/**
 * @brief   Initialise the mouse/touch.
 *
 * @notapi
 */
void ginput_lld_mouse_init(void) {
	init_board();
}

/**
 * @brief   Read the mouse/touch position.
 *
 * @param[in] pt	A pointer to the structure to fill
 *
 * @note			For drivers that don't support returning a position
 *					when the touch is up (most touch devices), it should
 *					return the previous position with the new Z value.
 *					The z value is the pressure for those touch devices
 *					that support it (-100 to 100 where > 0 is touched)
 *					or, 0 or 100 for those drivers that don't.
 *
 * @notapi
 */
void ginput_lld_mouse_get_reading(MouseReading *pt) {
	uint16_t i;

	// If touch-off return the previous results
	if (!getpin_pressed()) {
		pt->x = lastx;
		pt->y = lasty;
		pt->z = 0;
		pt->buttons = 0;
		return;
	}
	
	// Read the port to get the touch settings
	aquire_bus();

	/* Get the X value
	 * Discard the first conversion - very noisy and keep the ADC on hereafter
	 * till we are done with the sampling.
	 * Once we have the readings, find the medium using our filter function
 	 */
	read_x_value();
	for(i = 0; i < 7; i++)
		sampleBuf[i] = read_x_value();
	filter();
	lastx = (coord_t)sampleBuf[3];

	/* Get the Y value using the same process as above */
	read_y_value();
	for(i = 0; i < 7; i++)
		sampleBuf[i] = read_y_value();
	filter();
	lasty = (coord_t)sampleBuf[3];

	// Release the bus
	release_bus();
	
	// Return the results
	pt->x = lastx;
	pt->y = lasty;
	pt->z = 100;
	pt->buttons = GINPUT_TOUCH_PRESSED;
}

#endif /* GFX_USE_GINPUT && GINPUT_NEED_MOUSE */
/** @} */
