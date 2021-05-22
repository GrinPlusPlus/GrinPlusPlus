#pragma once

#include <string>

enum EOutputStatus
{
	SPENDABLE = 0,
	IMMATURE = 1, // DEPRECATED: Outputs should be marked as spendable.
	NO_CONFIRMATIONS = 2,
	SPENT = 3,
	LOCKED = 4,
	CANCELED = 5
};

namespace OutputStatus
{
	static EOutputStatus FromString(const std::string& value)
	{
		if (value == "Spendable")
		{
			return EOutputStatus::SPENDABLE;
		}
		else if (value == "Immature")
		{
			return EOutputStatus::IMMATURE;
		}
		else if (value == "No Confirmations")
		{
			return EOutputStatus::NO_CONFIRMATIONS;
		}
		else if (value == "Spent")
		{
			return EOutputStatus::SPENT;
		}
		else if (value == "Locked")
		{
			return EOutputStatus::LOCKED;
		}
		else if (value == "Canceled")
		{
			return EOutputStatus::CANCELED;
		}

		throw DESERIALIZATION_EXCEPTION_F("Invalid output status: {}", value);
	}

	static std::string ToString(const EOutputStatus status)
	{
		switch (status)
		{
			case SPENDABLE:
			{
				return "Spendable";
			}
			case IMMATURE:
			{
				return "Immature";
			}
			case NO_CONFIRMATIONS:
			{
				return "No Confirmations";
			}
			case SPENT:
			{
				return "Spent";
			}
			case LOCKED:
			{
				return "Locked";
			}
			case CANCELED:
			{
				return "Canceled";
			}
		}

		return "";
	}
}