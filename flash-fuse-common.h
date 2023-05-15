#include <string>
#include <map>
#include <array>
#include <memory>

constexpr uint32_t BIT(int n)
{
	return 1 << n;
}

constexpr uint32_t MASK(int start, int end)
{
	uint32_t ret = 0;
	for (int n = start; n <= end; n++)
		ret |= 1 << n;
	return ret;
}

class IFuse {
public:
	virtual ~IFuse() = default;

	/* Check if argument valid */
	virtual bool valid_arg(const std::string& arg) const = 0;
	/* Check if argument can be set on top of currently fused value */
	virtual bool is_fuseable(const std::string& arg) const = 0;
	/* Return fused value */
	virtual std::string get() const = 0;
	/* Write arg */
	virtual void set(const std::string& arg) = 0;
};

/* Define per platform */
std::unique_ptr<IFuse> make_fuse(std::string nvmem, const std::string& name);
/* For print_usage*/
std::string available_fuses();

/* Check fuse against bits in flags map.
 *
 * Will read fuse bank from offset, mask with ctor argument mask
 * and compare with bits value in flags vector. I.e:
 *    for (const auto& [name, bits] : flags)
 *        if (fuse_value & mask) == bits
 *           return name;
 *    return "UNKNOWN"
 * */
class FlagFuse : public IFuse {
public:
	FlagFuse(std::string nvmem, int offset, uint32_t mask, std::map<std::string, uint32_t> flags);

	bool valid_arg(const std::string& arg) const override;
	bool is_fuseable(const std::string& arg) const override;
	std::string get() const override;
	void set(const std::string& arg) override;

private:
	std::string mnvmem;
	int moffset;
	uint32_t mmask;
	std::map<std::string, uint32_t> mflags;
};

class MACFuse : public IFuse {
public:
	MACFuse(std::string nvmem, int offset1, int offset2);

	bool valid_arg(const std::string& arg) const override;
	bool is_fuseable(const std::string& arg) const override;
	std::string get() const override;
	void set(const std::string& arg) override;

private:
	std::string mnvmem;
	const int moffset1;
	const int moffset2;
};

class SRKFuse : public IFuse {
public:
	SRKFuse(std::string nvmem, std::array<int, 8> offset);

	bool valid_arg(const std::string& arg) const override;
	bool is_fuseable(const std::string& arg) const override;
	std::string get() const override;
	void set(const std::string& arg) override;

private:
	std::string mnvmem;
	std::array<int, 8> moffset;
};
