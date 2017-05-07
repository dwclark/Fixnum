#include "Decode.hpp"
#include <iostream>

namespace decode {

    void print_vector(const std::vector<uint8_t>& vec, const char* prefix) {
        std::cout << prefix;
        std::for_each(vec.begin(), vec.end(), [](auto n) { std::cout << (int) n << " "; });
        std::cout << std::endl;
    }

    std::vector<uint8_t> convert_base(std::vector<uint8_t> working, const int source, const int target) {
        std::vector<uint8_t> remainder;
        int sindex = 0;
        const int looping = working.size() - 1;

        while(sindex < working.size()) {
            for(int i = sindex; i < looping; ++i) {
                const uint8_t new_val = working[i] / target;
                const uint8_t rem = working[i] % target;
                working[i] = new_val;
                working[i+1] = working[i+1] + (source * rem);

                if(i == sindex && working[i] == 0) {
                    ++sindex;
                }
            }

            const uint8_t new_val = working[looping] / target;
            const uint8_t rem = working[looping] % target;
            remainder.push_back(rem);
            working[looping] = new_val;

            if(looping == sindex && working[looping] == 0) {
                ++sindex;
            }
        }
        
        std::reverse(remainder.begin(), remainder.end());
        return remainder;
    }

    std::vector<uint8_t> convert_base(const std::string& input, const int source, const int target) {
        return convert_base(to_division_vector<uint8_t>(input, source), source, target);
    }
    
    std::vector<uint8_t> convert_base(const char* input, const int source, const int target) {
        return convert_base(to_division_vector<uint8_t>(input, source), source, target);
    }

    void sign_extend(uint8_t* data, const int size, const int bit) {
        const int bit_pos = bit - 1;
        const int slot = bit_pos / 8;
        const int pos = (bit_pos % 8);

        if(slot >= size) {
            throw std::overflow_error("computed slot > size");
        }

        const int extension = (data[slot] & (1 << pos)) > 0 ? 1 : 0;
        const int beyond_current = extension ? 0xFF : 0;

        if(extension) {
            data[slot] |= (0xFF << (pos+1));
        }
        else {
            data[slot] &= (0xFF >> (8-(pos+1)));
        }
        
        for(int i = slot + 1; i < size; ++i) {
            data[i] = beyond_current;
        }
    }

    int first_set_bit(const uint8_t d) {
        if((0x80 & d) != 0) {
            return 7;
        }
        else if((0x40 & d) != 0) {
            return 6;
        }
        else if((0x20 & d) != 0) {
            return 5;
        }
        else if((0x10 & d) != 0) {
            return 4;
        }
        else if((0x8 & d) != 0) {
            return 3;
        }
        else if((0x4 & d) != 0) {
            return 2;
        }
        else if((0x2 & d) != 0) {
            return 1;
        }
        else if((0x1 & d) != 0) {
            return 0;
        }
        else {
            return -1;
        }
    }

}
