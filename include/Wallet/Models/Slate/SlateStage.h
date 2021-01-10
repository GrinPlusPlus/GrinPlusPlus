#pragma once

#include <string>

enum class ESlateStage
{
    NONE = -1,
    STANDARD_SENT,
    STANDARD_RECEIVED,
    STANDARD_FINALIZED,
    INVOICE_SENT,
    INVOICE_PAID,
    INVOICE_FINALIZED
};

struct SlateStage
{
    SlateStage(const ESlateStage& _value) : value(_value) { }
    ESlateStage value;

    bool operator==(const SlateStage& rhs) const noexcept { return value == rhs.value; }
    bool operator==(const ESlateStage& rhs) const noexcept { return value == rhs; }

    bool operator!=(const SlateStage& rhs) const noexcept { return value != rhs.value; }
    bool operator!=(const ESlateStage& rhs) const noexcept { return value != rhs; }

    bool operator<(const SlateStage& rhs) const noexcept { return ToByte() < rhs.ToByte(); }
    bool operator<(const ESlateStage& rhs) const noexcept { return ToByte() < SlateStage::ToByte(rhs); }

    bool operator>(const SlateStage& rhs) const noexcept { return ToByte() > rhs.ToByte(); }
    bool operator>(const ESlateStage& rhs) const noexcept { return ToByte() > SlateStage::ToByte(rhs); }

    bool operator>=(const SlateStage& rhs) const noexcept { return ToByte() >= rhs.ToByte(); }
    bool operator>=(const ESlateStage& rhs) const noexcept { return ToByte() >= SlateStage::ToByte(rhs); }

    std::string ToString() const noexcept { return ToString(value); }
    uint8_t ToByte() const noexcept { return ToByte(value); }

    static std::string ToString(const ESlateStage stage)
    {
        switch (stage)
        {
            case ESlateStage::STANDARD_SENT: return "S1";
            case ESlateStage::STANDARD_RECEIVED: return "S2";
            case ESlateStage::STANDARD_FINALIZED: return "S3";
            case ESlateStage::INVOICE_SENT: return "I1";
            case ESlateStage::INVOICE_PAID: return "I2";
            case ESlateStage::INVOICE_FINALIZED: return "I3";
        }

        throw std::exception();
    }

    static SlateStage FromString(const std::string& value)
    {
        if (value == "S1") return ESlateStage::STANDARD_SENT;
        else if (value == "S2") return ESlateStage::STANDARD_RECEIVED;
        else if (value == "S3") return ESlateStage::STANDARD_FINALIZED;
        else if (value == "I1") return ESlateStage::INVOICE_SENT;
        else if (value == "I2") return ESlateStage::INVOICE_PAID;
        else if (value == "I3") return ESlateStage::INVOICE_FINALIZED;
        
        throw std::exception();
    }

    static uint8_t ToByte(const ESlateStage stage)
    {
        switch (stage)
        {
            case ESlateStage::STANDARD_SENT: return 1;
            case ESlateStage::STANDARD_RECEIVED: return 2;
            case ESlateStage::STANDARD_FINALIZED: return 3;
            case ESlateStage::INVOICE_SENT: return 4;
            case ESlateStage::INVOICE_PAID: return 5;
            case ESlateStage::INVOICE_FINALIZED: return 6;
        }

        return 0;
    }

    static SlateStage FromByte(const uint8_t value)
    {
        switch (value)
        {
            case 1: return ESlateStage::STANDARD_SENT;
            case 2: return ESlateStage::STANDARD_RECEIVED;
            case 3: return ESlateStage::STANDARD_FINALIZED;
            case 4: return ESlateStage::INVOICE_SENT;
            case 5: return ESlateStage::INVOICE_PAID;
            case 6: return ESlateStage::INVOICE_FINALIZED;
        }

        throw std::exception();
    }
};