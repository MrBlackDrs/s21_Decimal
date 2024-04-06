#include "s21_decimal.h"

// Функции возвращают код ошибки:
// - 0 - OK
// - 1 - число слишком велико или равно бесконечности
// - 2 - число слишком мало или равно отрицательной бесконечности
// - 3 - деление на 0

// суммируем decimal и выводим в result
int s21_add(s21_decimal value_1, s21_decimal value_2, s21_decimal *result) {
  int status = 0;
  // проверяем указатель на result
  if (result) {
    int equal_signs = 0;
    // обнулили result decimal
    zero_s21_decimal(result);

    // переводим все в big_decimal
    big_decimal bvalue_1 = {0};
    init_big(value_1, &bvalue_1);
    big_decimal bvalue_2 = {0};
    init_big(value_2, &bvalue_2);
    big_decimal result_b = {0};
    // print_decimal(*result);

    // нормализуем все в сторону большей степени
    normalize_big(&bvalue_1, &bvalue_2);

    // равные ли у нас знаки
    if (get_sign(value_1) == get_sign(value_2)) equal_signs = 1;

    // если равные, то складываем мантисы
    if (equal_signs) {
      sum_mantissa(&bvalue_1, &bvalue_2, &result_b);
      result_b.negative = bvalue_1.negative;
    } else {  // если разные, то вычитаем
      int compare = compare_mantis_big(&bvalue_1, &bvalue_2);
      if (compare > 0) {
        sub_mantis_big(bvalue_1, bvalue_2, &result_b);
        result_b.negative = bvalue_1.negative;
      } else if (compare < 0) {
        sub_mantis_big(bvalue_2, bvalue_1, &result_b);
        result_b.negative = bvalue_2.negative;
      }
    }

    // дописываем экспоненту
    result_b.exponenta = bvalue_1.exponenta;
    // print_big_decimal(&result_b);
    // записываем из биг децимал в обычный s21
    status = big_to_s21decimal(result, &result_b);

  } else {
    status = 1;
  }
  return status;
}

// переводим из бига в s21
int big_to_s21decimal(s21_decimal *result, big_decimal *result_big) {
  int status = 0;
  // сколько нулей слева, если занулеванное, то one_position_left = -1
  zeroes_left_big(result_big);
  big_decimal ten = {0};
  ten.bits[0] = 10;
  int diff = 0;
  big_decimal copy_result_big_x =
      *result_big;  // x
                    // для очень маленьких чисел, если экспонента очень большая
  while (result_big->exponenta > 28) {
    //
    if (compare_mantis_big(result_big, &ten) >= 0) {
      division_with_rest_for10(*result_big, result_big);  // y
      diff++;
    } else {
      status = 2;
      zero_s21_decimal(result);
      break;
    }
  }
  if (!status) {
    if (diff > 0) bank_round(copy_result_big_x, result_big, diff);

    diff = 0;
    // если мантиса бига выходит за пределы обычной, то
    if (go_beyond_big_decimal_s21(result_big)) {
      if (result_big->exponenta < 1) {
        status = 1;
      } else {
        copy_result_big_x = *result_big;
        // делим резалт на 10 до тех пор пока
        while (go_beyond_big_decimal_s21(result_big) &&
               (result_big->exponenta > 0)) {
          division_with_rest_for10(*result_big, result_big);
          diff++;
        }
        if (diff > 0) bank_round(copy_result_big_x, result_big, diff);
      }
    }

    // если все равно выходит за пределы мантисы
    if (go_beyond_big_decimal_s21(result_big)) status = 1;

    if ((status == 1) && result_big->negative) status = 2;

    if (!status) {
      if (result_big->negative) set_minus(result, 1);
      // если нет переполнения, то запихиваем биг децимал в s21_dec
      big_to_s21decimal_95(result_big, result);
      set_scale(result, result_big->exponenta);
    }
  }
  // print_big_decimal(result_big);
  return status;
}

// банковское округление
void bank_round(big_decimal copy_result_big_x, big_decimal *result_big,
                int exp_diff) {
  big_decimal one = {0};
  one.bits[0] = 1;
  // x - y
  big_decimal ostatok_xy = {0};
  big_decimal half = {0};
  half.bits[0] = 5;
  half.exponenta = 1;

  if (exp_diff > 0) exp_diff--;
  big_decimal copy_result_big_y = *result_big;
  normalize_big(&copy_result_big_x, &copy_result_big_y);

  sub_mantis_big(copy_result_big_x, copy_result_big_y, &ostatok_xy);
  ostatok_xy.negative = 0;

  multiply_10_mantis_big(&half, exp_diff);

  // сравнение мантис big, 1 больше = 1, 2 больше = -1, по ровну = 0
  int compare_res = compare_mantis_big(&ostatok_xy, &half);
  if (compare_res == 1) {
    sum_mantissa(result_big, &one, result_big);
  } else if (compare_res == 0) {
    if (get_bit_big(result_big, 0) == 1)
      sum_mantissa(result_big, &one, result_big);
  }
}

// деление биг децимал c остатком на 10 для удобства
unsigned int division_with_rest_for10(big_decimal val1, big_decimal *res) {
  big_decimal val2 = {0};
  val2.bits[0] = 10;
  zeroes_left_big(&val2);
  int q = 0;
  big_decimal part = {0};  //вычитаемое из делителя при найденном q
  big_decimal part_next = {0};  // вычитаемое из делителя при найденном q+1
  big_decimal sum = {0};  // текущая сумма, которая должна знать ответом
  big_decimal before_sum = {0};  // новый член в сумме
  // normalize_big(&val1, &val2);
  while (is_zero_big_decimal(val1) &&
         is_greater_or_equal_big_decimal(
             val1, val2)) {  // остаток (val1) != нулю или exp суммы < 31
    q = 0;
    zero_big_decimal(&part);
    zero_big_decimal(&before_sum);
    part_next = val2;
    part = val2;
    zeroes_left_big(&val1);
    while (is_greater_or_equal_big_decimal(
        val1, part_next)) {  // пока делитель(val2)*2^q < остаток
      if (q == 0) {
        int difference_elder_bit =
            val1.one_position_left - val2.one_position_left;
        if (difference_elder_bit > 2) {
          q = difference_elder_bit - 1;
          shift_left_big(&part_next, q);
        }
        // printf("\ndif: %d", difference_elder_bit);
      }
      part = part_next;
      shift_left_big(&part_next, 1);  // Домнажение на 2 (формирование 2^q)
      q++;
    }
    q--;
    set_bit_big(&before_sum, q, 1);  // формирование нового членa в сумме
    sum_mantissa(&sum, &before_sum, &sum);  // добавление нового члена к сумме

    if (is_greater_or_equal_big_decimal(val1, part))
      sub_mantis_big(val1, part, &val1);  // остаток записываем в val1
  }

  // устанавливаем экспоненту
  sum.exponenta = val1.exponenta - 1;
  // устанавливаем знак
  sum.negative = val1.negative;
  *res = sum;
  return val1.bits[0];
}

// вычитание decimal
int s21_sub(s21_decimal value_1, s21_decimal value_2, s21_decimal *result) {
  int status = 0;
  // проверяем указатель на result
  if (result) {
    int equal_signs = 0;
    // обнулили result decimal
    zero_s21_decimal(result);

    change_znak_s21(&value_2);

    // переводим все в big_decimal
    big_decimal bvalue_1 = {0};
    init_big(value_1, &bvalue_1);
    big_decimal bvalue_2 = {0};
    init_big(value_2, &bvalue_2);
    big_decimal result_b = {0};
    // print_decimal(*result);

    // нормализуем все в сторону большей степени
    normalize_big(&bvalue_1, &bvalue_2);

    // равные ли у нас знаки
    if (get_sign(value_1) == get_sign(value_2)) equal_signs = 1;

    // если равные, то складываем мантисы
    if (equal_signs) {
      sum_mantissa(&bvalue_1, &bvalue_2, &result_b);
      result_b.negative = bvalue_1.negative;
    } else {  // если разные, то вычитаем
      int compare = compare_mantis_big(&bvalue_1, &bvalue_2);
      if (compare > 0) {
        sub_mantis_big(bvalue_1, bvalue_2, &result_b);
        result_b.negative = bvalue_1.negative;
      } else if (compare < 0) {
        sub_mantis_big(bvalue_2, bvalue_1, &result_b);
        result_b.negative = bvalue_2.negative;
      }
    }

    // дописываем экспоненту
    result_b.exponenta = bvalue_1.exponenta;
    // записываем из биг децимал в обычный s21
    // print_big_decimal(&result_b);
    status = big_to_s21decimal(result, &result_b);

  } else {
    status = 1;
  }
  return status;
}

// умножение decimal
int s21_mul(s21_decimal value_1, s21_decimal value_2, s21_decimal *result) {
  int status = 0;
  // проверяем указатель на result
  if (result) {
    int equal_signs = 0;

    // обнулили result decimal
    zero_s21_decimal(result);

    // переводим все в big_decimal
    big_decimal bvalue_1 = {0};
    init_big(value_1, &bvalue_1);
    big_decimal bvalue_2 = {0};
    init_big(value_2, &bvalue_2);
    big_decimal result_b = {0};

    // normalize_big(&bvalue_1, &bvalue_2);

    // равные ли у нас знаки
    if (get_sign(value_1) == get_sign(value_2)) equal_signs = 1;

    if (multiply_mantis_big(bvalue_1, &bvalue_2, &result_b)) status = 1;

    // дописываем знак
    if (equal_signs) {
      result_b.negative = 0;
    } else {
      result_b.negative = 1;
    }

    // при умножении на 0
    zeroes_left_big(&result_b);
    if (result_b.one_position_left == -1) {
      result_b.negative = 0;
      result_b.exponenta = 0;
    }

    // print_big_decimal(&result_b);

    if (!status) status = big_to_s21decimal(result, &result_b);

  } else {
    status = 1;
  }
  return status;
}

// проверка на выход биг децимал за пределы массива s21
int go_beyond_big_decimal_s21(big_decimal *big) {
  int result = 0;
  for (int i = 3; i < BITS_B; i++) {
    if (big->bits[i] != 0) {
      result = 1;
      break;
    }
  }

  return result;
}

// 0 - OK 1 - число слишком велико или равно бесконечности 2 - число
/// слишком мало или равно отрицательной бесконечности 3 - деление на 0
// деление
int s21_div(s21_decimal value_1, s21_decimal value_2, s21_decimal *result) {
  int status = 0;
  int zero1 = 0;
  if (!is_zero_s21_decimal(value_1)) zero1 = 1;

  // проверяем указатель
  if (result) {
    // зануляем результат
    zero_s21_decimal(result);

    // проверка на 0, делить на 0 нельзя
    if (is_zero_s21_decimal(value_2)) {
      // создаем биг бецимал
      big_decimal bvalue1 = {0}, bvalue2 = {0}, b_result = {0};
      // переносим из децимал в биг
      init_big(value_1, &bvalue1);
      init_big(value_2, &bvalue2);

      // деление с big_decimal
      division(bvalue1, bvalue2, &b_result);

      // устанавливаем знак в результат
      if (get_sign(value_1) != get_sign(value_2)) b_result.negative = 1;
      // print_big_decimal(&b_result);
      if (!status) status = big_to_s21decimal(result, &b_result);

    } else {
      status = 3;
    }
    if (status || zero1) zero_s21_decimal(result);
  } else {
    status = 1;
  }
  return status;
}

// зануляем s21_decimal
void zero_s21_decimal(s21_decimal *value) {
  value->bits[0] = value->bits[1] = value->bits[2] = value->bits[3] = 0;
}

// больше или равно биг дец 1 биг дец 2
int is_greater_or_equal_big_decimal(big_decimal value_1, big_decimal value_2) {
  int result = 1;
  for (int i = 7; i >= 0; i--) {
    if (value_1.bits[i] > value_2.bits[i]) {
      result = 1;
      break;
    } else if (value_1.bits[i] < value_2.bits[i]) {
      result = 0;
      break;
    }
  }
  return result;
}

// больше ли биг дец 1 биг дец 2
int is_greater_big_decimal(big_decimal value_1, big_decimal value_2) {
  int result = 0, out = 0;
  for (int i = 7; i >= 0 && !result && !out; i--) {
    if (value_1.bits[i] || value_2.bits[i]) {
      if (value_1.bits[i] > value_2.bits[i]) {
        result = 1;
      }
      if (value_1.bits[i] != value_2.bits[i]) out = 1;
    }
  }
  return result;
}

// если big decimal = 0, то возвращает 0
int is_zero_big_decimal(big_decimal big) {
  int result = 0;
  for (int i = 7; i >= 0; i--) {
    if (big.bits[i] != 0) {
      result = 1;
      break;
    }
  }
  return result;
}

// проверяет на ноль s21_decimal
int is_zero_s21_decimal(s21_decimal value) {
  int res = 0;
  for (int i = 2; i >= 0; i--) {
    if (value.bits[i] != 0) {
      res = 1;
      break;
    }
  }
  // res = value.bits[0] + value.bits[1] + value.bits[2];
  return res;
}

// вычитание в формате большого децимала
void sub_mantis_big(big_decimal value_1, big_decimal value_2,
                    big_decimal *result) {
  int tmp = 0, res = 0;
  for (int i = 0; i <= BITS_BIG; i++) {
    res = get_bit_big(&value_1, i) - get_bit_big(&value_2, i) - tmp;
    tmp = res < 0;
    res = abs(res);
    set_bit_big(result, i, res % 2);
  }
}

// для приведения к одной экспоненте, домножаем на 10 биг децимал
int multiply_10_mantis_big(big_decimal *bvalue, int def) {
  int status = 0;

  // создаем 10 для биг децимал
  s21_decimal ten_s = {0};
  s21_from_int_to_decimal(10, &ten_s);
  big_decimal ten = {0};
  init_big(ten_s, &ten);
  // big_decimal result = {0};

  for (int i = 1; i <= def; i++) {
    if (multiply_mantis_big(*bvalue, &ten, bvalue)) status = 1;
  }
  bvalue->exponenta += def;
  return status;
}

// сравнение мантис big, 1 больше = 1, 2 больше = -1, по ровну = 0
int compare_mantis_big(big_decimal *bvalue1, big_decimal *bvalue2) {
  int result = 0;
  for (int i = BITS_BIG; i >= 0; i--) {
    int rvalue1 = 0, rvalue2 = 0;
    rvalue1 = get_bit_big(bvalue1, i);
    rvalue2 = get_bit_big(bvalue2, i);
    if (rvalue1 != rvalue2) {
      result = rvalue1 - rvalue2;
      break;
    }
  }
  return result;
}

// переводим из биг ту с21 со старшим битом не больше 95
void big_to_s21decimal_95(big_decimal *result_big, s21_decimal *result) {
  for (int i = 0; i < 3; i++) {
    result->bits[i] = result_big->bits[i];
  }
}

// складываем мантисы big decimal
int sum_mantissa(big_decimal *bvalue_1, big_decimal *bvalue_2,
                 big_decimal *result) {
  int status = 0;
  int tmp = 0;
  int var = 0;

  for (int i = 0; i <= BITS_BIG; i++) {
    var = (get_bit_big(bvalue_1, i) + get_bit_big(bvalue_2, i) + tmp);
    if (var == 3) {
      tmp = 1;
      set_bit_big(result, i, 1);
    } else if (var == 2) {
      tmp = 1;
      set_bit_big(result, i, 0);
    } else if (var == 1) {
      set_bit_big(result, i, 1);
      tmp = 0;
    } else if (var == 0) {
      set_bit_big(result, i, 0);
      tmp = 0;
    }
  }
  if (tmp == 1) status = 1;
  return status;
}

// приводим big_decimal к одной экспоненте
void normalize_big(big_decimal *bvalue_1, big_decimal *bvalue_2) {
  int def = bvalue_1->exponenta - bvalue_2->exponenta;
  if (def > 0) {
    multiply_10_mantis_big(bvalue_2, def);
    zeroes_left_big(bvalue_2);
  } else if (def < 0) {
    multiply_10_mantis_big(bvalue_1, -def);
    zeroes_left_big(bvalue_1);
  }
}

// умножение мантис биг децимал
int multiply_mantis_big(big_decimal bvalue_1, big_decimal *bvalue_2,
                        big_decimal *result) {
  int status = 0;

  // обнуляем result
  zero_big_decimal(result);

  zeroes_left_big(bvalue_2);
  zeroes_left_big(&bvalue_1);

  // проходимся в цикле по второму значению справа налево, сдвигая value 1 на 1
  for (int i = 0; i <= bvalue_2->one_position_left; i++) {
    if (i != 0)
      if (shift_left_big(&bvalue_1, 1) == 1) {
        status = 1;
        break;
      }
    if (get_bit_big(bvalue_2, i))
      if (sum_mantissa(result, &bvalue_1, result)) status = 1;
  }

  int equal_znak = (bvalue_1.negative == bvalue_2->negative);
  if (!equal_znak) result->negative = 1;

  // дописываем экспоненту
  result->exponenta = bvalue_1.exponenta + bvalue_2->exponenta;

  return status;
}

// обнуялем мантису биг децимал
void zero_mantisa_big(big_decimal *result) {
  for (int i = 0; i < BITS_B; i++) {
    result->bits[i] = 0;
  }
}

// полное обнуление decimal
void zero_big_decimal(big_decimal *result) {
  zero_mantisa_big(result);
  result->exponenta = 0;
  result->negative = 0;
  result->one_position_left = 0;
  result->one_right = 0;
  result->zero_left = 0;
}

// сдвигаем big_decimal налево по битам, если вылезли за пределы, вернет 1, если
// все ок то 0
int shift_left_big(big_decimal *bvalue, int def) {
  int status = 0;

  zeroes_left_big(bvalue);  // устанавливаем нули слева и позицию первой 1

  if ((BITS_BIG - bvalue->one_position_left) < def) status = 1;

  // идем по циклу от первой 1 до 0, и передвигаем бит
  for (int i = bvalue->one_position_left; i >= 0; i--) {
    // для того, чтобы не выходить за пределы массива
    if ((i + def) <= BITS_BIG) {
      set_bit_big(bvalue, i + def, get_bit_big(bvalue, i));
    }
  }
  // доставляем нули справа
  for (int i = 0; i < def; i++) {
    set_bit_big(bvalue, i, 0);
  }
  zeroes_left_big(bvalue);  // устанавливаем нули слева и позицию первой 1

  return status;
}

// сколько нулей слева, если занулеванное, то one_position_left = -1
void zeroes_left_big(big_decimal *bvalue) {
  int i = BITS_BIG;
  while (1) {
    if (get_bit_big(bvalue, i) != 0) {
      bvalue->zero_left = BITS_BIG - i;
      bvalue->one_position_left = i;
      break;
    }
    i--;
    if (!(i >= 0)) break;
  }
  if (i == -1) {
    bvalue->zero_left = BITS_BIG;
    bvalue->one_position_left = -1;
  }
  for (int j = 0; j <= BITS_BIG; j++) {
    if (get_bit_big(bvalue, j) != 0) {
      bvalue->one_right = j;
      break;
    }
  }
}

// деление с big_decimal
void division(big_decimal val1, big_decimal val2, big_decimal *res) {
  int scale_dif =
      (val1.exponenta - val2.exponenta);  // чисел с разными экспонентами
  int q = 0;
  big_decimal part = {0};  //вычитаемое из делителя при найденном q
  big_decimal part_next = {0};  // вычитаемое из делителя при найденном q+1
  big_decimal sum = {0};  // текущая сумма, которая должна знать ответом
  big_decimal before_sum = {0};  // новый член в сумме
  while (is_zero_big_decimal(val1) &&
         sum.exponenta < 31) {  // остаток (val1) != нулю или exp суммы < 31
    while (is_greater_big_decimal(
        val2, val1)) {  // если остаток (изначально - это делимое) < делителя
      multiply_10_mantis_big(
          &val1, 1);  // домнажение остатка на 10 с учетом экспоненты
      multiply_10_mantis_big(&sum,
                             1);  // домнажение суммы на 10 с учетом экспоненты
    }
    q = 0;
    zero_big_decimal(&part);
    zero_big_decimal(&before_sum);
    part_next = val2;
    part = val2;
    zeroes_left_big(&val1);
    while (is_greater_or_equal_big_decimal(
        val1, part_next)) {  // пока делитель(val2)*2^q < остаток
      if (q == 0) {
        int difference_elder_bit =
            val1.one_position_left - val2.one_position_left;
        if (difference_elder_bit > 2) {
          q = difference_elder_bit - 2;
          shift_left_big(&part_next, q);
        }
      }
      part = part_next;
      shift_left_big(&part_next, 1);  // Домнажение на 2 (формирование 2^q)
      q++;
    }
    q--;
    set_bit_big(&before_sum, q, 1);  // формирование нового член в сумме
    sum_mantissa(&sum, &before_sum, &sum);  // добавление нового члена к сумме
                                            // printf("\nval1");
                                            // print_big_decimal(&val1);
                                            // printf("\npart");
                                            // print_big_decimal(&part);
    if (is_greater_or_equal_big_decimal(val1, part))
      sub_mantis_big(val1, part, &val1);  // остаток записываем в val1
  }
  sum.exponenta += scale_dif;  // учет экспоненты (если < 0, то нужно умножить
                               // мантиссу на 10)
  if (scale_dif < 0) {
    multiply_10_mantis_big(&sum, -scale_dif);
  }
  *res = sum;
}

// устанавливаем big_decimal по s21_decimal
void init_big(s21_decimal value, big_decimal *big) {
  big->exponenta = get_scale(value);
  big->negative = get_sign(value);
  big->bits[0] = value.bits[0];
  big->bits[1] = value.bits[1];
  big->bits[2] = value.bits[2];
  zeroes_left_big(big);
}

void change_znak_s21(s21_decimal *value) {
  set_minus(value, !get_sign(*value));
}

// Возвращает результат умножения указанного Decimal на -1.
int s21_negate(s21_decimal value, s21_decimal *result) {
  int status = 0;
  if (result) {
    *result = value;
    change_znak_s21(result);
  } else {
    status = 1;
  }
  return status;
}

// Возвращает целые цифры указанного Decimal числа; любые дробные цифры
// отбрасываются, включая конечные нули
int s21_truncate(s21_decimal value, s21_decimal *result) {
  int status = S21_OK;
  if (result) {
    // зануляем result
    zero_s21_decimal(result);
    big_decimal bvalue = {0};
    init_big(value, &bvalue);
    big_decimal ten = {0};
    ten.bits[0] = 10;
    while ((bvalue.exponenta > 0) &&
           is_greater_or_equal_big_decimal(bvalue, ten)) {
      division_with_rest_for10(bvalue, &bvalue);
    }

    if (bvalue.exponenta > 0 && !is_greater_or_equal_big_decimal(bvalue, ten))
      zero_big_decimal(&bvalue);
    else if (!status)
      big_to_s21decimal(result, &bvalue);
  } else {
    status = S21_ERROR;
  }

  return status;
}

// Округляет Decimal до ближайшего целого числа.
int s21_round(s21_decimal value, s21_decimal *result) {
  int status = S21_OK;
  if (result) {
    zero_s21_decimal(result);
    s21_decimal s21_one = {{1, 0, 0, 0}};
    s21_decimal s21_half = {{2, 0, 0, 0}};
    s21_decimal s21_trunc = {{0, 0, 0, 0}};
    int sign = get_sign(value);

    // получение 1/2
    s21_div(s21_one, s21_half, &s21_half);
    // print_decimal(s21_half);

    // убираем знак
    set_minus(&value, 0);

    // в result записываем только целую часть
    s21_truncate(value, result);
    s21_sub(value, *result, &s21_trunc);

    if (s21_is_less(s21_half, s21_trunc)) {
      s21_add(*result, s21_one, result);
    }

    // устанавливаем знак
    if (sign) {
      set_minus(result, sign);
    }

    if (!result->bits[0] && !result->bits[1] && !result->bits[2]) {
      set_minus(result, 0);
    }
  } else {
    status = S21_ERROR;
  }

  return status;
}

// Округляет указанное Decimal число до ближайшего целого числа в сторону
// отрицательной бесконечности.
int s21_floor(s21_decimal value, s21_decimal *result) {
  int status = S21_OK;
  if (result) {
    zero_s21_decimal(result);
    s21_decimal s21_one = {{1, 0, 0, 0}};
    s21_decimal s21_trunc = {{0, 0, 0, 0}};

    // в транк записываем целую часть знак сохраняется
    s21_truncate(value, &s21_trunc);

    // в result записываем дробную часть знак сохраняется
    s21_sub(value, s21_trunc, result);

    // если в дробной части отрицательный знак, то
    if (get_sign(*result)) {
      s21_sub(s21_trunc, s21_one, result);
    } else {
      *result = s21_trunc;
    }

    if (!result->bits[0] && !result->bits[1] && !result->bits[2])
      set_minus(result, 0);

    // s21_mul(*result, s21_one, result);

  } else {
    status = S21_ERROR;
  }

  return status;
}