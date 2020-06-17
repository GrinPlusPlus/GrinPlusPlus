#pragma once
#pragma warning(disable: 4505) // Unreferenced local function has been removed

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include <Core/Exceptions/DeserializationException.h>

enum EOutputFeatures
{
	// 0 = No Flags
	DEFAULT,

	// 1 = Output is a coinbase output, must not be spent until maturity
	COINBASE_OUTPUT
};

namespace OutputFeatures
{
	static std::string ToString(const EOutputFeatures& features)
	{
		switch (features)
		{
			case EOutputFeatures::DEFAULT:
				return "Plain";
			case EOutputFeatures::COINBASE_OUTPUT:
				return "Coinbase";
		}

		return "";
	}

	static EOutputFeatures FromString(const std::string& string)
	{
		std::string upper = StringUtil::ToUpper(string);
		if (upper == "TRANSACTION" || upper == "PLAIN")
		{
			return EOutputFeatures::DEFAULT;
		}
		else if (upper == "COINBASE")
		{
			return EOutputFeatures::COINBASE_OUTPUT;
		}

		throw DESERIALIZATION_EXCEPTION_F("Failed to deserialize output feature: {}", string);
	}
}

enum EKernelFeatures
{
	// No flags
	DEFAULT_KERNEL = 0,

	// Kernel matching a coinbase output
	COINBASE_KERNEL = 1,

	// Absolute height locked kernel; has fee and lock_height
	HEIGHT_LOCKED = 2,

	// No recent duplicate (NRD)
	NO_RECENT_DUPLICATE = 3
};

namespace KernelFeatures
{
	static std::string ToString(const EKernelFeatures& features)
	{
		switch (features)
		{
			case DEFAULT_KERNEL:
				return "Plain";
			case COINBASE_KERNEL:
				return "Coinbase";
			case HEIGHT_LOCKED:
				return "HeightLocked";
			case NO_RECENT_DUPLICATE:
				return "NoRecentDuplicate";
		}

		return "";
	}

	static EKernelFeatures FromString(const std::string& string)
	{
		if (string == "Plain")
		{
			return EKernelFeatures::DEFAULT_KERNEL;
		}
		else if (string == "Coinbase")
		{
			return EKernelFeatures::COINBASE_KERNEL;
		}
		else if (string == "HeightLocked")
		{
			return EKernelFeatures::HEIGHT_LOCKED;
		}
		else if (string == "NoRecentDuplicate")
		{
			return EKernelFeatures::NO_RECENT_DUPLICATE;
		}

		throw DESERIALIZATION_EXCEPTION_F("Failed to deserialize kernel feature: {}", string);
	}
}