#include "s21_decimal.h"

// перевести из int в decimal (всегда успешно)
// int s21_from_int_to_decimal(int src, s21_decimal *dst){
//     *dst = init_decimal(src<0 ? ~(src) + 1 : src, 0, 0, src<0 ? 1:0, 0);
//     return 0;
// }

int s21_from_int_to_decimal(int src, s21_decimal *dst) {
  int status = S21_OK;

  if (dst) {
    zero_s21_decimal(dst);
    dst->bits[0] = abs(src);
    if (src < 0) {
      set_minus(dst, 1);
    }
  } else {
    status = S21_ERROR;
  }

  return status;
}

int s21_from_decimal_to_int(s21_decimal src, int *dst) {
  int res = S21_OK;
  if (dst) {
    *dst = 0;  // избавляеся от мусора
    //  - 0 - OK
    //  - 1 - ошибка конвертации
    int exp = get_scale(src);
    big_decimal full = {0};
    int special_case = 0;
    init_big(src, &full);
    if (exp) {
      // init_big(init_decimal(10,0,0,0,0), &ten);
      // full.exponenta = 0; // отбросить exp (у нас int)
      for (int i = 0; i < exp; i++) {
        division_with_rest_for10(full, &full);  // делим мантиссу на 10^exp
      }
    }
    zeroes_left_big(&full);
    // print_big_decimal(&full);
    if (full.one_position_left >
        30)  // так как 1 бит в int зарезервирован под знак
      res = S21_ERROR;  // decimal > int
    if (full.one_position_left == 31 &&
        full.negative) {  // отлавливаем случай отрицательного числа с мантиссой
                          // 2147483648
      int all_zeros = 1;
      for (int i = 30; i >= 0; i--) {
        if (get_bit_big(&full, i)) {
          all_zeros = 0;
          break;
        }
      }
      if (all_zeros) {
        res = S21_OK;
        special_case = 1;
        *dst = -2147483648;
      }
    }
    // перевод последнего bits в int
    int bit = 0;
    for (int i = 30; i >= 0; i--) {
      bit = get_bit_big(&full, i);
      *dst = *dst | bit << i;
    }
    if (full.negative) {
      if (!special_case) *dst = ~(*dst) + 1;
    }
    if (exp > 28) res = S21_ERROR;
  } else
    res = S21_ERROR;
  return res;
}

//Возвращаемое значение - код ошибки:
//  - 0 - OK
//  - 1 - ошибка конвертации

int s21_from_float_to_decimal(float src, s21_decimal *dst) {
  int status = S21_OK;

  if (dst) {
    zero_s21_decimal(dst);
  } else {
    status = S21_ERROR;
  }

  if (status || (0 < fabsf(src) && fabsf(src) < 1e-28) || isnan(src) ||
      isinf(src) || 79228162514264337593543950335.f < src) {
    status = S21_ERROR;
  } else {
    char part[128] = {0};
    int scale = 0;         // экспа
    int decimal_part = 0;  // дробная часть после точки
    // длина после точки
    int length = 0;

    // считываем дробную часть
    sprintf(part, "%.7g", src);
    // printf("!!!%s!!!", part);
    s21_get_float_part(part, &scale, &length, &decimal_part);
    // если scale отрицательный, то мы считываем число заново, только уже через
    // обозначением точности для флага g
    if (scale < 0 && 28 < abs(scale) + length) {
      memset(part, 0, 128);

      sprintf(part, "%.*g", 29 - abs(scale), src);
      s21_get_float_part(part, &scale, &length, &decimal_part);
    }

    // считываем целую часть
    int number = abs(atoi(part));
    // выставляем место для дробной части
    for (int i = 0; i < length; ++i) {
      number *= 10;
    }
    // прибавляем дробную часть
    number += decimal_part;

    // устанавливаем минус
    if (src < 0) {
      set_minus(dst, 1);
    }

    // устанавливаем экспоненту
    if (scale < 0) {  // если была отрицательная экспа, то к ней нужно добавить
                      // длину после точки
      set_scale(dst, abs(scale) + length);
    } else if (!scale) {
      set_scale(dst, length);
    }

    // побитово записываем в мантиссу
    for (int i = 0; number; ++i, number /= 2) {
      if (number % 2) {
        set_bit(dst, i, 1);
      }
    }

    // домножаем мантиссу на 10 до тех пор пока не выведем из дробной части
    if (0 < scale - length) {
      s21_decimal s21_mul_by_10 = {{10, 0, 0, 0}};
      for (int i = 0; !status && i < scale - length; ++i) {
        status = s21_mul(*dst, s21_mul_by_10, dst);
      }
    }
  }

  return status;
}

void s21_get_float_part(char *part, int *scale, int *length,
                        int *decimal_part) {
  // ищем позицию точки
  char *dot = strchr(part, '.');
  // ищем позицию e
  char *exponenta = strchr(part, 'e');

  *scale = 0;
  *decimal_part = 0;
  // длина полсе точки
  *length = 0;

  // записываем значени экспоненты
  if (exponenta) {
    *exponenta++ = '\0';
    *scale = atoi(exponenta);
  }

  // записываем значение после точки
  if (dot) {
    *dot++ = '\0';
    *length = strlen(dot);
    *decimal_part = atoi(dot);
  }
}

// перевод из децимал ту флоат
int s21_from_decimal_to_float(s21_decimal src, float *dst) {
  int status = S21_OK;
  unsigned int scale = get_scale(src);

  if (!dst || 28 < scale) {
    status = S21_ERROR;
  } else {
    *dst = 0.f;
    // получаем сумму мантиссы
    for (int i = 0; i < 96; ++i) {
      if (get_bit(src, i)) {
        *dst += powf(2, i);
      }
    }
    // делим с учетом scale
    *dst /= powf(10, scale);

    // устанавливаем знак
    if (get_sign(src)) {
      *dst *= -1;
    }
  }

  return status;
}
