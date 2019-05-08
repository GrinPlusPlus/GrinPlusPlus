#pragma once

enum EOutputStatus
{
	SPENDABLE = 0,
	IMMATURE = 1,
	NO_CONFIRMATIONS = 2,
	SPENT = 3,
	LOCKED = 4,
	CANCELED = 5
};

namespace OutputStatus
{
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