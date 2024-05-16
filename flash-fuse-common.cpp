#include <array>
#include <vector>
#include <algorithm>
#include <map>
#include <stdexcept>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstdio>
#include <cinttypes>
#include "flash-fuse-common.h"
#include "log.h"

static std::array<uint8_t, 4> read_fuse(const std::string& path, int offset)
{
	int fd = open(path.c_str(), O_RDONLY);
	if (fd < 0)
		throw std::runtime_error(std::string("Failed opening for reading: ") + path + ": " + std::to_string(errno));
	if (lseek(fd, offset, SEEK_SET) == (off_t) - 1) {
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
	if (lseek(fd, offset, SEEK_SET) == (off_t) - 1) {
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

static void write_bank(const std::string& nvmem, int offset, uint32_t val)
{
	pr_dbg("write: 0x%08x at offset 0x%x to %s\n", val, offset, nvmem.c_str());
	const std::array<uint8_t, 4> buf = {
		static_cast<uint8_t>(val & 0xff),
		static_cast<uint8_t>(val >> 8 & 0xff),
		static_cast<uint8_t>(val >> 16 & 0xff),
		static_cast<uint8_t>(val >> 24 & 0xff),
	};
	write_fuse(nvmem, offset, buf);
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
	write_bank(mnvmem, moffset, mflags.at(arg));
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

MAC2Fuse::MAC2Fuse(std::string nvmem, int offset1, int offset2)
	: mnvmem(std::move(nvmem)), moffset1(offset1), moffset2(offset2)
{}
bool MAC2Fuse::valid_arg(const std::string& arg) const
{
	if (arg.size() != 12)
		return false;
	unsigned char buf[6];
	int r = sscanf(arg.c_str(), "%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX", &buf[0], &buf[1], &buf[2], &buf[3], &buf[4], &buf[5]);
	if (r != 6)
		throw std::runtime_error("internal error, sscanf ret != 6");
	return true;
}
bool MAC2Fuse::is_fuseable(const std::string& arg) const
{
	(void) arg;
	// Lazy implementation by simply checking for all zero
	return get() == "000000000000";
}
std::string MAC2Fuse::get() const
{
	const std::array<uint8_t, 4> mac1 = read_fuse(mnvmem, moffset1);
	const std::array<uint8_t, 4> mac2 = read_fuse(mnvmem, moffset2);
	char buf[13];
	int r = snprintf(buf, 13, "%02X%02X%02X%02X%02X%02X", mac2.at(3), mac2.at(2), mac2.at(1), mac2.at(0), mac1.at(3), mac1.at(2));
	if (r != 12)
		throw std::runtime_error("internal error, snprintf ret != 12");
	return std::string(buf, r);
}

void MAC2Fuse::set(const std::string& arg)
{
	std::array<uint8_t, 4> mac1 {};
	std::array<uint8_t, 4> mac2 {};
	int r = sscanf(arg.c_str(), "%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX", &mac2.at(3), &mac2.at(2), &mac2.at(1), &mac2.at(0), &mac1.at(3), &mac1.at(2));
	if (r != 6)
		throw std::runtime_error("internal error, sscanf ret != 6");
	write_fuse(mnvmem, moffset1, mac1);
	write_fuse(mnvmem, moffset2, mac2);
}

SRKFuse::SRKFuse(std::string nvmem, std::array<int, 8> offset)
	: mnvmem(std::move(nvmem)), moffset(offset)
{}

static bool parse_srk(const std::string& arg, std::array<uint32_t, 8>& buf)
{
	if (arg.size() != 87)
		return false;
	// Test upper case 0X
	int r = sscanf(arg.c_str(), "0X%08" PRIX32 ",0X%08" PRIX32 ",0X%08" PRIX32 ",0X%08" PRIX32 ",0X%08" PRIX32 ",0X%08" PRIX32 ",0X%08" PRIX32 ",0X%08" PRIX32 "",
					&buf[0], &buf[1], &buf[2], &buf[3], &buf[4], &buf[5], &buf[6], &buf[7]);
	// Test lower case 0x, if needed
	if (r != 8)
		r = sscanf(arg.c_str(), "0x%08" PRIX32 ",0x%08" PRIX32 ",0x%08" PRIX32 ",0x%08" PRIX32 ",0x%08" PRIX32 ",0x%08" PRIX32 ",0x%08" PRIX32 ",0x%08" PRIX32 "",
						&buf[0], &buf[1], &buf[2], &buf[3], &buf[4], &buf[5], &buf[6], &buf[7]);
	return r == 8;
}

bool SRKFuse::valid_arg(const std::string& arg) const
{
	std::array<uint32_t, 8> buf;
	return parse_srk(arg, buf);
}
bool SRKFuse::is_fuseable(const std::string& arg) const
{
	(void) arg;
	// Lazy implementation by simply checking for all zero
	return get() == "0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000";
}
std::string SRKFuse::get() const
{
	std::vector<uint32_t> v;
	for (int offset : moffset)
		v.push_back(read_bank(mnvmem, offset));
	char buf[88];
	const int r = snprintf(buf, 88, "0x%08X,0x%08X,0x%08X,0x%08X,0x%08X,0x%08X,0x%08X,0x%08X",
							v.at(0), v.at(1), v.at(2), v.at(3), v.at(4), v.at(5), v.at(6), v.at(7));
	if (r != 87)
		throw std::runtime_error("internal error, snprintf ret != 87");
	return {buf};
}
void SRKFuse::set(const std::string& arg)
{
	std::array<uint32_t, 8> buf;
	if (!parse_srk(arg, buf))
		throw std::runtime_error("failed parsing fuse arg");
	for (int i = 0; i < 8; ++i)
		write_bank(mnvmem, moffset.at(i), buf[i]);
}
