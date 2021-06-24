GIT_VERSION := $(shell git describe --tags --dirty --always)
CXXFLAGS += -Wall
CXXFLAGS += -Wextra
CXXFLAGS += -Werror
CXXFLAGS += -std=gnu++17
CXXFLAGS += -pedantic
CXXFLAGS += -DSRC_VERSION=\"$(GIT_VERSION)\"


CLANG_TIDY_CHECKS = -checks=-*,modernize-*,cppcoreguidelines-*,readability-*,bugprone-*,clang-analyzer-*$\
					,-cppcoreguidelines-pro-bounds-pointer-arithmetic,-cppcoreguidelines-pro-bounds-array-to-pointer-decay$\
					,-cppcoreguidelines-pro-type-vararg

CLANG_TIDY_FILES = flash-fuse-imx8mm.cpp

all: flash-fuse-imx8mm

flash-fuse-imx8mm: flash-fuse-imx8mm.o
	$(CXX) -o $@ $^ $(LDFLAGS)

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clang-tidy
clang-tidy:
	clang-tidy $(CLANG_TIDY_FILES) $(CLANG_TIDY_CHECKS) -header-filter=.*,/usr/local/include/* -- $(CXXFLAGS)

.PHONY: clean
clean:
	rm -f flash-fuse-imx8mm.o
	rm -f flash-fuse-imx8mm
