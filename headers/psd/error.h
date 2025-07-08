
#pragma once

#include <stdexcept>

namespace PSD {

class Error : public std::runtime_error {
public:
  Error(const std::string &message) : std::runtime_error(message) {}

}; // class Error
}; // namespace PSD
