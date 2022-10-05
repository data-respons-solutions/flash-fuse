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

	SECTION("Single flag") {
		FlagFuse fuse(tmpf.path(), 0x0, std::map<std::string, uint32_t> {{"A", 0x8}});
		REQUIRE(fuse.valid_arg("A"));
		REQUIRE(fuse.get() == "NONE");
		tmpf.write(0x0, std::vector<uint8_t>{0x8});
		REQUIRE(fuse.get() == "A");
	}
}
