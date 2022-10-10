#include <map>
#include <string>
#include <memory>
#include "flash-fuse-common.h"

/*
 * Offset calculations
 * *
 * OCOTP value offset calculation:
 *   Offset=(Bank * 8 + Word) * 4
 * Example, MAC0 Bank4 Word 2
 *   (4 * 8 + 2) * 4 = 136 (0x88)
 * Example, MAC1 Bank4 Word 3
 *	 (4 * 8 + 3) * 4 = 140 (0x8C)
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
	constexpr int word_per_bank = 8;
	constexpr int byte_per_word = 4;
	return ((bank.value * word_per_bank) + word.value) * byte_per_word;
}

constexpr int OCOTP_LOCK =		OFFSET(BANK(0), WORD(0));
constexpr int OCOTP_CFG4 =		OFFSET(BANK(0), WORD(5));
constexpr int OCOTP_CFG5 =		OFFSET(BANK(0), WORD(6));
constexpr int OCOTP_SRK0 =		OFFSET(BANK(3), WORD(0));
constexpr int OCOTP_SRK1 =		OFFSET(BANK(3), WORD(1));
constexpr int OCOTP_SRK2 =		OFFSET(BANK(3), WORD(2));
constexpr int OCOTP_SRK3 =		OFFSET(BANK(3), WORD(3));
constexpr int OCOTP_SRK4 =		OFFSET(BANK(3), WORD(4));
constexpr int OCOTP_SRK5 =		OFFSET(BANK(3), WORD(5));
constexpr int OCOTP_SRK6 =		OFFSET(BANK(3), WORD(6));
constexpr int OCOTP_SRK7 =		OFFSET(BANK(3), WORD(7));
constexpr int OCOTP_MAC0 =		OFFSET(BANK(4), WORD(2));
constexpr int OCOTP_MAC1 =		OFFSET(BANK(4), WORD(3));

struct FlagFuseDesc {
	int offset;
	uint32_t mask;
	std::map<std::string, uint32_t> bits;
};

const std::map<std::string, FlagFuseDesc> flag_fuses = {
	{"MAC_LOCK", {.offset = OCOTP_LOCK, .mask = MASK(8, 9), .bits = {
			{"NONE", 0x0},
			{"WP", BIT(8)},
			{"OP", BIT(9)},
			{"WP+OP", MASK(8, 9)},
		}
	}},
	{"BT_FUSE_SEL", {.offset = OCOTP_CFG5, .mask = BIT(4), .bits = {
			{"BOARD", 0x0},
			{"FUSE", BIT(4)},
		}
	}},
	{"SJC_DISABLE", {.offset = OCOTP_CFG5, .mask = BIT(20), .bits = {
			{"SJC_ENABLED", 0x0},
			{"SJC_DISABLED", BIT(20)},
		}
	}},
	{"SEC_DISABLE", {.offset = OCOTP_CFG5, .mask = BIT(1), .bits = {
			{"OPEN", 0x0},
			{"CLOSED", BIT(1)},
		}
	}},
	{"DIR_BT_DIS", {.offset = OCOTP_CFG5, .mask = BIT(3), .bits = {
			{"NXP_RESERVED", 0x0},
			{"PRODUCTION",  BIT(3)},
		}
	}},
	/* nxp reference manual states some boot devices ignore some of the bits.
	 * We assume bits were written correctly and simply check them all */
	{"BOOT_DEVICE", {.offset = OCOTP_CFG4, .mask = MASK(3, 7), .bits = {
			{"NOR_Flash", 0x0},
			{"OneNAND", BIT(3)},
			{"SERIAL_ROM", MASK(4, 5)},
			{"SD/eSD", BIT(6)},
			{"MMC/eMMC", MASK(5, 6)},
			{"NAND_Flash", BIT(7)},
		}
	}},
	{"BOOT_MMC_PORT", {.offset = OCOTP_CFG4, .mask = MASK(11, 12), .bits = {
			{"uSDHC1", 0x0},
			{"uSDHC2", BIT(11)},
			{"uSDHC3", BIT(12)},
			{"uSDHC4", MASK(11, 12)},
		}
	}},
	{"BOOT_MMC_WIDTH", {.offset = OCOTP_CFG5, .mask = MASK(13, 15), .bits = {
			{"1-BIT", 0x0},
			{"4-BIT", BIT(13)},
			{"8-BIT", BIT(14)},
			{"4-BIT", BIT(13) | BIT(15)},
			{"8-BIT-DDR", MASK(14, 15)},
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
		"   SRK\n"
		"      0XXXXXXXXX,...,n8 (capital letters, comma separated, 8 fields of 8 bytes)"
		+ to_string(flag_fuses)
		+ "\n";
}

std::unique_ptr<IFuse> make_fuse(std::string nvmem, const std::string& name)
{
	if (name == "MAC")
		return std::make_unique<MACFuse>(std::move(nvmem), OCOTP_MAC0, OCOTP_MAC1);
	if (name == "SRK")
		return std::make_unique<SRKFuse>(std::move(nvmem), std::array<int, 8>{
			OCOTP_SRK0, OCOTP_SRK1, OCOTP_SRK2, OCOTP_SRK3,
			OCOTP_SRK4, OCOTP_SRK5, OCOTP_SRK6, OCOTP_SRK7});
	if (flag_fuses.contains(name)) {
		const auto& desc = flag_fuses.at(name);
		return std::make_unique<FlagFuse>(std::move(nvmem), desc.offset, desc.mask, desc.bits);
	}

	return {};
}
