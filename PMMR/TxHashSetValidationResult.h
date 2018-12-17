#pragma once

#include <Crypto/Commitment.h>

class TxHashSetValidationResult
{
public:
	TxHashSetValidationResult(const bool success, Commitment&& outputSum, Commitment&& kernelSum)
		: m_success(success), m_outputSum(std::move(outputSum)), m_kernelSum(std::move(kernelSum))
	{

	}

	static TxHashSetValidationResult Fail()
	{
		Commitment outputSum(CBigInteger<33>::ValueOf(0));
		Commitment kernelSum(CBigInteger<33>::ValueOf(0));
		return TxHashSetValidationResult(false, std::move(outputSum), std::move(kernelSum));
	}

	inline bool Successful() const { return m_success; }
	inline const Commitment& GetOutputSum() const { return m_outputSum; }
	inline const Commitment& GetKernelSum() const { return m_kernelSum; }

private:
	bool m_success; // TODO: Change to enum
	Commitment m_outputSum;
	Commitment m_kernelSum;
};