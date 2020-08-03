#include <Core/File/MappedFile.h>

#pragma warning(push)
#pragma warning(disable:4244)
#pragma warning(disable:4267)
#pragma warning(disable:4334)
#pragma warning(disable:4018)
#include <mio/mmap.hpp>
#pragma warning(pop)

class MappedFile : public IMappedFile
{
public:
	using UPtr = std::unique_ptr<MappedFile>;

	MappedFile(const fs::path& path) noexcept
		: m_path(path) { }
	virtual ~MappedFile();

	bool Write(const size_t startIndex, const std::vector<uint8_t>& data) final;
	void Read(const uint64_t position, const uint64_t numBytes, std::vector<uint8_t>& data) const final;

private:
	void Map() const;
	void Unmap() const;

	fs::path m_path;
	mutable mio::mmap_source m_mmap;
	mutable std::mutex m_mutex;
};