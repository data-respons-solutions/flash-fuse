#include <iostream>
#include <string>
#include <stdexcept>
#include <cstdio>
#include <cstring>
#include <memory>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

//FIXME: MAC LOCK_BIT

static std::string read_fuse(const std::string& path, int offset)
{
	int fd = open(path.c_str(), O_RDONLY);
	if (fd < 0)
		throw std::runtime_error(std::string("Failed opening for reading: ") + path + ": " + std::to_string(errno));
	char buf[4];
	const ssize_t bytes = pread(fd, buf, 4, offset);
	const int read_errno = errno;
	const int r = close(fd);
	if (bytes != 4)
		throw std::runtime_error(std::string("Failed reading: ") + path + ": " + std::to_string(read_errno));
	if (r != 0)
		throw std::runtime_error(std::string("Failed closing: ") + path + ": " + std::to_string(errno));
	return std::string(buf, 4);
}

static void write_fuse(const std::string& path, int offset, const std::string& buf)
{
	if (buf.size() != 4)
		throw std::runtime_error("internal error, write buf.size() != 4");
	int fd = open(path.c_str(), O_WRONLY);
	if (fd < 0)
		throw std::runtime_error(std::string("Failed opening for reading: ") + path + ": " + std::to_string(errno));
	const ssize_t bytes = pwrite(fd, buf.data(), buf.size(), offset);
	const int write_errno = errno;
	const int r = close(fd);
	if (bytes != 4)
		throw std::runtime_error(std::string("Failed writing: ") + path + ": " + std::to_string(write_errno));
	if (r != 0)
		throw std::runtime_error(std::string("Failed closing: ") + path + ": " + std::to_string(errno));
}

class IFuse {
public:
	virtual ~IFuse() = default;
	virtual bool valid_arg(const std::string& arg) const = 0;
	virtual std::string get() const = 0;
	virtual void set(const std::string& arg) = 0;
};

class FuseMAC : public IFuse {
public:
	FuseMAC(const std::string& path) : path(path) {}

	bool valid_arg(const std::string& arg) const override
	{
		if (arg.size() != 17)
			return false;
		unsigned char buf[6];
		int r = sscanf(arg.c_str(), "%02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX", &buf[0], &buf[1], &buf[2], &buf[3], &buf[4], &buf[5]);
		if (r != 6)
			throw std::runtime_error("internal error, sscanf ret != 6");
		return true;
	}

	std::string get() const override
	{
		const std::string mac0 = read_fuse(path, mac0_offset);
		const std::string mac1 = read_fuse(path, mac1_offset);
		char buf[18];
		int r = snprintf(buf, 18, "%02x:%02x:%02x:%02x:%02x:%02x", mac0.at(1), mac0.at(0), mac0.at(3), mac0.at(2), mac1.at(1), mac1.at(0));
		if (r != 17)
			throw std::runtime_error("internal error, snprintf ret != 17");
		return std::string(buf, r);
	}

	void set(const std::string& arg) override
	{
		std::cout << "SET: " << arg << "\n";
		unsigned char buf[6];
		int r = sscanf(arg.c_str(), "%02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX", &buf[1], &buf[0], &buf[3], &buf[2], &buf[5], &buf[4]);
		if (r != 6)
			throw std::runtime_error("internal error, sscanf ret != 6");
		const std::string mac0(reinterpret_cast<char*>(&buf[0]), 4);
		const std::string mac1 {static_cast<char>(buf[4]), static_cast<char>(buf[5]), 0, 0};

		write_fuse(path, mac0_offset, mac0);
		write_fuse(path, mac1_offset, mac1);
		std::string mac0_read = read_fuse(path, mac0_offset);
		std::cout << "WRITE: " << path << " " << mac0_offset << ": 0x";
		for (char c : mac0_read) {
			printf("%02x", c);
		}
		std::cout << "\n";

		std::string mac1_read = read_fuse(path, mac1_offset);
		std::cout << "WRITE: " << path << " " << mac1_offset << ": 0x";
		for (char c : mac1_read) {
			printf("%02x", c);
		}
		std::cout << "\n";
	}

private:
	const int mac0_offset = 0x90;
	const int mac1_offset = 0x94;
	const std::string path;
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
		<< "  --path:           Override default ocopt nvmem path\n"
		<< "\n"
		<< "Return values:\n"
		<< "  0 if OK\n"
		<< "  1 for error or test failed\n"
		<< "\n"
		<< "Available --fuses, optional argument in parenthesis: \n"
		<< "   # General\n"
		<< "   MAC (lower case, i.e. xx:xx:xx:xx:xx:xx)\n"
		<< "\n"
		<< "WARNING\n"
		<< "Changes are permanent and irreversible\n"
		<< "WARNING\n";
}

static std::unique_ptr<IFuse> make_fuse(const std::string& path, const std::string& name)
{
	if (name == "MAC")
		return std::make_unique<FuseMAC>(path);
	return std::unique_ptr<IFuse>(nullptr);
}

int main(int argc, char** argv)
{
	std::string fuse_name;
	std::string fuse_arg;
	bool verify = false;
	bool commit = false;
	std::string path = "/sys/bus/nvmem/devices/imx-ocotp0/nvmem";

	for (int i = 1; i < argc; i++) {
		if (!strcmp("--fuse", argv[i])) {
			if (++i >= argc) {
				std::cerr << "invalid --fuse\n";
				return 1;
			}
			fuse_name = argv[i];
		}
		if (!strcmp("--path", argv[i])) {
			if (++i >= argc) {
				std::cerr << "invalid --path\n";
				return 1;
			}
			path = argv[i];
		}
		else
		if (!strcmp("--verify", argv[i])) {
			verify = true;
		}
		else
		if (!strcmp("--commit", argv[i])) {
			commit = true;
		}
		else
		if (!strcmp("--help", argv[i]) || !strcmp("-h", argv[i])) {
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

	if (fuse_name.empty()) {
		std::cerr << "invalid argument, no --fuse selected\n";
		return 1;
	}

	std::cout << "fuse_name: " << fuse_name << ": fuse_arg: " << fuse_arg << ": commit: " << commit << ": verify: " << verify << "\n";

	auto fuse = make_fuse(path, fuse_name);
	if (!fuse) {
		std::cerr << "invalid argument, --fuse " << fuse_name << " not found\n";
		return 1;
	}

	if (!fuse->valid_arg(fuse_arg)) {
		std::cerr << "invalid --fuse " << fuse_name << " argument\n";
		return 1;
	}

	if (verify) {
		const std::string fused_value = fuse->get();
		if (fused_value != fuse_arg) {
			std::cerr << "Fused value \"" << fused_value << "\" not equal to requested \"" << fuse_arg << "\"\n";
			return 1;
		}
		std::cout << "Got: " << fused_value << "\n"; // FIXME DEBUG OUTPUT
		return 0;
	}
	else
	if (commit) {
		fuse->set(fuse_arg);
		return 0;
	}
	else {
		std::cerr << "invalid argument, no operation selected\n";
		return 1;
	}

	return 0;
}
