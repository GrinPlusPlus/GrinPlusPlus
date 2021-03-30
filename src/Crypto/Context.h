#pragma once

#include <secp256k1-zkp/secp256k1.h>
#include <secp256k1-zkp/secp256k1_bulletproofs.h>
#include <Crypto/CSPRNG.h>
#include <Crypto/Models/SecretKey.h>
#include <Core/Exceptions/CryptoException.h>

namespace secp256k1 {

class Context
{
public:
    Context()
    {
        m_pContext = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
        m_pGenerators = secp256k1_bulletproof_generators_create(m_pContext, &secp256k1_generator_const_g, 256);
    }

    ~Context()
    {
        secp256k1_bulletproof_generators_destroy(m_pContext, m_pGenerators);
        secp256k1_context_destroy(m_pContext);
    }

    secp256k1_context* Randomized()
    {
        const SecretKey randomSeed = CSPRNG::GenerateRandom32();
        const int randomize_result = secp256k1_context_randomize(m_pContext, randomSeed.data());
        if (randomize_result != 1) {
            throw CRYPTO_EXCEPTION_F("secp256k1_context_randomize failed with error {}", randomize_result);
        }

        return m_pContext;
    }

    secp256k1_context* Get() noexcept { return m_pContext; }
    const secp256k1_context* Get() const noexcept { return m_pContext; }

    const secp256k1_bulletproof_generators* GetGenerators() const noexcept { return m_pGenerators; }

private:
    secp256k1_context* m_pContext;
    secp256k1_bulletproof_generators* m_pGenerators;
};

}