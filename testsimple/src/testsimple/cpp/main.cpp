#include "Decode.hpp"
#include "Fixnum.hpp"
#include <iostream>
#include <cassert>
#include <algorithm>
#include <limits>

#define NDEBUG 1

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

void test_slots() {
    assert(Fixnum<8>::slots == 1);
    assert(Fixnum<8>::hex_slots == 2);
    assert(Fixnum<8>::top_index == 0);
    assert(Fixnum<8>::top_mask == 0xFF);
    assert(Fixnum<8>::sign_mask == 0x80);
    
    assert(Fixnum<15>::slots == 2);
    assert(Fixnum<15>::hex_slots == 4);
    assert(Fixnum<15>::top_index == 1);
    assert(Fixnum<15>::top_mask == 0x7F);
    assert(Fixnum<15>::sign_mask == 0x40);

    assert(Fixnum<16>::slots == 2);
    assert(Fixnum<16>::hex_slots == 4);
    assert(Fixnum<16>::top_index == 1);
    assert(Fixnum<8>::top_mask == 0xFF);
    assert(Fixnum<8>::sign_mask == 0x80);

    assert(Fixnum<17>::slots == 3);
    assert(Fixnum<17>::hex_slots == 6);
    assert(Fixnum<17>::top_index == 2);
    assert(Fixnum<17>::top_mask == 0x01);
    assert(Fixnum<17>::sign_mask == 0x01);
}

void test_convert_to_digits() {
    using namespace decode;
    
    uint8_t data[8];
    convert_to_digits("12345678", data, 8);
    for(int i = 0; i < 8; ++i) {
        assert(data[i] == (i+1));
    }
}

void test_main_constructor() {
    // Fixnum<32> f1{"111", 2};
    // Fixnum<4> f2{"F", 16};
    // Fixnum<5> f3{"10101", 2};
    // try {
    //     Fixnum<4> f4{"FF", 16};
    //     assert(false);
    // }
    // catch(std::overflow_error& e) {
    //     assert(true);
    // }

    // Fixnum<3> f5{"11", 2};
    // uint8_t f5u = 3;
    // std::cout << (int) f5.data()[0] << "\n";
    // assert(f5.data()[0] == f5u); 
}

void test_int_basics() {
    using namespace decode;
    
    Fixnum<32> from_short(static_cast<int16_t>(400));
    assert(from_short == Fixnum<32>(400));
    
    Fixnum<32> one(200);
    assert(!one.is_negative());
    assert(one.str() == "200");
    
    Fixnum<32> two(-200);
    assert(two.is_negative());
    assert(one != two);
    assert(two.str() == "-200");
    
    Fixnum<32> eq_one(200);
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

    Fixnum<8> small8(-128);
    assert(small8.is_lowest());
    assert(small8 == Fixnum<8>::lowest());

    Fixnum<8> large8(127);
    assert(large8.is_max());
    assert(large8 == Fixnum<8>::max());

    Fixnum<8> mid8(25);
    assert(!mid8.is_lowest());
    assert(!mid8.is_max());

    Fixnum<32> small32(std::numeric_limits<int32_t>::lowest());
    assert(small32.is_lowest());
    assert(small32 == Fixnum<32>::lowest());

    Fixnum<32> large32(std::numeric_limits<int32_t>::max());
    assert(large32.is_max());
    assert(large32 == Fixnum<32>::max());

    Fixnum<32> mid32(-1234567);
    assert(!mid32.is_lowest());
    assert(!mid32.is_max());
}

void test_complement() {
    Fixnum<3> val(3);
    assert(val.str() == "3");
    assert(val.complement().str() == "-3");

    Fixnum<3> val_min(-4);
    assert(val_min.str() == "-4");
    assert(val_min.complement().str() == "-4");
}

void test_cast() {
    Fixnum<3> val3(3);
    Fixnum<32> val32 = fixnum_cast<32>(val3);
    assert(val3.str() == val32.str());

    Fixnum<3> neg_val3(-3);
    Fixnum<32> neg_val32 = fixnum_cast<32>(neg_val3);
    assert(neg_val3.str() == neg_val32.str());

    Fixnum<3> min_val3(-4);
    Fixnum<32> min_val32 = fixnum_cast<32>(min_val3);
    assert(min_val32.str() == "-4");

    Fixnum<32> neg4(-4);
    assert(neg4 == min_val32);
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
}

void test_subtract() {
    Fixnum<31> one(100);
    one -= Fixnum<31>(1500);
    Fixnum<31> two(-1400);
    assert(one == two);

    assert((Fixnum<31>(100000000) - Fixnum<31>(99000000)) == Fixnum<31>(1000000));
}

void test_shifting() {
    Fixnum<8> one(2);
    one <<= 1;
    assert(one.str() == "4");
    one >>= 1;
    assert(one.str() == "2");

    Fixnum<32> two(0x606060);
    two <<= 1;
    assert(two.str(16) == "C0C0C0");
    two <<= 1;
    assert(two.str(16) == "1818180");
    two >>= 1;
    two >>= 1;
    assert(two.str(16) == "606060");

    Fixnum<17> three(0xC000);
    assert((three << 1).str()[0] == '-');
    assert((three << 3).str() == "0");
    assert((three >> 3).str(16) == "1800");
}

void test_multiplication() {
    Fixnum<16> source(4);
    Fixnum<16> eight(8);
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

    Fixnum<32> overflow(1000000);
    overflow *= Fixnum<32>(1000000);
    assert(overflow.str() == "-727379968");

    assert((Fixnum<32>(1000000) * Fixnum<32>(1000000)).str() == "-727379968");
}

void test_increment_decrement() {
    Fixnum<8> s1(15);
    --s1;
    assert(s1.str() == "14");
    ++s1;
    assert(s1.str() == "15");

    assert((++Fixnum<16>(0)).str() == "1");
    assert((Fixnum<16>(0)++).str() == "0");
    assert((--Fixnum<16>(0)).str() == "-1");
    assert((Fixnum<16>(0)--).str() == "0");
    assert((--Fixnum<32>(0x40000000)).str(16) == "3FFFFFFF");

    Fixnum<32> lowest(std::numeric_limits<int32_t>::lowest());
    assert((--lowest).is_positive());

    Fixnum<32> max(std::numeric_limits<int32_t>::max());
    assert((++max).is_negative());
}

void test_bit_operator() {
    Fixnum<8> one(8);
    assert(one[4] == 1);
    assert(one[5] == 0);

    Fixnum<24> tf(0x100000);
    assert(tf[21] == 1);
    assert(tf[22] == 0);
}

void test_unary_pos_neg() {
    Fixnum<9> one(19);
    assert((-one) == Fixnum<9>(-19));

    Fixnum<9> two(-21);
    assert(-two == Fixnum<9>(21));
}

void test_same_representation() {
    Fixnum<31> one(100);
    one -= Fixnum<31>(1500);
    Fixnum<31> two(-1400);
    assert(one.str() == two.str());
    
    for(int i = 0; i < Fixnum<31>::slots; ++i) {
        assert(one.data()[i] == two.data()[i]);
    }
}

void test_initializer_list() {
    using namespace decode;
    Fixnum<16> full { narrow_cast<uint8_t>(0xFF), narrow_cast<uint8_t>(0x7F) };
    assert(full.str() == "32767");

    Fixnum<16> min { narrow_cast<uint8_t>(0xFF), narrow_cast<uint8_t>(0xFF) };
    assert(min.str() == "-1");

    Fixnum<16> ten { narrow_cast<uint8_t>(10), narrow_cast<uint8_t>(0) };
    assert(ten.str() == "10");

    Fixnum<32> bigger = { narrow_cast<uint8_t>(0x52), narrow_cast<uint8_t>(0x52),
                          narrow_cast<uint8_t>(0x52), narrow_cast<uint8_t>(0x52) };
    assert(bigger.str(16) == "52525252");
}

void test_cmp() {
    Fixnum<16> one(0x8123);
    Fixnum<16> one1(0x8123);
    Fixnum<16> two(0x8124);
    Fixnum<16> three(0x8122);
    Fixnum<16> four(0x123);

    assert(one < two);
    assert(one <= two);
    
    assert(one == one1);
    assert(one <= one1);
    assert(one >= one1);
    
    assert(one > three);
    assert(one < four);
    assert(four > one);
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
}

void test_division() {
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

    Fixnum<16> third_dividend(1024);
    Fixnum<16> third_divisor(64);
    third_dividend /= third_divisor;
    assert(third_dividend.str() == "16");

    Fixnum<16> fourth_dividend(8192);
    Fixnum<16> fourth_divisor(121);
    fourth_dividend /= fourth_divisor;
    assert(fourth_dividend.str() == "67");

    Fixnum<32> fifth_dividend(123456789);
    Fixnum<32> fifth_divisor(925);
    fifth_dividend /= fifth_divisor;
    assert(fifth_dividend.str() == "133466");

    Fixnum<32> sixth_dividend(123456789);
    Fixnum<32> sixth_divisor(-925);
    sixth_dividend /= sixth_divisor;
    assert(sixth_dividend.str() == "-133466");

    assert((Fixnum<32>(123456789) / Fixnum<32>(925)).str() == "133466");

    std::array<Fixnum<32>, 2> res = Fixnum<32>(123456789).div_and_mod(Fixnum<32>(925));
    assert(res[0].str() == "133466");
    assert(res[1].str() == "739");

    res = Fixnum<32>(16).div_and_mod(Fixnum<32>(4));
    assert(res[0].str() == "4");
    assert(res[1].str() == "0");

    Fixnum<32> seventh_dividend(123456789);
    Fixnum<32> seventh_divisor(925);
    seventh_dividend %= seventh_divisor;
    assert(seventh_dividend.str() == "739");

    assert((Fixnum<32>(123456789) % Fixnum<32>(925)).str() == "739");
}

void test_bit_set() {
    assert(Fixnum<8>(1).fsb() == 1);
    assert(Fixnum<8>(8).fsb() == 4);
    assert(Fixnum<32>(0x1000).fsb() == 13);
    assert(Fixnum<32>(0x2000).fsb() == 14);
}

void test_string_constructors() {
    Fixnum<8> one("F", 16);
    assert(one.str(16) == "F");

    Fixnum<8> two("-F", 16);
    assert(two.str(16) == "-F");

    Fixnum<64> three("7FFFFFFFFFFFFFFF", 16);
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

int main(int argc, char* argv[]) {

    using namespace decode;
    using namespace std;

    test_to_division_vector();
    test_convert_base();
    test_convert_to_digits();
    
    test_slots();
    test_main_constructor();

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
}
