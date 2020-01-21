#pragma once

#include <filesystem.h>
#include <string>
#include <fstream>
#include <memory>

//
// Wraps a std::fstream and automatically closes the stream when it falls out of scope.
//
class File
{
private:
	class Stream
	{
	public:
		Stream() = default;

		void Open(const fs::path& path, const std::ios_base::openmode mode)
		{
			m_stream.open(path, mode);
		}

		~Stream()
		{
			if (m_stream.is_open())
			{
				m_stream.close();
			}
		}

		std::fstream m_stream;
	};

public:
	static File Load(const fs::path& path, const std::ios_base::openmode mode)
	{
		std::shared_ptr<Stream> pStream = std::make_shared<Stream>();
		pStream->Open(path, mode);

		return File(pStream);
	}

	~File() = default;

	std::fstream* operator->()
	{
		return &(m_pStream->m_stream);
	}

	const std::fstream* operator->() const
	{
		return &(m_pStream->m_stream);
	}

private:
	File(std::shared_ptr<Stream> pStream) : m_pStream(pStream) { }

	std::shared_ptr<Stream> m_pStream;
};