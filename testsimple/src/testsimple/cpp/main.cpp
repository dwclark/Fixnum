#include "Decode.hpp"
#include "Fixnum.hpp"
#include <iostream>
#include <cassert>
#include <algorithm>
#include <limits>

#define NDEBUG 1

using bit8 = Fixnum<8>;
using bit16 = Fixnum<16>;
using bit32 = Fixnum<32>;
using bit64 = Fixnum<64>;

void test_to_division_vector() {
    using namespace decode;

    std::vector<uint8_t> dec = to_division_vector<uint8_t>("12345", 10);
    assert(dec.size() == 5);
    assert(dec[0] == 1 && dec[1] == 2 && dec[2] == 3 && dec[3] == 4 && dec[4] == 5);

    std::vector<uint8_t> hex = to_division_vector<uint8_t>("ABCDEF", 16);
    assert(hex.size() == 6);
    assert(hex[0] == 10 && hex[1] == 11 && hex[2] == 12 && hex[3] == 13 && hex[4] == 14 && hex[5] == 15);
}

void test_convert_base() {
    using namespace decode;

    std::vector<uint8_t> c1 = convert_base("F", 16, 2);
    assert(c1.size() == 4);
    std::for_each(c1.begin(), c1.end(), [](auto num) { assert(num == 1); });

    std::vector<uint8_t> c2 = convert_base("FF", 16, 2);
    assert(c2.size() == 8);
    std::for_each(c2.begin(), c2.end(), [](auto num) { assert(num == 1); });

    std::vector<uint8_t> c3 = convert_base("765", 8, 2);
    assert(c3.size() == 9);
    assert(c3[0] == 1 && c3[1] == 1 && c3[2] == 1 && c3[3] == 1 && c3[4] == 1 &&
           c3[5] == 0 && c3[6] == 1 && c3[7] == 0 && c3[8] == 1);

    std::vector<uint8_t> c4 = convert_base(std::string("765"), 8, 2);
    assert(c4.size() == 9);
    assert(c4[0] == 1 && c4[1] == 1 && c4[2] == 1 && c4[3] == 1 && c4[4] == 1 &&
           c4[5] == 0 && c4[6] == 1 && c4[7] == 0 && c4[8] == 1);

}

void test_convert_to_digits() {
    using namespace decode;
    
    uint8_t data[8];
    convert_to_digits("12345678", data, 8);
    for(int i = 0; i < 8; ++i) {
        assert(data[i] == (i+1));
    }
}

void test_bytes() {
    assert(Fixnum<8>::bytes == 1);
    assert(Fixnum<8>::hex_bytes == 2);
    
    assert(Fixnum<15>::bytes == 2);
    assert(Fixnum<15>::hex_bytes == 4);
    assert(Fixnum<15>::top_index == 1);
    assert(Fixnum<15>::top_mask == 0x7F);
    assert(Fixnum<15>::sign_mask == 0x40);

    assert(Fixnum<16>::bytes == 2);
    assert(Fixnum<16>::hex_bytes == 4);

    assert(Fixnum<32>::bytes == 4);
    assert(Fixnum<32>::hex_bytes == 8);

    assert(Fixnum<64>::bytes == 8);
    assert(Fixnum<64>::hex_bytes == 16);
    
    assert(Fixnum<17>::bytes == 3);
    assert(Fixnum<17>::hex_bytes == 6);
    assert(Fixnum<17>::top_index == 2);
    assert(Fixnum<17>::top_mask == 0x01);
    assert(Fixnum<17>::sign_mask == 0x01);
}

void test_int_basics() {
    using namespace decode;

    assert(bit8(100).str() == "100");
    assert(bit8(127).str() == "127");
    assert(bit8(128).str() == "-128");
    assert(bit8(-128).str() == "-128");
    
    bit32 from_short(static_cast<int16_t>(400));
    assert(from_short == bit32(400));
    
    bit32 one(200);
    assert(!one.is_negative());
    assert(one.str() == "200");
    
    bit32 two(-200);
    assert(two.is_negative());
    assert(one != two);
    assert(two.str() == "-200");
    
    bit32 eq_one(200);
    assert(one == eq_one);
    assert(eq_one.str() == "200");
    
    Fixnum<64> from_long(static_cast<int64_t>(700000L));
    assert(from_long.str() == "700000");
}

void test_is_lowest_max() {
    Fixnum<3> small3(-4);
    assert(small3.is_lowest());
    assert(small3 == Fixnum<3>::lowest());

    Fixnum<3> large3(3);
    assert(large3.is_max());
    assert(large3 == Fixnum<3>::max());

    Fixnum<3> mid3(-1);
    assert(!mid3.is_lowest());
    assert(!mid3.is_max());

    bit8 small8(-128);
    assert(small8.is_lowest());
    assert(small8 == bit8::lowest());

    bit8 large8(127);
    assert(large8.is_max());
    assert(large8 == bit8::max());

    bit8 mid8(25);
    assert(!mid8.is_lowest());
    assert(!mid8.is_max());

    bit32 small32(std::numeric_limits<int32_t>::lowest());
    assert(small32.is_lowest());
    assert(small32 == bit32::lowest());

    bit32 large32(std::numeric_limits<int32_t>::max());
    assert(large32.is_max());
    assert(large32 == bit32::max());

    bit32 mid32(-1234567);
    assert(!mid32.is_lowest());
    assert(!mid32.is_max());

    Fixnum<64> small64(std::numeric_limits<int64_t>::lowest());
    assert(small64.is_lowest());
    assert(small64 == Fixnum<64>::lowest());

    Fixnum<64> large64(std::numeric_limits<int64_t>::max());
    assert(large64.is_max());
    assert(large64 == Fixnum<64>::max());
}

void test_complement() {
    Fixnum<3> val(3);
    assert(val.str() == "3");
    assert(val.complement().str() == "-3");

    Fixnum<3> val_min(-4);
    assert(val_min.str() == "-4");
    assert(val_min.complement().str() == "-4");

    assert(bit8(18).complement().str() == "-18");
    assert(bit16(18).complement().str() == "-18");
    assert(bit32(18).complement().str() == "-18");
    assert(Fixnum<64>(18).complement().str() == "-18");
}

void test_cast() {
    Fixnum<3> val3(3);
    bit32 val32 = fixnum_cast<32>(val3);
    Fixnum<64> val64 = fixnum_cast<64>(val3);
    assert(val3.str() == val32.str());
    assert(val3.str() == val64.str());

    Fixnum<3> neg_val3(-3);
    bit32 neg_val32 = fixnum_cast<32>(neg_val3);
    Fixnum<64> neg_val64 = fixnum_cast<64>(neg_val3);
    assert(neg_val3.str() == neg_val32.str());
    assert(neg_val3.str() == neg_val64.str());

    Fixnum<3> min_val3(-4);
    bit32 min_val32 = fixnum_cast<32>(min_val3);
    Fixnum<64> min_val64 = fixnum_cast<64>(min_val3);
    assert(min_val32.str() == "-4");
    assert(min_val64.str() == "-4");

    bit32 neg4(-4);
    bit32 neg4_64(-4);
    assert(neg4 == min_val32);
    assert(neg4_64 == min_val32);

    bit8 single1 = fixnum_cast<8>(bit16(120));
    assert(single1.str() == "120");

    assert(fixnum_cast<8>(bit16(128)).str() == "-128");
}

void test_sign_extend() {
    using namespace decode;
    
    uint8_t one = 0b00000001;
    sign_extend(&one, 1, 1);
    assert(one == 0xFF);

    uint8_t two = 0b00001101;
    sign_extend(&two, 1, 4);
    assert(two == 0b11111101);

    uint8_t three = 0b11110111;
    sign_extend(&three, 1, 4);
    assert(three == 0b00000111);

    uint8_t top1 = 0b10000000;
    sign_extend(&top1, 1, 8);
    assert(top1 == 0b10000000);

    uint8_t four = 0b00111111;
    sign_extend(&four, 1, 7);
    assert(four == 0b00111111);

    uint8_t ary1[2] = { 0b11101111, 0xFF };
    sign_extend(ary1, 2, 5);
    assert(ary1[0] == 0b00001111);
    assert(ary1[1] == 0);

    uint8_t ary2[2] = { 0b11101101, 0 };
    sign_extend(ary2, 2, 3);
    assert(ary2[0] == 0b11111101);
    assert(ary2[1] == 0xFF);
}

void test_add() {
    Fixnum<17> one(5);
    one += Fixnum<17>(5);
    assert(one == Fixnum<17>(10));

    Fixnum<19> two(-217);
    two += Fixnum<19>(75);
    assert(two == Fixnum<19>(-142));

    assert((Fixnum<23>(450) + Fixnum<23>(900)) == Fixnum<23>(1350));
    assert(bit8(10) + bit8(11) == bit8(21));
    assert(bit16(0x800) + bit16(0x800) == bit16(0x1000));
    assert(bit32(0x400000) + bit32(0x400000) == bit32(0x800000));
    assert(bit64(0x400000C00000L) + bit64(0x400000C00000L) == bit64(0x800001800000L));
}

void test_subtract() {
    Fixnum<31> one(100);
    one -= Fixnum<31>(1500);
    Fixnum<31> two(-1400);
    assert(one == two);

    assert((Fixnum<31>(100000000) - Fixnum<31>(99000000)) == Fixnum<31>(1000000));
    assert(bit8(100) - bit8(200) == bit8(-100));
    assert(bit16(0x1000) - bit16(0x800) == bit16(0x800));
    assert(bit32(0x800000) - bit32(0x400000) == bit32(0x400000));
    assert(bit64(0x800001800000L) - bit64(0x400000C00000L) == bit64(0x400000C00000L));
}

void test_shifting() {
    bit8 one(2);
    one <<= 1;
    assert(one.str() == "4");
    one >>= 1;
    assert(one.str() == "2");
    assert(bit8((0x7F) >> 3) == bit8(0xF));
    assert(bit8(0xF << 3) == bit8(0x78));

    assert((bit16(0x9) << 9) == bit16(0x1200));
    assert((bit16(0x1200) >> 7) == bit16(0x24));

    bit32 two(0x606060);
    two <<= 1;
    assert(two.str(16) == "C0C0C0");
    two <<= 1;
    assert(two.str(16) == "1818180");
    two >>= 1;
    two >>= 1;
    assert(two.str(16) == "606060");

    assert((bit64(0x204000L) << 31) == bit64(0x10200000000000L));
    assert((bit64(0x10200000000000L) >> 13) == bit64(0x8100000000));
    
    Fixnum<17> three(0xC000);
    assert((three << 1).str()[0] == '-');
    assert((three << 3).str() == "0");
    assert((three >> 3).str(16) == "1800");
}

void test_multiplication() {
    bit16 source(4);
    bit16 eight(8);
    source *= eight;
    assert(source.str() == "32");

    Fixnum<18> source2(25);
    Fixnum<18> multiplier2(25);
    source2 *= multiplier2;
    assert(source2.str() == "625");

    Fixnum<18> multiplier3(-7);
    source2 *= multiplier3;
    assert(source2.str() == "-4375");

    source2 *= Fixnum<18>(0);
    assert(source2.str() == "0");

    bit32 overflow(1000000);
    overflow *= bit32(1000000);
    assert(overflow.str() == "-727379968");

    assert((bit32(1000000) * bit32(1000000)).str() == "-727379968");
    assert(bit8(10) * bit8(10) == bit8(100));
    assert(bit64(0x8100000000L) * bit64(0x255L) == bit64(0x12CD500000000L));
}

void test_increment_decrement() {
    bit8 s1(15);
    --s1;
    assert(s1.str() == "14");
    ++s1;
    assert(s1.str() == "15");

    assert((++Fixnum<14>(0)).str() == "1");
    assert((Fixnum<14>(0)++).str() == "0");
    assert((--Fixnum<14>(0)).str() == "-1");
    assert((Fixnum<14>(0)--).str() == "0");

    assert((++bit16(0)).str() == "1");
    assert((bit16(0)++).str() == "0");
    assert((--bit16(0)).str() == "-1");
    assert((bit16(0)--).str() == "0");

    assert((++Fixnum<30>(0x4000000)).str(16) == "4000001");
    assert((Fixnum<30>(0x4000000)++).str(16) == "4000000");
    assert((Fixnum<30>(0x4000000)--).str(16) == "4000000");
    assert((--Fixnum<30>(0x4000000)).str(16) == "3FFFFFF");
    
    assert((++bit32(0x40000000)).str(16) == "40000001");
    assert((bit32(0x40000000)++).str(16) == "40000000");
    assert((bit32(0x40000000)--).str(16) == "40000000");
    assert((--bit32(0x40000000)).str(16) == "3FFFFFFF");

    assert((++bit64(0x12CD500000000L)).str(16) == "12CD500000001");
    assert((bit64(0x12CD500000000L)++).str(16) == "12CD500000000");
    assert((--bit64(0x12CD500000000L)).str(16) == "12CD4FFFFFFFF");
    assert((bit64(0x12CD500000000L)--).str(16) == "12CD500000000");

    assert((++Fixnum<100>(0x12CD500000000L)).str(16) == "12CD500000001");
    assert((Fixnum<100>(0x12CD500000000L)++).str(16) == "12CD500000000");
    assert((--Fixnum<100>(0x12CD500000000L)).str(16) == "12CD4FFFFFFFF");
    assert((Fixnum<100>(0x12CD500000000L)--).str(16) == "12CD500000000");

    assert((--bit8::lowest()).is_positive());
    assert((++bit8::max()).is_negative());

    assert((--bit16::lowest()).is_positive());
    assert((++bit16::max()).is_negative());

    assert((--bit32::lowest()).is_positive());
    assert((++bit32::max()).is_negative());
    
    assert((--bit64::lowest()).is_positive());
    assert((++bit64::max()).is_negative());

    assert((--Fixnum<77>::lowest()).is_positive());
    assert((++Fixnum<77>::max()).is_negative());
}

void test_bit_operator() {
    bit8 one(8);
    assert(one[3]);
    assert(!one[4]);

    bit16 two(0x8000);
    assert(two[15]);
    assert(!two[14]);

    assert(bit32::lowest()[31]);
    assert(!bit32::lowest()[30]);

    assert(bit64::lowest()[63]);
    assert(!bit64::lowest()[62]);

    assert(Fixnum<256>().bit(100, true).bit(100));
    assert(Fixnum<256>::lowest()[255]);
    assert(!Fixnum<256>::lowest()[254]);
    
    Fixnum<24> tf(0x100000);
    assert(tf[20]);
    assert(!tf[21]);
}

void test_unary_pos_neg() {
    Fixnum<9> one(19);
    assert((-one) == Fixnum<9>(-19));

    Fixnum<9> two(-21);
    assert(-two == Fixnum<9>(21));

    assert(-bit8(10) == bit8(-10));
    assert(-bit8(-127) == bit8(127));

    assert(-bit16(10) == bit16(-10));
    assert(-bit16(-127) == bit16(127));

    assert(-bit32(10) == bit32(-10));
    assert(-bit32(-127) == bit32(127));

    assert(-bit64(10) == bit64(-10));
    assert(-bit64(-127) == bit64(127));

    assert(-Fixnum<256>(10) == Fixnum<256>(-10));
    assert(-Fixnum<256>(-127) == Fixnum<256>(127));
}

void test_same_representation() {
    Fixnum<31> one(100);
    one -= Fixnum<31>(1500);
    Fixnum<31> two(-1400);
    assert(one.str() == two.str());
    
    for(int i = 0; i < Fixnum<31>::bits; ++i) {
        assert(one[i] == two[i]);
    }

    for(int i = 0; i < Fixnum<31>::bytes; ++i) {
        assert(one.byte(i) == two.byte(i));
    }
}

void test_initializer_list() {
    using namespace decode;
    bit16 full { narrow_cast<uint8_t>(0xFF), narrow_cast<uint8_t>(0x7F) };
    assert(full.str() == "32767");

    bit16 min { narrow_cast<uint8_t>(0xFF), narrow_cast<uint8_t>(0xFF) };
    assert(min.str() == "-1");

    bit16 ten { narrow_cast<uint8_t>(10), narrow_cast<uint8_t>(0) };
    assert(ten.str() == "10");

    bit32 bigger = { narrow_cast<uint8_t>(0x52), narrow_cast<uint8_t>(0x52),
                     narrow_cast<uint8_t>(0x52), narrow_cast<uint8_t>(0x52) };
    assert(bigger.str(16) == "52525252");
    assert(bit8 { narrow_cast<uint8_t>(0xFF) }.str() == "-1");
}

void test_cmp() {
    using bit24 = Fixnum<24>;
        
    bit16 one(0x8123);
    bit16 one1(0x8123);
    bit16 two(0x8124);
    bit16 three(0x8122);
    bit16 four(0x123);

    assert(one < two);
    assert(one <= two);
    
    assert(one == one1);
    assert(one <= one1);
    assert(one >= one1);
    
    assert(one > three);
    assert(one < four);
    assert(four > one);

    assert(bit8(1) == bit8(1));
    assert(bit8(1) > bit8(0));
    assert(bit8(1) < bit8(2));

    assert(bit16(0x7FFF) > bit16(0x7FFE));
    assert(bit16(0x7FFE) < bit16(0x7FFF));
    assert(bit16(0x7FFF) == bit16(0x7FFF));
    
    assert(bit24(0x7FFFFF) > bit24(1900));
    assert(bit24(0xFFFFFF) < bit24(0));
    assert(bit24(1239) == bit24(1239));

    assert(bit32(0x7FFFFFFF) > bit32(0x7FFFFFFE));
    assert(bit32(0x7FFFFFFE) < bit32(0x7FFFFFFF));
    assert(bit32(0x7FFFFFFF) == bit32(0x7FFFFFFF));

    assert(bit64(0x7FFFFFFFFFFFFFFFL) > bit64(0x7FFFFFFFFFFFFFFEL));
    assert(bit64(0x7FFFFFFFFFFFFFFEL) < bit64(0x7FFFFFFFFFFFFFFFL));
    assert(bit64(0x7FFFFFFFFFFFFFFFL) == bit64(0x7FFFFFFFFFFFFFFFL));

    assert(Fixnum<128>("7FFFFFFFFFFFFFFFFFFF", 16) > Fixnum<128>("7FFFFFFFFFFFFFFFFFFE", 16));
    assert(Fixnum<128>("7FFFFFFFFFFFFFFFFFFE", 16) < Fixnum<128>("7FFFFFFFFFFFFFFFFFFF", 16));
    assert(Fixnum<128>("7FFFFFFFFFFFFFFFFFFF", 16) == Fixnum<128>("7FFFFFFFFFFFFFFFFFFF", 16));
}

void test_ands_ors() {
    Fixnum<13> one(0xFFF);
    Fixnum<13> toAnd(0x40);

    one &= toAnd;
    assert(one.str(16) == "40");
    assert((Fixnum<13>(0xFFF) & Fixnum<13>(0x40)).str(16) == "40");

    Fixnum<13> two(0xF);
    Fixnum<13> toOr(0xF0);
    two |= toOr;
    assert(two.str(16) == "FF");
    assert((Fixnum<13>(0xF) | Fixnum<13>(0xF0)).str(16) == "FF");

    Fixnum<13> three(0xAA);
    Fixnum<13> toXor(0x55);
    three ^= toXor;
    assert(three.str(16) == "FF");
    assert((Fixnum<13>(0xAA) ^ Fixnum<13>(0x55)).str(16) == "FF");

    assert((bit8(0xF) | bit8(0x70)) == bit8(127));
    assert((bit8(0xF) & bit8(0x70)) == bit8(0));

    assert((bit16(0xFF) | bit16(0x7F00)) == bit16(0x7FFF));
    assert((bit16(0xFF) & bit16(0x7F00)) == bit16(0));

    assert((bit32(0xFFFF) | bit32(0x7FFF0000)) == bit32(0x7FFFFFFF));
    assert((bit32(0xFFFF) & bit32(0x7FFF0000)) == bit32(0));

    assert((bit64(0xFFFFFFFFL) | bit64(0x7FFFFFFF00000000L)) == bit64(0x7FFFFFFFFFFFFFFFL));
    assert((bit64(0xFFFFFFFFL) & bit64(0x7FFFFFFF00000000L)) == bit64(0));

    assert((Fixnum<128>("FFFFFFFFFF", 16) | Fixnum<128>("7FFFFFFF0000000000", 16)) == Fixnum<128>("7FFFFFFFFFFFFFFFFF", 16));
    assert((Fixnum<128>("FFFFFFFFFF", 16) & Fixnum<128>("7FFFFFFF0000000000", 16)) == Fixnum<128>(0));
}

void test_division() {
    using bit256 = Fixnum<256>;
    
    try {
        Fixnum<25> one(100);
        one /= Fixnum<25>(0);
        assert(false);
    }
    catch(std::invalid_argument& a) {
        assert(true);
    }

    Fixnum<25> first_dividend(4);
    Fixnum<25> first_divisor(2);
    first_dividend /= first_divisor;
    assert(first_dividend.str() == "2");

    Fixnum<9> second_dividend(33);
    Fixnum<9> second_divisor(8);
    second_dividend /= second_divisor;
    assert(second_dividend.str(10) == "4");

    bit16 third_dividend(1024);
    bit16 third_divisor(64);
    third_dividend /= third_divisor;
    assert(third_dividend.str() == "16");

    bit16 fourth_dividend(8192);
    bit16 fourth_divisor(121);
    fourth_dividend /= fourth_divisor;
    assert(fourth_dividend.str() == "67");

    bit32 fifth_dividend(123456789);
    bit32 fifth_divisor(925);
    fifth_dividend /= fifth_divisor;
    assert(fifth_dividend.str() == "133466");

    bit32 sixth_dividend(123456789);
    bit32 sixth_divisor(-925);
    sixth_dividend /= sixth_divisor;
    assert(sixth_dividend.str() == "-133466");

    assert((bit32(123456789) / bit32(925)).str() == "133466");

    std::array<bit32, 2> res = bit32(123456789).div_and_mod(bit32(925));
    assert(res[0].str() == "133466");
    assert(res[1].str() == "739");

    res = bit32(16).div_and_mod(bit32(4));
    assert(res[0].str() == "4");
    assert(res[1].str() == "0");

    bit32 seventh_dividend(123456789);
    bit32 seventh_divisor(925);
    seventh_dividend %= seventh_divisor;
    assert(seventh_dividend.str() == "739");

    assert((bit32(123456789) % bit32(925)).str() == "739");
    assert(bit8(100) / bit8(10) == bit8(10));
    assert(bit16(10000) / bit16(100) == bit16(100));
    assert(bit32(100000000) / bit32(1000) == bit32(100000));
    assert(bit64(10000000000000000L) / bit64(1000000) == bit64(10000000000));
    assert(bit256("100000000000000000000", 10) / bit256("1000000000", 10) == bit256("100000000000", 10));
}

void test_bit_set() {
    assert(bit8(0xF).bit(6, true).bit(3, false).str(16) == "47");
    assert(bit16(0xFFF).bit(13, true).bit(9, false).str(16) == "2DFF");
    
    assert(bit8(1).fsb() == 0);
    assert(bit8(8).fsb() == 3);

    assert(bit16(0x1000).fsb() == 12);
    assert(bit16(0x2000).fsb() == 13);

    assert(Fixnum<30>(0x1000).fsb() == 12);
    assert(Fixnum<30>(0x2000).fsb() == 13);
    
    assert(bit32(0x1000).fsb() == 12);
    assert(bit32(0x2000).fsb() == 13);

    assert(bit8().bit(3, true).fsb() == 3);
    assert(bit16().bit(9, true).fsb() == 9);
    assert(bit32().bit(17, true).fsb() == 17);
    assert(bit64().bit(25, true).fsb() == 25);
    assert(Fixnum<128>().bit(17, true).bit(95, true).fsb() == 95);
}

void test_string_constructors() {
    bit8 one("F", 16);
    assert(one.str(16) == "F");

    bit8 two("-F", 16);
    assert(two.str(16) == "-F");

    bit16 one16("7FFF", 16);
    assert(one16.str(16) == "7FFF");

    bit16 two16("8000", 16);
    assert(two16.str(16) == "-8000");

    assert(bit32("7FFFFFFF", 16).str(16) == "7FFFFFFF");
    
    bit64 three("7FFFFFFFFFFFFFFF", 16);
    assert(three.str(16) == "7FFFFFFFFFFFFFFF");

    Fixnum<128> four("7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF", 16);
    assert(four.str(16) == "7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF");
    assert((four + Fixnum<128>(1)).str(16) == "-80000000000000000000000000000000");

    Fixnum<162> big("1234567890123456789012345678901234567890", 10);
    assert(big.str() == "1234567890123456789012345678901234567890");
    assert((big + big).str() == "2469135780246913578024691357802469135780");

    Fixnum<162> bigNeg("-1234567890123456789012345678901234567890", 10);
    assert(bigNeg.str() == "-1234567890123456789012345678901234567890");
    assert((bigNeg + bigNeg).str() == "-2469135780246913578024691357802469135780");
}

void test_large_numbers() {
    using bit512 = Fixnum<512>;
    std::string minus = bit512::max().str(16).substr(1);
    std::string minus_result = std::string("7") + std::string(127, '0');
    std::string fives(127, '5');
    std::string plus_result = std::string("7") + std::string(127, '5');
    std::string ones(127, '1');
    std::string nines(127, '9');

    assert((bit512::max() - bit512(minus,16)) == bit512(minus_result,16));
    assert((bit512(minus_result,16) + bit512(fives,16)) == bit512(plus_result,16));
    assert((bit512(ones,16) * bit512(9)) == bit512(nines,16));
    assert((bit512("9999999999999999999999999", 16) / bit512(9)) == bit512("1111111111111111111111111", 16));
    assert((bit512(nines,16) / bit512(9)) == bit512(ones, 16));
}

void test_int_adds() {
    bit16 one;
    one += 15;
    assert(one.str() == "15");
    one -= 7;
    assert(one.str() == "8");
    one *= 8;
    assert(one.str() == "64");
    one /= 32;
    assert(one.str() == "2");
    one %= 2;
    assert(one.str() == "0");

    bit32 two;
    two |= 0xFFFF;
    assert(two.str(16) == "FFFF");
    two &= 0xF00F;
    assert(two.str(16) == "F00F");
    two ^= 0xFF0;
    assert(two.str(16) == "FFFF");

    assert((100 + bit32(100) + 100).str() == "300");
    assert((300 - bit32(100) - 100).str() == "100");
    assert((9 * bit32(9) * 9).str() == "729");
    assert((729 / bit32(9) / 9).str() == "9");
    assert((25 % bit32(4)).str() == "1");
    assert((bit32(25) % 4).str() == "1");
}

int main(int argc, char* argv[]) {

    using namespace decode;
    using namespace std;

    test_to_division_vector();
    test_convert_base();
    test_convert_to_digits();
    
    test_bytes();

    test_int_basics();
    test_complement();
    test_is_lowest_max();
    test_cast();
    test_sign_extend();
    test_same_representation();
    test_initializer_list();
    
    test_add();
    test_unary_pos_neg();
    test_subtract();
    test_shifting();
    test_multiplication();
    test_increment_decrement();
    test_bit_operator();
    test_cmp();
    test_bit_set();
    test_division();
    
    test_ands_ors();
    test_string_constructors();

    test_large_numbers();
    test_int_adds();
}
