#include <iostream>
#include <string>
#include <stdexcept>
#include <cstdio>
#include <cstring>
#include <memory>
#include <algorithm>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>

/*
 * OCOTP value offset calculation:
 *   Offset=(Bank * 4 + Word) * 4
 * Example, MAC0 Bank9 Word 0
 *   (9 * 4 + 0) * 4 = 144 (0x90)
 * Example, MAC1 Bank9 Word 1
 *	 (9 * 4 + 1) * 4 = 148 (0x94)
 */

static std::string read_fuse(const std::string& path, int offset)
{
	int fd = open(path.c_str(), O_RDONLY);
	if (fd < 0) {
		throw std::runtime_error(std::string("Failed opening for reading: ") + path + ": " + std::to_string(errno));
	}
	char buf[4];
	const ssize_t bytes = pread(fd, buf, 4, offset);
	const int read_errno = errno;
	const int r = close(fd);
	if (bytes != 4) {
		throw std::runtime_error(std::string("Failed reading: ") + path + ": " + std::to_string(read_errno));
	}
	if (r != 0) {
		throw std::runtime_error(std::string("Failed closing: ") + path + ": " + std::to_string(errno));
	}
	return std::string(buf, 4);
}

static void write_fuse(const std::string& path, int offset, const std::string& buf)
{
	if (buf.size() != 4) {
		throw std::runtime_error("internal error, write buf.size() != 4");
	}
	int fd = open(path.c_str(), O_WRONLY);
	if (fd < 0) {
		throw std::runtime_error(std::string("Failed opening for reading: ") + path + ": " + std::to_string(errno));
	}
	const ssize_t bytes = pwrite(fd, buf.data(), buf.size(), offset);
	const int write_errno = errno;
	const int r = close(fd);
	if (bytes != 4) {
		throw std::runtime_error(std::string("Failed writing: ") + path + ": " + std::to_string(write_errno));
	}
	if (r != 0) {
		throw std::runtime_error(std::string("Failed closing: ") + path + ": " + std::to_string(errno));
	}
}

/*
 * Default implementation supports simple 1 or 0 fuse.
 * Override for more complex scenario.
 */
class IFuse {
public:
	virtual ~IFuse() = default;
	IFuse(std::string path) : path(std::move(path)) {fuse_mask = 0; fuse_offset = 0;}
	IFuse(std::string path, int fuse_offset, uint32_t fuse_mask)
		: path(std::move(path)), fuse_mask(fuse_mask), fuse_offset(fuse_offset) {}
	IFuse(const IFuse&) = default;
	IFuse& operator=(const IFuse&) = default;
	IFuse(IFuse&&) = default;
	IFuse& operator=(IFuse&&) = default;

	virtual bool valid_arg(const std::string& arg) const
	{
		return arg == "1" || arg == "0";
	}
	virtual bool is_fused() const
	{
		return get() == "1";
	}
	virtual std::string get() const
	{
		const std::string str = read_fuse(path, fuse_offset);
		const uint32_t val = str.at(0) | str.at(1) << 8 | str.at(2) << 16 | str.at(3) << 24;
		return (val & fuse_mask) == fuse_mask ? "1" : "0";
	}
	virtual void set(const std::string& arg)
	{
		const uint32_t val = arg == "1" ? fuse_mask : 0;
		const std::string str = {
			static_cast<char>(val & 0xff),
			static_cast<char>(val >> 8 & 0xff),
			static_cast<char>(val >> 16 & 0xff),
			static_cast<char>(val >> 24 & 0xff),
		};
		write_fuse(path, fuse_offset, str);
	}

protected:
	std::string path;
	uint32_t fuse_mask = 0x4000;
	int fuse_offset = 0x0;
};

class FuseMAC : public IFuse {
public:
	FuseMAC(std::string path) : IFuse(std::move(path)) {}

	bool valid_arg(const std::string& arg) const override
	{
		if (arg.size() != 17) {
			return false;
		}
		unsigned char buf[6];
		int r = sscanf(arg.c_str(), "%02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX", &buf[0], &buf[1], &buf[2], &buf[3], &buf[4], &buf[5]);
		if (r != 6) {
			throw std::runtime_error("internal error, sscanf ret != 6");
		}
		return true;
	}

	bool is_fused() const override
	{
		std::string val = get();
		val.erase(std::remove(val.begin(), val.end(), ':'), val.end());
		return std::any_of(val.begin(), val.end(), [](char c){return c != '0';});
	}

	std::string get() const override
	{
		const std::string mac0 = read_fuse(path, mac0_offset);
		const std::string mac1 = read_fuse(path, mac1_offset);
		char buf[18];
		int r = snprintf(buf, 18, "%02x:%02x:%02x:%02x:%02x:%02x", mac0.at(1), mac0.at(0), mac0.at(3), mac0.at(2), mac1.at(1), mac1.at(0));
		if (r != 17) {
			throw std::runtime_error("internal error, snprintf ret != 17");
		}
		return std::string(buf, r);
	}

	void set(const std::string& arg) override
	{
		unsigned char buf[6];
		int r = sscanf(arg.c_str(), "%02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX", &buf[1], &buf[0], &buf[3], &buf[2], &buf[5], &buf[4]);
		if (r != 6) {
			throw std::runtime_error("internal error, sscanf ret != 6");
		}
		const std::string mac0(reinterpret_cast<char*>(&buf[0]), 4);
		const std::string mac1 {static_cast<char>(buf[4]), static_cast<char>(buf[5]), 0, 0};

		write_fuse(path, mac0_offset, mac0);
		write_fuse(path, mac1_offset, mac1);
	}

private:
	const int mac0_offset = 0x90;
	const int mac1_offset = 0x94;
};

class FuseLockMAC : public IFuse {
public:
	FuseLockMAC(std::string path) : IFuse(std::move(path), 0x0, 0x4000) {}
};

static void print_usage()
{
	std::cout
		<< "flash-fuse-imx8mm, imx8mm ocotp tool, Data Respons Solutions AB\n"
		<< "Version: " << SRC_VERSION << "\n"
		<< "Usage:   " << "flash-fuse-imx8mm [OPTIONS] --fuse NAME ARG\n"
		<< "Example: " << "flash-fuse-imx8mm --verify --fuse MAC 00:10:30:20:50:a2\n"
		<< "\n"
		<< "Options:\n"
		<< "  --fuse:      		Name of fuse\n"
		<< "  --verify:         Only verify, overrides commit\n"
		<< "  --commit:         Burn fuses\n"
		<< "  --get:            Get curret fuse value\n"
		<< "  --path:           Override default ocopt nvmem path\n"
		<< "\n"
		<< "Return values:\n"
		<< "  0 if OK\n"
		<< "  1 for error or test failed\n"
		<< "\n"
		<< "Available --fuses, optional argument in parenthesis: \n"
		<< "   # General\n"
		<< "   MAC          (xx:xx:xx:xx:xx:xx)\n"
		<< "   # Locks\n"
		<< "   LOCK_MAC    (1 or 0)\n"
		<< "\n"
		<< "WARNING\n"
		<< "Changes are permanent and irreversible\n"
		<< "WARNING\n";
}

static std::unique_ptr<IFuse> make_fuse(const std::string& path, const std::string& name)
{
	if (name == "MAC") {
		return std::make_unique<FuseMAC>(path);
	}
	if (name == "LOCK_MAC") {
		return std::make_unique<FuseLockMAC>(path);
	}
	return std::unique_ptr<IFuse>(nullptr);
}

int main(int argc, char** argv)
{
	std::string fuse_name;
	std::string fuse_arg;
	bool verify = false;
	bool commit = false;
	bool get = false;
	std::string path = "/sys/bus/nvmem/devices/imx-ocotp0/nvmem";

	for (int i = 1; i < argc; i++) {
		if (strcmp("--fuse", argv[i]) == 0) {
			if (++i >= argc) {
				std::cerr << "invalid --fuse\n";
				return 1;
			}
			fuse_name = argv[i];
		}
		if (strcmp("--path", argv[i]) == 0) {
			if (++i >= argc) {
				std::cerr << "invalid --path\n";
				return 1;
			}
			path = argv[i];
		}
		else
		if (strcmp("--verify", argv[i]) == 0) {
			verify = true;
		}
		else
		if (strcmp("--get", argv[i]) == 0) {
			get = true;
		}
		else
		if (strcmp("--commit", argv[i]) == 0) {
			commit = true;
		}
		else
		if (strcmp("--help", argv[i]) == 0 || strcmp("-h", argv[i]) == 0) {
			print_usage();
			return 1;
		}
		else
		if ('-' == argv[i][0]) {
			std::cerr << "invalid option: " << argv[i] << "\n";
			return 1;
		}
		else {
			fuse_arg = argv[i];
		}
	}

	if (!verify && !commit && !get) {
		std::cerr << "invalid argument, no operation selected (--get,--commit,--verify)\n";
		return 1;
	}

	if (fuse_name.empty()) {
		std::cerr << "invalid argument, no --fuse selected\n";
		return 1;
	}

	auto fuse = make_fuse(path, fuse_name);
	if (!fuse) {
		std::cerr << "invalid argument, --fuse " << fuse_name << " not found\n";
		return 1;
	}

	if (get) {
		std::cout << fuse->get() << "\n";
		return 0;
	}

	if (!fuse->valid_arg(fuse_arg)) {
		std::cerr << "invalid --fuse " << fuse_name << " argument\n";
		return 1;
	}
	const std::string fused_value = fuse->get();
	if (fused_value == fuse_arg) {
		return 0;
	}

	if (verify) {
		std::cerr << "Fused value \"" << fused_value << "\" not equal to requested \"" << fuse_arg << "\"\n";
		return 1;
	}

	if (commit) {
		if (fuse->is_fused()) {
			std::cerr << "Error -- Already fused: " << fused_value << "\n";
			return 1;
		}
		fuse->set(fuse_arg);
		return 0;
	}

	std::cerr << "invalid argument, no operation selected\n";
	return 1;
}
