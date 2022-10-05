#include <map>
#include <string>
#include <memory>
#include "flash-fuse-common.h"

std::string available_fuses()
{
	return {
		"Available --fuses, optional argument in parenthesis: \n"
		"   # General\n"
		"   MAC         XXXXXXXXXXXX (capital letters)\n"
		"\n"};
}

struct FlagFuseDesc {
	int offset;
	uint32_t mask;
	std::map<std::string, uint32_t> bits;
};

constexpr int OCOTP_LOCK = 0x0;
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
			{"GPIO", 0x0},
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
};

std::unique_ptr<IFuse> make_fuse(std::string nvmem, const std::string& name)
{
	if (name == "MAC")
		return std::make_unique<MACFuse>(std::move(nvmem), 0x22, 0x23);

	if (flag_fuses.contains(name)) {
		const auto& desc = flag_fuses.at(name);
		return std::make_unique<FlagFuse>(std::move(nvmem), desc.offset, desc.mask, desc.bits);
	}

	return {};
}
