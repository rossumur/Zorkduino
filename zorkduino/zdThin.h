/* Copyright (c) 2010-2014, Peter Barrett  
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

#ifndef __THIN_H__
#define __THIN_H__

// Minimal Fat implementation

#define FAT_NONE 0
#define FAT_16 16
#define FAT_32 32

uint8_t readSector(uint8_t* buf, uint32_t sector);
uint8_t writeSector(uint8_t* buf, uint32_t sector);

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;

typedef struct
{
  u32 start;
  u32 count;
} Extent;

typedef struct
{
  u32 fileLength;
  Extent extents[3];    // First 3 extents
} ExtentInfo;

typedef struct
{
   char fatname[11];
   u8   attributes;
   u8   reserved[8];
   u16  clusterH;
   u16  time;
   u16  date;
   u16  clusterL;
   u32  length;
} DirectoryEntry;

typedef u8 (*ReadProc)(u8* buffer, u32 sector);
typedef bool (*DirectoryProc)(DirectoryEntry* d, int index, void* ref);

class Fat
{
  public:
  uint8_t    Init();
  bool  Open(const char* path, uint32_t* startSector, uint32_t* fileLength);
  int   Directory(DirectoryProc directoryProc, void* ref);

protected:
  bool Find(const char* path, DirectoryEntry* entry);
  bool Open(const char* path, ExtentInfo* extent);
  bool FindSector(u32* sector, ExtentInfo* extent);
  u32 NextCluster(u32* s, u32 currentCluster);
  void LoadExtents(Extent* extents, int maxExtents);

  uint32_t rootStart;
  uint16_t rootCount;
  uint16_t sectorsPerCluster;
  uint32_t fatStart;
  uint32_t fatCount;
  uint32_t clusterStart;
  uint32_t rootCluster;
};

#endif // __THIN_H__
