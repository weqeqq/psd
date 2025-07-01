
#include <psd/structure/main_info/extra_info.h>

namespace PSD {
ExtraInfo::Iterator begin(ExtraInfo &input) {
  #ifdef PSD_DEBUG
  return ExtraInfo::Iterator(input, input.InsertionOrder().begin());
  #else
  return ExtraInfo::Iterator(input.data_.begin());
  #endif
}
ExtraInfo::ConstIterator begin(const ExtraInfo &input) {
  #ifdef PSD_DEBUG
  return ExtraInfo::ConstIterator(input, input.InsertionOrder().begin());
  #else
  return ExtraInfo::ConstIterator(input.data_.begin());
  #endif
}
ExtraInfo::Iterator end(ExtraInfo &input) {
  #ifdef PSD_DEBUG
  return ExtraInfo::Iterator(input, input.InsertionOrder().end());
  #else
  return ExtraInfo::Iterator(input.data_.end());
  #endif
}
ExtraInfo::ConstIterator end(const ExtraInfo &input) {
  #ifdef PSD_DEBUG
  return ExtraInfo::ConstIterator(input, input.InsertionOrder().end());
  #else
  return ExtraInfo::ConstIterator(input.data_.end());
  #endif
}
}
