#include <map>
#include <string>
#include <memory>
#include "flash-fuse-common.h"

std::unique_ptr<IFuse> make_fuse(std::string nvmem, const std::string& name)
{
	if (name == "MAC")
		return std::make_unique<MACFuse>(std::move(nvmem), 0x90, 0x94);
	if (name == "LOCK_MAC") {
		const std::map<std::string, uint32_t> flags = {
			{"WP", 0x4000},
			{"OP", 0x8000},
			{"WP+OP", 0xC000},
		};
		return std::make_unique<FlagFuse>(std::move(nvmem), 0x0, flags);
	}
	return {};
}
