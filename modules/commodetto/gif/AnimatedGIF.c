//
// GIF Animator
// written by Larry Bank
// bitbank@pobox.com
// Arduino port started 7/5/2020
// Original GIF code written 20+ years ago :)
// The goal of this code is to decode images up to 480x320
// using no more than 22K of RAM (if sent directly to an LCD display)
//
// Copyright 2020 BitBank Software, Inc. All Rights Reserved.
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//    http://www.apache.org/licenses/LICENSE-2.0
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//===========================================================================
#include "AnimatedGIF.h"

#ifdef HAL_ESP32_HAL_H_
#define memcpy_P memcpy
#endif

static const unsigned char cGIFBits[9] = {1,4,4,4,8,8,8,8,8}; // convert odd bpp values to ones we can handle

// forward references
static int GIFInit(GIFIMAGE *pGIF);
static int GIFParseInfo(GIFIMAGE *pPage, int bInfoOnly);
static int GIFGetMoreData(GIFIMAGE *pPage);
static void GIFMakePels(GIFIMAGE *pPage, unsigned int code);
static int DecodeLZW(GIFIMAGE *pImage, int iOptions);
static int32_t readMem(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen);
static int32_t seekMem(GIFFILE *pFile, int32_t iPosition);
int GIFGetInfo(GIFIMAGE *pPage, GIFINFO *pInfo, unsigned short *pPalette);
#if defined( PICO_BUILD ) || defined( __LINUX__ ) || defined( __MCUXPRESSO )
static int32_t readFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen);
static int32_t seekFile(GIFFILE *pFile, int32_t iPosition);
static void closeFile(void *handle);

// C API
int GIF_openRAM(GIFIMAGE *pGIF, uint8_t *pData, int iDataSize, GIF_DRAW_CALLBACK *pfnDraw)
{
    pGIF->iError = GIF_SUCCESS;
    pGIF->pfnRead = readMem;
    pGIF->pfnSeek = seekMem;
    pGIF->pfnDraw = pfnDraw;
    pGIF->pfnOpen = NULL;
    pGIF->pfnClose = NULL;
    pGIF->GIFFile.iSize = iDataSize;
    pGIF->GIFFile.pData = pData;
    return GIFInit(pGIF);
} /* GIF_openRAM() */

#ifdef __LINUX__
int GIF_openFile(GIFIMAGE *pGIF, const char *szFilename, GIF_DRAW_CALLBACK *pfnDraw)
{
    pGIF->iError = GIF_SUCCESS;
    pGIF->pfnRead = readFile;
    pGIF->pfnSeek = seekFile;
    pGIF->pfnDraw = pfnDraw;
    pGIF->pfnOpen = NULL;
    pGIF->pfnClose = closeFile;
    pGIF->GIFFile.fHandle = fopen(szFilename, "r+b");
    if (pGIF->GIFFile.fHandle == NULL)
       return 0;
    fseek((FILE *)pGIF->GIFFile.fHandle, 0, SEEK_END);
    pGIF->GIFFile.iSize = (int)ftell((FILE *)pGIF->GIFFile.fHandle);
    fseek((FILE *)pGIF->GIFFile.fHandle, 0, SEEK_SET);
    return GIFInit(pGIF);
} /* GIF_openFile() */
#endif

void GIF_close(GIFIMAGE *pGIF)
{
    if (pGIF->pfnClose)
        (*pGIF->pfnClose)(pGIF->GIFFile.fHandle);
} /* GIF_close() */

void GIF_begin(GIFIMAGE *pGIF, int iEndian, unsigned char ucPaletteType)
{
    memset(pGIF, 0, sizeof(GIFIMAGE));
    pGIF->ucLittleEndian = (iEndian == LITTLE_ENDIAN_PIXELS);
    pGIF->ucPaletteType = ucPaletteType;
} /* GIF_begin() */

void GIF_reset(GIFIMAGE *pGIF)
{
    (*pGIF->pfnSeek)(&pGIF->GIFFile, 0);
} /* GIF_reset() */

//
// Return value:
// 1 = good decode, more frames exist
// 0 = good decode, no more frames
// -1 = error
//
int GIF_playFrame(GIFIMAGE *pGIF, int *delayMilliseconds)
{
int rc;

    if (delayMilliseconds)
       *delayMilliseconds = 0; // clear any old valid
    if (pGIF->GIFFile.iPos >= pGIF->GIFFile.iSize-1) // no more data exists
    {   
        (*pGIF->pfnSeek)(&pGIF->GIFFile, 0); // seek to start
    }
    if (GIFParseInfo(pGIF, 0))
    {
        if (pGIF->iError == GIF_EMPTY_FRAME) // don't try to decode it
            return 0;
        rc = DecodeLZW(pGIF, 0);
        if (rc != 0) // problem
            return -1;
    }
    else
    {
        return -1; // error parsing the frame info, we may be at the end of the file
    }
    // Return 1 for more frames or 0 if this was the last frame
    if (delayMilliseconds) // if not NULL, return the frame delay time
        *delayMilliseconds = pGIF->iFrameDelay;
    return (pGIF->GIFFile.iPos < pGIF->GIFFile.iSize-1);
} /* GIF_playFrame() */

int GIF_getCanvasWidth(GIFIMAGE *pGIF)
{
    return pGIF->iCanvasWidth;
} /* GIF_getCanvasWidth() */

int GIF_getCanvasHeight(GIFIMAGE *pGIF)
{
    return pGIF->iCanvasHeight;
} /* GIF_getCanvasHeight() */

int GIF_getComment(GIFIMAGE *pGIF, char *pDest)
{
int32_t iOldPos;

    iOldPos = pGIF->GIFFile.iPos; // keep old position
    (*pGIF->pfnSeek)(&pGIF->GIFFile, pGIF->iCommentPos);
    (*pGIF->pfnRead)(&pGIF->GIFFile, (uint8_t *)pDest, pGIF->sCommentLen);
    (*pGIF->pfnSeek)(&pGIF->GIFFile, iOldPos);
    pDest[pGIF->sCommentLen] = 0; // zero terminate the string
    return (int)pGIF->sCommentLen;

} /* GIF_getComment() */

int GIF_getLastError(GIFIMAGE *pGIF)
{
    return pGIF->iError;
} /* GIF_getLastError() */

#endif // !__cplusplus
//
// Helper functions for memory based images
//
static int32_t readMem(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen)
{
    int32_t iBytesRead;

    iBytesRead = iLen;
    if ((pFile->iSize - pFile->iPos) < iLen)
       iBytesRead = pFile->iSize - pFile->iPos;
    if (iBytesRead <= 0)
       return 0;
    c_memcpy(pBuf, &pFile->pData[pFile->iPos], iBytesRead);
    pFile->iPos += iBytesRead;
    return iBytesRead;
} /* readMem() */

static int32_t seekMem(GIFFILE *pFile, int32_t iPosition)
{
    if (iPosition < 0) iPosition = 0;
    else if (iPosition >= pFile->iSize) iPosition = pFile->iSize-1;
    pFile->iPos = iPosition;
    return iPosition;
} /* seekMem() */

#if defined ( __LINUX__ ) || defined( __MCUXPRESSO )
static void closeFile(void *handle)
{
    fclose((FILE *)handle);
} /* closeFile() */

static int32_t seekFile(GIFFILE *pFile, int32_t iPosition)
{
    if (iPosition < 0) iPosition = 0;
    else if (iPosition >= pFile->iSize) iPosition = pFile->iSize-1;
    pFile->iPos = iPosition;
    fseek((FILE *)pFile->fHandle, iPosition, SEEK_SET);
    return iPosition;
} /* seekMem() */

static int32_t readFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen)
{
    int32_t iBytesRead;

    iBytesRead = iLen;
    if ((pFile->iSize - pFile->iPos) < iLen)
       iBytesRead = pFile->iSize - pFile->iPos;
    if (iBytesRead <= 0)
       return 0;
    iBytesRead = (int)fread(pBuf, 1, iBytesRead, (FILE *)pFile->fHandle);
    pFile->iPos += iBytesRead;
    return iBytesRead;
} /* readFile() */

#endif // __LINUX__
//
// The following functions are written in plain C and have no
// 3rd party dependencies, not even the C runtime library
//
//
// Initialize a GIF file and callback access from a file on SD or memory
// returns 1 for success, 0 for failure
// Fills in the canvas size of the GIFIMAGE structure
//
static int GIFInit(GIFIMAGE *pGIF)
{
    pGIF->GIFFile.iPos = 0; // start at beginning of file
    if (!GIFParseInfo(pGIF, 1)) // gather info for the first frame
       return 0; // something went wrong; not a GIF file?
    (*pGIF->pfnSeek)(&pGIF->GIFFile, 0); // seek back to start of the file
    if (pGIF->iCanvasWidth > MAX_WIDTH) { // need to allocate more space
        pGIF->iError = GIF_TOO_WIDE;
        return 0;
    }
  return 1;
} /* GIFInit() */

//
// Parse the GIF header, gather the size and palette info
// If called with bInfoOnly set to true, it will test for a valid file
// and return the canvas size only
// Returns 1 for success, 0 for failure
//
static int GIFParseInfo(GIFIMAGE *pPage, int bInfoOnly)
{
    int i, j, iColorTableBits;
    int iBytesRead;
    unsigned char c, *p;
    int32_t iOffset = 0;
    int32_t iStartPos = pPage->GIFFile.iPos; // starting file position
    int iReadSize;
    
    pPage->bUseLocalPalette = 0; // assume no local palette
    pPage->bEndOfFrame = 0; // we're just getting started
    pPage->iFrameDelay = 0; // may not have a gfx extension block
    iReadSize = (bInfoOnly) ? 12 : MAX_CHUNK_SIZE;
    // If you try to read past the EOF, the SD lib will return garbage data
    if (iStartPos + iReadSize > pPage->GIFFile.iSize)
       iReadSize = (pPage->GIFFile.iSize - iStartPos - 1);
    p = pPage->ucFileBuf;
    iBytesRead =  (*pPage->pfnRead)(&pPage->GIFFile, pPage->ucFileBuf, iReadSize); // 255 is plenty for now

    if (iBytesRead != iReadSize) // we're at the end of the file
    {
       pPage->iError = GIF_EARLY_EOF;
       return 0;
    }
    if (iStartPos == 0) // start of the file
    { // canvas size
        if (c_memcmp(p, "GIF89", 5) != 0 && c_memcmp(p, "GIF87", 5) != 0) // not a GIF file
        {
           pPage->iError = GIF_BAD_FILE;
           return 0;
        }
        pPage->iCanvasWidth = pPage->iWidth = INTELSHORT(&p[6]);
        pPage->iCanvasHeight = pPage->iHeight = INTELSHORT(&p[8]);
        pPage->iBpp = ((p[10] & 0x70) >> 4) + 1;
        if (bInfoOnly)
           return 1; // we've got the info we needed, leave
        iColorTableBits = (p[10] & 7) + 1; // Log2(size) of the color table
        pPage->ucBackground = p[11]; // background color
        pPage->ucGIFBits = 0;
        pPage->sGlobalColorTableCount = 0;
        iOffset = 13;
        if (p[10] & 0x80) // global color table?
        { // by default, convert to byte-reversed RGB565 for immediate use
            // Read enough additional data for the color table
            iBytesRead += (*pPage->pfnRead)(&pPage->GIFFile, &pPage->ucFileBuf[iBytesRead], 3*(1<<iColorTableBits));
            if (pPage->ucPaletteType == GIF_PALETTE_RGB565)
            {
                for (i=0; i<(1<<iColorTableBits); i++)
                {
                    uint16_t usRGB565;
                    usRGB565 = ((p[iOffset] >> 3) << 11); // R
                    usRGB565 |= ((p[iOffset+1] >> 2) << 5); // G
                    usRGB565 |= (p[iOffset+2] >> 3); // B
                    if (pPage->ucLittleEndian)
                        pPage->pPalette[i] = usRGB565;
                    else
#if WIN32
                        pPage->pPalette[i] = _byteswap_ushort(usRGB565); // SPI wants MSB first
#else
                        pPage->pPalette[i] = __builtin_bswap16(usRGB565); // SPI wants MSB first
#endif
                    iOffset += 3;
                }
            }
            else // just copy it as-is
            {
                c_memcpy(pPage->pPalette, &p[iOffset], (1<<iColorTableBits) * 3);
                iOffset += (1 << iColorTableBits) * 3;
            }
			pPage->sGlobalColorTableCount = 1 << iColorTableBits;
        }
    }
    while (p[iOffset] != ',') /* Wait for image separator */
    {
        if (p[iOffset] == '!') /* Extension block */
        {
            iOffset++;
            switch(p[iOffset++]) /* Block type */
            {
                case 0xf9: /* Graphic extension */
                    if (p[iOffset] == 4) // correct length
                    {
                        pPage->ucGIFBits = p[iOffset+1]; // packed fields
                        pPage->iFrameDelay = (INTELSHORT(&p[iOffset+2]))*10; // delay in ms
                        if (pPage->iFrameDelay <= 1) // 0-1 is going to make it run at 60fps; use 100 (10fps) as a reasonable substitute
                           pPage->iFrameDelay = 100;
                        if (pPage->ucGIFBits & 1) // transparent color is used
                            pPage->ucTransparent = p[iOffset+4]; // transparent color index
                        iOffset += 6;
                    }
                    //                     else   // error
                    break;
                case 0xff: /* App extension */
                    c = 1;
                    while (c) /* Skip all data sub-blocks */
                    {
                        c = p[iOffset++]; /* Block length */
                        if ((iBytesRead - iOffset) < (c+32)) // need to read more data first
                        {
                            c_memcpy(pPage->ucFileBuf, &pPage->ucFileBuf[iOffset], (iBytesRead-iOffset)); // move existing data down
                            iBytesRead -= iOffset;
                            iStartPos += iOffset;
                            iOffset = 0;
                            iBytesRead += (*pPage->pfnRead)(&pPage->GIFFile, &pPage->ucFileBuf[iBytesRead], c+32);
                        }
                        if (c == 11) // fixed block length
                        { // Netscape app block contains the repeat count
                            if (c_memcmp(&p[iOffset], "NETSCAPE2.0", 11) == 0)
                            {
                                //                              if (p[iOffset+11] == 3 && p[iOffset+12] == 1) // loop count
                                //                                  pPage->iRepeatCount = INTELSHORT(&p[iOffset+13]);
                            }
                        }
                        iOffset += (int)c; /* Skip to next sub-block */
                    }
                    break;
                case 0x01: /* Text extension */
                    c = 1;
                    j = 0;
                    while (c) /* Skip all data sub-blocks */
                    {
                        c = p[iOffset++]; /* Block length */
                        if (j == 0) // use only first block
                        {
                            j = c;
                            if (j > 127)   // max comment length = 127
                                j = 127;
                            //                           memcpy(pPage->szInfo1, &p[iOffset], j);
                            //                           pPage->szInfo1[j] = '\0';
                            j = 1;
                        }
                        iOffset += (int)c; /* Skip this sub-block */
                    }
                    break;
                case 0xfe: /* Comment */
                    c = 1;
                    while (c) /* Skip all data sub-blocks */
                    {
                        c = p[iOffset++]; /* Block length */
                        if ((iBytesRead - iOffset) < (c+32)) // need to read more data first
                        {
                            c_memcpy(pPage->ucFileBuf, &pPage->ucFileBuf[iOffset], (iBytesRead-iOffset)); // move existing data down
                            iBytesRead -= iOffset;
                            iStartPos += iOffset;
                            iOffset = 0;
                            iBytesRead += (*pPage->pfnRead)(&pPage->GIFFile, &pPage->ucFileBuf[iBytesRead], c+32);
                        }
                        if (pPage->iCommentPos == 0) // Save first block info
                        {
                            pPage->iCommentPos = iStartPos + iOffset;
                            pPage->sCommentLen = c;
                        }
                        iOffset += (int)c; /* Skip this sub-block */
                    }
                    break;
                default:
                    /* Bad header info */
                    pPage->iError = GIF_DECODE_ERROR;
                    return 0;
            } /* switch */
        }
        else // invalid byte, stop decoding
        {
			if (0x3b == p[iOffset])
				pPage->iError = GIF_EARLY_EOF;
            else if (pPage->GIFFile.iSize - iStartPos < 32) // non-image bytes at end of file?
                pPage->iError = GIF_EMPTY_FRAME;
            else
                /* Bad header info */
                pPage->iError = GIF_DECODE_ERROR;
            return 0;
        }
    } /* while */
    if (p[iOffset] == ',')
        iOffset++;
    // This particular frame's size and position on the main frame (if animated)
    pPage->iX = INTELSHORT(&p[iOffset]);
    pPage->iY = INTELSHORT(&p[iOffset+2]);
    pPage->iWidth = INTELSHORT(&p[iOffset+4]);
    pPage->iHeight = INTELSHORT(&p[iOffset+6]);
    iOffset += 8;
    
    /* Image descriptor
     7 6 5 4 3 2 1 0    M=0 - use global color map, ignore pixel
     M I 0 0 0 pixel    M=1 - local color map follows, use pixel
     I=0 - Image in sequential order
     I=1 - Image in interlaced order
     pixel+1 = # bits per pixel for this image
     */
    pPage->ucMap = p[iOffset++];
    if (pPage->ucMap & 0x80) // local color table?
    {// by default, convert to byte-reversed RGB565 for immediate use
        j = (1<<((pPage->ucMap & 7)+1));
        // Read enough additional data for the color table
        iBytesRead += (*pPage->pfnRead)(&pPage->GIFFile, &pPage->ucFileBuf[iBytesRead], j*3);            
        if (pPage->ucPaletteType == GIF_PALETTE_RGB565)
        {
            for (i=0; i<j; i++)
            {
                uint16_t usRGB565;
                usRGB565 = ((p[iOffset] >> 3) << 11); // R
                usRGB565 |= ((p[iOffset+1] >> 2) << 5); // G
                usRGB565 |= (p[iOffset+2] >> 3); // B
                if (pPage->ucLittleEndian)
                    pPage->pLocalPalette[i] = usRGB565;
                else
#if WIN32
                    pPage->pLocalPalette[i] = _byteswap_ushort(usRGB565); // SPI wants MSB first
#else
                    pPage->pLocalPalette[i] = __builtin_bswap16(usRGB565); // SPI wants MSB first
#endif
                iOffset += 3;
            }
        }
        else // just copy it as-is
        {
            c_memcpy(pPage->pLocalPalette, &p[iOffset], j * 3);
            iOffset += j*3;
        }
        pPage->bUseLocalPalette = 1;
    }
    pPage->ucCodeStart = p[iOffset++]; /* initial code size */
    /* Since GIF can be 1-8 bpp, we only allow 1,4,8 */
    pPage->iBpp = cGIFBits[pPage->ucCodeStart];
    // we are re-using the same buffer turning GIF file data
    // into "pure" LZW
   pPage->iLZWSize = 0; // we're starting with no LZW data yet
   c = 1; // get chunk length
   while (c && iOffset < iBytesRead)
   {
//     Serial.printf("iOffset=%d, iBytesRead=%d\n", iOffset, iBytesRead);
     c = p[iOffset++]; // get chunk length
//     Serial.printf("Chunk size = %d\n", c);
     if (c <= (iBytesRead - iOffset))
     {
       c_memcpy(&pPage->ucLZW[pPage->iLZWSize], &p[iOffset], c);
       pPage->iLZWSize += c;
       iOffset += c;
     }
     else // partial chunk in our buffer
     {
       int iPartialLen = (iBytesRead - iOffset);
       c_memcpy(&pPage->ucLZW[pPage->iLZWSize], &p[iOffset], iPartialLen);
       pPage->iLZWSize += iPartialLen;
       iOffset += iPartialLen;
       (*pPage->pfnRead)(&pPage->GIFFile, &pPage->ucLZW[pPage->iLZWSize], c - iPartialLen);
       pPage->iLZWSize += (c - iPartialLen);
     }
     if (c == 0)
        pPage->bEndOfFrame = 1; // signal not to read beyond the end of the frame
   }
// seeking on an SD card is VERY VERY SLOW, so use the data we've already read by de-chunking it
// in this case, there's too much data, so we have to seek backwards a bit
   if (iOffset < iBytesRead)
   {
//     Serial.printf("Need to seek back %d bytes\n", iBytesRead - iOffset);
     (*pPage->pfnSeek)(&pPage->GIFFile, iStartPos + iOffset); // position file to new spot
   }
    return 1; // we are now at the start of the chunk data
} /* GIFParseInfo() */
//
// Gather info about an animated GIF file
//
int GIFGetInfo(GIFIMAGE *pPage, GIFINFO *pInfo, unsigned short *pPalette)
{
    int iOff, iNumFrames;
    int iDelay, iMaxDelay, iMinDelay, iTotalDelay;
    int iReadAmount;
    int iDataAvailable = 0;
    int iDataRemaining = 0;
    uint32_t lFileOff = 0;
    int bDone = 0;
    int bExt;
    uint8_t c, *cBuf;
	uint16_t sGlobalColorTableSize = 0;
	uint8_t ucHasLocalColorTable = 0, ucHasTransparent = 0, ucBackground = 0;

    iMaxDelay = iTotalDelay = 0;
    iMinDelay = 10000;
    iNumFrames = 1;
    iDataRemaining = pPage->GIFFile.iSize;
    cBuf = (uint8_t *) pPage->ucFileBuf;
    (*pPage->pfnSeek)(&pPage->GIFFile, 0);
    iDataAvailable = (*pPage->pfnRead)(&pPage->GIFFile, cBuf, FILE_BUF_SIZE);
    iOff = 10;
    c = cBuf[iOff]; // get info bits
    ucBackground = cBuf[iOff + 1];
    iOff += 3;   /* Skip flags, background color & aspect ratio */
    if (c & 0x80) /* Deal with global color table */
    {
        c &= 7;  /* Get the number of colors defined */
        sGlobalColorTableSize = 2 << c;
        if (pPalette) {
			unsigned short i;
			for (i=0; i<sGlobalColorTableSize; i++)
			{
				pPalette[i] =
					((cBuf[iOff] >> 3) << 11) |
					((cBuf[iOff+1] >> 2) << 5) |
					(cBuf[iOff+2] >> 3);
				iOff += 3;
			}
        }
        else
			iOff += sGlobalColorTableSize*3; /* skip color table */
    }
    while (!bDone) // && iNumFrames < MAX_FRAMES)
    {
        bExt = 1; /* skip extension blocks */
        while (bExt && iOff < iDataAvailable)
        {
            switch(cBuf[iOff])
            {
                case 0x3b: /* End of file */
                    /* we were fooled into thinking there were more pages */
                    iNumFrames--;
                    goto gifpagesz;
    // F9 = Graphic Control Extension (fixed length of 4 bytes)
    // FE = Comment Extension
    // FF = Application Extension
    // 01 = Plain Text Extension
                case 0x21: /* Extension block */
                    if (cBuf[iOff+1] == 0xf9 && cBuf[iOff+2] == 4) // Graphic Control Extension
                    {
                       //cBuf[iOff+3]; // page disposition flags
                        iDelay = cBuf[iOff+4]; // delay low byte
                        iDelay |= (cBuf[iOff+5] << 8); // delay high byte
                        if (iDelay < 2) // too fast, provide a default
                            iDelay = 2;
                        iDelay *= 10; // turn JIFFIES into milliseconds
                        iTotalDelay += iDelay;
                        if (iDelay > iMaxDelay) iMaxDelay = iDelay;
                        else if (iDelay < iMinDelay) iMinDelay = iDelay;
                       // (cBuf[iOff+6]; // transparent color index
                        if ((cBuf[iOff+3] & 1) && (ucBackground == cBuf[iOff+6]))
							ucHasTransparent = 1;
                    }
                    iOff += 2; /* skip to length */
                    iOff += (int)cBuf[iOff]; /* Skip the data block */
                    iOff++;
                   // block terminator or optional sub blocks
                    c = cBuf[iOff++]; /* Skip any sub-blocks */
                    while (c && iOff < (iDataAvailable - c))
                    {
                        iOff += (int)c;
                        c = cBuf[iOff++];
                    }
                    if (c != 0) // problem, we went past the end
                    {
                        iNumFrames--; // possible corrupt data; stop
                        goto gifpagesz;
                    }
                    break;
                case 0x2c: /* Start of image data */
                    bExt = 0; /* Stop doing extension blocks */
                    break;
                default:
                   /* Corrupt data, stop here */
                    iNumFrames--;
                    goto gifpagesz;
            } // switch
        } // while
        if (iOff >= iDataAvailable) // problem
        {
             iNumFrames--; // possible corrupt data; stop
             goto gifpagesz;
        }
          /* Start of image data */
        c = cBuf[iOff+9]; /* Get the flags byte */
        iOff += 10; /* Skip image position and size */
        if (c & 0x80) /* Local color table */
        {
            c &= 7;
            iOff += (2<<c)*3;
            ucHasLocalColorTable = 1;
        }
        iOff++; /* Skip LZW code size byte */
        c = cBuf[iOff++];
        while (c) /* While there are more data blocks */
        {
            if (iOff > (3*FILE_BUF_SIZE/4) && iDataRemaining > 0) /* Near end of buffer, re-align */
            {
                lFileOff += iOff; /* adjust total file pointer */
                iDataRemaining -= iOff;
                iReadAmount = (iDataRemaining > FILE_BUF_SIZE) ? FILE_BUF_SIZE:iDataRemaining;
                (*pPage->pfnSeek)(&pPage->GIFFile, lFileOff);
                iDataAvailable = (*pPage->pfnRead)(&pPage->GIFFile, cBuf, iReadAmount);
                iOff = 0; /* Start at beginning of buffer */
            }
            iOff += (int)c;  /* Skip this data block */
            if ((int)lFileOff + iOff > pPage->GIFFile.iSize) // past end of file, stop
            {
                iNumFrames--; // don't count this page
                break; // last page is corrupted, don't use it
            }
            c = cBuf[iOff++]; /* Get length of next */
        }
        /* End of image data, check for more pages... */
        if (((int)lFileOff + iOff > pPage->GIFFile.iSize) || cBuf[iOff] == 0x3b)
        {
            bDone = 1; /* End of file has been reached */
        }
        else /* More pages to scan */
        {
            iNumFrames++;
            if (!pInfo) goto gifpagesz;		//@	signal to stop on first frame
             // read new page data starting at this offset
            if (pPage->GIFFile.iSize > FILE_BUF_SIZE && iDataRemaining > 0) // since we didn't read the whole file in one shot
            {
                lFileOff += iOff; /* adjust total file pointer */
                iDataRemaining -= iOff;
                iReadAmount = (iDataRemaining > FILE_BUF_SIZE) ? FILE_BUF_SIZE : iDataRemaining;
                (*pPage->pfnSeek)(&pPage->GIFFile, lFileOff);
                iDataAvailable = (*pPage->pfnRead)(&pPage->GIFFile, cBuf, iReadAmount);
                iOff = 0; /* Start at beginning of buffer */
            }
        }
    } /* while !bDone */
gifpagesz:
	if (pInfo) {
		pInfo->iFrameCount = iNumFrames;
		pInfo->iMaxDelay = iMaxDelay;
		pInfo->iMinDelay = iMinDelay;
		pInfo->iDuration = iTotalDelay;
		pInfo->sGlobalColorTableSize = sGlobalColorTableSize;
		pInfo->ucHasLocalColorTable = ucHasLocalColorTable;
		pInfo->ucHasTransparent = ucHasTransparent;
	}
    return iNumFrames ? 1 : 0;
} /* GIFGetInfo() */

//
// Unpack more chunk data for decoding
// returns 1 to signify more data available for this image
// 0 indicates there is no more data
//
static int GIFGetMoreData(GIFIMAGE *pPage)
{
    int iDelta = (pPage->iLZWSize - pPage->iLZWOff);
    unsigned char c = 1;
    // move any existing data down
    if (pPage->bEndOfFrame ||  iDelta >= (LZW_BUF_SIZE - MAX_CHUNK_SIZE) || iDelta <= 0)
        return 1; // frame is finished or buffer is already full; no need to read more data
    if (pPage->iLZWOff != 0)
    {
// NB: memcpy() fails on some systems because the src and dest ptrs overlap
// so copy the bytes in a simple loop to avoid problems
      for (int i=0; i<pPage->iLZWSize - pPage->iLZWOff; i++) {
         pPage->ucLZW[i] = pPage->ucLZW[i + pPage->iLZWOff];
      }
      pPage->iLZWSize -= pPage->iLZWOff;
      pPage->iLZWOff = 0;
    }
    while (c && pPage->GIFFile.iPos < pPage->GIFFile.iSize && pPage->iLZWSize < (LZW_BUF_SIZE-MAX_CHUNK_SIZE))
    {
        (*pPage->pfnRead)(&pPage->GIFFile, &c, 1); // current length
        (*pPage->pfnRead)(&pPage->GIFFile, &pPage->ucLZW[pPage->iLZWSize], c);
        pPage->iLZWSize += c;
    }
    if (c == 0) // end of frame
        pPage->bEndOfFrame = 1;
    return (c != 0 && pPage->GIFFile.iPos < pPage->GIFFile.iSize); // more data available?
} /* GIFGetMoreData() */
#if 0
//
// Handle transparent pixels and disposal method
// Used only when a frame buffer is allocated
//
static void DrawNewPixels(GIFIMAGE *pPage, GIFDRAW *pDraw)
{
    uint8_t *d, *s;
    int x, iPitch = pPage->iCanvasWidth;

    s = pDraw->pPixels;
    d = &pPage->pFrameBuffer[pDraw->iX + (pDraw->y + pDraw->iY)  * iPitch]; // dest pointer in our complete canvas buffer
    if (pDraw->ucDisposalMethod == 2) // restore to background color
    {
        memset(d, pDraw->ucBackground, pDraw->iWidth);
    }
    // Apply the new pixels to the main image
    if (pDraw->ucHasTransparency) // if transparency used
    {
        uint8_t c, ucTransparent = pDraw->ucTransparent;
        for (x=0; x<pDraw->iWidth; x++)
        {
            c = *s++;
            if (c != ucTransparent)
                *d = c;
            d++;
        }
    }
    else
    {
        c_memcpy(d, s, pDraw->iWidth); // just overwrite the old pixels
    }
} /* DrawNewPixels() */
//
// Convert current line of pixels through the palette
// to either RGB565 or RGB888 output
// Used only when a frame buffer has been allocated
//
static void ConvertNewPixels(GIFIMAGE *pPage, GIFDRAW *pDraw)
{
    uint8_t *d, *s;
    int x;

    s = &pPage->pFrameBuffer[(pPage->iCanvasWidth * (pDraw->iY + pDraw->y)) + pDraw->iX];
    d = &pPage->pFrameBuffer[pPage->iCanvasHeight * pPage->iCanvasWidth]; // point past bottom of frame buffer
    if (pPage->ucPaletteType == GIF_PALETTE_RGB565)
    {
        uint16_t *pPal, *pu16;
        pPal = (uint16_t *)pDraw->pPalette;
        pu16 = (uint16_t *)d;
        for (x=0; x<pPage->iWidth; x++)
        {
            *pu16++ = pPal[*s++]; // convert to RGB565 pixels
        }
    }
    else
    {
        uint8_t *pPal;
        int pixel;
        pPal = (uint8_t *)pDraw->pPalette;
        for (x=0; x<pPage->iWidth; x++)
        {
            pixel = *s++;
            *d++ = pPal[(pixel * 3) + 0]; // convert to RGB888 pixels
            *d++ = pPal[(pixel * 3) + 1];
            *d++ = pPal[(pixel * 3) + 2];
        }
    }
} /* ConvertNewPixels() */
#endif

//
// GIFMakePels
//
static void GIFMakePels(GIFIMAGE *pPage, unsigned int code)
{
    int iPixCount;
    unsigned short *giftabs;
    unsigned char *buf, *s, *pEnd, *gifpels;
    unsigned char ucNeedMore = 0;
    /* Copy this string of sequential pixels to output buffer */
    //   iPixCount = 0;
    s = pPage->ucFileBuf + FILE_BUF_SIZE; /* Pixels will come out in reversed order */
    buf = pPage->ucLineBuf + (pPage->iWidth - pPage->iXCount);
    giftabs = pPage->usGIFTable;
    gifpels = &pPage->ucGIFPixels[PIXEL_LAST];
    while (code < LINK_UNUSED)
    {
        if (s == pPage->ucFileBuf) /* Houston, we have a problem */
        {
            return; /* Exit with error */
        }
        *(--s) = gifpels[code];
        code = giftabs[code];
    }
    iPixCount = (int)(intptr_t)(pPage->ucFileBuf + FILE_BUF_SIZE - s);
    
    while (iPixCount && pPage->iYCount > 0)
    {
        if (pPage->iXCount > iPixCount)  /* Pixels fit completely on the line */
        {
                //            memcpy(buf, s, iPixCount);
                //            buf += iPixCount;
                pEnd = buf + iPixCount;
                while (buf < pEnd)
                {
                    *buf++ = *s++;
                }
            pPage->iXCount -= iPixCount;
            //         iPixCount = 0;
            if (ucNeedMore)
                GIFGetMoreData(pPage); // check if we need to read more LZW data every 4 lines
            return;
        }
        else  /* Pixels cross into next line */
        {
            GIFDRAW gd;
            pEnd = buf + pPage->iXCount;
            while (buf < pEnd)
            {
                *buf++ = *s++;
            }
            iPixCount -= pPage->iXCount;
            pPage->iXCount = pPage->iWidth; /* Reset pixel count */
            // Prepare GIDRAW structure for callback
            gd.iX = pPage->iX;
            gd.iY = pPage->iY;
            gd.iWidth = pPage->iWidth;
            gd.iHeight = pPage->iHeight;
            gd.pPixels = pPage->ucLineBuf;
            gd.pPalette = (pPage->bUseLocalPalette) ? pPage->pLocalPalette : pPage->pPalette;
//            gd.pPalette24 = (uint8_t *)gd.pPalette; // just cast the pointer for RGB888
            gd.y = pPage->iHeight - pPage->iYCount;
            // Ugly logic to handle the interlaced line position, but it
            // saves having to have another set of state variables
            if (pPage->ucMap & 0x40) { // interlaced?
               int height = pPage->iHeight-1;
               if (gd.y > height / 2)
                  gd.y = gd.y * 2 - (height | 1);
               else if (gd.y > height / 4)
                  gd.y = gd.y * 4 - ((height & ~1) | 2);
               else if (gd.y > height / 8)
                  gd.y = gd.y * 8 - ((height & ~3) | 4);
               else
                  gd.y = gd.y * 8;
            }
            gd.ucDisposalMethod = (pPage->ucGIFBits & 0x1c)>>2;
            gd.ucTransparent = pPage->ucTransparent;
            gd.ucHasTransparency = pPage->ucGIFBits & 1;
            gd.ucBackground = pPage->ucBackground;
//            if (pPage->pFrameBuffer) // update the frame buffer
//            {
//                DrawNewPixels(pPage, &gd);
//                if (pPage->ucDrawType == GIF_DRAW_COOKED)
//                {
//                    ConvertNewPixels(pPage, &gd); // prepare for output
//                    gd.pPixels = &pPage->pFrameBuffer[pPage->iCanvasWidth * pPage->iCanvasHeight];
//                }
//            }
            (*pPage->pfnDraw)(&gd); // callback to handle this line
            pPage->iYCount--;
            buf = pPage->ucLineBuf;
            if ((pPage->iYCount & 3) == 0) // since we support only small images...
                ucNeedMore = 1;
        }
    } /* while */
    if (ucNeedMore)
        GIFGetMoreData(pPage); // check if we need to read more LZW data every 4 lines
    return;
} /* GIFMakePels() */
//
// Macro to extract a variable length code
//
#define GET_CODE if (bitnum > (REGISTER_WIDTH - codesize)) { pImage->iLZWOff += (bitnum >> 3); \
            bitnum &= 7; ulBits = INTELLONG(&p[pImage->iLZWOff]); } \
        code = (unsigned short) (ulBits >> bitnum); /* Read a 32-bit chunk */ \
        code &= sMask; bitnum += codesize;

//
// Decode LZW into an image
//
static int DecodeLZW(GIFIMAGE *pImage, int iOptions)
{
    int i, bitnum;
    unsigned short oldcode, codesize, nextcode, nextlim;
    unsigned short *giftabs, cc, eoi;
    signed short sMask;
    unsigned char *gifpels, *p;
    //    int iStripSize;
    //unsigned char **index;
    uint32_t ulBits;
    unsigned short code;
    (void)iOptions; // not used for now
    // if output can be used for string table, do it faster
    //       if (bGIF && (OutPage->cBitsperpixel == 8 && ((OutPage->iWidth & 3) == 0)))
    //          return PILFastLZW(InPage, OutPage, bGIF, iOptions);
    p = pImage->ucLZW; // un-chunked LZW data
    sMask = 0xffff << (pImage->ucCodeStart + 1);
    sMask = 0xffff - sMask;
    cc = (sMask >> 1) + 1; /* Clear code */
    eoi = cc + 1;
    giftabs = pImage->usGIFTable;
    gifpels = pImage->ucGIFPixels;
    pImage->iYCount = pImage->iHeight; // count down the lines
    pImage->iXCount = pImage->iWidth;
    bitnum = 0;
    pImage->iLZWOff = 0; // Offset into compressed data
    GIFGetMoreData(pImage); // Read some data to start

    // Initialize code table
    // this part only needs to be initialized once
    for (i = 0; i < cc; i++)
    {
        gifpels[PIXEL_FIRST + i] = gifpels[PIXEL_LAST + i] = (unsigned short) i;
        giftabs[i] = LINK_END;
    }
init_codetable:
    codesize = pImage->ucCodeStart + 1;
    sMask = 0xffff << (pImage->ucCodeStart + 1);
    sMask = 0xffff - sMask;
    nextcode = cc + 2;
    nextlim = (unsigned short) ((1 << codesize));
    // This part of the table needs to be reset multiple times
    memset(&giftabs[cc], LINK_UNUSED, (4096 - cc)*sizeof(short));
    ulBits = INTELLONG(&p[pImage->iLZWOff]); // start by reading 4 bytes of LZW data
    GET_CODE
    if (code == cc) // we just reset the dictionary, so get another code
    {
      GET_CODE
    }
    oldcode = code;
    GIFMakePels(pImage, code); // first code is output as the first pixel
    // Main decode loop
    while (code != eoi && pImage->iYCount > 0) // && y < pImage->iHeight+1) /* Loop through all lines of the image (or strip) */
    {
        GET_CODE
        if (code == cc) /* Clear code?, and not first code */
            goto init_codetable;
        if (code != eoi)
        {
                if (nextcode < nextlim) // for deferred cc case, don't let it overwrite the last entry (fff)
                {
                    giftabs[nextcode] = oldcode;
                    gifpels[PIXEL_FIRST + nextcode] = gifpels[PIXEL_FIRST + oldcode];
                    if (giftabs[code] == LINK_UNUSED) /* Old code */
                        gifpels[PIXEL_LAST + nextcode] = gifpels[PIXEL_FIRST + oldcode];
                    else
                        gifpels[PIXEL_LAST + nextcode] = gifpels[PIXEL_FIRST + code];
                }
                nextcode++;
                if (nextcode >= nextlim && codesize < 12)
                {
                    codesize++;
                    nextlim <<= 1;
                    sMask = (sMask << 1) | 1;
                }
            GIFMakePels(pImage, code);
            oldcode = code;
        }
    } /* while not end of LZW code stream */
    return 0;
//gif_forced_error:
//    free(pImage->pPixels);
//    pImage->pPixels = NULL;
//    return -1;
} /* DecodeLZW() */
