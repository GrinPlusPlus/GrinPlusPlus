#pragma once

#include <Core/Util/JsonUtil.h>
#include <Crypto/Models/Commitment.h>
#include <Crypto/Models/ed25519_public_key.h>
#include <Crypto/Models/ed25519_signature.h>

struct PaymentProofDTO
{
	uint64_t amount;
	Commitment excess;
	ed25519_public_key_t recipient_address;
	ed25519_signature_t recipient_sig;
	ed25519_public_key_t sender_address;
	ed25519_signature_t sender_sig;

	/*
	{
		"amount": "60000000000",
		"excess": "091f151170bfac881479bfb56c7012c52cd4ce4198ad661586374dd499925922fb",
		"recipient_address": "tgrin10qlk22rxjap2ny8qltc2tl996kenxr3hhwuu6hrzs6tdq08yaqgqq6t83r",
		"recipient_sig": "b9b1885a3f33297df32e1aa4db23220bd305da8ed92ff6873faf3ab2c116fea25e9d0e34bd4f567f022b88a37400821ffbcaec71c9a8c3a327c4626611886d0d",
		"sender_address": "tgrin1xtxavwfgs48ckf3gk8wwgcndmn0nt4tvkl8a7ltyejjcy2mc6nfs9gm2lp",
		"sender_sig": "611b92331e395c3d29871ac35b1fce78ec595e28ccbe8cc55452da40775e8e46d35a2e84eaffd986935da3275e34d46a8d777d02dabcf4339704c2a621da9700"
	}
	*/
    Json::Value ToJSON() const
    {
		Json::Value proof_json;
		proof_json["amount"] = amount;
		proof_json["excess"] = excess.Format();
		proof_json["recipient_address"] = Bech32Address::proof.GetReceiverAddress().Format();
		proof_json["recipient_sig"] = proof.GetReceiverSignature().value().Format();
		proof_json["sender_address"] = proof.GetSenderAddress().Format();
    }
};