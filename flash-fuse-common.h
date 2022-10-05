#include <string>
#include <map>
#include <memory>

class IFuse {
public:
	virtual ~IFuse() = default;

	/* Check if argument valid */
	virtual bool valid_arg(const std::string& arg) const = 0;
	/* Return fused value */
	virtual std::string get() const = 0;
	/* Write arg */
	virtual void set(const std::string& arg) = 0;
};

/* Define per platform */
std::unique_ptr<IFuse> make_fuse(std::string nvmem, const std::string& name);

/* Check fuse against masks in flags parameter */
class FlagFuse : public IFuse {
public:
	FlagFuse(std::string nvmem, int offset, std::map<std::string, uint32_t> flags);

	bool valid_arg(const std::string& arg) const override;
	std::string get() const override;
	void set(const std::string& arg) override;

private:
	std::string mnvmem;
	int moffset;
	std::map<std::string, uint32_t> mflags;
};

class MACFuse : public IFuse {
public:
	MACFuse(std::string nvmem, int offset1, int offset2);

	bool valid_arg(const std::string& arg) const override;
	std::string get() const override;
	void set(const std::string& arg) override;

private:
	std::string mnvmem;
	const int moffset1 = 0x90;
	const int moffset2 = 0x94;
};

