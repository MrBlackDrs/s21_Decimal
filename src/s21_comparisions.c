#include "s21_decimal.h"

int comparator(s21_decimal *x1, s21_decimal *x2) {
  // 1 : x1 > x2
  // 0 : x1 == x2
  // -1 : x1 < x2
  // варианты знаков: 0 0, 1 1, 1 0 , 0 1
  int res = 0;
  int sign1 = get_sign(*x1), sign2 = get_sign(*x2);
  int both_negative = 0;
  // int both_positive = 0;

  // проверка чисел на разные знаки
  if (sign1 == 1 && sign2 == 0 &&
      (is_zero_s21_decimal(*x1) || is_zero_s21_decimal(*x2)))
    res = -1;
  else if (sign1 == 0 && sign2 == 1 &&
           (is_zero_s21_decimal(*x1) || is_zero_s21_decimal(*x2)))
    res = 1;
  else if (sign1 == 1 && sign2 == 1)
    both_negative = 1;
  // else
  //     both_positive = 1;

  if (!res) {
    // приведение к одной степени
    big_decimal big1 = {0}, big2 = {0};
    init_big(*x1, &big1);
    init_big(*x2, &big2);
    normalize_big(&big1, &big2);
    int difference_position = big1.one_position_left - big2.one_position_left;
    // пока сравним только модули
    if (difference_position > 0) {
      res = 1;
    } else if (difference_position < 0) {
      res = -1;
    } else {
      // проверка мантисс
      int difference_bit = 0;
      for (int i = big1.one_position_left; i >= 0; i--) {
        difference_bit = get_bit_big(&big1, i) - get_bit_big(&big2, i);
        if (difference_bit > 0) {
          res = 1;
          break;
        } else if (difference_bit < 0) {
          res = -1;
          break;
        }
      }
    }
    // вспоминаем про знак
    if (res) {
      if (both_negative) res = -res;
    }
  }

  return res;
}

int s21_is_less(s21_decimal x1, s21_decimal x2) {
  int res = comparator(&x2, &x1);
  // если 1, то x2 > x1 (x1 < x2)
  if (res == -1) res = 0;
  return res;
}

int s21_is_less_or_equal(s21_decimal x1, s21_decimal x2) {
  int res = comparator(&x2, &x1);
  if (res == 0)
    res = 1;
  else if (res == -1)
    res = 0;
  return res;
}

int s21_is_greater(s21_decimal x1, s21_decimal x2) {
  int res = comparator(&x1, &x2);
  if (res == -1) res = 0;
  return res;
}

int s21_is_greater_or_equal(s21_decimal x1, s21_decimal x2) {
  int res = comparator(&x1, &x2);
  if (res == 0)
    res = 1;
  else if (res == -1)
    res = 0;
  return res;
}

int s21_is_equal(s21_decimal x1, s21_decimal x2) {
  return (comparator(&x1, &x2)) ? 0 : 1;
}

int s21_is_not_equal(s21_decimal x1, s21_decimal x2) {
  return (comparator(&x1, &x2)) ? 1 : 0;
}