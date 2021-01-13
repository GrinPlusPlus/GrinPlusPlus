#include <iostream>

#include <Core/Global.h>
#include <Core/Context.h>
#include <Common/Util/FileUtil.h>
#include <Core/Models/Transaction.h>
#include <Core/Util/JsonUtil.h>
#include <Core/Validation/TransactionValidator.h>

#include <Wallet/Models/Slate/Slate.h>
#include <Wallet/Models/Slatepack/Armor.h>

static std::unique_ptr<Slate> ParseSlate(const fs::path& file)
{
    std::vector<uint8_t> bytes;
    if (!FileUtil::ReadFile(file, bytes)) {
        std::cout << "Failed to read json file at: " << file << std::endl;
        exit(-1);
    }

    std::string tx_json_str = std::string(bytes.cbegin(), bytes.cend());
    Json::Value json;
    if (JsonUtil::Parse(tx_json_str, json)) {
        return std::make_unique<Slate>(Slate::FromJSON(json));
    } else {
        std::cout << "Failed to parse json. Trying as slatepack." << std::endl;

        SlatepackMessage message = Armor::Unpack(tx_json_str, x25519_keypair_t{ }); // We're not decrypting, so just pass in an empty keypair.

        ByteBuffer byteBuffer(message.m_payload);
        return std::make_unique<Slate>(Slate::Deserialize(byteBuffer));
    }
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cout << "Usage: slate_tool <slate.json>" << std::endl;
        return -1;
    }

    auto pContext = Context::Create(Environment::MAINNET, Config::Default(Environment::MAINNET));
    Global::Init(pContext);

    std::unique_ptr<Slate> pSlate = nullptr;
    
    try
    {
        pSlate = ParseSlate(argv[1]);
    }
    catch (std::exception& e)
    {
        std::cout << "Failed to parse slate." << std::endl;
        std::cout << e.what() << std::endl;
        return -1;
    }

    std::cout << "Slate successfully parsed" << std::endl;

    auto slatepack_path = fs::path(argv[1]).replace_extension("slatepack");
    if (slatepack_path != argv[1]) {
        try {
            FileUtil::WriteTextToFile(slatepack_path, Armor::Pack(SlatepackAddress{}, *pSlate, {}));
        }
        catch (std::exception& e) {
            std::cout << "Failed to write slatepack file at " << slatepack_path << std::endl;
            std::cout << e.what() << std::endl;
        }
    } else {
        std::cout << "Slatepack file already exists at " << slatepack_path << std::endl;
    }

    auto json_path = fs::path(argv[1]).replace_extension("json");
    if (json_path != argv[1]) {
        try {
            FileUtil::WriteTextToFile(json_path, pSlate->ToJSON().toStyledString());
        }
        catch (std::exception& e) {
            std::cout << "Failed to write json file at " << json_path << std::endl;
            std::cout << e.what() << std::endl;
        }
    } else {
        std::cout << "Json file already exists at " << json_path << std::endl;
    }

    system("pause");
    return 0;
}