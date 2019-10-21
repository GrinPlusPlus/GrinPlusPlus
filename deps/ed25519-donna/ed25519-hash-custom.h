/*
	a custom hash must have a 512bit digest and implement:

	struct ed25519_hash_context;

	void ed25519_hash_init(ed25519_hash_context *ctx);
	void ed25519_hash_update(ed25519_hash_context *ctx, const uint8_t *in, size_t inlen);
	void ed25519_hash_final(ed25519_hash_context *ctx, uint8_t *hash);
	void ed25519_hash(uint8_t *hash, const uint8_t *in, size_t inlen);
*/

#include "sha512.h"

typedef struct ed25519_hash_context {
	mbedtls_sha512_context* ctx;
} ed25519_hash_context;


static void
ed25519_hash_init(ed25519_hash_context *ctx)
{
	ctx->ctx = malloc(sizeof(mbedtls_sha512_context));
	mbedtls_sha512_init(ctx->ctx);
	mbedtls_sha512_starts(ctx->ctx);
}
static void
ed25519_hash_update(ed25519_hash_context *ctx, const uint8_t *in, size_t inlen)
{
	mbedtls_sha512_update(ctx->ctx, in, inlen);
}
static void
ed25519_hash_final(ed25519_hash_context *ctx, uint8_t *hash)
{
	mbedtls_sha512_finish(ctx->ctx, hash);
	free(ctx->ctx);
	ctx->ctx = NULL;
}
static void
ed25519_hash(uint8_t *hash, const uint8_t *in, size_t inlen)
{
	SHA512(in, inlen, hash);
}

