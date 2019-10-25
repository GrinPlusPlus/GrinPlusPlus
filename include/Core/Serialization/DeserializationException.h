#pragma once

#include <exception>
#include <string>

#define DESERIALIZATION_EXCEPTION(msg) DeserializationException(__FUNCTION__, msg)

class DeserializationException : public std::exception
{
  public:
    DeserializationException(const std::string &function, const std::string &message) : std::exception()
    {
        m_message = message;
        m_what = function + " - " + message;
    }

    DeserializationException() : std::exception()
    {
        m_message = "Attempted to read past end of ByteBuffer.";
    }

    virtual const char *what() const throw()
    {
        return m_what.c_str();
    }

    inline const std::string &GetMsg() const
    {
        return m_message;
    }

  private:
    std::string m_message;
    std::string m_what;
};