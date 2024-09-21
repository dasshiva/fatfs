#include <stdint.h>

typedef struct __attribute__((packed)) __header {
	uint8_t  boot[3];   // Three byte jmp instruction
	uint8_t  name[8];   // Implementer's name
	uint16_t bps;       // bytes per sector
	uint8_t  spc;       // sectors per cluster
	uint16_t rsvd;      // Reserved sectors
	uint8_t  nfats;     // Number of FATs
	uint16_t _root;     // Root cluster entry = 0 (we don't use this in FAT32)
	uint16_t _n0;       // Total sectors (unused in FAT16)
	uint8_t  media;     // Media type = 0xFF
	uint8_t _n1[2];     // Not used in FAT32 = 0
	uint16_t _n2;       // Not used because we don't care about CHS addressing
	uint16_t _n3;       // Same as above
	uint32_t _n4;       // Same as above
	uint32_t total;     // The actual number of sectors relevant for us
	uint32_t spf;       // Sectors occupied by one FAT 
	uint16_t flags;     // Flags for this filesystem
	uint16_t ver;       // Filesystem version
	uint32_t root;      // The root cluster entry we care about
	uint16_t fsinfo;    // FSINFO structure location
	uint16_t bkup;      // Backup bootsector location
	uint64_t rsvd1;     // Reserved field
	uint32_t rsvd2;     // Reserved field 2
	uint8_t  drv;       // Drive number
	uint8_t  rsvd3;     // Reserved field 3
	uint8_t  sig;       // Boot signature = 0x29
	uint32_t volid;     // Volume ID
	uint8_t  label[11]; // Filesystem label
	uint8_t  type[8];   // (Unused) String representing file type
	uint8_t  info[420]; // Actually used for storing boot code but we can use this as we like
	uint16_t sign;      // Marks bootable region equal to 0xAA55
} header;

typedef uint32_t fatent; //  A entry in the FAT

typedef struct __attribute__((packed)) __dirent {
	uint8_t  name[64];        // File name 64 characters in ASCII or 32 characters of UNICODE 
	uint64_t atime;           // Access time from epoch
	uint64_t mtime;           // Modification time from epoch
	uint64_t ctime;           // Creation time from epoch
	uint64_t attributes;      // Attributes associated with file or directory 
	uint64_t size;            // Either size of  file in bytes or number of entries in directory
	uint32_t cluster;         // First cluster allocated to file/directory
	uint64_t hash;            // xxHash64 for name
	uint64_t hash_store;      // xxHash64 for stored data to check for inconsistencies
	uint32_t crc32;           // CRC for the dirent structure to detect inconsistencies
} dirent;

#include <memory.h>

void initialise_header(header* hdr, uint64_t size) {
	// Initialise everything to zero
	// This prevents us from zero initialising unused fields by ourselves
	// Also means that if you don't see any fields initialised here, they are implicitly zero
	memset(hdr, 0, sizeof(header));
	// Assembles to:
	// 	jmp short $(LOAD_ADDRESS) 
	// 	nop
	// Effectively putting the processor in an infinite loop, 
	// this is intentional because we don't intend operating systems to boot off this filesystem
	hdr->boot[0] = 0xEB;
	hdr->boot[1] = 0xFE;
	hdr->boot[2] = 0x90;
	memcpy(hdr->name, "MYFATFS ", 8);
	hdr->bps = 512; // Fixed constant
	hdr->spc = 1;   // 1 cluster per sector so effectively sectors are equivalent to clusters from this point on
	hdr->nfats = 1; // For now
	hdr->media = 0xFF; // Fixed constant
	hdr->total = (uint32_t) (size / 512);
	hdr->spf = (hdr->total * sizeof(fatent)) / 512 + 1;
	hdr->sign = 0xAA55;
}

#include <stdio.h>
#include <stdlib.h>

#ifdef DEBUG
#define BUG_IF(cond, ...) if (cond) { printf("Bug detected: "  __VA_ARGS__); exit(1); }
#else
#define BUG_IF(cond, ...) if (cond) { printf("Implementation bug detected. Aborting."); exit(1); }
#endif

int main(int argc, const char** argv) {
	BUG_IF(sizeof(header) != 512, "Filesystem header is %d bytes instead 512 bytes", sizeof(header));
	BUG_IF(sizeof(dirent) != 128, "Directory info structure is %d bytes instead of 128 bytes", sizeof(dirent));
	if (argc < 2) 
		return 0;
	header hdr;
	initialise_header(&hdr, (1 << 20) * (1 << 9));
	FILE* file = fopen(argv[1], "wb");
	fwrite(&hdr, 512, 1, file);
	fatent t = 0;
	for (uint32_t i = 0; i < hdr.total; i++)
		fwrite(&t, 4, 1, file);
	for (uint32_t i = 0; i < hdr.total; i++) {
		for (uint16_t j = 0; j < 512; j += 4) {
			fwrite(&t,  4, 1, file);
		}
	}
	return 0;
}
