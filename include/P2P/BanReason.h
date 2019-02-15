#pragma once

#include <string>

enum EBanReason
{
	None = 0,
	BadBlock = 1,
	BadCompactBlock = 2,
	BadBlockHeader = 3,
	BadTxHashSet = 4,
	ManualBan = 5,
	FraudHeight = 6,
	BadHandshake = 7
};

namespace BanReason
{
	static std::string Format(const EBanReason& banReason)
	{
		switch (banReason)
		{
			case None:
				return "None";
			case BadBlock:
				return "BadBlock";
			case BadCompactBlock:
				return "BadCompactBlock";
			case BadBlockHeader:
				return "BadBlockHeader";
			case BadTxHashSet:
				return "BadTxHashSet";
			case ManualBan:
				return "ManualBan";
			case FraudHeight:
				return "FraudHeight";
			case BadHandshake:
				return "BadHandshake";
		}

		return "";
	}
}