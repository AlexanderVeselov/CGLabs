#ifndef UTILS_HPP
#define UTILS_HPP

#include <stdexcept>
#include <sstream>

#define THROW_RUNTIME(message)                          \
    throw std::runtime_error(((std::ostringstream&) (std::ostringstream("") << message)).str().c_str());

#endif // UTILS_HPP
