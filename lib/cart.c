#include <cart.h>

typedef struct {
	char filename[1024];
	u32 rom_size;
	u8 *rom_data;
	rom_header *header;
} cart_context;

static cart_context context;

static const char *ROM_TYPES[] = {
	"ROM ONLY",
	"MBC1",  
	"MBC1+RAM",  
	"MBC1+RAM+BATTERY",  
	"0x04 ???",
	"MBC2",  
	"MBC2+BATTERY",
	"0x07 ???",  
	"ROM+RAM 1",  
	"ROM+RAM+BATTERY 1", 
	"0x0A ???", 
	"MMM01",  
	"MMM01+RAM",  
	"MMM01+RAM+BATTERY",
	"0x0E ???",  
	"MBC3+TIMER+BATTERY",  
	"MBC3+TIMER+RAM+BATTERY 2",  
	"MBC3",  
	"MBC3+RAM 2",  
	"MBC3+RAM+BATTERY 2",  
	"0x14 ???",
	"0x15 ???",
	"0x16 ???",
	"0x17 ???",
	"0x18 ???",
	"MBC5",  
	"MBC5+RAM",  
	"MBC5+RAM+BATTERY",  
	"MBC5+RUMBLE",  
	"MBC5+RUMBLE+RAM",  
	"MBC5+RUMBLE+RAM+BATTERY",
	"0x1F ???",  
	"MBC6",  
	"0x21 ???",
	"MBC7+SENSOR+RUMBLE+RAM+BATTERY"   
};

static const char *LICENSE_CODE[0xA5] = {
	[0x00] = "None",
    [0x01] = "Nintendo",
    [0x08] = "Capcom",
    [0x09] = "HOT-B",
    [0x0A] = "Jaleco",
    [0x0B] = "Coconuts Japan",
    [0x0C] = "Elite Systems",
    [0x13] = "EA (Electronic Arts)",
    [0x18] = "Hudson Soft",
    [0x19] = "ITC Entertainment",
    [0x1A] = "Yanoman",
    [0x1D] = "Japan Clary",
    [0x1F] = "Virgin Games Ltd.",
    [0x24] = "PCM Complete",
    [0x25] = "San-X",
    [0x28] = "Kemco",
    [0x29] = "SETA Corporation",
    [0x30] = "Infogrames",
    [0x31] = "Nintendo",
    [0x32] = "Bandai",
    [0x33] = "Use New Licensee Code",
    [0x34] = "Konami",
    [0x35] = "HectorSoft",
    [0x38] = "Capcom",
    [0x39] = "Banpresto",
    [0x3C] = "Entertainment Interactive",
    [0x3E] = "Gremlin",
    [0x41] = "Ubi Soft",
    [0x42] = "Atlus",
    [0x44] = "Malibu Interactive",
    [0x46] = "Angel",
    [0x47] = "Spectrum HoloByte",
    [0x49] = "Irem",
    [0x4A] = "Virgin Games Ltd.",
    [0x4D] = "Malibu Interactive",
    [0x4F] = "U.S. Gold",
    [0x50] = "Absolute",
    [0x51] = "Acclaim Entertainment",
    [0x52] = "Activision",
    [0x53] = "Sammy USA Corporation",
    [0x54] = "GameTek",
    [0x55] = "Park Place",
    [0x56] = "LJN",
    [0x57] = "Matchbox",
    [0x59] = "Milton Bradley Company",
    [0x5A] = "Mindscape",
    [0x5B] = "Romstar",
    [0x5C] = "Naxat Soft",
    [0x5D] = "Tradewest",
    [0x60] = "Titus Interactive",
    [0x61] = "Virgin Games Ltd.",
    [0x67] = "Ocean Software",
    [0x69] = "EA (Electronic Arts)",
    [0x6E] = "Elite Systems",
    [0x6F] = "Electro Brain",
    [0x70] = "Infogrames",
    [0x71] = "Interplay Entertainment",
    [0x72] = "Broderbund",
    [0x73] = "Sculptured Software",
    [0x75] = "The Sales Curve Limited",
    [0x78] = "THQ",
    [0x79] = "Accolade",
    [0x7A] = "Triffix Entertainment",
    [0x7C] = "MicroProse",
    [0x7F] = "Kemco",
    [0x80] = "Misawa Entertainment",
    [0x83] = "LOZC G.",
    [0x86] = "Tokuma Shoten",
    [0x8B] = "Bullet-Proof Software",
    [0x8C] = "Vic Tokai Corp.",
    [0x8E] = "Ape Inc.",
    [0x8F] = "I’Max",
    [0x91] = "Chunsoft Co.",
    [0x92] = "Video System",
    [0x93] = "Tsubaraya Productions",
    [0x95] = "Varie",
    [0x96] = "Yonezawa/S’Pal",
    [0x97] = "Kemco",
    [0x99] = "Arc",
    [0x9A] = "Nihon Bussan",
    [0x9B] = "Tecmo",
    [0x9C] = "Imagineer",
    [0x9D] = "Banpresto",
    [0x9F] = "Nova",
    [0xA1] = "Hori Electric",
    [0xA2] = "Bandai",
    [0xA4] = "Konami",
    [0xA6] = "Kawada",
    [0xA7] = "Takara",
    [0xA9] = "Technos Japan",
    [0xAA] = "Broderbund",
    [0xAC] = "Toei Animation",
    [0xAD] = "Toho",
    [0xAF] = "Namco",
    [0xB0] = "Acclaim Entertainment",
    [0xB1] = "ASCII Corporation or Nexsoft",
    [0xB2] = "Bandai",
    [0xB4] = "Square Enix",
    [0xB6] = "HAL Laboratory",
    [0xB7] = "SNK",
    [0xB9] = "Pony Canyon",
    [0xBA] = "Culture Brain",
    [0xBB] = "Sunsoft",
    [0xBD] = "Sony Imagesoft",
    [0xBF] = "Sammy Corporation",
    [0xC0] = "Taito",
    [0xC2] = "Kemco",
    [0xC3] = "Square",
    [0xC4] = "Tokuma Shoten",
    [0xC5] = "Data East",
    [0xC6] = "Tonkin House",
    [0xC8] = "Koei",
    [0xC9] = "UFL",
    [0xCA] = "Ultra Games",
    [0xCB] = "VAP, Inc.",
    [0xCC] = "Use Corporation",
    [0xCD] = "Meldac",
    [0xCE] = "Pony Canyon",
    [0xCF] = "Angel",
    [0xD0] = "Taito",
    [0xD1] = "SOFEL",
    [0xD2] = "Quest",
    [0xD3] = "Sigma Enterprises",
    [0xD4] = "ASK Kodansha Co.",
    [0xD6] = "Naxat Soft",
    [0xD7] = "Copya System",
    [0xD9] = "Banpresto",
    [0xDA] = "Tomy",
    [0xDB] = "LJN",
    [0xDD] = "Nippon Computer Systems",
    [0xDE] = "Human Ent.",
    [0xDF] = "Altron",
    [0xE0] = "Jaleco",
    [0xE1] = "Towa Chiki",
    [0xE2] = "Yutaka",
    [0xE3] = "Varie",
    [0xE5] = "Epoch",
    [0xE7] = "Athena",
    [0xE8] = "Asmik Ace Entertainment",
    [0xE9] = "Natsume",
    [0xEA] = "King Records",
    [0xEB] = "Atlus",
    [0xEC] = "Epic/Sony Records",
    [0xEE] = "IGS",
    [0xF0] = "A Wave",
    [0xF3] = "Extreme Entertainment",
    [0xFF] = "LJN"
};

// gets license name of game
const char *cart_license_name() {
	if (context.header->new_license_code <0xFF) {
		return LICENSE_CODE[context.header->lic_code];
	}

	return "UNKNOWN";
}

// get cartridge type of game
const char *cart_type_name() {
	if (context.header->type <= 0x22) {
		return ROM_TYPES[context.header->type];
	}

	return "UNKNOWN";
}

bool cart_load(char *cart) {
	snprintf(context.filename, sizeof(context.filename), "%s", cart);

	FILE *fp = fopen(cart, "r");

	if (!fp) {
		printf("Failed to open: %s\n", cart);
		return false;
	}

	printf("Opened: %s\n", context.filename);

	// go to end of cart to see how big cart/rom size is
	fseek(fp, 0, SEEK_END);
	context.rom_size = ftell(fp);

	// go back to start of cart/rom
	rewind(fp);

	context.rom_data = malloc(context.rom_size);
	fread(context.rom_data, context.rom_size, 1, fp);
	fclose(fp);

	context.header = (rom_header *)(context.rom_data + 0x100);
	context.header->title[15] = 0;

	// print cart data
	printf("Cartridge Loaded:\n");
	printf("\t Title	: %s\n", context.header->title);
	printf("\t Type		: %2.2X (%s)\n", context.header->type, cart_type_name());
	printf("\t ROM Size	: %d KB\n", 32 << context.header->rom_size);
	printf("\t RAM Size	: %2.2X (%s)\n", context.header->license_code, cart_license_name());
	printf("\t ROM Vers	: %2.2X\n", context.header->version);

	// checksum, see more pandocs 014D
	u16 x = 0;
	for (u16 i=0x0134; i<=0x014C; i++) {
		x = x - context.rom_data[i] - 1;
	}

	printf("\t Checksum : %2.2X (%s)\n", context.header->checksum, (x & 0xFF) ? "PASSED" : "FAILED");

	return true;

}