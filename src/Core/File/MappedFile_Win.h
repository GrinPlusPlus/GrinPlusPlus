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
	struct MemMap
	{
		bool IsMapped() const noexcept { return mapping_handle != INVALID_HANDLE_VALUE; }

		mio::file_handle_type mapping_handle;
		const char* mapped_view;
	};

public:
	using UPtr = std::unique_ptr<MappedFile>;

	MappedFile(const fs::path& path, const mio::file_handle_type handle) noexcept
		: m_path(path), m_handle(handle)
	{
		m_mmap.mapping_handle = INVALID_HANDLE_VALUE;
		m_mmap.mapped_view = nullptr;
	}
	virtual ~MappedFile();

	bool Write(const size_t startIndex, const std::vector<uint8_t>& data) final;
	void Read(const uint64_t position, const uint64_t numBytes, std::vector<uint8_t>& data) const final;

private:
	void Map() const;
	void Unmap() const;

	fs::path m_path;
	mio::file_handle_type m_handle;
	mutable MemMap m_mmap;
	mutable std::mutex m_mutex;
};