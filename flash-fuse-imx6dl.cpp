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

std::unique_ptr<IFuse> make_fuse(std::string nvmem, const std::string& name)
{
	if (name == "MAC")
		return std::make_unique<MACFuse>(std::move(nvmem), 0x22, 0x23);
	return {};
}
