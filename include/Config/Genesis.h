#pragma once

#include <Core/BlockHeader.h>
#include <Core/FullBlock.h>

namespace Genesis
{
	static FullBlock TESTNET4_GENESIS
	(
		BlockHeader
		(
			(uint16_t)1 /* version */,
			0 /*height*/,
			1539806400 /*1534204800*/ /*timestamp*/, // Utc.ymd(2018, 8, 14).and_hms(0, 0, 0),
			CBigInteger<32>(std::vector<unsigned char>(32, (unsigned char)0xFF)) /*prevHash*/,
			CBigInteger<32>(std::vector<unsigned char>(32, (unsigned char)0x00)) /*prevRoot*/,
			CBigInteger<32>(std::vector<unsigned char>(32, (unsigned char)0x00)) /*outputRoot*/,
			CBigInteger<32>(std::vector<unsigned char>(32, (unsigned char)0x00)) /*rangeProofRoot*/,
			CBigInteger<32>(std::vector<unsigned char>(32, (unsigned char)0x00)) /*kernelRoot*/,
			CBigInteger<32>(std::vector<unsigned char>(32, (unsigned char)0x00)) /*totalKernelOffset*/,
			0/*outputMMRSize*/,
			0/*kernelMMRSize*/,
			ProofOfWork
			(
				1856000 /*totalDifficulty*/,
				1856 /*scalingDifficulty*/,
				8612241555342799290,
				29,
				std::vector<uint64_t>
				({
					0x46f3b4, 0x1135f8c, 0x1a1596f, 0x1e10f71, 0x41c03ea, 0x63fe8e7, 0x65af34f,
					0x73c16d3, 0x8216dc3, 0x9bc75d0, 0xae7d9ad, 0xc1cb12b, 0xc65e957, 0xf67a152,
					0xfac6559, 0x100c3d71, 0x11eea08b, 0x1225dfbb, 0x124d61a1, 0x132a14b4, 0x13f4ec38,
					0x1542d236, 0x155f2df0, 0x1577394e, 0x163c3513, 0x19349845, 0x19d46953, 0x19f65ed4,
					0x1a0411b9, 0x1a2fa039, 0x1a72a06c, 0x1b02ddd2, 0x1b594d59, 0x1b7bffd3, 0x1befe12e,
					0x1c82e4cd, 0x1d492478, 0x1de132a5, 0x1e578b3c, 0x1ed96855, 0x1f222896, 0x1fea0da6
				})
			)
		),
		TransactionBody
		{
			std::vector<TransactionInput>(),
			std::vector<TransactionOutput>(),
			std::vector<TransactionKernel>()
		}
	);

	// MAINNET: Add Genesis Block
}