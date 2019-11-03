#pragma once

#include <Common/Exceptions/UnimplementedException.h>
#include <Core/Traits/Batchable.h>
#include <memory>
#include <shared_mutex>

template<class T>
class Reader
{
	template<class U>
	class InnerReader
	{
	public:
		InnerReader(std::shared_ptr<const U> pObject, std::shared_ptr<std::shared_mutex> pMutex, const bool lock)
			: m_pObject(pObject), m_pMutex(pMutex), m_lock(lock)
		{
			if (m_lock)
			{
				m_pMutex->lock_shared();
			}
		}

		~InnerReader()
		{
			if (m_lock)
			{
				m_pMutex->unlock_shared();
			}
		}

		std::shared_ptr<const U> m_pObject;
		std::shared_ptr<std::shared_mutex> m_pMutex;
		bool m_lock;
	};

public:
	static Reader Create(std::shared_ptr<T> pObject, std::shared_ptr<std::shared_mutex> pMutex, const bool lock = true)
	{
		return Reader(std::shared_ptr<InnerReader<T>>(new InnerReader<T>(pObject, pMutex, lock)));
	}

	Reader() = default;
	virtual ~Reader() = default;

	const T* operator->() const
	{
		return m_pReader->m_pObject.get();
	}

	std::shared_ptr<const T> GetShared() const
	{
		return m_pReader->m_pObject;
	}

private:
	Reader(std::shared_ptr<InnerReader<T>> pReader)
		: m_pReader(pReader)
	{

	}

	std::shared_ptr<InnerReader<T>> m_pReader;
};

template<class T>
class Writer : virtual public Reader<T>
{
	template<class U>
	class InnerWriter
	{
	public:
		InnerWriter(const bool batched, std::shared_ptr<U> pObject, std::shared_ptr<std::shared_mutex> pMutex) // TODO: Pass lock guard around instead
			: m_batched(batched), m_pObject(pObject), m_pMutex(pMutex)
		{
			m_pMutex->lock();
			OnInitWrite();
		}

		virtual ~InnerWriter()
		{
			try
			{
				if (m_batched)
				{
					if (IsDirty())
					{
						Rollback();
					}
				}
				else
				{
					Commit();
				}

				OnEndWrite();
			}
			catch (std::exception& e)
			{
				m_pMutex->unlock();
				//throw;
			}

			m_pMutex->unlock();
		}

		void Commit()
		{
			if (std::is_base_of<Traits::Batchable, U>::value)
			{
				((Traits::Batchable*)m_pObject.get())->Commit();
			}
		}

		void Rollback()
		{
			if (std::is_base_of<Traits::Batchable, U>::value)
			{
				((Traits::Batchable*)m_pObject.get())->Rollback();
			}
		}

		void OnInitWrite()
		{
			if (std::is_base_of<Traits::Batchable, U>::value)
			{
				auto pBatchable = (Traits::Batchable*)m_pObject.get();
				pBatchable->SetDirty(false);
				pBatchable->OnInitWrite();
			}
		}

		void OnEndWrite()
		{
			if (std::is_base_of<Traits::Batchable, U>::value)
			{
				auto pBatchable = (Traits::Batchable*)m_pObject.get();
				pBatchable->SetDirty(false);
				pBatchable->OnEndWrite();
			}
		}

		bool IsDirty()
		{
			if (std::is_base_of<Traits::Batchable, U>::value)
			{
				return ((Traits::Batchable*)m_pObject.get())->IsDirty();
			}

			return false;
		}

		bool m_batched;
		std::shared_ptr<U> m_pObject;
		std::shared_ptr<std::shared_mutex> m_pMutex;
	};

public:
	static Writer Create(const bool batched, std::shared_ptr<T> pObject, std::shared_ptr<std::shared_mutex> pMutex)
	{
		return Writer(std::shared_ptr<InnerWriter<T>>(new InnerWriter<T>(batched, pObject, pMutex)));
	}

	Writer() = default;
	virtual ~Writer() = default;

	T* operator->()
	{
		return m_pWriter->m_pObject.get();
	}

	const T* operator->() const
	{
		return m_pWriter->m_pObject.get();
	}

	std::shared_ptr<T> GetShared()
	{
		return m_pWriter->m_pObject;
	}

	std::shared_ptr<const T> GetShared() const
	{
		return m_pWriter->m_pObject;
	}

	bool IsNull() const
	{
		return m_pWriter == nullptr;
	}

	void Clear()
	{
		m_pWriter = nullptr;
	}

private:
	Writer(std::shared_ptr<InnerWriter<T>> pWriter)
		: m_pWriter(pWriter), Reader(Reader::Create(pWriter->m_pObject, pWriter->m_pMutex, false))
	{

	}

	std::shared_ptr<InnerWriter<T>> m_pWriter;
};

template<class T>
class Locked
{
public:
	Locked(std::shared_ptr<T> pObject)
		: m_pObject(pObject), m_pMutex(std::make_shared<std::shared_mutex>())
	{

	}

	virtual ~Locked() = default;

	Reader<T> Read() const
	{
		return Reader<T>::Create(m_pObject, m_pMutex, true);
	}

	Writer<T> Write()
	{
		return Writer<T>::Create(false, m_pObject, m_pMutex);
	}

	Writer<T> BatchWrite()
	{
		if (!std::is_base_of<Traits::Batchable, T>::value)
		{
			throw UNIMPLEMENTED_EXCEPTION;
		}

		return Writer<T>::Create(true, m_pObject, m_pMutex);
	}

private:
	std::shared_ptr<T> m_pObject;
	std::shared_ptr<std::shared_mutex> m_pMutex;
};