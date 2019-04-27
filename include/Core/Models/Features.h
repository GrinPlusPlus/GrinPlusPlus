#pragma once
#pragma warning(disable: 4505) // Unreferenced local function has been removed

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include <Core/Serialization/DeserializationException.h>

enum EOutputFeatures
{
	// No Flags
	DEFAULT_OUTPUT = 0,

	// Output is a coinbase output, must not be spent until maturity
	COINBASE_OUTPUT = 1
};

namespace OutputFeatures
{
	static std::string ToString(const EOutputFeatures& features)
	{
		switch (features)
		{
			case DEFAULT_OUTPUT:
				return "Plain";
			case COINBASE_OUTPUT:
				return "Coinbase";
		}

		return "";
	}

	static EOutputFeatures FromString(const std::string& string)
	{
		if (string == "Plain")
		{
			return EOutputFeatures::DEFAULT_OUTPUT;
		}
		else if (string == "Coinbase")
		{
			return EOutputFeatures::COINBASE_OUTPUT;
		}

		throw DeserializationException();
	}
}

enum EKernelFeatures
{
	// No flags
	DEFAULT_KERNEL = 0,

	// Kernel matching a coinbase output
	COINBASE_KERNEL = 1,

	// Absolute height locked kernel; has fee and lock_height
	HEIGHT_LOCKED = 2
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

		throw DeserializationException();
	}
}