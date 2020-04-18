#pragma once

#include <string>
#include <Common/Util/StringUtil.h>

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

	static std::string ToKey(const EWalletTxType type)
	{
		switch (type)
		{
			case COINBASE:
			{
				return "COINBASE";
			}
			case SENT:
			{
				return "SENT";
			}
			case RECEIVED:
			{
				return "RECEIVED";
			}
			case SENT_CANCELED:
			{
				return "SENT_CANCELED";
			}
			case RECEIVED_CANCELED:
			{
				return "RECEIVED_CANCELED";
			}
			case SENDING_STARTED:
			{
				return "SENDING_NOT_FINALIZED";
			}
			case RECEIVING_IN_PROGRESS:
			{
				return "RECEIVING_IN_PROGRESS";
			}
			case SENDING_FINALIZED:
			{
				return "SENDING_FINALIZED";
			}
		}

		return "";
	}

	static EWalletTxType FromKey(const std::string& input)
	{
		std::string key = StringUtil::ToUpper(input);
		if (key == "COINBASE")
		{
			return EWalletTxType::COINBASE;
		}
		else if (key == "SENT")
		{
			return EWalletTxType::SENT;
		}
		else if (key == "RECEIVED")
		{
			return EWalletTxType::RECEIVED;
		}
		else if (key == "SENT_CANCELED")
		{
			return EWalletTxType::SENT_CANCELED;
		}
		else if (key == "RECEIVED_CANCELED")
		{
			return EWalletTxType::RECEIVED_CANCELED;
		}
		else if (key == "SENDING_NOT_FINALIZED")
		{
			return EWalletTxType::SENDING_STARTED;
		}
		else if (key == "RECEIVING_IN_PROGRESS")
		{
			return EWalletTxType::RECEIVING_IN_PROGRESS;
		}
		else if (key == "SENDING_FINALIZED")
		{
			return EWalletTxType::SENDING_FINALIZED;
		}

		throw std::exception();
	}
}