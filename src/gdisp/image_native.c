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
 * @file    src/gdisp/image_native.c
 * @brief   GDISP native image code.
 */
#include "ch.h"
#include "hal.h"
#include "gfx.h"

#if GFX_USE_GDISP && GDISP_NEED_IMAGE && GDISP_NEED_IMAGE_NATIVE

/**
 * How big a pixel array to allocate for blitting
 * Bigger is faster but uses more RAM.
 */
#define BLIT_BUFFER_SIZE	32

#define HEADER_SIZE			8
#define FRAME0POS			(HEADER_SIZE)

typedef struct gdispImagePrivate {
	pixel_t		*frame0cache;
	pixel_t		buf[BLIT_BUFFER_SIZE];
	} gdispImagePrivate;

gdispImageError gdispImageOpen_NATIVE(gdispImage *img) {
	uint8_t		hdr[HEADER_SIZE];

	/* Read the 8 byte header */
	if (img->io.fns->read(&img->io, hdr, 8) != 8)
		return GDISP_IMAGE_ERR_BADFORMAT;		// It can't be us

	if (hdr[0] != 'N' || hdr[1] != 'I')
		return GDISP_IMAGE_ERR_BADFORMAT;		// It can't be us

	if (hdr[6] != GDISP_PIXELFORMAT/256 || hdr[7] != (GDISP_PIXELFORMAT & 0xFF))
		return GDISP_IMAGE_ERR_UNSUPPORTED;		// Unsupported pixel format

	/* We know we are a native format image */
	img->flags = 0;
	img->width = (((uint16_t)hdr[2])<<8) | (hdr[3]);
	img->height = (((uint16_t)hdr[4])<<8) | (hdr[5]);
	if (img->width < 1 || img->height < 1)
		return GDISP_IMAGE_ERR_BADDATA;
	if (!(img->priv = (gdispImagePrivate *)chHeapAlloc(NULL, sizeof(gdispImagePrivate))))
		return GDISP_IMAGE_ERR_NOMEMORY;
	img->membytes = sizeof(gdispImagePrivate);
	img->priv->frame0cache = 0;

	return GDISP_IMAGE_ERR_OK;
}

void gdispImageClose_NATIVE(gdispImage *img) {
	if (img->priv) {
		if (img->priv->frame0cache)
			chHeapFree((void *)img->priv->frame0cache);
		chHeapFree((void *)img->priv);
		img->priv = 0;
	}
	img->membytes = 0;
	img->io.fns->close(&img->io);
}

gdispImageError gdispImageCache_NATIVE(gdispImage *img) {
	size_t		len;

	/* If we are already cached - just return OK */
	if (img->priv->frame0cache)
		return GDISP_IMAGE_ERR_OK;

	/* We need to allocate the cache */
	len = img->width * img->height * sizeof(pixel_t);
	img->priv->frame0cache = (pixel_t *)chHeapAlloc(NULL, len);
	if (!img->priv->frame0cache)
		return GDISP_IMAGE_ERR_NOMEMORY;
	img->membytes += len;

	/* Read the entire bitmap into cache */
	img->io.fns->seek(&img->io, FRAME0POS);
	if (img->io.fns->read(&img->io, img->priv->frame0cache, len) != len)
		return GDISP_IMAGE_ERR_BADDATA;

	return GDISP_IMAGE_ERR_OK;
}

gdispImageError gdispImageDraw_NATIVE(gdispImage *img, coord_t x, coord_t y, coord_t cx, coord_t cy, coord_t sx, coord_t sy) {
	coord_t		mx, mcx;
	size_t		pos, len;

	/* Check some reasonableness */
	if (sx >= img->width || sy >= img->height) return GDISP_IMAGE_ERR_OK;
	if (sx + cx > img->width) cx = img->width - sx;
	if (sy + cy > img->height) cy = img->height - sy;

	/* Draw from the image cache - if it exists */
	if (img->priv->frame0cache) {
		gdispBlitAreaEx(x, y, cx, cy, sx, sy, img->width, img->priv->frame0cache);
		return GDISP_IMAGE_ERR_OK;
	}

	/* For this image decoder we cheat and just seek straight to the region we want to display */
	pos = FRAME0POS + (img->width * sy + cx) * sizeof(pixel_t);

	/* Cycle through the lines */
	for(;cy;cy--, y++) {
		/* Move to the start of the line */
		img->io.fns->seek(&img->io, pos);

		/* Draw the line in chunks using BitBlt */
		for(mx = x, mcx = cx; mcx > 0; mcx -= len, mx += len) {
			// Read the data
			len = img->io.fns->read(&img->io,
						img->priv->buf,
						mx > BLIT_BUFFER_SIZE ? (BLIT_BUFFER_SIZE*sizeof(pixel_t)) : (mx * sizeof(pixel_t)))
					/ sizeof(pixel_t);
			if (!len)
				return GDISP_IMAGE_ERR_BADDATA;

			/* Blit the chunk of data */
			gdispBlitAreaEx(mx, y, len, 1, 0, 0, len, img->priv->buf);
		}

		/* Get the position for the start of the next line */
		pos += img->width*sizeof(pixel_t);
	}

	return GDISP_IMAGE_ERR_OK;
}

systime_t gdispImageNext_NATIVE(gdispImage *img) {
	(void) img;

	/* No more frames/pages */
	return TIME_INFINITE;
}

#endif /* GFX_USE_GDISP && GDISP_NEED_IMAGE && GDISP_NEED_IMAGE_NATIVE */
/** @} */
