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

std::string available_fuses()
{
	return {
		"Available --fuses, optional argument in parenthesis: \n"
		"   # General\n"
		"   MAC         XXXXXXXXXXXX (capital letters)\n"
		"   # Locks\n"
		"   LOCK_MAC    WP or OP\n"
		"\n"};
}

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
		return std::make_unique<FlagFuse>(std::move(nvmem), 0x0, 0xC000, flags);
	}
	return {};
}
