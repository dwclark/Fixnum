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
#include <type_traits>

template<size_t N>
class Fixnum {
public:
    static constexpr size_t bits = N;
    static constexpr int bytes = (N / 8) + ((N % 8) > 0 ? 1 : 0);
    static constexpr int hex_bytes = bytes * 2;
    static constexpr int top_index = bytes - 1;
    static constexpr int top_mask = 0xFF >> ((bytes * 8) - N);
    static constexpr int sign_mask = 0x80 >> ((bytes * 8) - N);
    static constexpr int unsigned_mask = 0xFF >> (1 + (bytes * 8) - N);

    static Fixnum lowest() {
        Fixnum fn;
        fn._data[top_index] = sign_mask;
        return fn;
    }

    static Fixnum max() {
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
        for(int i = 0; i < init.size() && i < bytes; ++i) {
            _data[i] = ptr[i];
        }

        if(bytes <= init.size()) {
            _truncate();
        }
    }

    Fixnum(const char* input, const int base) : Fixnum(decode::ConvertBase<uint8_t>(input, base, 16)) { }

    Fixnum(const std::string& input, const int base) : Fixnum(decode::ConvertBase<uint8_t>(input, base, 16)) { }
    
    Fixnum(const decode::ConvertBase<uint8_t>& cb) : Fixnum() {
        if(cb.is_zero()) {
            return;
        }

        const bool overflows = (bytes * 2) < cb.converted.size();
        
        int source_index = cb.converted.size() - 1;
        for(int i = 0; i < bytes; ++i) {
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
        if(bytes >= 2) _data[1] = (val >> 8) & 0xFF;
        
        //sign extend if needed
        if(bytes > 2) {
            sign_extend(15);
        }
        else {
            _truncate();
        }
    }
    
    Fixnum(const int32_t val) : Fixnum() {
        _data[0] = val & 0xFF;
        if(bytes >= 2) _data[1] = (val >> 8) & 0xFF;
        if(bytes >= 3) _data[2] = (val >> 16) & 0xFF;
        if(bytes >= 4) _data[3] = (val >> 24) & 0xFF;

        //sign extend if needed
        if(bytes > 4) {
            sign_extend(31);
        }
        else {
            _truncate();
        }
    }

    Fixnum(const int64_t val) : Fixnum(0) {
        _data[0] = val & 0xFF;
        if(bytes >= 2) _data[1] = (val >> 8) & 0xFF;
        if(bytes >= 3) _data[2] = (val >> 16) & 0xFF;
        if(bytes >= 4) _data[3] = (val >> 24) & 0xFF;
        if(bytes >= 5) _data[4] = (val >> 32) & 0xFF;
        if(bytes >= 6) _data[5] = (val >> 40) & 0xFF;
        if(bytes >= 7) _data[6] = (val >> 48) & 0xFF;
        if(bytes >= 8) _data[7] = (val >> 56) & 0xFF;

        //sign extend if needed
        if(bytes > 8) {
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
        return memcmp(_data, rhs._data, bytes) == 0;
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

    Fixnum& operator/=(const Fixnum& n) {
        _check_divide_by_zero(n);
        Fixnum dividend { *this };
        Fixnum divisor { n };
        _div_and_mod(*this, dividend, divisor);
        return *this;
    }

    Fixnum& operator%=(const Fixnum& n) {
        _check_divide_by_zero(n);
        Fixnum dividend { *this };
        Fixnum divisor { n };
        _div_and_mod(*this, dividend, divisor);
        std::memcpy(_data, dividend._data, bytes);
        return *this;
    }

    Fixnum& operator<<=(const int by) {
        switch(by) {
        case 0: break;
        case 1: _left_shift_1(_data); break;
        default: _left_shift(_data, by);
        }

        return *this;
    }

    Fixnum& operator>>=(const int by) {
        switch(by) {
        case 0: break;
        case 1: _right_shift_1(_data); break;
        default: _right_shift(_data, by);
        }

        return *this;
    }

    Fixnum& operator&=(const Fixnum& n) {
        for(int i = 0; i < bytes; ++i) {
            _data[i] &= n._data[i];
        }

        _truncate(_data);
        return *this;
    }

    Fixnum& operator|=(const Fixnum& n) {
        for(int i = 0; i < bytes; ++i) {
            _data[i] |= n._data[i];
        }

        _truncate(_data);
        return *this;
    }

    Fixnum& operator^=(const Fixnum& n) {
        for(int i = 0; i < bytes; ++i) {
            _data[i] ^= n._data[i];
        }

        _truncate(_data);
        return (*this);
    }

    bool operator[](const int index) const {
        return bit(index);
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
            uint8_t pos_hex[hex_bytes] = { 0 };
            _single_hex_values(pos_hex);
            std::string ret = decode::convert_pos_str<hex_bytes>(pos_hex, base);
            return !is_lowest() ? ret : std::string("-") + ret;
        }
    }

    Fixnum complement() const {
        Fixnum ret { *this };
        ret._complement();
        return ret;
    }

    const uint8_t byte(const int index) const {
        if(index > top_index || index < 0) {
            throw std::overflow_error("can't access bits beyond size of fixnum");
        }
        
        return _data[index];
    }

    void byte(const int index, const uint8_t b) {
        if(index >= top_index || index < 0) {
            throw std::overflow_error("can't access bits beyond size of fixnum");
        }
        
        _data[index] = b;
    }

    bool bit(const int index) const {
        if(index >= N || index < 0) {
            throw std::overflow_error("can't access bits beyond size of fixnum");
        }
        
        return _is_bit_set(_data, index);
    }

    Fixnum& bit(const int index, const bool val) {
        if(index >= N || index < 0) {
            throw std::overflow_error("can't access bits beyond size of fixnum");
        }

        if(val) _set_bit(_data, index);
        else _unset_bit(_data, index);

        return *this;
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
        
        for(int i = slot + 1; i < bytes; ++i) {
            _data[i] = beyond_current;
        }

        _truncate();
    }
    
private:
    uint8_t _data[bytes];

    bool _is_bit_set(const uint8_t* d, const int bit) const {
        const int index = bit / 8;
        const int bit_pos = bit % 8;
        const int mask = 1 << bit_pos;
        return (d[index] & mask) > 0;
    }

    void _set_bit(uint8_t* d, const int bit) {
        const int index = bit / 8;
        const int bit_pos = bit % 8;
        const int mask = 1 << bit_pos;
        d[index] |= mask;
    }

    void _unset_bit(uint8_t* d, const int bit) {
        const int index = bit / 8;
        const int bit_pos = bit % 8;
        const int mask = 1 << bit_pos;
        d[index] &= ~mask;
    }
    
    void _add_to(uint8_t* target, const uint8_t* n) {
        uint16_t rem = 0;
        for(int i = 0; i < bytes; ++i) {
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
        
        while(rem > 0 && i < bytes) {
            sum = (uint16_t) target[i] + rem;
            target[i] = sum & 0xFF;
            rem = (sum & 0xF00) >> 8;
            ++i;
        }

        _truncate(target);
    }

    void _subtract_one(uint8_t* target) {
        int nonzero_slot;
        for(nonzero_slot = 0; nonzero_slot < bytes; ++nonzero_slot) {
            if(target[nonzero_slot] != 0) {
                break;
            }
        }

        if(nonzero_slot == bytes) {
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
        for(int i = 0; i < bytes; ++i) {
            d[i] = ~d[i];
        }

        _add_one(d);
    }

    void _complement() {
        _complement(_data);
    }

    void _single_hex_values(uint8_t* d) const {
        for(int i = 0; i < bytes; ++i) {
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
        std::memset(d, 0, bytes);
    }

    static void _fill_1(uint8_t* d) {
        std::memset(d, 0xFF, bytes);
        _truncate(d);
    }

    static bool _is_zero(const uint8_t* d) {
        for(int i = 0; i < bytes; ++i) {
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
    static constexpr int bytes = 1;
    static constexpr int hex_bytes = bytes * 2;
    
    static Fixnum lowest() {
        return Fixnum(std::numeric_limits<int8_t>::lowest());
    }

    static Fixnum max() {
        return Fixnum(std::numeric_limits<int8_t>::max());
    }
    
    Fixnum() : _data { 0 } {}

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

        const bool overflows = (bytes * 2) < cb.converted.size();
        
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

    Fixnum(const int8_t val) : Fixnum() {
        _data = val;
    }
    
    Fixnum(const int16_t val) : Fixnum() {
        _data = val;
    }
    
    Fixnum(const int32_t val) : Fixnum() {
        _data = val;
    }

    Fixnum(const int64_t val) : Fixnum(0) {
        _data = val;
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

    Fixnum& operator/=(const Fixnum& n) {
        _data /= n._data;
        return *this;
    }

    Fixnum& operator%=(const Fixnum& n) {
        _data %= n._data;
        return *this;
    }

    Fixnum& operator<<=(const int by) {
        _data <<= by;
        return *this;
    }
    
    Fixnum& operator>>=(const int by) {
        _data >>= by;
        return *this;
    }

    Fixnum& operator&=(const Fixnum& n) {
        _data &= n._data;
        return *this;
    }

    Fixnum& operator|=(const Fixnum& n) {
        _data |= n._data;
        return *this;
    }

    Fixnum& operator^=(const Fixnum& n) {
        _data ^= n._data;
        return (*this);
    }

    bool operator[](const int index) const {
        return bit(index);
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
            uint8_t pos_hex[hex_bytes] = { static_cast<uint8_t>(0xF & _data),
                                           static_cast<uint8_t>(0xF & (_data >> 4)) };
            std::string ret = decode::convert_pos_str<hex_bytes>(pos_hex, base);
            return !is_lowest() ? ret : std::string("-") + ret;
        }
    }

    Fixnum complement() const {
        Fixnum ret { *this };
        ret._data = -ret._data;
        return ret;
    }

    const uint8_t byte(const int index) const {
        if(index == 0) return 0xFF & _data;
        else throw std::overflow_error("can't access bits beyond size of fixnum");
    }

    void byte(const int index, const uint8_t b) {
        if(index == 0) _data = 0xFF & b;
        else throw std::overflow_error("can't access bits beyond size of fixnum");
    }

    bool bit(const int index) const {
        if(index >= N || index < 0) {
            throw std::overflow_error("can't access bits beyond size of fixnum");
        }

        return (_data & (1 << index)) != 0;
    }

    Fixnum& bit(const int index, const bool val) {
        if(index >= N || index < 0) {
            throw std::overflow_error("can't access bits beyond size of fixnum");
        }
        
        if(val) _data |= (1 << index);
        else _data &= ~(1 << index);
        return *this;
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
    static constexpr int bytes = 2;
    static constexpr int hex_bytes = bytes * 2;
    
    static Fixnum lowest() {
        return Fixnum(std::numeric_limits<int16_t>::lowest());
    }

    static Fixnum max() {
        return Fixnum(std::numeric_limits<int16_t>::max());
    }
    
    Fixnum() : _data { 0 } {}

    Fixnum(const Fixnum& f) {
        _data = f._data;
    }

    Fixnum(Fixnum&& f) {
        _data = f._data;
    }

    Fixnum(const std::initializer_list<uint8_t> init) : _data { 0 } {
        const uint8_t* ptr = init.begin();
        for(int i = 0, shift_by = 0; i < init.size() && i < bytes; ++i, shift_by += 8) {
            _data |= (ptr[i] << shift_by);
        }
    }

    Fixnum(const char* input, const int base) : Fixnum(decode::ConvertBase<uint8_t>(input, base, 16)) { }

    Fixnum(const std::string& input, const int base) : Fixnum(decode::ConvertBase<uint8_t>(input, base, 16)) { }

    Fixnum(const decode::ConvertBase<uint8_t>& cb) : Fixnum() {
        if(cb.is_zero()) {
            return;
        }

        const bool overflows = (bytes * 2) < cb.converted.size();

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

    Fixnum(const int8_t val) : Fixnum() {
        _data = val;
    }
    
    Fixnum(const int16_t val) : Fixnum() {
        _data = val;
    }
    
    Fixnum(const int32_t val) : Fixnum() {
        _data = val;
    }

    Fixnum(const int64_t val) : Fixnum(0) {
        _data = val;
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

    Fixnum& operator/=(const Fixnum& n) {
        _data /= n._data;
        return *this;
    }

    Fixnum& operator%=(const Fixnum& n) {
        _data %= n._data;
        return *this;
    }

    Fixnum& operator<<=(const int by) {
        _data <<= by;
        return *this;
    }
    
    Fixnum& operator>>=(const int by) {
        _data >>= by;
        return *this;
    }

    Fixnum& operator&=(const Fixnum& n) {
        _data &= n._data;
        return *this;
    }

    Fixnum& operator|=(const Fixnum& n) {
        _data |= n._data;
        return *this;
    }

    Fixnum& operator^=(const Fixnum& n) {
        _data ^= n._data;
        return (*this);
    }

    bool operator[](const int index) const {
        return bit(index);
    }

    int fsb() const {
        int b = decode::first_set_bit(0xFF & (_data >> 8));
        if(b > 0) return b + 8;
        
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
            uint8_t pos_hex[hex_bytes] = { static_cast<uint8_t>(0xF & _data),
                                           static_cast<uint8_t>(0xF & (_data >> 4)),
                                           static_cast<uint8_t>(0xF & (_data >> 8)),
                                           static_cast<uint8_t>(0xF & (_data >> 12)) };

            std::string ret = decode::convert_pos_str<hex_bytes>(pos_hex, base);
            return !is_lowest() ? ret : (std::string("-") + ret);
        }
    }

    Fixnum complement() const {
        Fixnum ret { *this };
        ret._data = -ret._data;
        return ret;
    }

    const uint8_t byte(const int index) const {
        switch(index) {
        case 0: return 0xFF & _data;
        case 1: return 0xFF & (_data >> 8);
        default:throw std::overflow_error("can't access bits beyond size of fixnum");
        }
    }

    void byte(const int index, const uint8_t b) {
        switch(index) {
        case 0: _data = (_data & ~0xFF) | b; break;
        case 1: _data = (_data & ~(0xFF << 8)) | (b << 8); break;
        default: throw std::overflow_error("can't access bits beyond size of fixnum");
        }
    }

    bool bit(const int index) const {
        if(index >= N || index < 0) {
            throw std::overflow_error("can't access bits beyond size of fixnum");
        }
        
        return (_data & (1 << index)) != 0;
    }

    Fixnum& bit(const int index, const bool val) {
        if(index >= N || index < 0) {
            throw std::overflow_error("can't access bits beyond size of fixnum");
        }

        if(val) _data |= (1 << index);
        else _data &= ~(1 << index);
        return *this;
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

template<>
class Fixnum<32> {
public:
    static constexpr size_t N = 32;
    static constexpr size_t bits = N;
    static constexpr int bytes = 4;
    static constexpr int hex_bytes = bytes * 2;
    
    static Fixnum lowest() {
        return Fixnum(std::numeric_limits<int32_t>::lowest());
    }

    static Fixnum max() {
        return Fixnum(std::numeric_limits<int32_t>::max());
    }
    
    Fixnum() : _data { 0 } {}

    Fixnum(const Fixnum& f) {
        _data = f._data;
    }

    Fixnum(Fixnum&& f) {
        _data = f._data;
    }

    Fixnum(const std::initializer_list<uint8_t> init) : _data { 0 } {
        const uint8_t* ptr = init.begin();
        for(int i = 0, shift_by = 0; i < init.size() && i < bytes; ++i, shift_by += 8) {
            _data |= (ptr[i] << shift_by);
        }
    }

    Fixnum(const char* input, const int base) : Fixnum(decode::ConvertBase<uint8_t>(input, base, 16)) { }

    Fixnum(const std::string& input, const int base) : Fixnum(decode::ConvertBase<uint8_t>(input, base, 16)) { }

    Fixnum(const decode::ConvertBase<uint8_t>& cb) : Fixnum() {
        if(cb.is_zero()) {
            return;
        }

        const bool overflows = (bytes * 2) < cb.converted.size();

        int source_index, shift_by;
        for(source_index = cb.converted.size() - 1, shift_by = 0;
            source_index >= 0 && shift_by <= 28;
            --source_index, shift_by += 4) {

            _data |= (cb.converted[source_index] << shift_by);

        }

        if(!overflows && !is_negative() && cb.is_negative()) {
            _data = -_data;
        }
    }

    Fixnum(const int8_t val) : Fixnum() {
        _data = val;
    }
    
    Fixnum(const int16_t val) : Fixnum() {
        _data = val;
    }
    
    Fixnum(const int32_t val) : Fixnum() {
        _data = val;
    }

    Fixnum(const int64_t val) : Fixnum() {
        _data = val;
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

    Fixnum& operator/=(const Fixnum& n) {
        _data /= n._data;
        return *this;
    }

    Fixnum& operator%=(const Fixnum& n) {
        _data %= n._data;
        return *this;
    }

    Fixnum& operator<<=(const int by) {
        _data <<= by;
        return *this;
    }
    
    Fixnum& operator>>=(const int by) {
        _data >>= by;
        return *this;
    }

    Fixnum& operator&=(const Fixnum& n) {
        _data &= n._data;
        return *this;
    }

    Fixnum& operator|=(const Fixnum& n) {
        _data |= n._data;
        return *this;
    }

    Fixnum& operator^=(const Fixnum& n) {
        _data ^= n._data;
        return (*this);
    }

    bool operator[](const int index) const {
        return bit(index);
    }

    int fsb() const {
        int b = decode::first_set_bit(0xFF & (_data >> 24));
        if(b > 0) return b + 24;

        b = decode::first_set_bit(0xFF & (_data >> 16));
        if(b > 0) return b + 16;

        b = decode::first_set_bit(0xFF & (_data >> 8));
        if(b > 0) return b + 8;
        
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
        return _data == std::numeric_limits<int32_t>::lowest(); 
    }

    bool is_max() const {
        return _data == std::numeric_limits<int32_t>::max();
    }
    
    std::string str() const {
        return str(10);
    }

    std::string str(const int base) const {
        if(is_negative() && !is_lowest()) {
            return std::string("-") + complement().str(base);
        }
        else {
            uint8_t pos_hex[hex_bytes] = { static_cast<uint8_t>(0xF & _data),
                                           static_cast<uint8_t>(0xF & (_data >> 4)),
                                           static_cast<uint8_t>(0xF & (_data >> 8)),
                                           static_cast<uint8_t>(0xF & (_data >> 12)),
                                           static_cast<uint8_t>(0xF & (_data >> 16)),
                                           static_cast<uint8_t>(0xF & (_data >> 20)),
                                           static_cast<uint8_t>(0xF & (_data >> 24)),
                                           static_cast<uint8_t>(0xF & (_data >> 28)) };
                                           
            std::string ret = decode::convert_pos_str<hex_bytes>(pos_hex, base);
            return !is_lowest() ? ret : std::string("-") + ret;
        }
    }

    Fixnum complement() const {
        Fixnum ret { *this };
        ret._data = -ret._data;
        return ret;
    }

    const uint8_t byte(const int index) const {
        switch(index) {
        case 0: return 0xFF & _data;
        case 1: return 0xFF & (_data >> 8);
        case 2: return 0xFF & (_data >> 16);
        case 3: return 0xFF & (_data >> 24);
        default: throw std::overflow_error("can't access bits beyond size of fixnum");
        }
    }

    void byte(const int index, const uint8_t b) {
        switch(index) {
        case 0: _data = (_data & ~0xFF) | b; break;
        case 1: _data = (_data & ~(0xFF << 8)) | (b << 8); break;
        case 2: _data = (_data & ~(0xFF << 16)) | (b << 16); break;
        case 3: _data = (_data & ~(0xFF << 24)) | (b << 24); break;
        default: throw std::overflow_error("can't access bits beyond size of fixnum");
        }
    }

    bool bit(const int index) const {
        if(index >= N || index < 0) {
            throw std::overflow_error("can't access bits beyond size of fixnum");
        }
        
        return (_data & (1 << index)) != 0;
    }

    Fixnum& bit(const int index, const bool val) {
        if(index >= N || index < 0) {
            throw std::overflow_error("can't access bits beyond size of fixnum");
        }

        if(val) _data |= (1 << index);
        else _data &= ~(1 << index);
        return *this;
    }

    void sign_extend(const int bit) {
        const int extension = (_data & (1 << bit)) > 0 ? 1 : 0;

        if(extension) {
            _data |= (0xFFFFFFFF << (bit+1));
        }
        else {
            _data &= (0xFFFFFFFF >> (8-(bit+1)));
        }
    }
    
private:
    int32_t _data;
};

template<>
class Fixnum<64> {
public:
    static constexpr size_t N = 64;
    static constexpr size_t bits = N;
    static constexpr int bytes = 8;
    static constexpr int hex_bytes = bytes * 2;
    
    static Fixnum lowest() {
        return Fixnum(std::numeric_limits<int64_t>::lowest());
    }

    static Fixnum max() {
        return Fixnum(std::numeric_limits<int64_t>::max());
    }
    
    Fixnum() : _data { 0 } {}

    Fixnum(const Fixnum& f) {
        _data = f._data;
    }

    Fixnum(Fixnum&& f) {
        _data = f._data;
    }

    Fixnum(const std::initializer_list<uint8_t> init) : _data { 0 } {
        const uint8_t* ptr = init.begin();
        for(int i = 0, shift_by = 0; i < init.size() && i < bytes; ++i, shift_by += 8) {
            _data |= (ptr[i] << shift_by);
        }
    }

    Fixnum(const char* input, const int base) : Fixnum(decode::ConvertBase<uint8_t>(input, base, 16)) { }

    Fixnum(const std::string& input, const int base) : Fixnum(decode::ConvertBase<uint8_t>(input, base, 16)) { }

    Fixnum(const decode::ConvertBase<uint8_t>& cb) : Fixnum() {
        if(cb.is_zero()) {
            return;
        }
        
        const bool overflows = (bytes * 2) < cb.converted.size();

        int source_index, shift_by;
        for(source_index = cb.converted.size() - 1, shift_by = 0;
            source_index >= 0 && shift_by <= 60;
            --source_index, shift_by += 4) {

            _data |= (static_cast<uint64_t>(cb.converted[source_index]) << shift_by);
        }

        if(!overflows && !is_negative() && cb.is_negative()) {
            _data = -_data;
        }
    }

    Fixnum(const int8_t val) : Fixnum() {
        _data = val;
    }
    
    Fixnum(const int16_t val) : Fixnum() {
        _data = val;
    }
    
    Fixnum(const int32_t val) : Fixnum() {
        _data = val;
    }

    Fixnum(const int64_t val) : Fixnum() {
        _data = val;
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

    Fixnum& operator/=(const Fixnum& n) {
        _data /= n._data;
        return *this;
    }

    Fixnum& operator%=(const Fixnum& n) {
        _data %= n._data;
        return *this;
    }

    Fixnum& operator<<=(const int by) {
        _data <<= by;
        return *this;
    }
    
    Fixnum& operator>>=(const int by) {
        _data >>= by;
        return *this;
    }

    Fixnum& operator&=(const Fixnum& n) {
        _data &= n._data;
        return *this;
    }

    Fixnum& operator|=(const Fixnum& n) {
        _data |= n._data;
        return *this;
    }

    Fixnum& operator^=(const Fixnum& n) {
        _data ^= n._data;
        return (*this);
    }

    bool operator[](const int index) const {
        return bit(index);
    }

    int fsb() const {
        int b = decode::first_set_bit(0xFF & (_data >> 56));
        if(b > -1) return b + 56;

        b = decode::first_set_bit(0xFF & (_data >> 48));
        if(b > -1) return b + 48;

        b = decode::first_set_bit(0xFF & (_data >> 40));
        if(b > -1) return b + 40;

        b = decode::first_set_bit(0xFF & (_data >> 32));
        if(b > -1) return b + 32;

        b = decode::first_set_bit(0xFF & (_data >> 24));
        if(b > -1) return b + 24;
        
        b = decode::first_set_bit(0xFF & (_data >> 16));
        if(b > -1) return b + 16;

        b = decode::first_set_bit(0xFF & (_data >> 8));
        if(b > -1) return b + 8;

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
        return _data == std::numeric_limits<int64_t>::lowest(); 
    }

    bool is_max() const {
        return _data == std::numeric_limits<int64_t>::max();
    }
    
    std::string str() const {
        return str(10);
    }

    std::string str(const int base) const {
        if(is_negative() && !is_lowest()) {
            return std::string("-") + complement().str(base);
        }
        else {
            uint8_t pos_hex[hex_bytes] = { static_cast<uint8_t>(0xF & _data),
                                           static_cast<uint8_t>(0xF & (_data >> 4)),
                                           static_cast<uint8_t>(0xF & (_data >> 8)),
                                           static_cast<uint8_t>(0xF & (_data >> 12)),
                                           static_cast<uint8_t>(0xF & (_data >> 16)),
                                           static_cast<uint8_t>(0xF & (_data >> 20)),
                                           static_cast<uint8_t>(0xF & (_data >> 24)),
                                           static_cast<uint8_t>(0xF & (_data >> 28)),
                                           static_cast<uint8_t>(0xF & (_data >> 32)),
                                           static_cast<uint8_t>(0xF & (_data >> 36)),
                                           static_cast<uint8_t>(0xF & (_data >> 40)),
                                           static_cast<uint8_t>(0xF & (_data >> 44)),
                                           static_cast<uint8_t>(0xF & (_data >> 48)),
                                           static_cast<uint8_t>(0xF & (_data >> 52)),
                                           static_cast<uint8_t>(0xF & (_data >> 56)),
                                           static_cast<uint8_t>(0xF & (_data >> 60)) };
                                           
            std::string ret = decode::convert_pos_str<hex_bytes>(pos_hex, base);
            return !is_lowest() ? ret : std::string("-") + ret;
        }
    }

    Fixnum complement() const {
        Fixnum ret { *this };
        ret._data = -ret._data;
        return ret;
    }

    const uint8_t byte(const int index) const {
        switch(index) {
        case 0: return 0xFF & _data;
        case 1: return 0xFF & (_data >> 8);
        case 2: return 0xFF & (_data >> 16);
        case 3: return 0xFF & (_data >> 24);
        case 4: return 0xFF & (_data >> 32);
        case 5: return 0xFF & (_data >> 40);
        case 6: return 0xFF & (_data >> 48);
        case 7: return 0xFF & (_data >> 56);
        default: throw std::overflow_error("can't access bits beyond size of fixnum");
        }
    }

    void byte(const int index, const uint8_t b) {
        switch(index) {
        case 0: _data = (_data & ~0xFFL) | static_cast<int64_t>(b); break;
        case 1: _data = (_data & ~(0xFFL << 8)) | (static_cast<int64_t>(b) << 8); break;
        case 2: _data = (_data & ~(0xFFL << 16)) | (static_cast<int64_t>(b) << 16); break;
        case 3: _data = (_data & ~(0xFFL << 24)) | (static_cast<int64_t>(b) << 24); break;
        case 4: _data = (_data & ~(0xFFL << 32)) | (static_cast<int64_t>(b) << 32); break;
        case 5: _data = (_data & ~(0xFFL << 40)) | (static_cast<int64_t>(b) << 40); break;
        case 6: _data = (_data & ~(0xFFL << 48)) | (static_cast<int64_t>(b) << 48); break;
        case 7: _data = (_data & ~(0xFFL << 56)) | (static_cast<int64_t>(b) << 56); break;
        default: throw std::overflow_error("can't access bits beyond size of fixnum");
        }
    }

    bool bit(const int index) const {
        if(index >= N || index < 0) {
            throw std::overflow_error("can't access bits beyond size of fixnum");
        }
        
        return (_data & (1L << index)) != 0;
    }

    Fixnum& bit(const int index, const bool val) {
        if(index >= N || index < 0) {
            throw std::overflow_error("can't access bits beyond size of fixnum");
        }

        if(val) _data |= (1L << index);
        else _data &= ~(1L << index);
        return *this;
    }

    void sign_extend(const int bit) {
        const int extension = (_data & (1 << bit)) > 0 ? 1 : 0;

        if(extension) {
            _data |= (0xFFFFFFFFFFFFFFFFL << (bit+1));
        }
        else {
            _data &= (0xFFFFFFFFFFFFFFFFL >> (8-(bit+1)));
        }
    }
    
private:
    int64_t _data;
};

template<size_t T, size_t S>
Fixnum<T> fixnum_cast(const Fixnum<S>& source) {
    Fixnum<T> ret { 0 };
    
    for(int i = 0; i < Fixnum<T>::bytes && i < Fixnum<S>::bytes; ++i) {
        ret.byte(i, source.byte(i));
    }
    
    if(T > S) {
        ret.sign_extend(S-1);
    }
    
    return ret;
}

//+ operators
template<size_t N>
Fixnum<N> operator+(const Fixnum<N>& one, const Fixnum<N>& two) {
    Fixnum<N> ret { one };
    ret += two;
    return ret;
}

template<size_t N, typename T>
Fixnum<N> operator+(const Fixnum<N>& one, const T val) {
    static_assert(std::is_integral<T>::value, "T must be integral type");
    
    Fixnum<N> ret { one };
    ret += val;
    return ret;
}

template<size_t N, typename T>
Fixnum<N> operator+(const T val, const Fixnum<N>& one) {
    static_assert(std::is_integral<T>::value, "T must be integral type");
    
    return operator+(one, val);
}

template<size_t N, typename T>
Fixnum<N>& operator+=(Fixnum<N>& one, const T val) {
    static_assert(std::is_integral<T>::value, "T must be integral type");
    
    return one += Fixnum<N>(val);
}

//- operators
template<size_t N>
Fixnum<N> operator-(const Fixnum<N>& one, const Fixnum<N>& two) {
    Fixnum<N> ret { one };
    ret -= two;
    return ret;
}

template<size_t N, typename T>
Fixnum<N>& operator-=(Fixnum<N>& one, const T val) {
    static_assert(std::is_integral<T>::value, "T must be integral type");
    
    return one -= Fixnum<N>(val);
}

template<size_t N, typename T>
Fixnum<N> operator-(const Fixnum<N>& one, const T val) {
    static_assert(std::is_integral<T>::value, "T must be integral type");
    
    Fixnum<N> ret { one };
    ret -= val;
    return ret;
}

template<size_t N, typename T>
Fixnum<N> operator-(const T val, const Fixnum<N>& one) {
    static_assert(std::is_integral<T>::value, "T must be integral type");
    
    Fixnum<N> ret(val);
    ret -= one;
    return ret;
}

//* operators
template<size_t N>
Fixnum<N> operator*(const Fixnum<N>& one, const Fixnum<N>& two) {
    Fixnum<N> ret { one };
    ret *= two;
    return ret;
}

template<size_t N, typename T>
Fixnum<N>& operator*=(Fixnum<N>& one, const T val) {
    static_assert(std::is_integral<T>::value, "T must be integral type");
    
    return one *= Fixnum<N>(val);
}

template<size_t N, typename T>
Fixnum<N> operator*(const Fixnum<N>& one, const T val) {
    static_assert(std::is_integral<T>::value, "T must be integral type");
    
    Fixnum<N> ret { one };
    ret *= val;
    return ret;
}

template<size_t N, typename T>
Fixnum<N> operator*(const T val, const Fixnum<N>& one) {
    static_assert(std::is_integral<T>::value, "T must be integral type");
    
    return operator*(one, val);
}

/// operators
template<size_t N>
Fixnum<N> operator/(const Fixnum<N>& one, const Fixnum<N>& two) {
    Fixnum<N> ret { one };
    ret /= two;
    return ret;
}

template<size_t N, typename T>
Fixnum<N>& operator/=(Fixnum<N>& one, const T val) {
    static_assert(std::is_integral<T>::value, "T must be integral type");
    
    return one /= Fixnum<N>(val);
}

template<size_t N, typename T>
Fixnum<N> operator/(const Fixnum<N>& one, const T val) {
    static_assert(std::is_integral<T>::value, "T must be integral type");
    
    Fixnum<N> ret { one };
    ret /= val;
    return ret;
}

template<size_t N, typename T>
Fixnum<N> operator/(const T val, const Fixnum<N>& one) {
    static_assert(std::is_integral<T>::value, "T must be integral type");
    
    Fixnum<N> ret(val);
    ret /= one;
    return ret;
}

//% operators
template<size_t N>
Fixnum<N> operator%(const Fixnum<N>& one, const Fixnum<N>& two) {
    Fixnum<N> ret { one };
    ret %= two;
    return ret;
}

template<size_t N, typename T>
Fixnum<N>& operator%=(Fixnum<N>& one, const T val) {
    static_assert(std::is_integral<T>::value, "T must be integral type");
    
    return one %= Fixnum<N>(val);
}

template<size_t N, typename T>
Fixnum<N> operator%(const Fixnum<N>& one, const T val) {
    static_assert(std::is_integral<T>::value, "T must be integral type");
    
    Fixnum<N> ret { one };
    ret %= val;
    return ret;
}

template<size_t N, typename T>
Fixnum<N> operator%(const T val, const Fixnum<N>& one) {
    static_assert(std::is_integral<T>::value, "T must be integral type");
    
    Fixnum<N> ret(val);
    ret %= one;
    return ret;
}

//bitwise operators
template<size_t N>
Fixnum<N> operator<<(const Fixnum<N>& one, const int by) {
    Fixnum<N> ret { one };
    ret <<= by;
    return ret;
}

template<size_t N>
Fixnum<N> operator>>(const Fixnum<N>& one, const int by) {
    Fixnum<N> ret { one };
    ret >>= by;
    return ret;
}

template<size_t N>
Fixnum<N> operator&(const Fixnum<N>& one, const Fixnum<N>& two) {
    Fixnum<N> ret { one };
    ret &= two;
    return ret;
}

template<size_t N, typename T>
Fixnum<N>& operator&=(Fixnum<N>& one, const T val) {
    static_assert(std::is_integral<T>::value, "T must be integral type");
    
    return one &= Fixnum<N>(val);
}

template<size_t N>
Fixnum<N> operator|(const Fixnum<N>& one, const Fixnum<N>& two) {
    Fixnum<N> ret { one };
    ret |= two;
    return ret;
}

template<size_t N, typename T>
Fixnum<N>& operator|=(Fixnum<N>& one, const T val) {
    static_assert(std::is_integral<T>::value, "T must be integral type");
    
    return one |= Fixnum<N>(val);
}

template<size_t N>
Fixnum<N> operator^(const Fixnum<N>& one, const Fixnum<N>& two) {
    Fixnum<N> ret { one };
    ret ^= two;
    return ret;
}

template<size_t N, typename T>
Fixnum<N>& operator^=(Fixnum<N>& one, const T val) {
    static_assert(std::is_integral<T>::value, "T must be integral type");
    
    return one ^= Fixnum<N>(val);
}

#endif
