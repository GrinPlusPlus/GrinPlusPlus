#pragma once

#include <Core/Util/JsonUtil.h>
#include <Core/Traits/Jsonable.h>
#include <Core/Exceptions/APIException.h>
#include <Core/Models/TransactionKernel.h>
#include <Core/Models/TransactionOutput.h>
#include <Wallet/KeyChainPath.h>

class BuildCoinbaseResponse : public Traits::IJsonable
{
public:
    BuildCoinbaseResponse(
        const TransactionKernel& kernel,
        const TransactionOutput& output,
        const KeyChainPath& keyChainPath)
        : m_kernel(kernel), m_output(output), m_path(keyChainPath) { }

    BuildCoinbaseResponse(
        TransactionKernel&& kernel,
        TransactionOutput&& output,
        KeyChainPath&& keyChainPath)
        : m_kernel(std::move(kernel)), m_output(std::move(output)), m_path(std::move(keyChainPath)) { }

    virtual ~BuildCoinbaseResponse() = default;

    const TransactionKernel& GetKernel() const noexcept { return m_kernel; }
    const TransactionOutput& GetOutput() const noexcept { return m_output; }
    const KeyChainPath& GetPath() const noexcept { return m_path; }

    Json::Value ToJSON() const final
    {
        Json::Value coinbaseJson;
        coinbaseJson["kernel"] = m_kernel.ToJSON();
        coinbaseJson["output"] = m_output.ToJSON();
        coinbaseJson["key_id"] = m_path.ToHex();

        Json::Value result;
        result["Ok"] = coinbaseJson;

        return result;
    }

private:
    TransactionKernel m_kernel;
    TransactionOutput m_output;
    KeyChainPath m_path;
};