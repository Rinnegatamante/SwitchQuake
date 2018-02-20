/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include <switch.h>
#include "quakedef.h"
#include "d_local.h"
#define u16 uint16_t
#define u8 uint8_t

CVAR (vid_vsync, 1, CVAR_ARCHIVE)

viddef_t	vid;				// global video state
int isKeyboard = false;
int old_char = 0;
float fixpalette = 0;
float rend_scale = 1.0;
char res_string[256];
int cur_width = 1280, cur_height = 720;

const int widths[4] = {1280, 1280, 1280, 1280};
const int heights[4] = {720, 720, 720, 720};
const float scales[4] = {1.777, 1.777, 1.777, 1.777};
extern cvar_t res_val;
#define SURFCACHE_SIZE 10485760

short	zbuffer[960*544];
byte*	surfcache;
uint8_t* tex_buffer = NULL;
u16	d_8to16table[256];

uint32_t mem_palette[256];

void	VID_SetPalette (unsigned char *palette)
{
	int i;
	uint32_t* palette_tbl = mem_palette;
	u8* pal = palette;
	unsigned r, g, b;
	
	for(i=0; i<256; i++){
		r = pal[0];
		g = pal[1];
		b = pal[2];
		palette_tbl[i] = r | (g << 8) | (b << 16) | (0xFF << 24);
		pal += 3;
	}
}

void	VID_ShiftPalette (unsigned char *palette)
{
	VID_SetPalette(palette);
}

void	VID_Init (unsigned char *palette)
{
	
	tex_buffer = malloc(widths[3] * heights[3]);
	vid.maxwarpwidth = vid.width = vid.conwidth = widths[3];
	vid.maxwarpheight = vid.height = vid.conheight = heights[3];
	vid.rowbytes = vid.conrowbytes = widths[3];
	
	vid.aspect = ((float)vid.height / (float)vid.width) * (320.0 / 240.0);
	vid.numpages = 2;
	vid.colormap = host_colormap;
	vid.fullbright = 256 - LittleLong (*((int *)vid.colormap + 2048));
	vid.buffer = vid.conbuffer = vid.direct = tex_buffer;
	
	// Set correct palette for the texture
	VID_SetPalette(palette);
		
	// Init Quake Cache
	d_pzbuffer = zbuffer;
	surfcache = malloc(SURFCACHE_SIZE);
	D_InitCaches (surfcache, SURFCACHE_SIZE);
	
	sprintf(res_string,"Current Resolution: %d x %d", widths[3], heights[3]);
	Cvar_RegisterVariable (&res_val);
	Cvar_RegisterVariable(&vid_vsync);
	
}

void VID_ChangeRes(float scale){
	
	// Freeing texture
	if (tex_buffer != NULL) free(tex_buffer);
	
	int idx = 0;
	if (scale == 1.777f) idx = 0;
	
	// Changing renderer resolution
	int width = cur_width = widths[idx];
	int height = cur_height = heights[idx];
	tex_buffer = malloc(widths[idx] * heights[idx]);
	vid.maxwarpwidth = vid.width = vid.conwidth = width;
	vid.maxwarpheight = vid.height = vid.conheight = height;
	vid.rowbytes = vid.conrowbytes = width;	
	vid.buffer = vid.conbuffer = vid.direct = tex_buffer;
	sprintf(res_string,"Current Resolution: %d x %d", widths[idx], heights[idx]);
	
	// Forcing a palette restoration
	fixpalette = v_gamma.value;
	Cvar_SetValue ("v_gamma", 0.1);
	
	// Changing scale value
	rend_scale = scales[idx];
	
}

void	VID_Shutdown (void)
{
	free(tex_buffer);
	free(surfcache);
}

void VID_DrawPalettedFramebuffer(uint8_t* fb, uint32_t* pal){
	int x, y;
	uint32_t w, h;
	uint32_t* framebuf = (uint32_t*)gfxGetFramebuffer((uint32_t*)&w, (uint32_t*)&h);
	for (y=0;y<h;y++){
		for (x=0;x<w;x++){
			framebuf[y*w+x] = pal[fb[y*w+x]];
		}
	}
}

void	VID_Update (vrect_t *rects)
{
	
	if (fixpalette > 0){
		Cvar_SetValue ("v_gamma", fixpalette);
		fixpalette = 0;
	}
	VID_DrawPalettedFramebuffer((uint8_t*)tex_buffer, mem_palette);
	gfxFlushBuffers();
	gfxSwapBuffers();
	if (vid_vsync.value) gfxWaitForVsync();
}

/*
================
D_BeginDirectRect
================
*/
void D_BeginDirectRect (int x, int y, byte *pbitmap, int width, int height)
{
}


/*
================
D_EndDirectRect
================
*/
void D_EndDirectRect (int x, int y, int width, int height)
{
}
