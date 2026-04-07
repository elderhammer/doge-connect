#include <array>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

#include "difficulty.h"

namespace
{
std::string compactToHexLE(const std::array<uint8_t, 4>& compactRep)
{
    std::ostringstream ss;
    ss << std::hex << std::setfill('0');
    for (uint8_t b : compactRep)
    {
        ss << std::setw(2) << static_cast<unsigned int>(b);
    }
    return ss.str();
}

std::string compactToHexBE(const std::array<uint8_t, 4>& compactRep)
{
    std::ostringstream ss;
    ss << std::hex << std::setfill('0');
    for (auto it = compactRep.rbegin(); it != compactRep.rend(); ++it)
    {
        ss << std::setw(2) << static_cast<unsigned int>(*it);
    }
    return ss.str();
}
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cout << "Usage: testdifficulty_from_decimal <difficulty_decimal>\n";
        std::cout << "Example: testdifficulty_from_decimal 65536\n";
        return 1;
    }

    try
    {
        const std::string input = argv[1];
        size_t parsedChars = 0;
        const unsigned long long parsed = std::stoull(input, &parsedChars, 10);
        if (parsedChars != input.size())
            throw std::invalid_argument("difficulty contains non-decimal characters");
        if (parsed == 0)
            throw std::invalid_argument("difficulty must be >= 1");
        if (parsed > std::numeric_limits<uint64_t>::max())
            throw std::out_of_range("difficulty is too large");

        const uint64_t difficulty = static_cast<uint64_t>(parsed);

        // Scrypt/Doge diff1 compact target: 0x1f00ffff (stored as LE bytes).
        const DifficultyTarget diff1Target(std::array<uint8_t, 4>{0xff, 0xff, 0x00, 0x1f});
        std::array<uint8_t, 32> target = diff1Target.getFullRep();
        target = divideTarget(target, difficulty);

        const DifficultyTarget dispatcherTarget(target);
        const std::array<uint8_t, 4> compactRep = dispatcherTarget.getCompactRep();

        std::cout << "Input difficulty (decimal): " << difficulty << "\n";
        std::cout << "Dispatcher difficulty compact (hex, LE): " << compactToHexLE(compactRep) << "\n";
        std::cout << "Dispatcher difficulty compact (hex, BE): " << compactToHexBE(compactRep) << "\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << "Invalid input: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
