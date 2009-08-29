/*
 * CGraphics.cpp
 *
 *  Created on: 03.05.2009
 *      Author: gerstrong
 *
 * This file contains low- to mid-level graphics functions,
 * which are NOT platform-specific. All the low-level stuff in
 * here is stuff that draws to the scroll buffer (and so is
 * not platform-specific).
 */

#include "keen.h"
#include "CGraphics.h"
#include "sdl/CVideoDriver.h"
#include "sdl/video/colourtable.h"
#include "sdl/CVideoDriver.h"
#include "CLogFile.h"
#include "StringUtils.h"
#include "common/palette.h"

CGraphics::CGraphics() {
	HQBitmap = NULL;
	scrollbuffer=NULL;
	blitbuffer=NULL;
	scrollbuf_memsize = 0;
	blitbuf_memsize = 0;

	pal_init();
}

bool CGraphics::allocScrollBufmem(void)
{
	scrollbuf_memsize = 512*(512+300);	// First I have to check the resolution and then evaluate the scroll buffer size
	blitbuf_memsize = (320)*(200+30);
	g_pLogFile->ftextOut("allocmem(): allocating %d bytes for scroll buffer...", scrollbuf_memsize);
	scrollbuffer = new Uint8[scrollbuf_memsize];
	memset(scrollbuffer,COLOUR_MASK,scrollbuf_memsize);

	if (!scrollbuffer)
	{
		g_pLogFile->textOut(RED,"Failure<br>");
		return false;
	} else g_pLogFile->textOut("OK<br>");

	if (g_pVideoDriver->getZoomValue() > 1)
	{
		g_pLogFile->ftextOut("allocmem(): allocating %d bytes for blit buffer...", blitbuf_memsize);
		blitbuffer = new unsigned char[blitbuf_memsize];
		if (!blitbuffer)
		{
			g_pLogFile->ftextOut(RED,"Failure<br>");
			return false;
		} else g_pLogFile->ftextOut("OK<br>");
  }

  return true;
}

void CGraphics::freemem(void)
{
  if (scrollbuffer)
  {
	  delete[] scrollbuffer; scrollbuffer = NULL;
     g_pLogFile->fltextOut(BLACK,true," Scrollbuffer memory released to system.<br>");
  }
  if (blitbuffer)
  {
     delete[] blitbuffer; blitbuffer = NULL;
     g_pLogFile->fltextOut(BLACK,true," Blitbuffer memory released to system.<br>");
  }
}

void CGraphics::sb_setpixel(int x, int y, unsigned char c)
{
  scrollbuffer[(y<<9) + x] = c;
}

unsigned char CGraphics::sb_getpixel(int x, int y)
{
  return scrollbuffer[(y<<9) + x];
}

// draw a tile directly to the display (bypass the scroll buffer)
void CGraphics::drawTile_direct(int x, int y, unsigned int t)
{
unsigned char xa,ya;
  for(ya=0;ya<16;ya++)
   for(xa=0;xa<16;xa++)
	    g_pVideoDriver->setpixel(x+xa, y+ya, tiledata[t][ya][xa]);
}

// draws a sprite directly to the display (only used by status window)
void CGraphics::drawSprite_direct(int x, int y, unsigned int t)
{
unsigned char xa,ya;
unsigned char oldpixel; // used for the or operation when drawing maked sprites

  for(ya=0;ya<sprites[t].ysize;ya++)
   for(xa=0;xa<sprites[t].xsize;xa++)
    if ( sprites[t].maskdata[ya][xa]  )
    {
    	oldpixel = g_pVideoDriver->getpixel(x+xa, y+ya);
    	g_pVideoDriver->setpixel(x+xa, y+ya, (oldpixel | sprites[t].imgdata[ya][xa]) );
    }
    else
    	g_pVideoDriver->setpixel(x+xa, y+ya, (sprites[t].imgdata[ya][xa]==0) ? 16 :
															sprites[t].imgdata[ya][xa]);
}

void CGraphics::drawTile(int x, int y, unsigned int t)
{
	if(HQBitmap)
	{
		unsigned char *offset = &scrollbuffer[(y<<9)+x];
		unsigned char ya;
		// Tile in which the player won't interact, are to be ignored!
		if((TileProperty[t][BEHAVIOR] == 0 && TileProperty[t][ANIMATION] <= 1) ||
			(TileProperty[t][BEHAVIOR] > 30) )
		{
			for(ya=0;ya<16;ya++)
			{
				memset(offset, COLOUR_MASK, 16);
				offset+=512;
			}
		}
		else
		{
			for(ya=0;ya<16;ya++)
			{
				memcpy(offset, &tiledata[t][ya][0], 16);
				offset+=512;
			}
		}
	}
	else
	{
		unsigned char *offset = &scrollbuffer[(y<<9)+x];
		unsigned char ya;

		for(ya=0;ya<16;ya++)
		{
			memcpy(offset, &tiledata[t][ya][0], 16);
			offset+=512;
		}
	}
}

// draws a masked tile ("til") to the scrollbuffer.
// adjusts based on the X&Y scroll so that when the buffer is blitted
// the tile will appear at (x,y). only pixels which have a corresponding
// black pixel in tile "tmask" will be drawn.
void CGraphics::drawTilewithmask(int x, int y, unsigned int til, unsigned int tmask)
{
unsigned char xa,ya;
unsigned int bufoffX,bufoffY;
unsigned int xstart,ystart;
// clip the tile
  if (x>320 || y>200) return;
  if (x<-16||y<-16) return;
  if (x<0) xstart=-x; else xstart = 0;
  if (y<0) ystart=-y; else ystart = 0;

  bufoffY = ((y+ystart+scrolly_buf)&511)<<9;   // points to start of line
  for(ya=ystart;ya<16;ya++)
  {
    bufoffX = (x+xstart+scrollx_buf)&511;       // offset within line
    for(xa=xstart;xa<16;xa++)
    {
      if (tiledata[tmask][ya][xa] != 15)
      {
        scrollbuffer[bufoffY+bufoffX] = tiledata[til][ya][xa];
      }
      bufoffX = (bufoffX+1)&511;
    }
    // move to next line and wrap to top of buffer if needed
    bufoffY += 512;
    if (bufoffY >= (512*512)) bufoffY = 0;
  }
}

// draws a tile ("til") to the scrollbuffer. adjusts based on the X&Y scroll
// so that when the buffer is blitted the tile will appear at (x,y).
// used for priority tiles (tiles[].priority)
void CGraphics::drawPrioritytile(int x, int y, unsigned int til)
{
unsigned char xa,ya;
unsigned int bufoffX,bufoffY;
unsigned int xstart,ystart;
// clip the tile
  if (x>320 || y>200) return;
  if (x<-16 || y<-16) return;
  if (x<0) xstart=-x; else xstart = 0;
  if (y<0) ystart=-y; else ystart = 0;

  bufoffY = ((y+ystart+scrolly_buf)&511)<<9;    // points to start of line

  if(HQBitmap)
  {
	  for(ya=ystart;ya<16;ya++)
	  {
	    bufoffX = (x+xstart+scrollx_buf)&511;       // offset within line
	    for(xa=xstart;xa<16;xa++)
	    {
	       scrollbuffer[bufoffY+bufoffX] = COLOUR_MASK;
	       bufoffX = (bufoffX+1)&511;
	    }
	    // move to next line and wrap to top of buffer if needed
	    bufoffY += 512;
	    if (bufoffY >= (512*512)) bufoffY = 0;
	  }

	  return;
  }

  for(ya=ystart;ya<16;ya++)
  {
    bufoffX = (x+xstart+scrollx_buf)&511;       // offset within line
    for(xa=xstart;xa<16;xa++)
    {
       scrollbuffer[bufoffY+bufoffX] = tiledata[til][ya][xa];
       bufoffX = (bufoffX+1)&511;
    }
    // move to next line and wrap to top of buffer if needed
    bufoffY += 512;
    if (bufoffY >= (512*512)) bufoffY = 0;
  }
}


// draws a sprite to the scrollbuffer.
// adjusts based on the X&Y scroll so that when the buffer is blitted
// the sprite will appear at (x,y). saves the image beneath the sprite
// into the erasedata[] of object objectnum.
void CGraphics::drawSprite(int x, int y, unsigned int s, int objectnum)
{
unsigned char xa,ya;
unsigned int bufoffX, bufoffY;
unsigned int xstart,ystart;

  // clip the sprite
  if (x>320 || y>200) return;
  if (x<-sprites[s].xsize||y<-sprites[s].ysize) return;
  // if sprite is partially off the top or left of the screen, invert
  // the sign on the coordinate to make it positive, and start drawing
  // the sprite from there.
  if (x<0) xstart=-x; else xstart = 0;
  if (y<0) ystart=-y; else ystart = 0;

  bufoffY = ((y+ystart+scrolly_buf)&511)<<9;   // points to start of line
  for(ya=ystart;ya<sprites[s].ysize;ya++)
  {
   bufoffX = (x+xstart+scrollx_buf)&511;       // offset within line
   for(xa=xstart;xa<sprites[s].xsize;xa++)
   {
     objects[objectnum].erasedata[ya][xa] = scrollbuffer[bufoffY+bufoffX];
     if ( sprites[s].maskdata[ya][xa]  )
    	 scrollbuffer[bufoffY+bufoffX] |= sprites[s].imgdata[ya][xa];
     else
    	 scrollbuffer[bufoffY+bufoffX] = sprites[s].imgdata[ya][xa];

     bufoffX = (bufoffX+1)&511;
   }
   // move to next line and wrap to top of buffer if needed
   bufoffY += 512;
   if (bufoffY >= (512*512)) bufoffY = 0;
  }
}

// complement of drawsprite(). uses the saved image in objectnum to erase
// a previously-drawn sprite.
void CGraphics::eraseSprite(int x, int y, unsigned int s, int objectnum)
{
unsigned char xa,ya;
unsigned int bufoffX, bufoffY;
unsigned int xstart,ystart;

// clip the sprite
  if (x>320 || y>200) return;
  if (x<-sprites[s].xsize||y<-sprites[s].ysize) return;
  // if sprite is partially off the top or left of the screen, invert
  // the sign on the coordinate to make it positive, and start drawing
  // the sprite from there.
  if (x<0) xstart=-x; else xstart = 0;
  if (y<0) ystart=-y; else ystart = 0;

  bufoffY = ((y+ystart+scrolly_buf)&511)<<9;   // points to start of line
  for(ya=ystart;ya<sprites[s].ysize;ya++)
  {
   bufoffX = (x+xstart+scrollx_buf)&511;       // offset within line
   for(xa=xstart;xa<sprites[s].xsize;xa++)
   {
      scrollbuffer[bufoffY+bufoffX] = objects[objectnum].erasedata[ya][xa];
      bufoffX = (bufoffX+1)&511;
   }
   // move to next line and wrap to top of buffer if needed
   bufoffY += 512;
   if (bufoffY >= (512*512)) bufoffY = 0;
  }
}

void CGraphics::sb_drawCharacter(int x, int y, int f)
{
	unsigned char xa,ya;
	unsigned int yb;

	  for(ya=0;ya<8;ya++)
	  {
	    yb = ((y+ya+scrolly_buf)&511)<<9;
	    for(xa=0;xa<8;xa++)
	    {
	   		scrollbuffer[yb+((x+xa+scrollx_buf)&511)] = font[f][ya][xa];
	    }
	  }
}

void CGraphics::sb_drawCharacterwithmask(int x, int y, int f, char mask)
{
unsigned char xa,ya;
unsigned int yb;

  for(ya=0;ya<8;ya++)
  {
    yb = ((y+ya+scrolly_buf)&511)<<9;
    for(xa=0;xa<8;xa++)
    {
    	if(font[f][ya][xa] != mask)
    		scrollbuffer[yb+((x+xa+scrollx_buf)&511)] = font[f][ya][xa];
    }
  }
}


void CGraphics::sb_drawColorcharacter(int x, int y, int f, unsigned short colour, unsigned short bgcolour)
{
unsigned char xa,ya;
unsigned int yb;

  for(ya=0;ya<8;ya++)
  {
    yb = ((y+ya+scrolly_buf)&511)<<9;
    for(xa=0;xa<8;xa++)
    {
    	if(font[f][ya][xa] == 16)
    		scrollbuffer[yb+((x+xa+scrollx_buf)&511)] = colour;
    	else if(bgcolour != COLOUR_MASK)
    		scrollbuffer[yb+((x+xa+scrollx_buf)&511)] = bgcolour;
    }
  }
}

unsigned char savebuf[200][320];
void CGraphics::saveArea(int x1, int y1, int x2, int y2)
{
unsigned char xa,ya;
unsigned int yb;

  for(ya=0;ya<y2-y1;ya++)
  {
    yb = ((y1+ya+scrolly_buf)&511)<<9;
    for(xa=0;xa<x2-y1;xa++)
    {
      savebuf[ya][xa] = scrollbuffer[yb+((x1+xa+scrollx_buf)&511)];
    }
  }
}

void CGraphics::restoreArea(int x1, int y1, int x2, int y2)
{
unsigned char xa,ya;
unsigned int yb;

  for(ya=0;ya<y2-y1;ya++)
  {
    yb = ((y1+ya+scrolly_buf)&511)<<9;
    for(xa=0;xa<x2-y1;xa++)
    {
      scrollbuffer[yb+((x1+xa+scrollx_buf)&511)] = savebuf[ya][xa];
    }
  }
}

void CGraphics::stopGraphics(void)
{
  // shut down the video driver
	g_pVideoDriver->stop();
}


void CGraphics::drawBitmap(int xa, int ya, int b)
{
int x,y;
unsigned char *bmdataptr;

  // for "b" arguments passed from GetBitmapNumberFromName(),
  // in case the specified name was not found
  if (b==-1) return;

  bmdataptr = bitmaps[b].bmptr;
  for(y=0;y<bitmaps[b].ysize;y++)
  {
   for(x=0;x<bitmaps[b].xsize;x++)
   {
     sb_setpixel((x+xa+scrollx_buf)&511,(y+ya+scrolly_buf)&511,*bmdataptr);
     bmdataptr++;
   }
  }
}

void CGraphics::drawBitmap2FG(int xa, int ya, int b)
{
int x,y;
unsigned char *bmdataptr;

  // for "b" arguments passed from GetBitmapNumberFromName(),
  // in case the specified name was not found
  if (b==-1) return;

  bmdataptr = bitmaps[b].bmptr;
  for(y=0;y<bitmaps[b].ysize;y++)
  {
   for(x=0;x<bitmaps[b].xsize;x++)
   {
	 g_pVideoDriver->setpixel((x+xa+scrollx_buf-130)&511,(y+ya+scrolly_buf-30)&511,*bmdataptr);
     bmdataptr++;
   }
  }
}


int CGraphics::getBitmapNumberFromName(const char *bmname)
{
	int i;
  for(i=0;i<MAX_BITMAPS;i++)
  {
    bitmaps[i].name[8] = 0;     // ensure null-terminated
    if (!strcmp(bmname, bitmaps[i].name))
    {
      return i;
    }
  }

  // If the Bitmap was not found,
  // try to take one basing on the position
  // where it should be

  return -1;
}

void CGraphics::sb_font_draw(const std::string& text, int xoff, int yoff, bool useHighlightcolour)
{
   unsigned int i,x,y;

   x=xoff;
   y=yoff;
   for(i=0;i<text.size();i++)
   {
     if (text[i]!=13)
     {
       sb_drawCharacter(x, y, (unsigned char)text[i] + 128*(useHighlightcolour==true));
       x+=8;
     }
     else
     {
       x=xoff;
       y+=8;
     }
   }
}

void CGraphics::sb_mask_font_draw(const std::string& text, int xoff, int yoff, char mask)
{
unsigned int i,x,y;

   x=xoff;
   y=yoff;
   for(i=0;i<text.size();i++)
   {
     if (text[i]!=13)
     {
       sb_drawCharacterwithmask(x, y, text[i], mask);
       x+=8;
     }
     else
     {
       x=xoff;
       y+=8;
     }
   }
}


void CGraphics::sb_color_font_draw(const std::string& text, int xoff, int yoff, unsigned int colour, unsigned short bgcolour)
{
unsigned int i,x,y;

   x=xoff;
   y=yoff;
   for(i=0;i< text.size(); i++)
   {
     if (text[i]!=13)
     {
       sb_drawColorcharacter(x, y, text[i], colour, bgcolour);
       x+=8;
     }
     else
     {
       x=xoff;
       y+=8;
     }
   }
}

void CGraphics::setFadeBlack(bool value)
{	fade_black = value;	}

Uint8 *CGraphics::getScrollbuffer(void)
{	return scrollbuffer;	}


void CGraphics::renderHQBitmap()
{
	if(HQBitmap)
	{
		SDL_Rect srcrect;
		//SDL_Rect dstrect;

		srcrect.x = scroll_x-32;
		srcrect.y = scroll_y-32;
		srcrect.w = g_pVideoDriver->getBGLayerSurface()->w;
		srcrect.h = g_pVideoDriver->getBGLayerSurface()->h;

		HQBitmap->updateHQBitmap(g_pVideoDriver->getBGLayerSurface(), &srcrect, NULL);
	}
}

void CGraphics::loadHQGraphics(unsigned char episode, unsigned char level, const std::string& datadir)
{
	SDL_Rect screen_rect;

	std::string buf = formatPathString(datadir);
	std::string buf2 = buf + "level" + itoa(level) + "ep" + itoa(episode) + ".bmp";

	screen_rect.x = 0;
	screen_rect.y = 0;
	screen_rect.w = g_pVideoDriver->getWidth();
	screen_rect.h = g_pVideoDriver->getHeight();

	HQBitmap = new CHQBitmap(screen_rect);
	if(!HQBitmap->loadImage(buf2, (int) map.xsize, (int) map.ysize))
	{
		delete HQBitmap;
		HQBitmap = NULL;
		return;
	}

	// Check if the tile have grey pixels in some tiles and remove them!
	for(int t=0 ; t < numtiles ; t++)
	{
		if(TileProperty[t][BEHAVIOR] > 0)
		{
			for(int xa=0 ; xa < 16 ; xa++)
			{
				for(int ya=0 ; ya < 16 ; ya++)
				{
					if(tiledata[t][ya][xa] == COLOUR_GREY)	// Which should be masked. In Episode 1 it is gray
					{
						tiledata[t][ya][xa] = COLOUR_MASK;
					}
				}
			}
		}
	}

}
void CGraphics::unloadHQGraphics()
{
	if(!HQBitmap)
		return;

	delete HQBitmap;
	HQBitmap = NULL;

	// Make unmask some tiles adding the original pixels back!
	for(int t=0 ; t < numtiles ; t++)
	{
		if(TileProperty[t][BEHAVIOR] > 0)
		{
			for(int xa=0 ; xa < 16 ; xa++)
			{
				for(int ya=0 ; ya < 16 ; ya++)
				{
					if(tiledata[t][ya][xa] == COLOUR_MASK)	// Which should be masked. In Episode 1 it is gray
					{
						tiledata[t][ya][xa] = COLOUR_GREY;
					}
				}
			}
		}
	}
}
