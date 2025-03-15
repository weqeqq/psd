
#pragma once

#include <stdexcept>

namespace PSD {

struct Error : std::runtime_error {
public:
  Error(const std::string &msg) : std::runtime_error(msg) {}

}; // Error 
}; // PSD
