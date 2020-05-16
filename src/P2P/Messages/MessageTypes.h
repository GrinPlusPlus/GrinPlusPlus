#pragma once

#include <P2P/Common.h>

#include <string>

namespace MessageTypes
{
	enum EMessageType
	{
		Error = 0,
		Hand = 1,
		Shake = 2,
		Ping = 3,
		Pong = 4,
		GetPeerAddrs = 5,
		PeerAddrs = 6,
		GetHeaders = 7,
		Header = 8,
		Headers = 9,
		GetBlock = 10,
		Block = 11,
		GetCompactBlock = 12,
		CompactBlockMsg = 13,
		StemTransaction = 14,
		TransactionMsg = 15,
		TxHashSetRequest = 16,
		TxHashSetArchive = 17,
		BanReasonMsg = 18,
		GetTransactionMsg = 19,
		TransactionKernelMsg = 20
	};

	static uint64_t GetMaximumSize(const EMessageType messageType)
	{
		switch (messageType)
		{
			case Error: 
				return 0;
			case Hand: 
				return 128;
			case Shake: 
				return 88;
			case Ping: 
				return 16;
			case Pong: 
				return 16;
			case GetPeerAddrs: 
				return 4;
			case PeerAddrs: 
				return 4 + (1 + 16 + 2) * ((uint64_t)P2P::MAX_PEER_ADDRS);
			case GetHeaders: 
				return 1 + 32 * ((uint64_t)P2P::MAX_LOCATORS);
			case Header:
				return 365;
			case Headers: 
				return 2 + 365 * ((uint64_t)P2P::MAX_BLOCK_HEADERS);
			case GetBlock: 
				return 32;
			case Block: 
				return P2P::MAX_BLOCK_SIZE;
			case GetCompactBlock: 
				return 32;
			case CompactBlockMsg: 
				return P2P::MAX_BLOCK_SIZE / 10;
			case StemTransaction: 
				return P2P::MAX_BLOCK_SIZE;
			case TransactionMsg:
				return P2P::MAX_BLOCK_SIZE;
			case TxHashSetRequest: 
				return 40;
			case TxHashSetArchive: 
				return 64;
			case BanReasonMsg:
				return 64;
			case GetTransactionMsg:
				return 32;
			case TransactionKernelMsg:
				return 32;
		}

		return 0;
	}

	static std::string ToString(const EMessageType messageType)
	{
		switch (messageType)
		{
			case Error:
				return "Msg::Error";
			case Hand:
				return "Msg::Hand";
			case Shake:
				return "Msg::Shake";
			case Ping:
				return "Msg::Ping";
			case Pong:
				return "Msg::Pong";
			case GetPeerAddrs:
				return "Msg::GetPeerAddrs";
			case PeerAddrs:
				return "Msg::PeerAddrs";
			case GetHeaders:
				return "Msg::GetHeaders";
			case Header:
				return "Msg::Header";
			case Headers:
				return "Msg::Headers";
			case GetBlock:
				return "Msg::GetBlock";
			case Block:
				return "Msg::Block";
			case GetCompactBlock:
				return "Msg::GetCompactBlock";
			case CompactBlockMsg:
				return "Msg::CompactBlockMsg";
			case StemTransaction:
				return "Msg::StemTransaction";
			case TransactionMsg:
				return "Msg::TransactionMsg";
			case TxHashSetRequest:
				return "Msg::TxHashSetRequest";
			case TxHashSetArchive:
				return "Msg::TxHashSetArchive";
			case BanReasonMsg:
				return "Msg::BanReasonMsg";
			case GetTransactionMsg:
				return "Msg::GetTransactionMsg";
			case TransactionKernelMsg:
				return "Msg::TransactionKernelMsg";
		}

		return "UNKNOWN";
	}
}