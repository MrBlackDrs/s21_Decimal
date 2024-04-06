#include "s21_decimal.h"

// узнать значение конкретного бита
int get_bit(s21_decimal num, int cur_bit) {
  int bit;
  if ((num.bits[cur_bit / 32] & (1 << cur_bit % 32)) == 0) {
    bit = 0;
  } else {
    bit = 1;
  }

  return bit;
}

// узнать значение конкретного бита для big decimal
int get_bit_big(big_decimal* num, int cur_bit) {
  int bit;
  if ((num->bits[cur_bit / 32] & (1 << cur_bit % 32)) == 0) {
    bit = 0;
  } else {
    bit = 1;
  }

  return bit;
}

// установить значение конкретного бита
void set_bit(s21_decimal* num, int bit, int result) {
  if (result == 1) {
    num->bits[bit / 32] = num->bits[bit / 32] | (1 << bit % 32);
  } else if (result == 0) {
    num->bits[bit / 32] = num->bits[bit / 32] & ~(1 << bit % 32);
  }
}

// установить значение конкретного бита для big_decimal
void set_bit_big(big_decimal* num, int bit, int result) {
  if (result == 1) {
    num->bits[bit / 32] = num->bits[bit / 32] | (1 << bit % 32);
  } else if (result == 0) {
    num->bits[bit / 32] = num->bits[bit / 32] & ~(1 << bit % 32);
  }
}

// устанавливает минус
void set_minus(s21_decimal* num, int sign) { set_bit(num, 127, sign); }

// // получить знак
int get_sign(s21_decimal num) { return get_bit(num, 127); }

// узнать значение scale коэфициента масштабирования
int get_scale(s21_decimal num) {
  int mask = 0b11111111;
  mask <<= 16;
  mask = num.bits[3] & mask;
  mask >>= 16;
  return mask;
}

// установить значение scale коэфициента масштабирования, 0 - успешно, 1 - еррор
int set_scale(s21_decimal* num, int scale) {
  int result = 0;
  if (scale < 256 && scale >= 0) {
    for (int i = 0; i < 8; i++) {
      set_bit(num, i + 16 + 96, scale % 2);
      scale >>= 1;
    }
  } else {
    result = 1;
  }
  return result;
}