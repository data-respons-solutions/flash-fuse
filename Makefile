BUILD ?= build
GIT_VERSION := $(shell git describe --tags --dirty --always)
CXXFLAGS += -Wall
CXXFLAGS += -Wextra
CXXFLAGS += -Werror
CXXFLAGS += -std=gnu++20
CXXFLAGS += -pedantic
CXXFLAGS += -O2
CXXFLAGS += -DSRC_VERSION=\"$(GIT_VERSION)\"


CLANG_TIDY_CHECKS = -checks=-*,modernize-*,cppcoreguidelines-*,readability-*,bugprone-*,clang-analyzer-*$\
					,-cppcoreguidelines-pro-bounds-pointer-arithmetic,-cppcoreguidelines-pro-bounds-array-to-pointer-decay$\
					,-cppcoreguidelines-pro-type-vararg

CLANG_TIDY_FILES = flash-fuse-imx8mm.cpp flash-fuse-main.cpp flash-fuse-common.cpp

all: flash-fuse-imx8mm flash-fuse-imx6dl  flash-fuse-imx8mn
.PHONY: all

.PHONY: flash-fuse-imx8mm
flash-fuse-imx8mm: $(BUILD)/flash-fuse-imx8mm 

.PHONY: flash-fuse-imx6dl
flash-fuse-imx6dl: $(BUILD)/flash-fuse-imx6dl

.PHONY: flash-fuse-imx8mn
flash-fuse-imx8mn: $(BUILD)/flash-fuse-imx8mn

.PHONY: test
test: $(BUILD)/test-flash-fuse
	for test in $^; do \
		echo "Running: $${test}"; \
		if ! ./$${test}; then \
			exit 1; \
		fi \
	done

$(BUILD)/flash-fuse-imx8mm: $(addprefix $(BUILD)/, flash-fuse-imx8mm.o flash-fuse-common.o flash-fuse-main.o log.o)
	$(CXX) -o $@ $^ $(LDFLAGS)
	
$(BUILD)/flash-fuse-imx6dl: $(addprefix $(BUILD)/, flash-fuse-imx6dl.o flash-fuse-common.o flash-fuse-main.o log.o)
	$(CXX) -o $@ $^ $(LDFLAGS)
	
$(BUILD)/flash-fuse-imx8mn: $(addprefix $(BUILD)/, flash-fuse-imx8mn.o flash-fuse-common.o flash-fuse-main.o log.o)
	$(CXX) -o $@ $^ $(LDFLAGS)
	
$(BUILD)/test-flash-fuse: $(addprefix $(BUILD)/, test-flash-fuse.o flash-fuse-common.o log.o)
	$(CXX) -o $@ $^ $(LDFLAGS) -lCatch2Main -lCatch2
	
$(BUILD)/%.o: %.cpp 
	mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clang-tidy
clang-tidy:
	clang-tidy $(CLANG_TIDY_FILES) $(CLANG_TIDY_CHECKS) -header-filter=.*,/usr/local/include/* -- $(CXXFLAGS)
