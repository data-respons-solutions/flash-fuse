#include <array>
#include <algorithm>
#include <map>
#include <stdexcept>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstdio>
#include "flash-fuse-common.h"
#include "log.h"

static std::array<uint8_t, 4> read_fuse(const std::string& path, int offset)
{
	int fd = open(path.c_str(), O_RDONLY);
	if (fd < 0)
		throw std::runtime_error(std::string("Failed opening for reading: ") + path + ": " + std::to_string(errno));
	if (lseek(fd, offset * 4, SEEK_SET) == (off_t) - 1) {
		close(fd);
		throw std::runtime_error(std::string("Failed seeking offset: ") + path + ": " + std::to_string(errno));
	}
	std::array<uint8_t, 4> buf {};
	const ssize_t bytes = read(fd, buf.data(), buf.size());
	const int read_errno = errno;
	const int r = close(fd);
	if (bytes < 0 || static_cast<std::size_t>(bytes) != buf.size())
		throw std::runtime_error(std::string("Failed reading: ") + path + ": " + std::to_string(read_errno));
	if (r != 0)
		throw std::runtime_error(std::string("Failed closing: ") + path + ": " + std::to_string(errno));
	return buf;
}

static void write_fuse(const std::string& path, int offset, const std::array<uint8_t, 4>& buf)
{
	int fd = open(path.c_str(), O_WRONLY);
	if (fd < 0)
		throw std::runtime_error(std::string("Failed opening for reading: ") + path + ": " + std::to_string(errno));
	if (lseek(fd, offset * 4, SEEK_SET) == (off_t) - 1) {
		close(fd);
		throw std::runtime_error(std::string("Failed seeking offset: ") + path + ": " + std::to_string(errno));
	}
	const ssize_t bytes = write(fd, buf.data(), buf.size());
	const int write_errno = errno;
	const int r = close(fd);
	if (bytes < 0 || static_cast<std::size_t>(bytes) != buf.size())
		throw std::runtime_error(std::string("Failed writing: ") + path + ": " + std::to_string(write_errno));
	if (r != 0)
		throw std::runtime_error(std::string("Failed closing: ") + path + ": " + std::to_string(errno));
}

static uint32_t read_bank(const std::string& nvmem, int offset)
{
	const std::array<uint8_t, 4> buf = read_fuse(nvmem, offset);
	const uint32_t val = buf.at(0) | buf.at(1) << 8 | buf.at(2) << 16 | buf.at(3) << 24;
	pr_dbg("read: 0x%08x at offset 0x%x from %s\n", val, offset, nvmem.c_str());
	return val;
}
FlagFuse::FlagFuse(std::string nvmem, int offset, uint32_t mask, std::map<std::string, uint32_t> flags)
	: mnvmem(std::move(nvmem)), moffset(offset), mmask(mask), mflags(std::move(flags))
{}
bool FlagFuse::valid_arg(const std::string& arg) const
{
	return mflags.contains(arg);
}
bool FlagFuse::is_fuseable(const std::string& arg) const
{
	return ((read_bank(mnvmem, moffset) & mmask) & (~mflags.at(arg))) == 0;
}
std::string FlagFuse::get() const
{
	const uint32_t val = read_bank(mnvmem, moffset) & mmask;
	for (const auto& [name, bits] : mflags) {
		if (val == bits)
			return name;
	}
	return "UNKNOWN";
}
void FlagFuse::set(const std::string& arg)
{
	const uint32_t val = mflags.at(arg);
	const std::array<uint8_t, 4> buf = {
		static_cast<uint8_t>(val & 0xff),
		static_cast<uint8_t>(val >> 8 & 0xff),
		static_cast<uint8_t>(val >> 16 & 0xff),
		static_cast<uint8_t>(val >> 24 & 0xff),
	};
	write_fuse(mnvmem, moffset, buf);
}

MACFuse::MACFuse(std::string nvmem, int offset1, int offset2)
	: mnvmem(std::move(nvmem)), moffset1(offset1), moffset2(offset2)
{}
bool MACFuse::valid_arg(const std::string& arg) const
{
	if (arg.size() != 12)
		return false;
	unsigned char buf[6];
	int r = sscanf(arg.c_str(), "%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX", &buf[0], &buf[1], &buf[2], &buf[3], &buf[4], &buf[5]);
	if (r != 6)
		throw std::runtime_error("internal error, sscanf ret != 6");
	return true;
}
bool MACFuse::is_fuseable(const std::string& arg) const
{
	(void) arg;
	// Lazy implementation by simply checking for all zero
	return get() == "000000000000";
}
std::string MACFuse::get() const
{
	const std::array<uint8_t, 4> mac0 = read_fuse(mnvmem, moffset1);
	const std::array<uint8_t, 4> mac1 = read_fuse(mnvmem, moffset2);
	char buf[13];
	int r = snprintf(buf, 13, "%02X%02X%02X%02X%02X%02X", mac1.at(1), mac1.at(0), mac0.at(3), mac0.at(2), mac0.at(1), mac0.at(0));
	if (r != 12)
		throw std::runtime_error("internal error, snprintf ret != 12");
	return std::string(buf, r);
}

void MACFuse::set(const std::string& arg)
{
	std::array<uint8_t, 4> mac0 {};
	std::array<uint8_t, 4> mac1 {};
	int r = sscanf(arg.c_str(), "%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX", &mac1.at(1), &mac1.at(0), &mac0.at(3), &mac0.at(2), &mac0.at(1), &mac0.at(0));
	if (r != 6)
		throw std::runtime_error("internal error, sscanf ret != 6");
	write_fuse(mnvmem, moffset1, mac0);
	write_fuse(mnvmem, moffset2, mac1);
}
