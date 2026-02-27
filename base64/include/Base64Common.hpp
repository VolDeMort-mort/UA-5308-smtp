#pragma once
#include <array>
#include <unordered_map>
#include <string>

namespace Base64
{
	inline constexpr char alphabet[64] =
    {
    'A','B','C','D','E','F','G','H',
    'I','J','K','L','M','N','O','P',
    'Q','R','S','T','U','V','W','X',
    'Y','Z','a','b','c','d','e','f',
    'g','h','i','j','k','l','m','n',
    'o','p','q','r','s','t','u','v',
    'w','x','y','z','0','1','2','3',
    '4','5','6','7','8','9','+','/'
    };

    inline const std::array<int, 256>& get_decode_table()
    {
        static const std::array<int, 256> table = []
            {
                std::array<int, 256> t{};
                t.fill(-1);

                for (int i = 0; i < 64; ++i)
                {
                    t[static_cast<unsigned char>(alphabet[i])] = i;
                }

                t[static_cast<unsigned char>('=')] = -2;
                return t;
            }();

        return table;
    }
}

