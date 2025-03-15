#pragma once

#include <psd/core/stream.h>
#include <psd/core/type/error.h>

namespace PSD {

class VersionIOError : Error {
public:

  struct UndefinedVersion;

protected:
  VersionIOError(const std::string &msg) : Error(msg) {}

}; // VersionIOError

struct VersionIOError::UndefinedVersion : VersionIOError {
  UndefinedVersion() : VersionIOError("UndefinedVersion") {}

}; // VersionIOError::UndefinedVersion

class Version {
public:

  class Reader;
  class Writer;

  enum Tp : std::uint16_t {
    PSD = 1,
    PSB = 2,
  };

  using UnderlyingTp = std::underlying_type_t<Tp>;

  template <Tp Ve>
  static constexpr UnderlyingTp UnderlyingValue = static_cast<UnderlyingTp>(Ve);

}; // Version

class Version::Reader {
public:

  Reader(Stream &stream) : stream_(stream) {}

  Version::Tp Read() {
    switch (stream_.Read<UnderlyingTp>()) {
      case UnderlyingValue<Version::PSD> : return Version::PSD;
      case UnderlyingValue<Version::PSB> : return Version::PSB;
      default: throw VersionIOError::UndefinedVersion();
    }
  }

private:
  Stream &stream_;

}; // Version::Reader

template <>
class Reader<Version::Tp> : public Version::Reader {
public:
  Reader(Stream &stream) : Version::Reader(stream) {}

}; // Reader<Version::Tp>

class Version::Writer {
public:

  Writer(Stream &stream, Version::Tp version) 
    : stream_  (stream)
    , version_ (version) {}

  void Write() {
    stream_.Write(static_cast<UnderlyingTp>(version_));
  }

private:
  Stream     &stream_;
  Version::Tp version_;

}; // Version::Writer

template <>
class Writer<Version::Tp> : public Version::Writer {
public:
  Writer(Stream &stream, Version::Tp version) : Version::Writer(stream, version) {}

}; // Writer<Version::Tp>
}; // PSD
