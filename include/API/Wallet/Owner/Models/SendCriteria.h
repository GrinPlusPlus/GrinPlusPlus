#pragma once

#include <Core/Util/JsonUtil.h>
#include <Wallet/SessionToken.h>
#include <Wallet/Models/DTOs/PostMethodDTO.h>
#include <Wallet/Models/DTOs/SelectionStrategyDTO.h>

class SendCriteria
{
public:
	static SendCriteria FromJSON(const Json::Value& json)
	{
		const SessionToken token = SessionToken::FromBase64(JsonUtil::GetRequiredString(json, "session_token"));
		const uint64_t amount = JsonUtil::GetRequiredUInt64(json, "amount");
		const uint64_t feeBase = JsonUtil::GetRequiredUInt64(json, "fee_base");
		const std::optional<std::string> messageOpt = JsonUtil::GetStringOpt(json, "message");
		const uint8_t numOutputs = (uint8_t)json.get("change_outputs", Json::Value(1)).asUInt();
		const Json::Value selectionStrategyJSON = JsonUtil::GetRequiredField(json, "selection_strategy");
		const SelectionStrategyDTO selectionStrategy = SelectionStrategyDTO::FromJSON(selectionStrategyJSON);

		const std::optional<std::string> addressOpt = JsonUtil::GetStringOpt(json, "address");
		const std::optional<std::string> filePathOpt = JsonUtil::GetStringOpt(json, "file");

		std::optional<PostMethodDTO> postMethodOpt = std::nullopt;
		std::optional<Json::Value> postJSON = JsonUtil::GetOptionalField(json, "post_tx");
		if (postJSON.has_value())
		{
			postMethodOpt = std::make_optional<PostMethodDTO>(PostMethodDTO::FromJSON(*postJSON));
		}

		return SendCriteria(
			token,
			amount,
			feeBase,
			messageOpt,
			numOutputs,
			selectionStrategy,
			addressOpt,
			filePathOpt,
			postMethodOpt
		);
	}

	//
	// Getters
	//
	const SessionToken& GetToken() const { return m_token; }
	uint64_t GetAmount() const { return m_amount; }
	uint64_t GetFeeBase() const { return m_feeBase; }
	const std::optional<std::string>& GetMsg() const { return m_messageOpt; }
	uint8_t GetNumOutputs() const { return m_numOutputs; }
	const SelectionStrategyDTO& GetSelectionStrategy() const { return m_selectionStrategy; }
	const std::optional<std::string>& GetAddress() const { return m_addressOpt; }
	const std::optional<std::string>& GetFile() const { return m_filePathOpt; }
	const std::optional<PostMethodDTO>& GetPostMethod() const { return m_postMethodOpt; }
	uint16_t GetSlateVersion() const { return m_slateVersion; }

	//
	// Setters
	//
	void SetSlateVersion(const uint16_t slateVersion) { m_slateVersion = slateVersion; }

private:
	SendCriteria(
		const SessionToken& token,
		const uint64_t amount,
		const uint64_t feeBase,
		const std::optional<std::string>& messageOpt,
		const uint8_t numOutputs,
		const SelectionStrategyDTO& selectionStrategy,
		const std::optional<std::string>& addressOpt,
		const std::optional<std::string>& filePathOpt,
		const std::optional<PostMethodDTO>& postMethodOpt
	)
		: m_token(token),
		m_amount(amount),
		m_feeBase(feeBase),
		m_messageOpt(messageOpt),
		m_numOutputs(numOutputs),
		m_selectionStrategy(selectionStrategy),
		m_addressOpt(addressOpt),
		m_filePathOpt(filePathOpt),
		m_postMethodOpt(postMethodOpt),
		m_slateVersion(2)
	{

	}

	SessionToken m_token;
	uint64_t m_amount;
	uint64_t m_feeBase;
	std::optional<std::string> m_messageOpt;
	uint8_t m_numOutputs;
	SelectionStrategyDTO m_selectionStrategy;
	std::optional<std::string> m_addressOpt;
	std::optional<std::string> m_filePathOpt;
	std::optional<PostMethodDTO> m_postMethodOpt;
	uint16_t m_slateVersion;
};