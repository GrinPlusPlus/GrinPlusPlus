#pragma once

#include <string>

enum EWalletTxType
{
	COINBASE = 0,
	SENT = 1,
	RECEIVED = 2,
	SENT_CANCELED = 3,
	RECEIVED_CANCELED = 4,
	SENDING_STARTED = 5,
	RECEIVING_IN_PROGRESS = 6,
	SENDING_FINALIZED = 7
};

namespace WalletTxType
{
	static std::string ToString(const EWalletTxType type)
	{
		switch (type)
		{
			case COINBASE:
			{
				return "Coinbase";
			}
			case SENT:
			{
				return "Sent";
			}
			case RECEIVED:
			{
				return "Received";
			}
			case SENT_CANCELED:
			case RECEIVED_CANCELED:
			{
				return "Canceled";
			}
			case SENDING_STARTED:
			{
				return "Sending (Not Finalized)";
			}
			case RECEIVING_IN_PROGRESS:
			{
				return "Receiving (Unconfirmed)";
			}
			case SENDING_FINALIZED:
			{
				return "Sending (Finalized)";
			}
		}

		return "";
	}
}