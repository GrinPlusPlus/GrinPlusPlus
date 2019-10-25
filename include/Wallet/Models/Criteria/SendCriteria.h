#pragma once

#include <Core/Util/JsonUtil.h>
#include <Wallet/Models/DTOs/SelectionStrategyDTO.h>
#include <Wallet/SessionToken.h>

class SendCriteria
{
  public:
    static SendCriteria FromJSON(const Json::Value &json)
    {
        const SessionToken token = SessionToken::FromBase64(JsonUtil::GetRequiredString(json, "session_token"));
        const uint64_t amount = JsonUtil::GetRequiredUInt64(json, "amount");
        const uint64_t feeBase = JsonUtil::GetRequiredUInt64(json, "fee_base");
        const std::optional<std::string> messageOpt = JsonUtil::GetStringOpt(json, "message");
        const uint8_t numOutputs = (uint8_t)json.get("change_outputs", Json::Value(1)).asUInt();
        const Json::Value selectionStrategyJSON = JsonUtil::GetRequiredField(json, "selection_strategy");
        const SelectionStrategyDTO selectionStrategy = SelectionStrategyDTO::FromJSON(selectionStrategyJSON);
        const std::optional<std::string> addressOpt = JsonUtil::GetStringOpt(json, "address");

        return SendCriteria(token, amount, feeBase, messageOpt, numOutputs, selectionStrategy, addressOpt);
    }

    inline const SessionToken &GetToken() const
    {
        return m_token;
    }
    inline uint64_t GetAmount() const
    {
        return m_amount;
    }
    inline uint64_t GetFeeBase() const
    {
        return m_feeBase;
    }
    inline const std::optional<std::string> &GetMsg() const
    {
        return m_messageOpt;
    }
    inline uint8_t GetNumOutputs() const
    {
        return m_numOutputs;
    }
    inline const SelectionStrategyDTO &GetSelectionStrategy() const
    {
        return m_selectionStrategy;
    }
    inline const std::optional<std::string> &GetAddress() const
    {
        return m_addressOpt;
    }

  private:
    SendCriteria(const SessionToken &token, const uint64_t amount, const uint64_t feeBase,
                 const std::optional<std::string> &messageOpt, const uint8_t numOutputs,
                 const SelectionStrategyDTO &selectionStrategy, const std::optional<std::string> &addressOpt)
        : m_token(token), m_amount(amount), m_feeBase(feeBase), m_messageOpt(messageOpt), m_numOutputs(numOutputs),
          m_selectionStrategy(selectionStrategy), m_addressOpt(addressOpt)
    {
    }

    SessionToken m_token;
    uint64_t m_amount;
    uint64_t m_feeBase;
    std::optional<std::string> m_messageOpt;
    uint8_t m_numOutputs;
    SelectionStrategyDTO m_selectionStrategy;
    std::optional<std::string> m_addressOpt;
};