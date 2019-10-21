/*
	a custom randombytes must implement:

	void ED25519_FN(ed25519_randombytes_unsafe) (void *p, size_t len);

	ed25519_randombytes_unsafe is used by the batch verification function
	to create random scalars
*/


#ifdef _WIN32
#include <windows.h>
#pragma comment(lib, "Bcrypt.lib")
#include <bcrypt.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

static void
ED25519_FN(ed25519_randombytes_unsafe) (void *p, size_t len)
{
#ifdef _WIN32
	const NTSTATUS status = BCryptGenRandom(NULL, p, (ULONG)len, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
	if (!BCRYPT_SUCCESS(status))
	{
		// TODO: throw CryptoException("RNG Failure");
	}
#else
	bool success = false;
	int hFile = open("/dev/urandom", O_RDONLY);
	if (hFile >= 0)
	{
		if (read(hFile, p, len) == len)
		{
			success = true;
		}

		close(hFile);
	}

	if (!success)
	{
		// TODO: throw CryptoException("RNG Failure");
	}
#endif
}
