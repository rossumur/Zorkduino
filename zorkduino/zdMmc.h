/* Copyright (c) 2010, Peter Barrett  
**  
** Permission to use, copy, modify, and/or distribute this software for  
** any purpose with or without fee is hereby granted, provided that the  
** above copyright notice and this permission notice appear in all copies.  
**  
** THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL  
** WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED  
** WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR  
** BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES  
** OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,  
** WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,  
** ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS  
** SOFTWARE.  
*/

#define GO_IDLE_TIMEOUT   1
#define READ_OCR_TIMEOUT  2
#define BAD_VOLTAGE       3
#define BAD_PATTERN       4
#define OP_COND_TIMEOUT   5
#define SET_BLOCKLEN_TIMEOUT 6

#define MMC_NOT_INITED    7
#define WRITE_FAILED      8
#define READ_FAILED       9

uint8_t MMC_Init();
uint8_t MMC_ReadSector(uint8_t *buffer, uint32_t sector);
uint8_t MMC_WriteSector(uint8_t *buffer, uint32_t sector);
