#ifndef THROABLES_H
#define THROABLES_H

#include <stdexcept>
#include <string>

class RuntimeError : public std::runtime_error {
public:
    explicit RuntimeError(const std::string &message)
        : std::runtime_error(message) {}
};

#endif //THROABLES_H