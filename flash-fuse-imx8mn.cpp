#include <map>
#include <string>
#include <memory>
#include "flash-fuse-common.h"

struct FlagFuseDesc {
	int offset;
	uint32_t mask;
	std::map<std::string, uint32_t> bits;
};

constexpr int OCOTP_HW_OCOTP_LOCK = 0x0;		// 0x400
constexpr int OCOTP_HW_OCOTP_TESTER4 = 0x14;	// 0x450
constexpr int OCOTP_HW_OCOTP_BOOT_CFG0 = 0x1c;	// 0x470
constexpr int OCOTP_HW_OCOTP_BOOT_CFG1 = 0x20;	// 0x480
constexpr int OCOTP_HW_OCOTP_SRK0 = 0x60;		// 0x580
constexpr int OCOTP_HW_OCOTP_SRK1 = 0x64;		// 0x590
constexpr int OCOTP_HW_OCOTP_SRK2 = 0x68;		// 0x5a0
constexpr int OCOTP_HW_OCOTP_SRK3 = 0x6c;		// 0x5b0
constexpr int OCOTP_HW_OCOTP_SRK4 = 0x70;		// 0x5c0
constexpr int OCOTP_HW_OCOTP_SRK5 = 0x74;		// 0x5d0
constexpr int OCOTP_HW_OCOTP_SRK6 = 0x78;		// 0x5e0
constexpr int OCOTP_HW_OCOTP_SRK7 = 0x7c;		// 0x5f0
constexpr int OCOTP_HW_OCOTP_MAC_ADDR0 = 0x90;	// 0x640
constexpr int OCOTP_HW_OCOTP_MAC_ADDR1 = 0x94;	// 0x650

const std::map<std::string, FlagFuseDesc> flag_fuses = {
	{"MAC_ADDR_LOCK", {.offset = OCOTP_HW_OCOTP_LOCK, .mask = 0xc000, .bits = {
			{"NONE", 0x0},
			{"WP", 0x4000},
			{"OP", 0x8000},
			{"WP+OP", 0xC000},
		}
	}},
	{"USB_ID_LOCK", {.offset = OCOTP_HW_OCOTP_LOCK, .mask = 0x3000, .bits = {
			{"NONE", 0x0},
			{"WP", 0x1000},
			{"OP", 0x2000},
			{"WP+OP", 0x3000},
		}
	}},
	{"BOOT_CFG_LOCK", {.offset = OCOTP_HW_OCOTP_LOCK, .mask = 0xc, .bits = {
			{"NONE", 0x0},
			{"WP", 0x4},
			{"OP", 0x8},
			{"WP+OP", 0xc},
		}
	}},
	{"SRK_LOCK", {.offset = OCOTP_HW_OCOTP_LOCK, .mask = 0x200, .bits = {
			{"NONE", 0x0},
			{"WP+OP", 0x200},
		}
	}},
	{"BT_FUSE_SEL", {.offset = OCOTP_HW_OCOTP_BOOT_CFG0, .mask = 0x10000000, .bits = {
			{"BOARD", 0x0},
			{"FUSE", 0x10000000},
		}
	}},
	{"SJC_DISABLE", {.offset = OCOTP_HW_OCOTP_BOOT_CFG0, .mask = 0x200000, .bits = {
			{"ENABLED", 0x0},
			{"DISABLED", 0x200000},
		}
	}},
	{"JTAG_SMODE", {.offset = OCOTP_HW_OCOTP_BOOT_CFG0, .mask = 0xc00000, .bits = {
			{"JTAG", 0x0},
			{"SECURE", 0x400000},
			{"DISABLED", 0xc00000},
		}
	}},
	{"SEC_CONFIG", {.offset = OCOTP_HW_OCOTP_BOOT_CFG0, .mask = 0x2000000, .bits = {
			{"OPEN", 0x0},
			{"CLOSED", 0x2000000},
		}
	}},
	{"BOOT_DEVICE", {.offset = OCOTP_HW_OCOTP_BOOT_CFG0, .mask = 0xf000, .bits = {
			{"FUSES", 0x0},
			{"SDP", 0x1},
			{"USDHC3", 0x2},
			{"USDHC2", 0x3},
			{"NAND-256", 0x4},
			{"NAND-512", 0x5},
			{"FLEXSPI-3b", 0x6},
			{"FLEXSPI-HYPERFLASH", 0x7},
			{"ECSPI", 0x8},

		}
	}},
	{"BOOT_ECSPI_PORT", {.offset = OCOTP_HW_OCOTP_BOOT_CFG1, .mask = 0xe0000000, .bits = {
			{"ECSPI1", 0x0},
			{"ECSPI2", 0x20000000},
			{"ECSPI3", 0x40000000},
		}
	}},
	{"BOOT_ECSPI_ADDR", {.offset = OCOTP_HW_OCOTP_BOOT_CFG1, .mask = 0x10000000, .bits = {
			{"3-BYTES", 0x0},
			{"2-BYTES", 0x10000000},
		}
	}},
	{"BOOT_ECSPI_CS", {.offset = OCOTP_HW_OCOTP_BOOT_CFG1, .mask = 0xc000000, .bits = {
			{"CS0", 0x0},
			{"CS1", 0x4000000},
			{"CS2", 0x8000000},
			{"CS3", 0xc000000},
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
		"      0XXXXXXXXX,...,n8 (capital letters, comma separated, 8 fields of 8 bytes)\n"
		+ to_string(flag_fuses)
		+ "\n";
}

std::unique_ptr<IFuse> make_fuse(std::string nvmem, const std::string& name)
{
	if (name == "MAC")
		return std::make_unique<MACFuse>(std::move(nvmem), OCOTP_HW_OCOTP_MAC_ADDR0, OCOTP_HW_OCOTP_MAC_ADDR1);
	if (name == "SRK")
		return std::make_unique<SRKFuse>(std::move(nvmem),
				std::array<int, 8>{OCOTP_HW_OCOTP_SRK0, OCOTP_HW_OCOTP_SRK1,
									OCOTP_HW_OCOTP_SRK2, OCOTP_HW_OCOTP_SRK3,
									OCOTP_HW_OCOTP_SRK4, OCOTP_HW_OCOTP_SRK5,
									OCOTP_HW_OCOTP_SRK6, OCOTP_HW_OCOTP_SRK7});
	if (flag_fuses.contains(name)) {
		const auto& desc = flag_fuses.at(name);
		return std::make_unique<FlagFuse>(std::move(nvmem), desc.offset, desc.mask, desc.bits);
	}

	return {};
}
