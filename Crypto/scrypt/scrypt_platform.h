#ifndef _SCRYPT_PLATFORM_H_
#define	_SCRYPT_PLATFORM_H_

#if defined(SCRYPT_CONFIG_H_FILE)
#include SCRYPT_CONFIG_H_FILE
#elif defined(HAVE_SCRYPT_CONFIG_H)
#include "config.h"
#else
#error Need either SCRYPT_CONFIG_H_FILE or HAVE_SCRYPT_CONFIG_H defined.
#endif

#endif /* !_SCRYPT_PLATFORM_H_ */
