#define _POSIX_C_SOURCE 200112L
#include <stdexcept>
#include <map>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>
#include "flash-fuse-common.h"

#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>

struct TmpFile {
public:
	TmpFile()
	{
		// filename modified by mkstemp and must not be placed in read-only section
		char filename[11] = {'f', 'u', 's', 'e', 'X', 'X', 'X', 'X', 'X', 'X'};
		filename[10] = '\0';
		const int fd = mkstemp(filename);
		if (fd < 0)
			throw std::system_error(errno, std::generic_category(), "Failed creating tmpfile");
		mpath = std::string(filename);
		if (close(fd) < 0)
			throw std::system_error(errno, std::generic_category(), "Failed closing tmpfile");
	}
	~TmpFile()
	{
		remove(mpath.c_str());
	}

	std::vector<uint8_t> read(int offset, size_t size) {
		std::vector<uint8_t> v(size, static_cast<uint8_t>(0));
		const int fd = open(mpath.c_str(), O_RDONLY);
		if (fd < 0)
			throw std::system_error(errno, std::generic_category(), "Failed opening tmpfile");
		const ssize_t bytes = pread(fd, v.data(), v.size(), offset);
		const int read_errno = errno;
		const int r = close(fd);
		if (r < 0)
			throw std::system_error(errno, std::generic_category(), "Failed closing tmpfile");
		if (bytes < 0 || static_cast<std::size_t>(bytes) != v.size())
			throw std::system_error(read_errno, std::generic_category(), "Read from tmpfile failed");
		return v;
	}

	void write(int offset, const std::vector<uint8_t>& buf)
	{
		const int fd = open(mpath.c_str(), O_WRONLY);
		if (fd < 0)
			throw std::system_error(errno, std::generic_category(), "Failed opening tmpfile");
		const ssize_t bytes = pwrite(fd, buf.data(), buf.size(), offset);
		const int write_errno = errno;
		const int r = close(fd);
		if (r < 0)
			throw std::system_error(errno, std::generic_category(), "Failed closing tmpfile");
		if (bytes < 0 || static_cast<std::size_t>(bytes) != buf.size())
			throw std::system_error(write_errno, std::generic_category(), "Write to tmpfile failed");
	}

	uint32_t read_bank(int offset) {
		auto v = read(offset, 4);
		return static_cast<uint32_t>(v.at(0) | v.at(1) << 8 | v.at(2) << 16 | v.at(3) << 24);
	}

	void write_bank(int offset, uint32_t value) {
		const std::vector<uint8_t> buf = {
			static_cast<uint8_t>(value & 0xff),
			static_cast<uint8_t>(value >> 8 & 0xff),
			static_cast<uint8_t>(value >> 16 & 0xff),
			static_cast<uint8_t>(value >> 24 & 0xff),
		};
		write(offset, buf);
	}

	void fill(int bytes, uint8_t value)
	{
		const std::vector<uint8_t> v(bytes, value);
		write(0, v);
	}

	const std::string& path() {return mpath;}

private:
	std::string mpath {};
};

TEST_CASE("Flag Fuse") {
	TmpFile tmpf;
	tmpf.fill(512, 0);

	SECTION("Not defined value returns unknown") {
		const int offset = 0x0;
		FlagFuse fuse(tmpf.path(), offset, 0x8, {{"A", 0x8}});
		REQUIRE(fuse.get() == "UNKNOWN");
	}

	SECTION("Defined values return valid arg") {
		const int offset = 0x0;
		FlagFuse fuse(tmpf.path(), offset, 0xC, {{"A", 0x8}, {"B", 0x4}});
		REQUIRE(fuse.valid_arg("A"));
		REQUIRE(fuse.valid_arg("B"));
	}

	SECTION("Undefined flag returns false valid_arg") {
		const int offset = 0x0;
		FlagFuse fuse(tmpf.path(), offset, 0x8, {{"A", 0x8}});
		REQUIRE(!fuse.valid_arg("B"));
	}

	SECTION("Single flag") {
		const int offset = 0x0;
		const uint32_t mask = 0x8;
		FlagFuse fuse(tmpf.path(), offset, mask, {{"A", 0x8}});

		SECTION("Read") {
			tmpf.write_bank(offset, 0x8);
			REQUIRE(fuse.get() == "A");
		}

		SECTION("Write") {
			fuse.set("A");
			REQUIRE(tmpf.read_bank(offset) == 0x8);
		}
	}

	SECTION("Multiple flags") {
		const int offset = 0x4;
		const uint32_t mask = 0x70;
		const std::map<std::string, uint32_t> flags = {
			{"A", 0x10},
			{"B", 0x20},
			{"C", 0x40},
			{"D", 0x0},
		};
		FlagFuse fuse(tmpf.path(), offset, mask, flags);

		SECTION("Read") {
			for (const auto& [name, bits] : flags) {
				tmpf.write_bank(offset, bits);
				REQUIRE(fuse.get() == name);
			}
		}

		SECTION ("Write") {
			for (const auto& [name, bits] : flags) {
				fuse.set(name);
				REQUIRE(tmpf.read_bank(offset) == bits);
			}
		}
	}

	SECTION("Is fuseable") {
		const int offset = 0x8;
		const uint32_t mask = 0x30;
		const std::map<std::string, uint32_t> flags = {
			{"bit1", 0x10},
			{"bit2", 0x20},
			{"both", 0x30},
		};
		FlagFuse fuse(tmpf.path(), offset, mask, flags);

		SECTION("True -- empty") {
			REQUIRE(fuse.is_fuseable("bit2"));
		}

		SECTION("True -- bit1 to both") {
			tmpf.write_bank(offset, flags.at("bit1"));
			REQUIRE(fuse.is_fuseable("both"));
		}

		SECTION("False -- bit1 to bit2") {
			tmpf.write_bank(offset, flags.at("bit1"));
			REQUIRE(!fuse.is_fuseable("bit2"));
		}

		SECTION("False -- both to bit2") {
			tmpf.write_bank(offset, flags.at("both"));
			REQUIRE(!fuse.is_fuseable("bit2"));
		}
	}
}
