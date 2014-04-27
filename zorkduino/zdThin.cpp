
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

#include "Arduino.h"
#include "zdThin.h"

extern uint8_t sector_data[512];
#define _buffer sector_data

// Stripped down Fat32 implementation

static u16 GET16(const u8* p)
{
    return *((u16*)p);
}

static u32 GET32(const u8* p)
{
    return *((u32*)p);
}

u8 Fat::Init()
{
    u8* buf = _buffer;
    if (readSector(buf, 0) != 0)
        return 0;

    u8 partitionType = buf[450];
    u32 bootSector = GET32(buf + 454);
    bool valid = false;
    switch (partitionType)
    {
        case 0x00:
            break;

        case 0x0B:  // FAT 32
        case 0x0C:  // FAT 32
        case 0x0E:
        case 0x0F:
            valid = true;
            break;

        default:
            valid = partitionType < 7;
    }

    if (!valid)
        bootSector = 0; // might not have a partition at start of disk (USB drives etc)

    if (readSector(buf, bootSector) != 0)
        return 0;

    if (GET16(buf + 11) != 512) // bytes per sector
        return 0;

    sectorsPerCluster = buf[13];
    u16 reservedSectors = GET16(buf + 14);
    u16 numFats = buf[16];
    u16 rootDirectectoryCount = GET16(buf+17);
    fatCount = GET16(buf+22);

    if (fatCount == 0)
    {
        fatCount = GET32(buf+36);   // is FAT32
        rootCluster = GET32(buf+44);
    } else
        rootCluster = 0;    // Indicates FAT16

    fatStart = bootSector + reservedSectors;
    rootStart = fatStart + fatCount*numFats;
    rootCount = rootDirectectoryCount >> 4;
    clusterStart = rootStart + rootCount;

    //  Calculate root count for FAT32 
    if (rootCluster)
    {
        Extent extents[1];
        extents[0].start = 2;
        extents[0].count = 1;
        LoadExtents(extents,1);
        rootCount = extents[0].count * sectorsPerCluster; // # of sectors in first cluster of root
    }
    return rootCluster ? FAT_32 : FAT_16;
}

//  Traverse Directory
//  TODO: This won't work properly on fragmented root directories; unlikely in our application
int Fat::Directory(DirectoryProc directoryProc, void* ref)
{
    for (u16 i = 0; i < rootCount; i++)
    {
        if (readSector(_buffer, rootStart + i))
            return -1;  // Dir read failed

        u8 d = 16;
        DirectoryEntry* entry = (DirectoryEntry*)_buffer;
        while (d--)
        {
            if (entry->fatname[0] == 0)
                return -1;
            
            if ((u8)entry->fatname[0] == 0xE5 || (entry->attributes & 0x18))
            {
                //  deleted, dir, volume label etc
            } else {
                //  file
                if (directoryProc(entry,i,ref))
                    return i;
            }
            entry++;
        }
    }
    return -1;
}

typedef struct
{
    char name[11];
    DirectoryEntry* entry;
} MatchName;

static bool MatchProc(DirectoryEntry* entry, int i, void* ref)
{
    MatchName* matchName = (MatchName*)ref;
    if (memcmp(entry->fatname, matchName->name, 11))
        return false;
    *matchName->entry = *entry;
    return true;
}

u32 Fat::NextCluster(u32* s, u32 currentCluster)
{
    bool fat32 = rootCluster != 0;
    u32 fatSector = currentCluster >> (fat32 ? 7 : 8);
    fatSector += fatStart;
    if (fatSector != *s)
    {
        *s = fatSector;
        readSector(_buffer,fatSector);  // Read sector containing next cluster
    }
    if (fat32)
        return ((u32*)_buffer)[currentCluster & 0x7F];
    return ((u16*)_buffer)[currentCluster & 0xFF];
}

//  File is bigger than a single cluster
//  Load first 3 extents
//  Bad news: Opening a very large file takes a long time
//  Good news: unless file is badly fragmented, subsequent acceses will be much faster
void Fat::LoadExtents(Extent* extents, int maxExtents)
{
    u8 i = 0;
    u32 state = 0;
    u32 current = extents[0].start;
    for (;;)
    {
        u32 next = NextCluster(&state,current);
        if (next != current + 1)
        {
            if (i == maxExtents-1)
                break;
            if (next == (rootCluster ? 0x0FFFFFFF : 0xFFFF))
                break;
            extents[++i].start = next;
            extents[i].count = 0;
        }
        extents[i].count++;
        current = next;
    }
    while (i < maxExtents-1)
        extents[++i].start = 0;
}

#define UPPER(_x) ((_x >= 'a' && _x <= 'z') ? (_x + 'A' - 'a') : _x)

void ToFAT(char fatname[11], const char* path)
{
    //  Convert name to 8.3 internal format
    u8 i = 11;
    while (i--)
        fatname[i] = ' ';
    i = 0;
    bool ext = false;
    while (i < 11)
    {
        char c = *path++;
        if (c == 0)
            break;
        c = UPPER(c);
        if (c == '.')
        {
            i = 8;
            ext = true;
        }
        else
        {
            if (i < 8 || ext)
                fatname[i++] = c;
        }
    }
}

//  Find a file entry
bool Fat::Find(const char* path, DirectoryEntry* entry)
{
    MatchName matchName;
    ToFAT(matchName.name,path);
    matchName.entry = entry;
    int index = Directory(MatchProc,&matchName);
    return index != -1;
}

//  Load extents when file is open. TODO: short check for very large files
bool Fat::Open(const char* path, ExtentInfo* extent)
{
    DirectoryEntry entry;

    if (!Find(path,&entry))
        return false;

    extent->fileLength = entry.length;
    u32 cluster = entry.clusterH;
    extent->extents[0].start = (cluster << 16) | entry.clusterL;
    extent->extents[0].count = 1;
    extent->extents[1].start = 0;
    if (extent->fileLength > (sectorsPerCluster<<9))
        LoadExtents(extent->extents,3);
    return true;
}

//  TODO: read beyond 3 extent!
//  TODO: Store extents as absolute sectors rather than clusters, avoid muls

bool Fat::FindSector(u32* sector, ExtentInfo* extent)
{
    u8 i = 0;
    while (i < 3)
    {
        u32 s = extent->extents[i].count*sectorsPerCluster;
        if (*sector < s)
        {
            u32 cluster = extent->extents[i].start;
            *sector += clusterStart + (cluster - 2)*sectorsPerCluster;
            return true;
        }
        *sector -= s;
        i++;
    }
    // TODO: Deal with nasty fragmentation
    return false;
}

bool Fat::Open(const char* path, uint32_t* startSector, uint32_t* fileLength)
{
  ExtentInfo extent;
  if (!Open(path,&extent))
    return false;
  *fileLength = extent.fileLength;
  *startSector = 0;
  return FindSector(startSector,&extent);
}
