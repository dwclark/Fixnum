#ifndef DECODE_HPP_b2c9755d6a5d964a2449a7f46768f74f7ccf28da
#define DECODE_HPP_b2c9755d6a5d964a2449a7f46768f74f7ccf28da

#include <string>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <string.h>
#include <iostream>

namespace decode {

    std::vector<uint8_t> convert_base(std::vector<uint8_t> working, const int source, const int target);
    std::vector<uint8_t> convert_base(const char* input, const int base, const int target);
    std::vector<uint8_t> convert_base(const std::string& input, const int base, const int target);

    template<typename R, typename A>
    R narrow_cast(const A& a) {
        R r = R(a);
        if(A(r) != a) {
            throw std::overflow_error("narrow_cast fail, information lost");
        }
        
        return r;
    }

    template<typename T>
    class ConvertBase {
    public:
        ConvertBase(const std::string& input, const int base, const int target) : sign('+'){
            if(input.length() == 0) {
                return;
            }
            
            char tmp_str[2] = { 0, 0 };
            
            int i = 0;
            if(input[i] == '+' || input[i] == '-') {
                sign = input[i];
                ++i;
            }
            
            for(; i < input.length(); ++i) {
                tmp_str[0] = input[i];
                working.push_back(narrow_cast<T>(strtol(tmp_str, nullptr, base)));
            }

            converted = convert_base(working, base, target);
        }

        ConvertBase(const char* input, const int base, const int target) : sign('+') {
            char tmp_str[2] = { 0, 0 };
            const int length = strlen(input);
            if(length == 0) {
                return;
            }

            int i = 0;
            if(input[i] == '+' || input[i] == '-') {
                sign = input[i];
                ++i;
            }
            
            for(; i < length; ++i) {
                tmp_str[0] = input[i];
                working.push_back(narrow_cast<T>(strtol(tmp_str, nullptr, base)));
            }

            converted = convert_base(working, base, target);
        }
        
        bool is_positive() const { return sign == '+'; }
        bool is_negative() const { return sign == '-'; }
        bool is_zero() const { return working.size() == 0; }

        void print_converted() const {
            for(auto p : converted) {
                std::cout << (int) p << " ";
            }

            std::cout << std::endl;
        }
        
        std::vector<T> converted;
        std::vector<T> working;
        char sign;
    };
    
    template<typename T>
    std::vector<T> to_division_vector(const char* input, const int base) {
        std::vector<T> working;
        char tmp_str[2] = { 0, 0 };
        const int length = strlen(input);
        for(int i = 0; i < length; ++i) {
            tmp_str[0] = input[i];
            working.push_back(narrow_cast<T>(strtol(tmp_str, nullptr, base)));
        }
        
        return working;
    }

    template<typename T>
    std::vector<T> to_division_vector(const std::string& input, const int base) {
        std::vector<T> working;
        char tmp_str[2] = { 0, 0 };
        for(int i = 0; i < input.length(); ++i) {
            tmp_str[0] = input[i];
            working.push_back(narrow_cast<T>(strtol(tmp_str, nullptr, base)));
        }
        
        return working;
    }
        
    constexpr char to_char(const uint8_t val) {
        switch(val) {
        case 0: return '0';
        case 1: return '1';
        case 2: return '2';
        case 3: return '3';
        case 4: return '4';
        case 5: return '5';
        case 6: return '6';
        case 7: return '7';
        case 8: return '8';
        case 9: return '9';
        case 10: return 'A';
        case 11: return 'B';
        case 12: return 'C';
        case 13: return 'D';
        case 14: return 'E';
        case 15: return 'F';
        default:
            throw std::out_of_range("invalid input for char digit");
        }
    }

    template<size_t N>
    std::string convert_pos_str(uint8_t* pos_hex, const int target) {
        std::string ret;
        
        int sindex;
        for(sindex = N - 1; sindex >= 0; --sindex) {
            if(pos_hex[sindex] != 0) {
                break;
            }
        }

        while(sindex >= 0) {
            for(int i = sindex; i > 0; --i) {
                const uint8_t new_val = pos_hex[i] / target;
                const uint8_t rem = pos_hex[i] % target;
                pos_hex[i] = new_val;
                pos_hex[i-1] = pos_hex[i-1] + (16 * rem);

                if(i == sindex && pos_hex[i] == 0) {
                    --sindex;
                }
            }
            
            const uint8_t new_val = pos_hex[0] / target;
            const uint8_t rem = pos_hex[0] % target;
            
            ret.append(1, to_char(rem));
            pos_hex[0] = new_val;

            if(0 == sindex && pos_hex[0] == 0) {
                --sindex;
            }
        }

        if(ret.empty()) {
            return std::string("0");
        }

        std::reverse(ret.begin(), ret.end());
        return ret;
    }

    constexpr uint8_t convert_char(const char c) {
        uint8_t val = 0;
        switch(c) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            return (c - 48);
        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
            return (c - 54);
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
            return (c - 86);
        default:
            throw std::out_of_range("invalid input for digit");
        }
    }
    
    constexpr void convert_to_digits(const char* input, uint8_t* data, const size_t slots) {
        for(size_t i = 0; i < slots; ++i) {
            data[i] = convert_char(input[i]);
        }
    }

    void sign_extend(uint8_t* data, const int size, const int bit);
    int first_set_bit(const uint8_t d);
}

#endif
