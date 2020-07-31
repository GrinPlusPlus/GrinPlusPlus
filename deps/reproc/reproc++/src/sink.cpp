#include <reproc++/sink.hpp>

#include <ostream>

namespace reproc {
namespace sink {

string::string(std::string &out, std::string &err) noexcept
    : out_(out), err_(err)
{}

bool string::operator()(reproc_stream reproc_stream, const uint8_t *buffer, unsigned int size)
{
  switch (reproc_stream) {
    case reproc_stream::out:
      out_.append(reinterpret_cast<const char *>(buffer), size);
      break;
    case reproc_stream::err:
      err_.append(reinterpret_cast<const char *>(buffer), size);
      break;
    case reproc_stream::in:
      break;
  }

  return true;
}

ostream::ostream(std::ostream &out, std::ostream &err) noexcept
    : out_(out), err_(err)
{}

bool ostream::operator()(reproc_stream reproc_stream,
                         const uint8_t *buffer,
                         unsigned int size)
{
  switch (reproc_stream) {
    case reproc_stream::out:
      out_.write(reinterpret_cast<const char *>(buffer), size);
      break;
    case reproc_stream::err:
      err_.write(reinterpret_cast<const char *>(buffer), size);
      break;
    case reproc_stream::in:
      break;
  }

  return true;
}

bool discard::operator()(reproc_stream reproc_stream,
                         const uint8_t *buffer,
                         unsigned int size) noexcept
{
  (void) reproc_stream;
  (void) buffer;
  (void) size;

  return true;
}

namespace thread_safe {

string::string(std::string &out, std::string &err, std::mutex &mutex) noexcept
    : sink_(out, err), mutex_(mutex)
{}

bool string::operator()(reproc_stream reproc_stream, const uint8_t *buffer, unsigned int size)
{
  std::lock_guard<std::mutex> lock(mutex_);
  return sink_(reproc_stream, buffer, size);
}

} // namespace thread_safe

} // namespace sink
} // namespace reproc
