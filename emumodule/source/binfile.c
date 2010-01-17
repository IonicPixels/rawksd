#include <stdio.h>
#include <string.h>

#include "binfile.h"
#include "rijndael.h"
#include "gcutil.h"
#include "syscalls.h"
#include "es.h"
#include "mem.h"

void LogPrintf(const char *fmt, ...);

#define setAccessMask(mask, bit) (mask[bit>>3] |= (1 << (bit&7)))
#define unsetAccessMask(mask, bit) (mask[bit>>3] &= ~(1 << (bit&7)))
#define checkAccessMask(mask, bit) (mask[bit>>3] & (1 << (bit&7)))

#define BIN_READ	1
#define BIN_WRITE	2

#define FSERR_EINVAL -4

int verify_bk(BK_Header *bk)
{
	int failed=0;
	u32 wii_id = 0;
	os_get_key(ES_KEY_CONSOLE, &wii_id);

	if (bk->size != 0x70)
	{
		LogPrintf("bk_header.size incorrect %d\n", bk->size);
		failed = 1;
	}
	if (bk->magic != 0x426B)
	{
		LogPrintf("bk_header.magic incorrect %d\n", bk->magic);
		failed = 1;
	}
	if (bk->version != 1)
	{
		LogPrintf("bk_header.version incorrect %d\n", bk->version);
		failed = 1;
	}
	if (bk->NG_id != wii_id)
	{
		LogPrintf("bk_header.NG_id incorrect %08X\n", bk->NG_id);
		failed = 1;
	}
	if (bk->zeroes != 0)
	{
		LogPrintf("bk_header.zeroes not zero %08X\n", bk->zeroes);
		failed = 1;
	}
	if (bk->title_id_1 != 0x00010000)
	{
		LogPrintf("bk_header.title_id_1 incorrect %08X\n", bk->title_id_1);
		failed =1;
	}
	if (bk->title_id_2 != *(u32*)0)
	{
		LogPrintf("bk_header.title_id_2 incorrect %08X %08X\n", bk->title_id_2, *(u32*)0);
		failed = 1;
	}
	if (bk->padding != 0)
	{
		LogPrintf("bk_header.padding incorrect %08X\n", bk->padding);
		failed =1;
	}

	if (!failed)
		LogPrintf("BK_Header verified\n");

	return failed;
}

int FileRead(BinFile* file, void *buf, u32 size)
{
	if (size==0)
		return 0;
	if (os_read(file->handle, buf, size)!=size)
		return 1;
	file->pos += size;
	return 0;
}

int FileWrite(BinFile* file, void *buf, u32 size)
{
	if (size==0)
		return 0;
	if (os_write(file->handle, buf, size)!=size)
		return 1;
	file->pos += size;
	return 0;
}

int FileSeek(BinFile* file, s32 where)
{
	if (os_seek(file->handle, where, SEEK_SET)!=where)
		return 0;
	file->pos = where;
	return 0;
}

BinFile* OpenBinRead(s32 file)
{
	u32 i, found=0;;
    BK_Header bk_header;
    tmd tmd_header;
    tmd_content content_rec;
    BinFile* binfile = Alloc(sizeof(BinFile));

    if (binfile==NULL)
    	return NULL;

    memset(binfile, 0, sizeof(BinFile));
    binfile->handle = file;
    binfile->mode = BIN_READ;

    if (FileSeek(binfile, 0) ||
		FileRead(binfile, &bk_header, sizeof(bk_header)) ||
    	FileSeek(binfile, 0x1C0) ||
    	FileRead(binfile, &tmd_header, sizeof(tmd)))
    	{
			LogPrintf("DLC Open failed reading bk_header\n");
    		goto open_error;
		}

    if (verify_bk(&bk_header))
    	goto open_error;

    binfile->header_size = ROUND_UP(bk_header.tmd_size+sizeof(bk_header), 64);
    binfile->data_size = bk_header.contents_size;

    for(i = 0; i < tmd_header.num_contents; i++)
    {
        if(checkAccessMask(bk_header.content_mask, i))
        {
			if (FileRead(binfile, &content_rec, sizeof(tmd_content)))
				goto open_error;
			binfile->index = content_rec.index;
			binfile->iv[0] = content_rec.index>>8;
			binfile->iv[1] = (u8)content_rec.index;
			if (bk_header.contents_size != ROUND_UP(content_rec.size, 64) ||
				content_rec.type != 0x4001)
				{
					LogPrintf("DLC Open size mismatch or contents not DLC\n");
				goto open_error;
				}
			found = 1;
			break;
        }
        else
            if (FileSeek(binfile, binfile->pos+sizeof(tmd_content)))
            	goto open_error;
    }

    if (!found)
    {
		LogPrintf("DLC Open failed to find index in TMD\n");
    	goto open_error;
	}

    if (FileSeek(binfile, binfile->header_size))
    	goto open_error;

    return binfile;
open_error:
	Dealloc(binfile);
	return NULL;
}

void CloseBin(BinFile* file)
{
    Dealloc(file);
}

s32 SeekBin(BinFile* file, s32 where, u32 origin)
{
	s32 result = FSERR_EINVAL;

	if (file && file->mode==BIN_READ)
	{
		switch(origin)
		{
			case SEEK_SET:
			{
				where += file->header_size;
				break;
			}
			case SEEK_CUR:
			{
				where += file->pos;
				break;
			}
			case SEEK_END:
			{
				where += file->header_size + file->data_size;
			}
			default:
				return result;
		}

		if (where == file->pos) // check if we're already there
			return where - file->header_size;

		// make sure we're not before the beginning, then seek
		// 16 bytes backwards to read the iv
		if (where >= file->header_size && !FileSeek(file, (where&~15)-16))
		{
			// use index for iv?
			if (file->pos < file->header_size+16)
			{
				memset(file->iv, 0, 16);
				file->iv[0] = file->index>>8;
				file->iv[1] = (u8)file->index;
				if (FileSeek(file, file->pos+16))
					return result;
			}
			else if (FileRead(file, file->iv, 16))
				return result;

			// decrypt initial bytes of block
			if (where&0xF)
			{
				if (FileRead(file, file->buf, 16))
					return result;
				file->pos -= (16-(where&0xF));
				dlc_aes_decrypt(file->iv, file->buf, file->buf, 16);
				memmove(file->buf, file->buf+(where&0xF), 16-(where&0xF));
			}
			result = where - file->header_size;
		}
	}

	return result;
}

s32 ReadBin(BinFile* file, u8* buffer, u32 numbytes)
{
	s32 result = FSERR_EINVAL;
	u32 i;

	if (file && file->mode==BIN_READ)
	{
		result = numbytes;

		// leading bytes, should already be decrypted in buffer
		i = MIN((16 - file->pos)&0xF, numbytes);
		memcpy(buffer, file->buf, i);
		numbytes -= i;
		buffer += i;

		// middle bytes (16-byte blocks)
		i = numbytes & ~15;
		if (FileRead(file, buffer, i))
			return FSERR_EINVAL;
		dlc_aes_decrypt(file->iv, buffer, buffer, i);
		numbytes -= i;
		buffer += i;

		// trailing bytes
		if (numbytes)
		{
			if (FileRead(file, file->buf, 16))
				return FSERR_EINVAL;
			file->pos -= 16 - numbytes;
			dlc_aes_decrypt(file->iv, file->buf, file->buf, 16);
			memcpy(buffer, file->buf, numbytes);
			memmove(file->buf, file->buf+numbytes, 16-numbytes);
		}
	}

	return result;
}

// THE FOLLOWING FUNCTIONS HAVE NOT BEEN TESTED

BinFile* CreateBinFile(u16 index, u8* tmd, u32 tmd_size, s32 file)
{
	BinFile *bin;

    BK_Header bk_header;

	if (!tmd || !tmd_size || index>=512)
		return NULL;

	bin = (BinFile*)Alloc(sizeof(BinFile));
	if (bin==NULL)
		return NULL;

	memset(bin, 0, sizeof(BinFile));
	bin->handle = file;
	bin->mode = BIN_WRITE;
	bin->iv[0] = index >> 16;
	bin->iv[1] = (u8)index;
	bin->header_size = ROUND_UP(tmd_size+sizeof(bk_header), 64);

	memset(&bk_header, 0, sizeof(bk_header));
    bk_header.size = sizeof(BK_Header);
    bk_header.magic = 0x426B /*'Bk'*/;
    bk_header.version = 1;
	os_get_key(ES_KEY_CONSOLE, &bk_header.NG_id);
    bk_header.tmd_size = tmd_size;
    bk_header.title_id_1 = 0x00010000;
    bk_header.title_id_2 = *(u32*)0;
    setAccessMask(bk_header.content_mask, index);

    if (FileSeek(bin, 0) ||
    	FileWrite(bin, &bk_header, sizeof(bk_header)) ||
    	FileSeek(bin, ROUND_UP(bin->pos, 64)) ||
    	FileWrite(bin, &tmd, tmd_size) ||
    	FileSeek(bin, ROUND_UP(bin->pos, 64)))
    	goto create_error;

    return bin;

create_error:
	Dealloc(bin);
	return NULL;
}

s32 WriteBin(BinFile* file, u8* buffer, u32 numbytes)
{
	s32 result = FSERR_EINVAL;

	if (file && file->mode == BIN_WRITE)
	{
		s32 bytes_left;
		result = numbytes;

		// leading bytes, up to 16-byte boundary
		bytes_left = MIN((16-file->pos)&0xF, numbytes);
		if (bytes_left)
		{
			memcpy(file->buf+(16-bytes_left), buffer, bytes_left);
			buffer += bytes_left;
			numbytes -= bytes_left;
			file->pos += bytes_left;
			if (!(file->pos&0xF))
			{
				file->pos -= 16;
				dlc_aes_encrypt(file->iv, file->buf, file->buf, 16);
				if (FileWrite(file, file->buf, 16))
					return FSERR_EINVAL;
			}
		}

		bytes_left = numbytes;
		while (bytes_left>0)
		{
			s32 crypt_bytes = MAX(16, bytes_left);
			memcpy(file->buf, buffer, crypt_bytes);
			buffer += crypt_bytes;
			bytes_left -= crypt_bytes;

			if (crypt_bytes==16)
			{
				dlc_aes_encrypt(file->iv, file->buf, file->buf, 16);
				if (FileWrite(file, file->buf, 16))
					return FSERR_EINVAL;
			}
			else
				file->pos += crypt_bytes;
		}

		file->data_size += numbytes;
	}

	return result;
}

void CloseWriteBin(BinFile* file)
{
	u8 zero=0;
	u32 total_size;
	u32 i;

	if (file==NULL)
		return;

	file->data_size = ROUND_UP(file->data_size, 64);
	total_size = file->header_size + file->data_size;

	for (i=file->pos; i < ROUND_UP(file->pos, 64); i++)
		FileWrite(file, &zero, 1);

	if (!FileSeek(file, 0x18))
	{
		if (!FileWrite(file, &file->data_size, sizeof(file->data_size)))
			FileWrite(file, &total_size, sizeof(total_size));
	}

	Dealloc(file);
}