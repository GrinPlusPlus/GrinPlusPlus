#include <ThirdParty/Catch2/catch.hpp>

#include <Core/Models/BlockHeader.h>

TEST_CASE("BlockHeader::Deserialize")
{
	const uint16_t version = 1;
	const uint64_t height = 2;
	const int64_t timestamp = 3;

	CBigInteger<32> previousBlockHash = CBigInteger<32>::FromHex("0x0102030405060708090A0B0C0D0E0F1112131415161718191A1B1C1D1E1F");
	CBigInteger<32> previousRoot = CBigInteger<32>::FromHex("0x1102030405060708090A0B0C0D0E0F1112131415161718191A1B1C1D1E1F");
	CBigInteger<32> outputRoot = CBigInteger<32>::FromHex("0x2102030405060708090A0B0C0D0E0F1112131415161718191A1B1C1D1E1F");
	CBigInteger<32> rangeProofRoot = CBigInteger<32>::FromHex("0x3102030405060708090A0B0C0D0E0F1112131415161718191A1B1C1D1E1F");
	CBigInteger<32> kernelRoot = CBigInteger<32>::FromHex("0x4102030405060708090A0B0C0D0E0F1112131415161718191A1B1C1D1E1F");
	CBigInteger<32> totalKernelOffset = CBigInteger<32>::FromHex("0x5102030405060708090A0B0C0D0E0F1112131415161718191A1B1C1D1E1F");

	const uint64_t outputMMRSize = 4;
	const uint64_t kernelMMRSize = 5;

	// TODO: Finish this
	//const BlockHeader blockHeader(version, height, timestamp, std::move(previousBlockHash), std::move(previousRoot), std::move(outputRoot), std::move(rangeProofRoot), std::move(kernelRoot), std::move(totalKernelOffset), outputMMRSize, kernelMMRSize, std::move(proofOfWork));
}