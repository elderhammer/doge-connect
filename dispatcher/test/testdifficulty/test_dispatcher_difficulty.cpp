#include <array>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include "hash_util/difficulty.h"

namespace
{
std::string normalizeHex(std::string hex)
{
    std::string out;
    out.reserve(hex.size());

    for (char ch : hex)
    {
        if (std::isspace(static_cast<unsigned char>(ch)))
            continue;
        out.push_back(ch);
    }

    if (out.rfind("0x", 0) == 0 || out.rfind("0X", 0) == 0)
        out = out.substr(2);

    if (out.size() != 8)
        throw std::invalid_argument("dispatcher difficulty must be exactly 4 bytes (8 hex chars)");

    return out;
}

std::array<uint8_t, 4> parseCompactDifficulty(const std::string& hexInput)
{
    const std::string hex = normalizeHex(hexInput);
    std::array<uint8_t, 4> out{};

    for (size_t i = 0; i < out.size(); ++i)
    {
        const std::string byteHex = hex.substr(i * 2, 2);
        out[i] = static_cast<uint8_t>(std::stoul(byteHex, nullptr, 16));
    }

    return out;
}

std::string littleEndianUint256ToDecimalString(const std::array<uint8_t, 32>& valueLe)
{
    std::vector<uint8_t> valueBe(valueLe.rbegin(), valueLe.rend());
    while (!valueBe.empty() && valueBe.front() == 0)
    {
        valueBe.erase(valueBe.begin());
    }
    if (valueBe.empty())
        return "0";

    std::string digits;
    while (!valueBe.empty())
    {
        std::vector<uint8_t> quotient;
        quotient.reserve(valueBe.size());

        uint32_t remainder = 0;
        for (uint8_t b : valueBe)
        {
            const uint32_t current = (remainder << 8) | b;
            const uint8_t q = static_cast<uint8_t>(current / 10);
            remainder = current % 10;

            if (!quotient.empty() || q != 0)
                quotient.push_back(q);
        }

        digits.push_back(static_cast<char>('0' + remainder));
        valueBe = std::move(quotient);
    }

    std::reverse(digits.begin(), digits.end());
    return digits;
}

long double compactToDifficulty(const std::array<uint8_t, 4>& compactRep)
{
    const uint32_t mantissa = static_cast<uint32_t>(compactRep[0])
        | (static_cast<uint32_t>(compactRep[1]) << 8)
        | (static_cast<uint32_t>(compactRep[2]) << 16);
    const uint8_t exponent = compactRep[3];

    if (mantissa == 0)
        return std::numeric_limits<long double>::infinity();

    // Scrypt/Doge diff1 compact is 0x1f00ffff.
    constexpr uint32_t diff1Mantissa = 0x00ffff;
    constexpr int diff1Exponent = 0x1f;

    const int exponentDelta = diff1Exponent - static_cast<int>(exponent);
    const long double scale = std::pow(256.0L, static_cast<long double>(exponentDelta));

    return (static_cast<long double>(diff1Mantissa) / static_cast<long double>(mantissa)) * scale;
}
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cout << "Usage: testdifficulty <dispatcherDifficultyHex>\n";
        std::cout << "Example: testdifficulty ffff001f\n";
        std::cout << "Note: input is compact 4-byte hex in little-endian byte order.\n";
        return 1;
    }

    try
    {
        const std::array<uint8_t, 4> compactRep = parseCompactDifficulty(argv[1]);
        const std::array<uint8_t, 32> fullTarget = calculateFullRepFromCompactRep(compactRep);
        const long double difficulty = compactToDifficulty(compactRep);

        std::cout << "Input compact (hex, LE): " << argv[1] << "\n";
        std::cout << "Target (decimal): " << littleEndianUint256ToDecimalString(fullTarget) << "\n";
        std::cout << std::fixed << std::setprecision(8);
        std::cout << "Difficulty (decimal): " << difficulty << "\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << "Invalid input: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
