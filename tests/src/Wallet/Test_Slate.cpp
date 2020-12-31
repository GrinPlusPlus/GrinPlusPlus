#include <catch.hpp>

#include <Wallet/Models/Slate/Slate.h>

TEST_CASE("Slate - Binary")
{
    std::string hex = "00040003eee651525e0e4238af7a266412c066a0010102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f200a000000368eda37f001030002faee655fdd08069cc95b9663b3805ac2129cf1bed76793eca8041d1ee36f439e02faee655fdd08069cc95b9663b3805ac2129cf1bed76793eca8041d1ee36f439e0102faee655fdd08069cc95b9663b3805ac2129cf1bed76793eca8041d1ee36f439e02faee655fdd08069cc95b9663b3805ac2129cf1bed76793eca8041d1ee36f439e0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0002faee655fdd08069cc95b9663b3805ac2129cf1bed76793eca8041d1ee36f439e02faee655fdd08069cc95b9663b3805ac2129cf1bed76793eca8041d1ee36f439e01000400000300000000000000000000000000000000000000000000000000000000000000000000030000000000000000000000000000000000000000000000000000000000000000000003000000000000000000000000000000000000000000000000000000000000000001000400000000000000000000000000000000000000000000000000000000000000000000000000000000";
    
    //std::vector<uint8_t> bytes = ;
    ByteBuffer buffer(HexUtil::FromHex(hex));
    Slate slate = Slate::Deserialize(buffer);


    std::cout << slate.ToJSON().toStyledString();

    REQUIRE(slate.version == 4);
    REQUIRE(slate.blockVersion == 3);
    REQUIRE(slate.stage == ESlateStage::STANDARD_SENT);
    REQUIRE(uuids::to_string(slate.slateId) == "eee65152-5e0e-4238-af7a-266412c066a0");
    REQUIRE(slate.numParticipants == 2);
    REQUIRE(slate.ttl == 0);
    REQUIRE(slate.kernelFeatures == EKernelFeatures::COINBASE_KERNEL);
    REQUIRE(!slate.featureArgsOpt.has_value());
    REQUIRE(slate.offset == Hash::FromHex("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20"));
    REQUIRE(slate.amount == 234324899824);
    REQUIRE(slate.fee == Fee());
    REQUIRE(slate.sigs.size() == 3);
    // TODO: sigs

    REQUIRE(slate.commitments.size() == 4);
    // TODO: commitments

    REQUIRE(!slate.proofOpt.has_value());
}