#pragma once

#include <Core/Exceptions/UnimplementedException.h>
#include <Core/Traits/Batchable.h>
#include <memory>
#include <shared_mutex>
#include <mutex>
#include <tuple>

template<class T>
class Reader
{
	template<class U>
	class InnerReader
	{
	public:
		InnerReader(std::shared_ptr<const U> pObject, std::shared_ptr<std::shared_mutex> pMutex, const bool lock, const bool unlock)
			: m_pObject(pObject), m_pMutex(pMutex), m_unlock(unlock)
		{
			if (lock)
			{
				m_pMutex->lock_shared();
			}
		}

		~InnerReader()
		{
			if (m_unlock)
			{
				m_pMutex->unlock_shared();
			}
		}

		std::shared_ptr<const U> m_pObject;
		std::shared_ptr<std::shared_mutex> m_pMutex;
		bool m_unlock;
	};

public:
	static Reader Create(std::shared_ptr<T> pObject, std::shared_ptr<std::shared_mutex> pMutex, const bool lock, const bool unlock)
	{
		return Reader(std::make_shared<InnerReader<T>>(pObject, pMutex, lock, unlock));
	}

	Reader() = default;
	virtual ~Reader() = default;

	const T* operator->() const
	{
		return m_pReader->m_pObject.get();
	}

	const T& operator*() const
	{
		return *m_pReader->m_pObject;
	}

	std::shared_ptr<const T> GetShared() const
	{
		return m_pReader->m_pObject;
	}

	bool IsNull() const
	{
		return m_pReader->m_pObject == nullptr;
	}

private:
	Reader(const std::shared_ptr<InnerReader<T>>& pReader)
		: m_pReader(pReader)
	{

	}

	std::shared_ptr<InnerReader<T>> m_pReader;
};

class MutexUnlocker
{
public:
	MutexUnlocker(const std::shared_ptr<std::shared_mutex>& pMutex)
		: m_pMutex(pMutex)
	{

	}

	~MutexUnlocker()
	{
		m_pMutex->unlock();
	}

private:
	std::shared_ptr<std::shared_mutex> m_pMutex;
};

template<class T>
class Writer : public Reader<T>
{
	template<class U>
	class InnerWriter
	{
	public:
		InnerWriter(const bool batched, std::shared_ptr<U> pObject, std::shared_ptr<std::shared_mutex> pMutex, const bool lock)
			: m_batched(batched), m_pObject(pObject), m_pMutex(pMutex)
		{
			if (lock)
			{
				m_pMutex->lock();
			}

			OnInitWrite(batched);
		}

		~InnerWriter()
		{
			try
			{
				// Using MutexUnlocker in case exception is thrown.
				MutexUnlocker unlocker(m_pMutex);
				if (m_batched)
				{
					Rollback();
				}
				else
				{
					Commit();
				}

				OnEndWrite();
			}
			catch (std::exception&)
			{
				// TODO: Handle
			}
		}

		void Commit()
		{
			Traits::IBatchable* pBatchable = GetBatchable(m_pObject);
			if (pBatchable != nullptr)
			{
				if (pBatchable->IsDirty())
				{
					pBatchable->Commit();
					pBatchable->SetDirty(false);
				}
			}
		}

		void Rollback() noexcept
		{
			Traits::IBatchable* pBatchable = GetBatchable(m_pObject);
			if (pBatchable != nullptr)
			{
				if (pBatchable->IsDirty())
				{
					pBatchable->Rollback();
					pBatchable->SetDirty(false);
				}
			}
		}

		void OnInitWrite(const bool batch)
		{
			Traits::IBatchable* pBatchable = GetBatchable(m_pObject);
			if (pBatchable != nullptr)
			{
				pBatchable->SetDirty(true);
				pBatchable->OnInitWrite(batch);
			}
		}

		void OnEndWrite()
		{
			Traits::IBatchable* pBatchable = GetBatchable(m_pObject);
			if (pBatchable != nullptr)
			{
				pBatchable->OnEndWrite();
			}
		}

		bool m_batched;
		std::shared_ptr<U> m_pObject;
		std::shared_ptr<std::shared_mutex> m_pMutex;

	private:
		template<typename V = void, typename std::enable_if_t<std::is_polymorphic_v<U>, V>* = nullptr>
		Traits::IBatchable* GetBatchable(const std::shared_ptr<U>& pObject) const
		{
			return dynamic_cast<Traits::IBatchable*>(pObject.get());
		}

		template<typename V = void, typename std::enable_if_t<!std::is_polymorphic_v<U>, V>* = nullptr>
		Traits::IBatchable* GetBatchable(const std::shared_ptr<U>&) const
		{
			return nullptr;
		}
	};

public:
	static Writer Create(const bool batched, const std::shared_ptr<T>& pObject, const std::shared_ptr<std::shared_mutex>& pMutex, const bool lock)
	{
		return Writer(std::shared_ptr<InnerWriter<T>>(new InnerWriter<T>(batched, pObject, pMutex, lock)));
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

	T& operator*()
	{
		return *m_pWriter->m_pObject;
	}

	const T& operator*() const
	{
		return *m_pWriter->m_pObject;
	}

	std::shared_ptr<T> GetShared()
	{
		return m_pWriter->m_pObject;
	}

	std::shared_ptr<const T> GetShared() const
	{
		return m_pWriter->m_pObject;
	}

	bool IsNull() const noexcept
	{
		return m_pWriter == nullptr;
	}

	void Clear()
	{
		m_pWriter = nullptr;
	}

private:
	Writer(const std::shared_ptr<InnerWriter<T>>& pWriter)
		: m_pWriter(pWriter), Reader<T>(Reader<T>::Create(pWriter->m_pObject, pWriter->m_pMutex, false, false))
	{

	}

	std::shared_ptr<InnerWriter<T>> m_pWriter;
};

template<class T>
class Locked
{
	friend class MultiLocker;

public:
	Locked(const std::shared_ptr<T>& pObject)
		: m_pObject(pObject), m_pMutex(std::make_shared<std::shared_mutex>())
	{

	}

	Reader<T> Read() const
	{
		return Reader<T>::Create(m_pObject, m_pMutex, true, true);
	}

	Reader<T> Read(std::adopt_lock_t) const
	{
		return Reader<T>::Create(m_pObject, m_pMutex, false, true);
	}

	Writer<T> Write()
	{
		return Writer<T>::Create(false, m_pObject, m_pMutex, true);
	}

	Writer<T> Write(std::adopt_lock_t)
	{
		return Writer<T>::Create(false, m_pObject, m_pMutex, false);
	}

	Writer<T> BatchWrite()
	{
		Traits::IBatchable* pBatchable = dynamic_cast<Traits::IBatchable*>(m_pObject.get());
		if (pBatchable == nullptr)
		{
			throw UNIMPLEMENTED_EXCEPTION;
		}

		return Writer<T>::Create(true, m_pObject, m_pMutex, true);
	}

	Writer<T> BatchWrite(std::adopt_lock_t)
	{
		Traits::IBatchable* pBatchable = dynamic_cast<Traits::IBatchable*>(m_pObject.get());
		if (pBatchable == nullptr)
		{
			throw UNIMPLEMENTED_EXCEPTION;
		}

		return Writer<T>::Create(true, m_pObject, m_pMutex, false);
	}

private:
	std::shared_ptr<T> m_pObject;
	std::shared_ptr<std::shared_mutex> m_pMutex;
};

//
// Safely obtains the lock for multiple Locked<> objects at once.
//
class MultiLocker
{
public:
	// Wrapper around shared_mutex which redirects lock/unlock/try_lock to the _shared variant.
	class SharedOnlyMutex
	{
	public:
		SharedOnlyMutex(std::shared_mutex& mutex) : m_mutex(mutex) {}

		void lock() noexcept { m_mutex.lock_shared(); }
		void unlock() { m_mutex.unlock_shared(); }
		bool try_lock() noexcept { return m_mutex.try_lock_shared(); }

	private:
		std::shared_mutex& m_mutex;
	};

	//
	// Obtains a lock for all of the Locked<> objects, and returns a tuple containing a Writer<> for each.
	//
	template <class T1, class T2, class... TN>
	std::tuple<Writer<T1>, Writer<T2>, Writer<TN> ...> Lock(const Locked<T1>& lock1, const Locked<T2>& lock2, const Locked<TN>&... lockN)
	{
		std::lock(*lock1.m_pMutex, *lock2.m_pMutex, ConvertArg(lockN) ...);
		return std::make_tuple(GetWriter(lock1), GetWriter(lock2), GetWriter(lockN) ...);
	}

	//
	// Obtains a lock for all of the Locked<> objects, and returns a tuple containing a batched Writer<> for each.
	//
	template <class T1, class T2, class... TN>
	std::tuple<Writer<T1>, Writer<T2>, Writer<TN> ...> BatchLock(const Locked<T1>& lock1, const Locked<T2>& lock2, const Locked<TN>&... lockN)
	{
		std::lock(*lock1.m_pMutex, *lock2.m_pMutex, ConvertArg(lockN) ...);
		return std::make_tuple(GetBatchWriter(lock1), GetBatchWriter(lock2), GetBatchWriter(lockN) ...);
	}

	//
	// Obtains a shared lock for all of the Locked<> objects, and returns a tuple containing a Reader<> for each.
	//
	template <class T1, class T2, class... TN>
	std::tuple<Reader<T1>, Reader<T2>, Reader<TN> ...> LockShared(const Locked<T1>& lock1, const Locked<T2>& lock2, const Locked<TN>&... lockN)
	{
		LockShared_(ConvertArgShared(lock1), ConvertArgShared(lock2), ConvertArgShared(lockN) ...);
		return std::make_tuple(GetReader(lock1), GetReader(lock2), GetReader(lockN) ...);
	}

private:
	template<class... T>
	void LockShared_(SharedOnlyMutex lock1, SharedOnlyMutex lock2, T... lockN)
	{
		std::lock(lock1, lock2, lockN ...);
	}

	template<class T>
	static std::shared_mutex& ConvertArg(Locked<T> lock)
	{
		return *lock.m_pMutex;
	}

	template<class T>
	static SharedOnlyMutex ConvertArgShared(Locked<T> lock)
	{
		return SharedOnlyMutex(*lock.m_pMutex);
	}

	template<class T>
	static Writer<T> GetWriter(Locked<T> lock)
	{
		return lock.Write(std::adopt_lock);
	}

	template<class T>
	static Writer<T> GetBatchWriter(Locked<T> lock)
	{
		return lock.BatchWrite(std::adopt_lock);
	}

	template<class T>
	static Reader<T> GetReader(Locked<T> lock)
	{
		return lock.Read(std::adopt_lock);
	}
};