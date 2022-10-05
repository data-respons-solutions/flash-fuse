#include <string>
#include <iostream>
#include <cstring>
#include "flash-fuse-common.h"

static void print_usage()
{
	std::cout
		<< "flash-fuse-imx8mm, imx8mm ocotp tool, Data Respons Solutions AB\n"
		<< "Version: " << SRC_VERSION << "\n"
		<< "Usage:   " << "flash-fuse-imx8mm [OPTIONS] --fuse NAME ARG\n"
		<< "Example: " << "flash-fuse-imx8mm --verify --fuse MAC 0010302050A2\n"
		<< "\n"
		<< "Options:\n"
		<< "  --fuse:                   Name of fuse\n"
		<< "  --verify:         Only verify, overrides commit\n"
		<< "  --commit:         Burn fuses\n"
		<< "  --get:            Get curret fuse value\n"
		<< "  --path:           Override default ocopt nvmem path\n"
		<< "\n"
		<< "Return values:\n"
		<< "  0 if OK\n"
		<< "  1 for error or test failed\n"
		<< "\n"
		<< available_fuses()
		<< "WARNING\n"
		<< "Changes are permanent and irreversible\n"
		<< "WARNING\n";
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
		if (!fuse->is_fuseable(fuse_arg)) {
			std::cerr << "Error -- Already fused: " << fused_value << "\n";
			return 1;
		}
		fuse->set(fuse_arg);
		return 0;
	}

	std::cerr << "invalid argument, no operation selected\n";
	return 1;
}
