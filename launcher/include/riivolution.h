#pragma once

#include <gccore.h>

#include <string>
#include <vector>

struct DiscNode
{
		u8 Type;
		u8 NameOffsetMSB;
		u16 NameOffset; //really a u24
		u32 DataOffset;
		u32 Size;
		u32 GetNameOffset() { return ((u32)NameOffsetMSB << 16) | NameOffset; }
		void SetNameOffset(u32 offset) { NameOffset = (u16)offset; NameOffsetMSB = offset >> 16; }
} __attribute__((packed));

namespace PatchType { enum Enum {
	Patch = 0,
	Shift,
	File
}; }

int RVL_Initialize();
void RVL_Close();
void RVL_SetFST(void* address, u32 size);
void* RVL_GetFST();
int RVL_SetClusters(bool clusters);
void RVL_SetAlwaysShift(bool shift);
int RVL_Allocate(PatchType::Enum type, int num);
int RVL_SetShiftBase(u64 shift);
int RVL_AddFile(const char* filename);
int RVL_AddFile(const char* filename, u64 identifier);
int RVL_AddShift(u64 original, u64 offset, u32 length);
int RVL_AddPatch(int file, u64 offset, u32 fileoffset, u32 length);
int RVL_AddEmu(const char* nandpath, const char* external);

u64 RVL_GetShiftOffset(u32 length);
DiscNode* RVL_FindNode(const char* fstname);

struct RiiDisc;

void RVL_Patch(RiiDisc* disc);
void RVL_PatchMemory(RiiDisc* disc);

static inline u64 RVL_GetShiftOffset() { return RVL_GetShiftOffset(0); }
