#include <iostream>

#include <Consensus.h>
#include <Common/Util/FileUtil.h>
#include <Core/Global.h>
#include <Core/Context.h>
#include <Core/Models/Transaction.h>
#include <Core/Util/JsonUtil.h>
#include <Core/Validation/TransactionValidator.h>

#include "../../src/P2P/Messages/TransactionMessage.h"

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cout << "Usage: tx_verifier <tx.json>" << std::endl;
        return -1;
    }

    auto pContext = Context::Create(Environment::MAINNET, Config::Default(Environment::MAINNET));
    Global::Init(pContext);

    std::vector<uint8_t> bytes;
    if (!FileUtil::ReadFile(argv[1], bytes)) {
        std::cout << "Failed to read json file at: " << argv[1] << std::endl;
        return -1;
    }

    std::string tx_json_str = std::string(bytes.cbegin(), bytes.cend());
    Json::Value json;
    if (!JsonUtil::Parse(tx_json_str, json)) {
        std::cout << "Failed to parse json" << std::endl;
        return -1;
    }

    std::unique_ptr<Transaction> pTransaction = nullptr;
    try
    {
        pTransaction = std::make_unique<Transaction>(Transaction::FromJSON(json));
    }
    catch (std::exception& e)
    {
        std::cout << "Failed to parse transaction." << std::endl;
        std::cout << e.what() << std::endl;
        return -1;
    }

    try
    {
        TransactionValidator().Validate(*pTransaction, (Consensus::HARD_FORK_INTERVAL * 4));
    }
    catch (std::exception& e)
    {
        std::cout << "Transaction validation failed." << std::endl;
        std::cout << e.what() << std::endl;
        return -1;
    }

    std::cout << "Transaction is valid" << std::endl;

    bytes = TransactionMessage(std::make_shared<Transaction>(*pTransaction)).Serialize(EProtocolVersion::V2);
    std::cout << HexUtil::ConvertToHex(bytes) << std::endl;


    std::cout << HexUtil::ConvertToHex(std::vector<uint8_t>{ bytes.begin() + 11, bytes.end() }) << std::endl;

    return 0;
}
