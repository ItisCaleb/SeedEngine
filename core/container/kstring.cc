#include "kstring.h"

namespace Seed {
KString::KString(const std::string &str) {
    this->data = (char *)malloc(str.size());
    this->_size = str.size();
    memcpy((void *)this->data, str.data(), this->_size);
}

KString::~KString() {}

size_t KString::utf8_size() { return 0; }

KString KString::clone() {
    KString s;
    s.data = (char *)malloc(this->_size);
    s._size = this->_size;
    return s;
}

}  // namespace Seed