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

    //TODO: Fix and test this, not working
    Fixnum(const std::initializer_list<uint8_t> init) {
        const uint8_t* ptr = init.begin();
        for(int i = 0; i < init.size() && i < slots; ++i) {
            _data[i] = ptr[i];
        }

        if(slots <= init.size()) {
            _truncate();
        }
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

    Fixnum(const int64_t val) {
        _data[0] = val & 0xFF;
        if(slots >= 2) _data[1] = (val >> 8) & 0xFF;
        if(slots >= 3) _data[2] = (val >> 16) & 0xFF;
        if(slots >= 4) _data[3] = (val >> 24) & 0xFF;
        if(slots >= 5) _data[4] = (val >> 32) & 0xFF;
        if(slots >= 6) _data[5] = (val >> 40) & 0xFF;
        if(slots >= 7) _data[6] = (val >> 48) & 0xFF;
        if(slots >= 8) _data[7] = (val >> 56) & 0xFF;

        //sign extend if needed
        if(slots > 8) {
            decode::sign_extend(_data, slots, 64);
        }

        if(slots <= 8) {
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

    Fixnum& operator++() {
        _add_one(_data);
        return *this;
    }

    Fixnum operator++(int) {
        Fixnum ret { *this };
        _add_one(_data);
        return ret;
    }

    Fixnum& operator-=(const Fixnum& rhs) {
        _add_to(_data, rhs.complement()._data);
        return *this;
    }

    Fixnum operator-(const Fixnum& n) const {
        Fixnum ret { *this };
        ret -= n;
        return ret;
    }

    Fixnum& operator--() {
        _subtract_one(_data);
        return *this;
    }

    Fixnum operator--(int) {
        Fixnum ret { *this };
        _subtract_one(_data);
        return ret;
    }

    Fixnum operator-() const {
        return complement();
    }

    Fixnum& operator*=(const Fixnum& n) {
        const bool end_with_complement = is_negative() ^ n.is_negative();
        Fixnum multiplier { n };
        Fixnum multiplicand(0);
        
        if(is_negative()) {
            _complement();
        }

        if(multiplier.is_negative()) {
            multiplier._complement();
        }

        for(int i = 0; i < N; ++i) {
            if(_is_bit_set_zero_index(_data, i)) {
                _add_to(multiplicand._data, multiplier._data);
            }

            multiplier <<= 1;
        }

        std::memcpy(_data, multiplicand._data, sizeof(_data));
        if(end_with_complement) {
            _complement();
        }

        return *this;
    }

    Fixnum operator*(const Fixnum& n) const {
        Fixnum ret { *this };
        ret *= n;
        return ret;
    }

    Fixnum& operator<<=(const int by) {
        switch(by) {
        case 0: break;
        case 1: _left_shift_1(_data); break;
        default: _left_shift(_data, by);
        }

        return *this;
    }

    Fixnum operator<<(const int by) const {
        Fixnum ret { *this };
        ret <<= by;
        return ret;
    }

    Fixnum& operator>>=(const int by) {
        switch(by) {
        case 0: break;
        case 1: _right_shift_1(_data); break;
        default: _right_shift(_data, by);
        }

        return *this;
    }

    Fixnum operator>>(const int by) const {
        Fixnum ret { *this };
        ret >>= by;
        return ret;
    }

    int operator[](const int index) const {
        if(index >= N) {
            throw std::overflow_error("can't access bits beyond size of fixnum");
        }
        
        return _is_bit_set_zero_index(_data, index - 1) ? 1 : 0;
    }
    
    bool is_negative() const {
        return (_data[top_index] & sign_mask) != 0;
    }

    bool is_positive() const {
        return (_data[top_index] & sign_mask) == 0;
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

    bool _is_bit_set_zero_index(const uint8_t* d, const int bit) const {
        const int index = bit / 8;
        const int bit_pos = bit % 8;
        const int mask = 1 << bit_pos;
        return (d[index] & mask) > 0;
    }
    
    void _add_to(uint8_t* target, const uint8_t* n) {
        uint16_t rem = 0;
        for(int i = 0; i < slots; ++i) {
            const uint16_t sum = (uint16_t) target[i] + (uint16_t) n[i] + rem;
            target[i] = sum & 0xFF;
            rem = (sum & 0xF00) >> 8;
        }

        _truncate(target);
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

        _truncate(target);
    }

    void _subtract_one(uint8_t* target) {
        int nonzero_slot;
        for(nonzero_slot = 0; nonzero_slot < slots; ++nonzero_slot) {
            if(target[nonzero_slot] != 0) {
                break;
            }
        }

        if(nonzero_slot == slots) {
            _fill_1(target);
        }
        else {
            target[nonzero_slot] -= 1;
            
            for(int index = nonzero_slot - 1; index >= 0; --index) {
                target[index] = 0xFF;
            }

            _truncate(target);
        }
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

    void _truncate(uint8_t* d) {
        d[top_index] = d[top_index] & top_mask;
    }

    void _truncate() {
        _data[top_index] = _data[top_index] & top_mask;
    }

    void _zero(uint8_t* d) {
        std::memset(d, 0, slots);
    }

    void _fill_1(uint8_t* d) {
        std::memset(d, 0xFF, slots);
        _truncate(d);
    }

    bool _is_zero(const uint8_t* d) {
        for(int i = 0; i < slots; ++i) {
            if(d[i] != 0) {
                return false;
            }
        }

        return true;
    }

    void _left_shift(uint8_t* d, const int by) {
        //TODO: Replace this with something more clever at some point
        for(int i = 0; i < by; ++i) {
            _left_shift_1(d);
        }
    }
    
    void _left_shift_1(uint8_t* d) {
        for(int i = top_index; i > 0; --i) {
            const int bottom = ((d[i-1] & 0x80) > 0) ? 1 : 0;
            d[i] = (d[i] << 1) | bottom;
        }

        d[0] <<= 1;
        _truncate();
    }

    void _right_shift(uint8_t* d, const int by) {
        //TODO: Replace this with something more clever at some point
        for(int i = 0; i < by; ++i) {
            _right_shift_1(d);
        }
    }
    
    void _right_shift_1(uint8_t* d) {
        for(int i = 0; i < top_index; ++i) {
            const int top = ((d[i+1] & 0x1) > 0) ? 0x80 : 0;
            d[i] = (d[i] >> 1) | top;
        }

        d[top_index] >>= 1;
        _truncate();
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

#endif
