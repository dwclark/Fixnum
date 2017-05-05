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
#include <array>

template<size_t N> class Fixnum;

template<size_t T, size_t S>
Fixnum<T> fixnum_cast(const Fixnum<S>& source);

template<size_t N>
class Fixnum {
public:
    static constexpr size_t bits = N;
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
    
    Fixnum() : _data { 0 } {}

    Fixnum(const Fixnum& f) {
        std::memcpy(_data, f._data, sizeof(_data));
    }

    Fixnum(Fixnum&& f) {
        std::memcpy(_data, f._data, sizeof(_data));
    }

    Fixnum(const std::initializer_list<uint8_t> init) : _data { 0 } {
        const uint8_t* ptr = init.begin();
        for(int i = 0; i < init.size() && i < slots; ++i) {
            _data[i] = ptr[i];
        }

        if(slots <= init.size()) {
            _truncate();
        }
    }

    Fixnum(const char* input, const int base) : Fixnum(decode::ConvertBase<uint8_t>(input, base, 16)) { }

    Fixnum(const std::string& input, const int base) : Fixnum(decode::ConvertBase<uint8_t>(input, base, 16)) { }
    
    Fixnum(const decode::ConvertBase<uint8_t>& cb) : Fixnum() {
        if(cb.is_zero()) {
            return;
        }

        const bool overflows = (slots * 2) < cb.converted.size();
        
        int source_index = cb.converted.size() - 1;
        for(int i = 0; i < slots; ++i) {
            if(source_index < 0) break;
            _data[i] = cb.converted[source_index--];
             if(source_index < 0) break;
            _data[i] |= (cb.converted[source_index--] << 4);
         }

        if(!overflows && !is_negative() && cb.is_negative()) {
            _complement();
        }
    }
    
    Fixnum(const int16_t val) : Fixnum() {
        _data[0] = val & 0xFF;
        if(slots >= 2) _data[1] = (val >> 8) & 0xFF;
        
        //sign extend if needed
        if(slots > 2) {
            sign_extend(15);
        }
        else {
            _truncate();
        }
    }
    
    Fixnum(const int32_t val) : Fixnum() {
        _data[0] = val & 0xFF;
        if(slots >= 2) _data[1] = (val >> 8) & 0xFF;
        if(slots >= 3) _data[2] = (val >> 16) & 0xFF;
        if(slots >= 4) _data[3] = (val >> 24) & 0xFF;

        //sign extend if needed
        if(slots > 4) {
            sign_extend(31);
        }
        else {
            _truncate();
        }
    }

    Fixnum(const int64_t val) : Fixnum(0) {
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
            sign_extend(63);
        }
        else {
            _truncate();
        }
    }

    Fixnum& operator=(const Fixnum& f) {
        std::memcpy(_data, f._data, sizeof(_data));
        return *this;
    }

    Fixnum& operator=(Fixnum&& f) {
        std::memcpy(_data, f._data, sizeof(_data));
        return *this;
    }

    bool operator==(const Fixnum& rhs) const {
        return memcmp(_data, rhs._data, slots) == 0;
    }

    bool operator!=(const Fixnum& rhs) const {
        return !(*this == rhs);
    }

    bool operator<(const Fixnum& rhs) const {
        return _cmp(_data, rhs._data) == -1;
    }

    bool operator<=(const Fixnum& rhs) const {
        return _cmp(_data, rhs._data) < 1;
    }

    bool operator>(const Fixnum& rhs) const {
        return _cmp(_data, rhs._data) == 1;
    }

    bool operator>=(const Fixnum& rhs) const {
        return _cmp(_data, rhs._data) > -1;
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
            if(_is_bit_set(_data, i)) {
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

    Fixnum& operator/=(const Fixnum& n) {
        _check_divide_by_zero(n);
        Fixnum dividend { *this };
        Fixnum divisor { n };
        _div_and_mod(*this, dividend, divisor);
        return *this;
    }

    Fixnum operator/(const Fixnum& n) {
        Fixnum ret { *this };
        ret /= n;
        return ret;
    }

    Fixnum& operator%=(const Fixnum& n) {
        _check_divide_by_zero(n);
        Fixnum dividend { *this };
        Fixnum divisor { n };
        _div_and_mod(*this, dividend, divisor);
        std::memcpy(_data, dividend._data, slots);
        return *this;
    }

    Fixnum operator%(const Fixnum& n) {
        Fixnum ret = { *this };
        ret %= n;
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

    Fixnum& operator&=(const Fixnum& n) {
        for(int i = 0; i < slots; ++i) {
            _data[i] &= n._data[i];
        }

        _truncate(_data);
        return *this;
    }

    Fixnum operator&(const Fixnum& n) const {
        Fixnum ret { *this };
        ret &= n;
        return ret;
    }

    Fixnum& operator|=(const Fixnum& n) {
        for(int i = 0; i < slots; ++i) {
            _data[i] |= n._data[i];
        }

        _truncate(_data);
        return *this;
    }

    Fixnum operator|(const Fixnum& n) const {
        Fixnum ret { *this };
        ret |= n;
        return ret;
    }

    Fixnum& operator^=(const Fixnum& n) {
        for(int i = 0; i < slots; ++i) {
            _data[i] ^= n._data[i];
        }

        _truncate(_data);
        return (*this);
    }

    Fixnum operator^(const Fixnum& n) const {
        Fixnum ret { *this };
        ret ^= n;
        return ret;
    }

    int operator[](const int index) const {
        if(index >= N || index < 0) {
            throw std::overflow_error("can't access bits beyond size of fixnum");
        }
        
        return _is_bit_set(_data, index) ? 1 : 0;
    }

    int fsb() const {
        const int slot = _first_non_zero_slot(_data);
        if(slot == -1) {
            return 0;
        }
        else {
            return (slot * 8) + decode::first_set_bit(_data[slot]);
        }
    }

    std::array<Fixnum,2> div_and_mod(const Fixnum& n) const {
        _check_divide_by_zero(n);
        Fixnum result { 0 };
        Fixnum dividend { *this };
        Fixnum divisor { n };
        _div_and_mod(result, dividend, divisor);
        return std::array<Fixnum,2> { result, dividend };
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

        return true;
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

    const uint8_t byte(const int index) const {
        return _data[index];
    }

    void byte(const int index, const uint8_t b) {
        _data[index] = b;
    }

    void sign_extend(const int bit) {
        const int slot = bit / 8;
        const int pos = (bit % 8);

        const int extension = (_data[slot] & (1 << pos)) > 0 ? 1 : 0;
        const int beyond_current = extension ? 0xFF : 0;

        if(extension) {
            _data[slot] |= (0xFF << (pos+1));
        }
        else {
            _data[slot] &= (0xFF >> (8-(pos+1)));
        }
        
        for(int i = slot + 1; i < slots; ++i) {
            _data[i] = beyond_current;
        }

        _truncate();
    }
    
private:
    uint8_t _data[slots];

    bool _is_bit_set(const uint8_t* d, const int bit) const {
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
        uint16_t sum = (uint16_t) target[0] + (uint16_t) 1;
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

    static void _truncate(uint8_t* d) {
        d[top_index] = d[top_index] & top_mask;
    }

    void _truncate() {
        _data[top_index] = _data[top_index] & top_mask;
    }

    static void _zero(uint8_t* d) {
        std::memset(d, 0, slots);
    }

    static void _fill_1(uint8_t* d) {
        std::memset(d, 0xFF, slots);
        _truncate(d);
    }

    static bool _is_zero(const uint8_t* d) {
        for(int i = 0; i < slots; ++i) {
            if(d[i] != 0) {
                return false;
            }
        }

        return true;
    }

    static void _left_shift(uint8_t* d, const int by) {
        //TODO: Replace this with something more clever at some point
        for(int i = 0; i < by; ++i) {
            _left_shift_1(d);
        }
    }
    
    static void _left_shift_1(uint8_t* d) {
        for(int i = top_index; i > 0; --i) {
            const int bottom = ((d[i-1] & 0x80) > 0) ? 1 : 0;
            d[i] = (d[i] << 1) | bottom;
        }

        d[0] <<= 1;
        _truncate(d);
    }

    static void _right_shift(uint8_t* d, const int by) {
        //TODO: Replace this with something more clever at some point
        for(int i = 0; i < by; ++i) {
            _right_shift_1(d);
        }
    }
    
    static void _right_shift_1(uint8_t* d) {
        for(int i = 0; i < top_index; ++i) {
            const int top = ((d[i+1] & 0x1) > 0) ? 0x80 : 0;
            d[i] = (d[i] >> 1) | top;
        }

        d[top_index] >>= 1;
        _truncate(d);
    }

    static int _cmp(const uint8_t* first, const uint8_t* second) {
        const bool fNeg = (first[top_index] & sign_mask) != 0;
        const bool sNeg = (second[top_index] & sign_mask) != 0;

        if(fNeg && !sNeg) {
            return -1;
        }
        else if(!fNeg && sNeg) {
            return 1;
        }

        //equal signs, now just compare the blocks
        for(int i = top_index; i >= 0; --i) {
            if(first[i] < second[i]) {
                return -1;
            }
            else if(first[i] > second[i]) {
                return 1;
            }
        }

        return 0;
    }

    static int _first_non_zero_slot(const uint8_t* d) {
        int i;
        
        for(i = top_index; i > -1; --i) {
            if(d[i] > 0) {
                return i;
            }
        }

        return i;
    }

    static void _check_divide_by_zero(const Fixnum& n) {
        if(_is_zero(n._data)) {
            throw std::invalid_argument("divide by zero");
        }
    }
    
    static void _div_and_mod(Fixnum& result, Fixnum& dividend, Fixnum& divisor) {
        const bool end_with_complement = dividend.is_negative() ^ divisor.is_negative();
        if(divisor.is_negative()) {
            divisor._complement();
        }

        if(dividend.is_negative()) {
            dividend._complement();
        }
        
        _zero(result._data);

        const int fsb_divisor = divisor.fsb();
        const int fsb_dividend = dividend.fsb();
        
        const int shift_it = fsb_dividend - fsb_divisor;
        if(shift_it < 0) {
            return;
        }

        int current_slot = shift_it / 8;
        int current_bit = shift_it % 8;
        _left_shift(divisor._data, shift_it);
        
        while(current_slot > -1) {
            if(divisor <= dividend) {
                result._data[current_slot] |= (1 << current_bit);
                dividend -= divisor;
            }

            divisor >>= 1;

            if(current_bit > 0) {
                --current_bit;
            }
            else {
                --current_slot;
                current_bit = 7;
            }
        }

        if(end_with_complement) {
            result._complement();
            dividend._complement();
        }
    }
};

template<>
class Fixnum<8> {
public:
    static constexpr size_t N = 8;
    static constexpr size_t bits = N;
    static constexpr int slots = 1;
    static constexpr int hex_slots = slots * 2;
    /*static constexpr int top_index = slots - 1;
    static constexpr int top_mask = 0xFF >> ((slots * 8) - N);
    static constexpr int sign_mask = 0x80 >> ((slots * 8) - N);
    static constexpr int unsigned_mask = 0xFF >> (1 + (slots * 8) - N);*/
    
    template<size_t T, size_t S>
    friend Fixnum<T> fixnum_cast(const Fixnum<S>& source);
    
    static constexpr Fixnum lowest() {
        return Fixnum(std::numeric_limits<int8_t>::lowest());
    }

    static constexpr Fixnum max() {
        return Fixnum(std::numeric_limits<int8_t>::max());
    }
    
    constexpr Fixnum() : _data { 0 } {}

    Fixnum(const Fixnum& f) {
        _data = f._data;
    }

    Fixnum(Fixnum&& f) {
        _data = f._data;
    }
    
    Fixnum(const std::initializer_list<uint8_t> init) : _data { 0 } {
        const uint8_t* ptr = init.begin();
        _data = static_cast<int8_t>(ptr[0]);
    }


    Fixnum(const char* input, const int base) : Fixnum(decode::ConvertBase<uint8_t>(input, base, 16)) { }

    Fixnum(const std::string& input, const int base) : Fixnum(decode::ConvertBase<uint8_t>(input, base, 16)) { }

    Fixnum(const decode::ConvertBase<uint8_t>& cb) : Fixnum() {
        if(cb.is_zero()) {
            return;
        }

        const bool overflows = (slots * 2) < cb.converted.size();
        
        int source_index = cb.converted.size() - 1;
        if(source_index >= 0) {
            _data = cb.converted[source_index--];
        }
        if(source_index >= 0) {
            _data |= (cb.converted[source_index--] << 4);
         }

        if(!overflows && !is_negative() && cb.is_negative()) {
            _data = -_data;
        }
    }

    constexpr Fixnum(const int8_t val) : Fixnum() {
        _data = val;
    }
    
    constexpr Fixnum(const int16_t val) : Fixnum() {
        _data = val;
    }
    
    constexpr Fixnum(const int32_t val) : Fixnum() {
        _data = val & 0xFF;
    }

    constexpr Fixnum(const int64_t val) : Fixnum(0) {
        _data = val & 0xFF;
    }

    Fixnum& operator=(const Fixnum& f) {
        _data = f._data;
        return *this;
    }

    Fixnum& operator=(Fixnum&& f) {
        _data = f._data;
        return *this;
    }

    bool operator==(const Fixnum& rhs) const {
        return _data == rhs._data;
    }

    bool operator!=(const Fixnum& rhs) const {
        return _data != rhs._data;
    }

    bool operator<(const Fixnum& rhs) const {
        return _data < rhs._data;
    }

    bool operator<=(const Fixnum& rhs) const {
        return _data <= rhs._data;
    }

    bool operator>(const Fixnum& rhs) const {
        return _data > rhs._data;
    }

    bool operator>=(const Fixnum& rhs) const {
        return _data >= rhs._data;
    }

    Fixnum& operator+=(const Fixnum& rhs) {
        _data += rhs._data;
        return *this;
    }
    
    Fixnum operator+(const Fixnum& n) const {
        Fixnum ret { *this };
        ret += n;
        return ret;
    }

    Fixnum& operator++() {
        ++_data;
        return *this;
    }

    Fixnum operator++(int) {
        Fixnum ret { *this };
        ++_data;
        return ret;
    }

    Fixnum& operator-=(const Fixnum& rhs) {
        _data -= rhs._data;
        return *this;
    }

    Fixnum operator-(const Fixnum& n) const {
        Fixnum ret { *this };
        ret -= n;
        return ret;
    }

    Fixnum& operator--() {
        --_data;
        return *this;
    }

    Fixnum operator--(int) {
        Fixnum ret { *this };
        --_data;
        return ret;
    }

    Fixnum operator-() const {
        return -_data;
    }

    Fixnum& operator*=(const Fixnum& n) {
        _data *= n._data;
        return *this;
    }

    Fixnum operator*(const Fixnum& n) const {
        Fixnum ret { *this };
        ret *= n;
        return ret;
    }

    Fixnum& operator/=(const Fixnum& n) {
        _data /= n._data;
        return *this;
    }

    Fixnum operator/(const Fixnum& n) {
        Fixnum ret { *this };
        ret /= n;
        return ret;
    }

    Fixnum& operator%=(const Fixnum& n) {
        _data %= n._data;
        return *this;
    }

    Fixnum operator%(const Fixnum& n) {
        Fixnum ret = { *this };
        ret %= n;
        return ret;
    }

    Fixnum& operator<<=(const int by) {
        _data <<= by;
        return *this;
    }
    
    Fixnum operator<<(const int by) const {
        Fixnum ret { *this };
        ret <<= by;
        return ret;
    }

    Fixnum& operator>>=(const int by) {
        _data >>= by;
        return *this;
    }

    Fixnum operator>>(const int by) const {
        Fixnum ret { *this };
        ret >>= by;
        return ret;
    }

    Fixnum& operator&=(const Fixnum& n) {
        _data &= n._data;
        return *this;
    }

    Fixnum operator&(const Fixnum& n) const {
        Fixnum ret { *this };
        ret &= n;
        return ret;
    }

    Fixnum& operator|=(const Fixnum& n) {
        _data |= n._data;
        return *this;
    }

    Fixnum operator|(const Fixnum& n) const {
        Fixnum ret { *this };
        ret |= n;
        return ret;
    }

    Fixnum& operator^=(const Fixnum& n) {
        _data ^= n._data;
        return (*this);
    }

    Fixnum operator^(const Fixnum& n) const {
        Fixnum ret { *this };
        ret ^= n;
        return ret;
    }

    int operator[](const int index) const {
        if(index >= N || index < 0) {
            throw std::overflow_error("can't access bits beyond size of fixnum");
        }

        return (_data & (1 << index)) > 0 ? 1 : 0;
    }

    int fsb() const {
        return decode::first_set_bit(0xFF & _data);
    }

    std::array<Fixnum,2> div_and_mod(const Fixnum& n) const {
        if(n._data == 0) {
            throw std::invalid_argument("divide by zero");
        }
        
        return std::array<Fixnum,2> { Fixnum(_data / n._data), Fixnum(_data % n._data) };
    }
    
    bool is_negative() const {
        return _data < 0;
    }

    bool is_positive() const {
        return _data > 0;
    }

    bool is_lowest() const {
        return _data == std::numeric_limits<int8_t>::lowest(); 
    }

    bool is_max() const {
        return _data == std::numeric_limits<int8_t>::max();
    }
    
    std::string str() const {
        return str(10);
    }

    std::string str(const int base) const {
        if(is_negative() && !is_lowest()) {
            return std::string("-") + complement().str(base);
        }
        else {
            uint8_t pos_hex[hex_slots] = { static_cast<uint8_t>(0xF & _data),
                                           static_cast<uint8_t>(0xF & (_data >> 4)) };
            std::string ret = decode::convert_pos_str<hex_slots>(pos_hex, base);
            return !is_lowest() ? ret : std::string("-") + ret;
        }
    }

    Fixnum complement() const {
        Fixnum ret { *this };
        ret._data = -ret._data;
        return ret;
    }

    const uint8_t byte(const int index) const {
        if(index != 0) {
            throw std::overflow_error("can't access bits beyond size of fixnum");
        }
        
        return 0xFF & _data;
    }

    void byte(const int index, const uint8_t b) {
        if(index != 0) {
            throw std::overflow_error("can't access bits beyond size of fixnum");
        }
        
        _data = 0xFF & b;
    }

    void sign_extend(const int bit) {
        const int extension = (_data & (1 << bit)) > 0 ? 1 : 0;

        if(extension) {
            _data |= (0xFF << (bit+1));
        }
        else {
            _data &= (0xFF >> (8-(bit+1)));
        }
    }
    
private:
    int8_t _data;
};

template<>
class Fixnum<16> {
public:
    static constexpr size_t N = 16;
    static constexpr size_t bits = N;
    static constexpr int slots = 2;
    static constexpr int hex_slots = slots * 2;
    /*static constexpr int top_index = slots - 1;
    static constexpr int top_mask = 0xFF >> ((slots * 8) - N);
    static constexpr int sign_mask = 0x80 >> ((slots * 8) - N);
    static constexpr int unsigned_mask = 0xFF >> (1 + (slots * 8) - N);*/
    
    template<size_t T, size_t S>
    friend Fixnum<T> fixnum_cast(const Fixnum<S>& source);
    
    static constexpr Fixnum lowest() {
        return Fixnum(std::numeric_limits<int16_t>::lowest());
    }

    static constexpr Fixnum max() {
        return Fixnum(std::numeric_limits<int16_t>::max());
    }
    
    constexpr Fixnum() : _data { 0 } {}

    Fixnum(const Fixnum& f) {
        _data = f._data;
    }

    Fixnum(Fixnum&& f) {
        _data = f._data;
    }

    Fixnum(const std::initializer_list<uint8_t> init) : _data { 0 } {
        const uint8_t* ptr = init.begin();
        for(int i = 0, shift_by = 0; i < init.size() && i < slots; ++i, shift_by += 8) {
            _data |= (ptr[i] << shift_by);
        }
    }

    Fixnum(const char* input, const int base) : Fixnum(decode::ConvertBase<uint8_t>(input, base, 16)) { }

    Fixnum(const std::string& input, const int base) : Fixnum(decode::ConvertBase<uint8_t>(input, base, 16)) { }

    Fixnum(const decode::ConvertBase<uint8_t>& cb) : Fixnum() {
        if(cb.is_zero()) {
            return;
        }

        const bool overflows = (slots * 2) < cb.converted.size();

        int source_index, shift_by;
        for(source_index = cb.converted.size() - 1, shift_by = 0;
            source_index >= 0 && shift_by <= 12;
            --source_index, shift_by += 4) {

            _data |= (cb.converted[source_index] << shift_by);

        }

        if(!overflows && !is_negative() && cb.is_negative()) {
            _data = -_data;
        }
    }

    constexpr Fixnum(const int8_t val) : Fixnum() {
        _data = val;
    }
    
    constexpr Fixnum(const int16_t val) : Fixnum() {
        _data = val;
    }
    
    constexpr Fixnum(const int32_t val) : Fixnum() {
        _data = val & 0xFFFF;
    }

    constexpr Fixnum(const int64_t val) : Fixnum(0) {
        _data = val & 0xFFFF;
    }

    Fixnum& operator=(const Fixnum& f) {
        _data = f._data;
        return *this;
    }

    Fixnum& operator=(Fixnum&& f) {
        _data = f._data;
        return *this;
    }

    bool operator==(const Fixnum& rhs) const {
        return _data == rhs._data;
    }

    bool operator!=(const Fixnum& rhs) const {
        return _data != rhs._data;
    }

    bool operator<(const Fixnum& rhs) const {
        return _data < rhs._data;
    }

    bool operator<=(const Fixnum& rhs) const {
        return _data <= rhs._data;
    }

    bool operator>(const Fixnum& rhs) const {
        return _data > rhs._data;
    }

    bool operator>=(const Fixnum& rhs) const {
        return _data >= rhs._data;
    }

    Fixnum& operator+=(const Fixnum& rhs) {
        _data += rhs._data;
        return *this;
    }
    
    Fixnum operator+(const Fixnum& n) const {
        Fixnum ret { *this };
        ret += n;
        return ret;
    }

    Fixnum& operator++() {
        ++_data;
        return *this;
    }

    Fixnum operator++(int) {
        Fixnum ret { *this };
        ++_data;
        return ret;
    }

    Fixnum& operator-=(const Fixnum& rhs) {
        _data -= rhs._data;
        return *this;
    }

    Fixnum operator-(const Fixnum& n) const {
        Fixnum ret { *this };
        ret -= n;
        return ret;
    }

    Fixnum& operator--() {
        --_data;
        return *this;
    }

    Fixnum operator--(int) {
        Fixnum ret { *this };
        --_data;
        return ret;
    }

    Fixnum operator-() const {
        return -_data;
    }

    Fixnum& operator*=(const Fixnum& n) {
        _data *= n._data;
        return *this;
    }

    Fixnum operator*(const Fixnum& n) const {
        Fixnum ret { *this };
        ret *= n;
        return ret;
    }

    Fixnum& operator/=(const Fixnum& n) {
        _data /= n._data;
        return *this;
    }

    Fixnum operator/(const Fixnum& n) {
        Fixnum ret { *this };
        ret /= n;
        return ret;
    }

    Fixnum& operator%=(const Fixnum& n) {
        _data %= n._data;
        return *this;
    }

    Fixnum operator%(const Fixnum& n) {
        Fixnum ret = { *this };
        ret %= n;
        return ret;
    }

    Fixnum& operator<<=(const int by) {
        _data <<= by;
        return *this;
    }
    
    Fixnum operator<<(const int by) const {
        Fixnum ret { *this };
        ret <<= by;
        return ret;
    }

    Fixnum& operator>>=(const int by) {
        _data >>= by;
        return *this;
    }

    Fixnum operator>>(const int by) const {
        Fixnum ret { *this };
        ret >>= by;
        return ret;
    }

    Fixnum& operator&=(const Fixnum& n) {
        _data &= n._data;
        return *this;
    }

    Fixnum operator&(const Fixnum& n) const {
        Fixnum ret { *this };
        ret &= n;
        return ret;
    }

    Fixnum& operator|=(const Fixnum& n) {
        _data |= n._data;
        return *this;
    }

    Fixnum operator|(const Fixnum& n) const {
        Fixnum ret { *this };
        ret |= n;
        return ret;
    }

    Fixnum& operator^=(const Fixnum& n) {
        _data ^= n._data;
        return (*this);
    }

    Fixnum operator^(const Fixnum& n) const {
        Fixnum ret { *this };
        ret ^= n;
        return ret;
    }

    int operator[](const int index) const {
        if(index >= N || index < 0) {
            throw std::overflow_error("can't access bits beyond size of fixnum");
        }

        return (_data & (1 << index)) > 0 ? 1 : 0;
    }

    int fsb() const {
        int b = decode::first_set_bit(_data >> 8);
        if(b > 0) {
            return b + 8;
        }
        
        return decode::first_set_bit(_data & 0xFF);
    }

    std::array<Fixnum,2> div_and_mod(const Fixnum& n) const {
        if(n._data == 0) {
            throw std::invalid_argument("divide by zero");
        }
        
        return std::array<Fixnum,2> { Fixnum(_data / n._data), Fixnum(_data % n._data) };
    }
    
    bool is_negative() const {
        return _data < 0;
    }

    bool is_positive() const {
        return _data > 0;
    }

    bool is_lowest() const {
        return _data == std::numeric_limits<int16_t>::lowest(); 
    }

    bool is_max() const {
        return _data == std::numeric_limits<int16_t>::max();
    }
    
    std::string str() const {
        return str(10);
    }

    std::string str(const int base) const {
        if(is_negative() && !is_lowest()) {
            return std::string("-") + complement().str(base);
        }
        else {
            uint8_t pos_hex[hex_slots] = { static_cast<uint8_t>(0xF & _data),
                                           static_cast<uint8_t>(0xF & (_data >> 4)),
                                           static_cast<uint8_t>(0xF & (_data >> 8)),
                                           static_cast<uint8_t>(0xF & (_data >> 12)) };
                                           
            std::string ret = decode::convert_pos_str<hex_slots>(pos_hex, base);
            return !is_lowest() ? ret : std::string("-") + ret;
        }
    }

    Fixnum complement() const {
        Fixnum ret { *this };
        ret._data = -ret._data;
        return ret;
    }

    const uint8_t byte(const int index) const {
        if(index > 1 || index < 0) {
            throw std::overflow_error("can't access bits beyond size of fixnum");
        }

        if(index == 0) {
            return 0xFF & _data;
        }
        else {
            return 0xFF & (_data >> 8);
        }
    }

    void byte(const int index, const uint8_t b) {
        if(index > 1 || index < 0) {
            throw std::overflow_error("can't access bits beyond size of fixnum");
        }

        if(index == 0) {
            _data &= (b & 0xFF);
        }
        else {
            _data &= ((b & 0xFF) << 8);
        }
    }

    void sign_extend(const int bit) {
        const int extension = (_data & (1 << bit)) > 0 ? 1 : 0;

        if(extension) {
            _data |= (0xFFFF << (bit+1));
        }
        else {
            _data &= (0xFFFF >> (8-(bit+1)));
        }
    }
    
private:
    int16_t _data;
};


template<size_t T, size_t S>
Fixnum<T> fixnum_cast(const Fixnum<S>& source) {
    Fixnum<T> ret { 0 };
    
    for(int i = 0; i < Fixnum<T>::slots && i < Fixnum<S>::slots; ++i) {
        ret.byte(i, source.byte(i));
    }
    
    if(T > S) {
        ret.sign_extend(S-1);
    }
    
    return ret;
}

#endif
