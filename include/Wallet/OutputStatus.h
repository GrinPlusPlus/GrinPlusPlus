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