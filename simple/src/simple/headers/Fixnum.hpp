#ifndef FIXNUM_HPP_aaa1b32b84e336737f7e4626a43d20ee6cbee3e3
#define FIXNUM_HPP_aaa1b32b84e336737f7e4626a43d20ee6cbee3e3

#include "Decode.hpp"

#include <cstdint>
#include <iostream>
#include <cstddef>
#include <cassert>
#include <exception>
#include <string>
#include <algorithm>
#include <string.h>
#include <vector>
#include <limits>
#include <cstring>

template<size_t N> class Fixnum;

template<size_t T, size_t S>
Fixnum<T> fixnum_cast(const Fixnum<S>& source);

template<size_t N>
class Fixnum {
public:
    static constexpr int slots = (N / 8) + ((N % 8) > 0 ? 1 : 0);
    static constexpr int hex_slots = slots * 2;
    static constexpr int top_index = slots - 1;
    static constexpr int top_mask = 0xFF >> ((slots * 8) - N);
    static constexpr int sign_mask = 0x80 >> ((slots * 8) - N);
    static constexpr int unsigned_mask = 0xFF >> (1 + (slots * 8) - N);

    template<size_t T, size_t S>
    friend Fixnum<T> fixnum_cast(const Fixnum<S>& source);
    
    static constexpr Fixnum lowest() {
        Fixnum fn;
        fn._data[top_index] = sign_mask;
        return fn;
    }

    static constexpr Fixnum max() {
        Fixnum fn;
        fn._data[top_index] = unsigned_mask;
        for(int i = 0; i < top_index; ++i) {
            fn._data[i] = 0xFF;
        }

        return fn;
    }
    
    static constexpr uint16_t one = 1;
    
    Fixnum() : _data { 0 } {}

    Fixnum(const Fixnum& f) {
        std::memcpy(_data, f._data, sizeof(_data));
    }

    Fixnum(Fixnum&& f) {
        std::memcpy(_data, f._data, sizeof(_data));
    }

    Fixnum(const int16_t val) {
        _data[0] = val & 0xFF;
        if(slots >= 2) _data[1] = (val >> 8) & 0xFF;
        
        //sign extend if needed
        if(slots > 2) {
            decode::sign_extend(_data, slots, 16);
        }

        if(slots <= 2) {
            _truncate();
        }
    }
    
    Fixnum(const int32_t val) {
        _data[0] = val & 0xFF;
        if(slots >= 2) _data[1] = (val >> 8) & 0xFF;
        if(slots >= 3) _data[2] = (val >> 16) & 0xFF;
        if(slots >= 4) _data[3] = (val >> 24) & 0xFF;

        //sign extend if needed
        if(slots > 4) {
            decode::sign_extend(_data, slots, 32);
        }

        if(slots <= 4) {
            _truncate();
        }
    }

    ~Fixnum() {}

    Fixnum& operator=(const Fixnum& f) {
        std::memcpy(_data, f._data, sizeof(_data));
        return *this;
    }

    Fixnum& operator=(Fixnum&& f) {
        std::memcpy(_data, f._data, sizeof(_data));
        return *this;
    }

    bool operator==(const Fixnum& rhs) {
        return memcmp(_data, rhs._data, slots) == 0;
    }

    bool operator!=(const Fixnum& rhs) {
        return !(*this == rhs);
    }

    Fixnum& operator+=(const Fixnum& rhs) {
        _add_to(_data, rhs._data);
        return *this;
    }

    Fixnum operator+(const Fixnum& n) const {
        Fixnum ret { *this };
        ret += n;
        return ret;
    }

    Fixnum& operator-=(const Fixnum& rhs) {
        _add_to(_data, rhs.complement()._data);
        return *this;
    }

    Fixnum operator-(const Fixnum& n) {
        Fixnum ret { *this };
        ret -= n;
        return ret;
    }

    Fixnum operator-() const {
        return complement();
    }
    
    bool is_negative() const {
        return (_data[top_index] & sign_mask) != 0;
    }

    bool is_lowest() const {
        //sign bit must be set
        if((_data[top_index] & sign_mask) == 0) {
            return false;
        }
        
        //top mask must return 0
        if((_data[top_index] & unsigned_mask) != 0) {
            return false;
        }

        //everything else must be zero
        for(int i = 0; i < top_index; ++i) {
            if(_data[i] != 0) {
                return false;
            }
        }

        return true;
    }

    bool is_max() const {
        //sign bit must be unset
        if((_data[top_index] & sign_mask) != 0) {
            return false;
        }
        
        //top mask must return unsigned mask
        if((_data[top_index] & unsigned_mask) != unsigned_mask) {
            return false;
        }

        //everything else must be 0xFF
        for(int i = 0; i < top_index; ++i) {
            if(_data[i] != 0xFF) {
                return false;
            }
        }
    }
    
    std::string str() const {
        return str(10);
    }

    std::string str(const int base) const {
        if(is_negative() && !is_lowest()) {
            return std::string("-") + complement().str(base);
        }
        else {
            uint8_t pos_hex[hex_slots] = { 0 };
            _single_hex_values(pos_hex);
            std::string ret = decode::convert_pos_str<hex_slots>(pos_hex, base);
            return !is_lowest() ? ret : std::string("-") + ret;
        }
    }

    Fixnum complement() const {
        Fixnum ret { *this };
        ret._complement();
        return ret;
    }

    const uint8_t* data() const {
        return _data;
    }
    
private:
    uint8_t _data[slots];

    void _add_to(uint8_t* target, const uint8_t* n) {
        uint16_t rem = 0;
        for(int i = 0; i < slots; ++i) {
            const uint16_t sum = (uint16_t) target[i] + (uint16_t) n[i] + rem;
            target[i] = sum & 0xFF;
            rem = (sum & 0xF00) >> 8;
        }
        
        target[top_index] = target[top_index] & top_mask;
    }

    void _add_one(uint8_t* target) {
        uint16_t sum = (uint16_t) target[0] + one;
        target[0] = sum & 0xFF;
        uint8_t rem = (sum & 0xF00) >> 8;
        int i = 1;
        
        while(rem > 0 && i < slots) {
            sum = (uint16_t) target[i] + rem;
            target[i] = sum & 0xFF;
            rem = (sum & 0xF00) >> 8;
            ++i;
        }

        target[top_index] = target[top_index] & top_mask;
    }

    void _complement(uint8_t* d) {
        for(int i = 0; i < slots; ++i) {
            d[i] = ~d[i];
        }

        _add_one(d);
    }

    void _complement() {
        _complement(_data);
    }

    void _single_hex_values(uint8_t* d) const {
        for(int i = 0; i < slots; ++i) {
            d[2 * i] = _data[i] & 0xF;
            d[(2 * i) + 1] = (_data[i] >> 4) & 0xF;
        }
    }

    void _truncate() {
        _data[top_index] = _data[top_index] & top_mask;
    }
};

template<size_t T, size_t S>
Fixnum<T> fixnum_cast(const Fixnum<S>& source) {
    Fixnum<T> ret { 0 };
    
    for(int i = 0; i < Fixnum<T>::slots && i < Fixnum<S>::slots; ++i) {
        ret._data[i] = source._data[i];
    }
    
    if(T > S) {
        decode::sign_extend(ret._data, Fixnum<T>::slots, S);
    }
    
    return ret;
}


template<size_t N>
Fixnum<N> operator+(const Fixnum<N>& f, const Fixnum<N>& s) {
    Fixnum<N> ret { f };
    ret += s;
    return ret;
}

#endif
