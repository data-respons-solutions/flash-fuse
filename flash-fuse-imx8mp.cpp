#include <map>
#include <string>
#include <memory>
#include "flash-fuse-common.h"

/*
 * Offset calculations
 *
 * OCOTP value offset calculation:
 *   Offset=(Bank * 4 + Word) * 4
 * Example, MAC0 Bank9 Word 0
 *   (9 * 4 + 0) * 4 = 144 (0x90)
 * Example, MAC1 Bank9 Word 1
 *	 (9 * 4 + 1) * 4 = 148 (0x94)
 */

struct BANK {
	constexpr BANK(int v) : value(v) {}
	int value;
};

struct WORD {
	constexpr WORD(int w) : value(w) {}
	int value;
};

constexpr int OFFSET(BANK bank, WORD word)
{
	constexpr int word_per_bank = 4;
	constexpr int byte_per_word = 4;
	return ((bank.value * word_per_bank) + word.value) * byte_per_word;
}

constexpr int OCOTP_LOCK =		OFFSET(BANK(0), WORD(0)); // 0x400
constexpr int OCOTP_BOOT_CFG0 =	OFFSET(BANK(1), WORD(3)); // 0x470
constexpr int OCOTP_BOOT_CFG1 =	OFFSET(BANK(2), WORD(0)); // 0x480
constexpr int OCOTP_BOOT_CFG2 = OFFSET(BANK(2), WORD(1)); // 0x490
constexpr int OCOTP_SRK0 =		OFFSET(BANK(6), WORD(0)); // 0x580
constexpr int OCOTP_SRK1 =		OFFSET(BANK(6), WORD(1)); // 0x590
constexpr int OCOTP_SRK2 =		OFFSET(BANK(6), WORD(2)); // 0x5a0
constexpr int OCOTP_SRK3 =		OFFSET(BANK(6), WORD(3)); // 0x5b0
constexpr int OCOTP_SRK4 =		OFFSET(BANK(7), WORD(0)); // 0x5c0
constexpr int OCOTP_SRK5 =		OFFSET(BANK(7), WORD(1)); // 0x5d0
constexpr int OCOTP_SRK6 =		OFFSET(BANK(7), WORD(2)); // 0x5e0
constexpr int OCOTP_SRK7 =		OFFSET(BANK(7), WORD(3)); // 0x5f0
constexpr int OCOTP_MAC_ADDR0 =	OFFSET(BANK(9), WORD(0)); // 0x640
constexpr int OCOTP_MAC_ADDR1 =	OFFSET(BANK(9), WORD(1)); // 0x650
constexpr int OCOTP_MAC_ADDR2 =	OFFSET(BANK(9), WORD(2)); // 0x660

struct FlagFuseDesc {
	int offset;
	uint32_t mask;
	std::map<std::string, uint32_t> bits;
};

const std::map<std::string, FlagFuseDesc> flag_fuses = {
	{"MAC_ADDR_LOCK", {.offset = OCOTP_LOCK, .mask = MASK(14, 15), .bits = {
			{"NONE", 0x0},
			{"WP", BIT(14)},
			{"OP", BIT(15)},
			{"WP+OP", MASK(14, 15)},
		}
	}},
	{"USB_ID_LOCK", {.offset = OCOTP_LOCK, .mask = MASK(12, 13), .bits = {
			{"NONE", 0x0},
			{"WP", BIT(12)},
			{"OP", BIT(13)},
			{"WP+OP", MASK(12, 13)},
		}
	}},
	/*
	{"BOOT_CFG_LOCK", {.offset = OCOTP_LOCK, .mask = MASK(2, 3), .bits = {
			{"NONE", 0x0},
			{"WP", BIT(2)},
			{"OP", BIT(3)},
			{"WP+OP", MASK(2, 3)},
		}
	}},
	{"SRK_LOCK", {.offset = OCOTP_LOCK, .mask = BIT(9), .bits = {
			{"NONE", 0x0},
			{"WP+OP", BIT(9)},
		}
	}},*/
	{"BT_FUSE_SEL", {.offset = OCOTP_BOOT_CFG0, .mask = BIT(28), .bits = {
			{"NONE", 0x0},
			{"PROGRAMMED", BIT(28)},
		}
	}},
	/*
	{"SJC_DISABLE", {.offset = OCOTP_BOOT_CFG0, .mask = BIT(21), .bits = {
			{"ENABLED", 0x0},
			{"DISABLED", BIT(21)},
		}
	}},
	{"JTAG_SMODE", {.offset = OCOTP_BOOT_CFG0, .mask = MASK(22, 23), .bits = {
			{"JTAG", 0x0},
			{"SECURE", BIT(22)},
			{"DISABLED", MASK(22, 23)},
		}
	}},
	{"SEC_CONFIG", {.offset = OCOTP_BOOT_CFG0, .mask = BIT(25), .bits = {
			{"OPEN", 0x0},
			{"CLOSED", BIT(25)},
		}
	}},*/
	{"BOOT_DEVICE", {.offset = OCOTP_BOOT_CFG0, .mask = MASK(12, 15), .bits = {
			{"FUSES", 0x0},
			{"SDP", BIT(12)},
			{"USDHC3", BIT(13)},
			{"USDHC2", MASK(12, 13)},
			{"NAND-256", BIT(14)},
			{"NAND-512", BIT(12) | BIT(14)},
			{"FLEXSPI-3b", MASK(13, 14)},
			{"FLEXSPI-HYPERFLASH", MASK(12, 14)},
			{"ECSPI", BIT(15)},
			{"FLEXSPI-SNAND-2K", BIT(13) | BIT(15)},
			{"FLEXSPI-SNAND-4K", BIT(12) | BIT(13) | BIT(15)},
		}
	}},
	{"FORCE_BT_FROM_FUSE", {.offset = OCOTP_BOOT_CFG1, .mask = BIT(20), .bits = {
			{"DISABLED", 0x0},
			{"ENABLED", BIT(20)},
		}
	}},
	{"BOOT_ECSPI_PORT", {.offset = OCOTP_BOOT_CFG1, .mask = MASK(29, 31), .bits = {
			{"ECSPI1", 0x0},
			{"ECSPI2", BIT(29)},
			{"ECSPI3", BIT(30)},
		}
	}},
	{"BOOT_ECSPI_ADDR", {.offset = OCOTP_BOOT_CFG1, .mask = BIT(28), .bits = {
			{"3-BYTES", 0x0},
			{"2-BYTES", BIT(28)},
		}
	}},
	{"BOOT_ECSPI_CS", {.offset = OCOTP_BOOT_CFG1, .mask = MASK(26, 27), .bits = {
			{"CS0", 0x0},
			{"CS1", BIT(26)},
			{"CS2", BIT(27)},
			{"CS3", MASK(26, 27)},
		}
	}},
	{"IMG_CNTN_SET1_OFFSET", {.offset = OCOTP_BOOT_CFG2, .mask = MASK(19, 22), .bits = {
			{"N_0", 0x0},
			{"N_1", BIT(19)},
			{"N_2", BIT(20)},
			{"N_3", MASK(19, 20)},
			{"N_4", BIT(21)},
			{"N_5", BIT(19) | BIT(21)},
			{"N_6", MASK(20, 21)},
			{"N_7", MASK(19, 21)},
			{"N_8", BIT(22)},
			{"N_9", BIT(19) | BIT(22)},
			{"N_10", BIT(20) | BIT(22)},
		}
	}},
};

std::string to_string(const std::map<std::string, FlagFuseDesc>& fusemap)
{
	std::string s;
	for (const auto& [name, desc] : fusemap) {
		s += "   " + name + "\n";
		for (const auto& [selection, value] : desc.bits)
			s += "      " + selection + "\n";
	}
	return s;
}

std::string available_fuses()
{
	return
		"Available --fuses and arguments: \n"
		"   MAC\n"
		"      XXXXXXXXXXXX (capital letters)\n"
		"   MAC2\n"
		"      XXXXXXXXXXXX (capital letters)\n"
		"   SRK\n"
		"      0XXXXXXXXX,...,n8 (capital letters, comma separated, 8 fields of 8 bytes)\n"
		+ to_string(flag_fuses)
		+ "\n";
}

std::unique_ptr<IFuse> make_fuse(std::string nvmem, const std::string& name)
{
	if (name == "MAC")
		return std::make_unique<MACFuse>(std::move(nvmem), OCOTP_MAC_ADDR0, OCOTP_MAC_ADDR1);
	if (name == "MAC2")
		return std::make_unique<MAC2Fuse>(std::move(nvmem), OCOTP_MAC_ADDR1, OCOTP_MAC_ADDR2);
	/*if (name == "SRK")
		return std::make_unique<SRKFuse>(std::move(nvmem),
				std::array<int, 8>{OCOTP_SRK0, OCOTP_SRK1,
									OCOTP_SRK2, OCOTP_SRK3,
									OCOTP_SRK4, OCOTP_SRK5,
									OCOTP_SRK6, OCOTP_SRK7});*/
	if (flag_fuses.contains(name)) {
		const auto& desc = flag_fuses.at(name);
		return std::make_unique<FlagFuse>(std::move(nvmem), desc.offset, desc.mask, desc.bits);
	}

	return {};
}
