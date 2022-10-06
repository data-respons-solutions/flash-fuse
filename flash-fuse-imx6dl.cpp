#include <map>
#include <string>
#include <memory>
#include "flash-fuse-common.h"

struct FlagFuseDesc {
	int offset;
	uint32_t mask;
	std::map<std::string, uint32_t> bits;
};

constexpr int OCOTP_LOCK = 0x0;
constexpr int OCOTP_CFG4 = 0x5;
constexpr int OCOTP_CFG5 = 0x6;

const std::map<std::string, FlagFuseDesc> flag_fuses = {
	{"MAC_LOCK", {.offset = OCOTP_LOCK, .mask = 0x300, .bits = {
			{"NONE", 0x0},
			{"WP", 0x100},
			{"OP", 0x200},
			{"WP+OP", 0x300},
		}
	}},
	{"BT_FUSE_SEL", {.offset = OCOTP_CFG5, .mask = 0x10, .bits = {
			{"BOARD", 0x0},
			{"FUSE", 0x10},
		}
	}},
	{"SJC_DISABLE", {.offset = OCOTP_CFG5, .mask = 0x100000, .bits = {
			{"SJC_ENABLED", 0x0},
			{"SJC_DISABLED", 0x100000},
		}
	}},
	{"SEC_DISABLE", {.offset = OCOTP_CFG5, .mask = 0x2, .bits = {
			{"OPEN", 0x0},
			{"CLOSED", 0x2},
		}
	}},
	{"DIR_BT_DIS", {.offset = OCOTP_CFG5, .mask = 0x8, .bits = {
			{"NXP_RESERVED", 0x0},
			{"PRODUCTION", 0x8},
		}
	}},
	/* nxp reference manual states some boot devices ignore some of the bits.
	 * We assume bits were written correctly and simply check them all */
	{"BOOT_DEVICE", {.offset = OCOTP_CFG4, .mask = 0xf8, .bits = {
			{"NOR_Flash", 0x0},
			{"OneNAND", 0x8},
			{"SERIAL_ROM", 0x30},
			{"SD/eSD", 0x40},
			{"MMC/eMMC", 0x60},
			{"NAND_Flash", 0x80},
		}
	}},
	{"BOOT_MMC_PORT", {.offset = OCOTP_CFG4, .mask = 0x1800, .bits = {
			{"uSDHC1", 0x0},
			{"uSDHC2", 0x800},
			{"uSDHC3", 0x1000},
			{"uSDHC4", 0x1800},
		}
	}},
	{"BOOT_MMC_WIDTH", {.offset = OCOTP_CFG5, .mask = 0xe000, .bits = {
			{"1-BIT", 0x0},
			{"4-BIT", 0x2000},
			{"8-BIT", 0x4000},
			{"4-BIT", 0xa000},
			{"8-BIT-DDR", 0xc000},
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
		return std::make_unique<MACFuse>(std::move(nvmem), 0x22, 0x23);
	if (name == "SRK")
		return std::make_unique<SRKFuse>(std::move(nvmem), std::array<int, 8>{0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f});
	if (flag_fuses.contains(name)) {
		const auto& desc = flag_fuses.at(name);
		return std::make_unique<FlagFuse>(std::move(nvmem), desc.offset, desc.mask, desc.bits);
	}

	return {};
}
