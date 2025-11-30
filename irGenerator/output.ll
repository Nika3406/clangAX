; ModuleID = 'C-ACCEL-Module'
source_filename = "C-ACCEL-Module"

@.str = private constant [12 x i8] c"Hello World\00", align 1
@.str.1 = private constant [15 x i8] c"This is a test\00", align 1
@.str.2 = private constant [20 x i8] c"Special chars: @#$%\00", align 1
@.str.3 = private constant [15 x i8] c"/usr/local/bin\00", align 1
@.str.4 = private constant [28 x i8] c"Primitive types initialized\00", align 1
@.str.5 = private constant [4 x i8] c"%s\0A\00", align 1
@.str.6 = private constant [6 x i8] c"alpha\00", align 1
@.str.7 = private constant [5 x i8] c"beta\00", align 1
@.str.8 = private constant [6 x i8] c"gamma\00", align 1
@.str.9 = private constant [6 x i8] c"delta\00", align 1
@.str.10 = private constant [5 x i8] c"text\00", align 1
@.str.11 = private constant [4 x i8] c"%d\0A\00", align 1
@.str.12 = private constant [25 x i8] c"Array examples completed\00", align 1
@.str.13 = private constant [4 x i8] c"%s\0A\00", align 1
@.str.14 = private constant [4 x i8] c"%d\0A\00", align 1
@.str.15 = private constant [26 x i8] c"Vector examples completed\00", align 1
@.str.16 = private constant [4 x i8] c"%s\0A\00", align 1
@.str.17 = private constant [28 x i8] c"Operator examples completed\00", align 1
@.str.18 = private constant [4 x i8] c"%s\0A\00", align 1
@.str.19 = private constant [4 x i8] c"%d\0A\00", align 1
@.str.20 = private constant [4 x i8] c"%d\0A\00", align 1
@.str.21 = private constant [4 x i8] c"%d\0A\00", align 1
@.str.22 = private constant [16 x i8] c"Greater than 50\00", align 1
@.str.23 = private constant [4 x i8] c"%s\0A\00", align 1
@.str.24 = private constant [25 x i8] c"Less than or equal to 50\00", align 1
@.str.25 = private constant [4 x i8] c"%s\0A\00", align 1
@.str.26 = private constant [25 x i8] c"Positive and less than y\00", align 1
@.str.27 = private constant [4 x i8] c"%s\0A\00", align 1
@.str.28 = private constant [32 x i8] c"Control flow examples completed\00", align 1
@.str.29 = private constant [4 x i8] c"%s\0A\00", align 1
@.str.30 = private constant [26 x i8] c"Object examples completed\00", align 1
@.str.31 = private constant [4 x i8] c"%s\0A\00", align 1
@.str.32 = private constant [12 x i8] c"Found three\00", align 1
@.str.33 = private constant [4 x i8] c"%s\0A\00", align 1
@.str.34 = private constant [4 x i8] c"%d\0A\00", align 1
@.str.35 = private constant [4 x i8] c"%d\0A\00", align 1
@.str.36 = private constant [30 x i8] c"Reserved words demo completed\00", align 1
@.str.37 = private constant [4 x i8] c"%s\0A\00", align 1
@.str.38 = private constant [21 x i8] c"Edge cases completed\00", align 1
@.str.39 = private constant [4 x i8] c"%s\0A\00", align 1
@.str.40 = private constant [41 x i8] c"========================================\00", align 1
@.str.41 = private constant [4 x i8] c"%s\0A\00", align 1
@.str.42 = private constant [33 x i8] c"C-Accel Comprehensive Test Suite\00", align 1
@.str.43 = private constant [4 x i8] c"%s\0A\00", align 1
@.str.44 = private constant [41 x i8] c"========================================\00", align 1
@.str.45 = private constant [4 x i8] c"%s\0A\00", align 1
@.str.46 = private constant [41 x i8] c"========================================\00", align 1
@.str.47 = private constant [4 x i8] c"%s\0A\00", align 1
@.str.48 = private constant [34 x i8] c"All tests completed successfully!\00", align 1
@.str.49 = private constant [4 x i8] c"%s\0A\00", align 1
@.str.50 = private constant [41 x i8] c"========================================\00", align 1
@.str.51 = private constant [4 x i8] c"%s\0A\00", align 1

declare i32 @printf(ptr, ...)

declare i32 @puts(ptr)

define void @primitives() {
entry:
  %null_var = alloca i32, align 4
  %is_active = alloca i1, align 1
  %flag_false = alloca i1, align 1
  %flag_true = alloca i1, align 1
  %char_space = alloca i8, align 1
  %char_symbol = alloca i8, align 1
  %char_digit = alloca i32, align 4
  %char_letter = alloca i8, align 1
  %str_path = alloca ptr, align 8
  %str_empty = alloca i32, align 4
  %str_special = alloca ptr, align 8
  %str_with_space = alloca ptr, align 8
  %str_simple = alloca ptr, align 8
  %float_negative = alloca double, align 8
  %float_small = alloca double, align 8
  %float_scientific = alloca double, align 8
  %float_decimal = alloca double, align 8
  %int_large = alloca i32, align 4
  %int_zero = alloca i32, align 4
  %int_negative = alloca i32, align 4
  %int_positive = alloca i32, align 4
  store i32 42, ptr %int_positive, align 4
  store i32 -100, ptr %int_negative, align 4
  store i32 0, ptr %int_zero, align 4
  store i32 999999, ptr %int_large, align 4
  store double 3.141590e+00, ptr %float_decimal, align 8
  store double 2.500000e+00, ptr %float_scientific, align 8
  store double 1.000000e-03, ptr %float_small, align 8
  store double 0xC058FF5C28F5C28F, ptr %float_negative, align 8
  store ptr @.str, ptr %str_simple, align 8
  store ptr @.str.1, ptr %str_with_space, align 8
  store ptr @.str.2, ptr %str_special, align 8
  store i32 0, ptr %str_empty, align 4
  store ptr @.str.3, ptr %str_path, align 8
  store i8 65, ptr %char_letter, align 1
  store i32 5, ptr %char_digit, align 4
  store i8 64, ptr %char_symbol, align 1
  store i8 32, ptr %char_space, align 1
  store i1 true, ptr %flag_true, align 1
  store i1 false, ptr %flag_false, align 1
  store i1 true, ptr %is_active, align 1
  store i32 0, ptr %null_var, align 4
  %0 = call i32 (ptr, ...) @printf(ptr @.str.5, ptr @.str.4)
  ret void
}

define void @arrayExamples() {
entry:
  %idx = alloca i32, align 4
  %last = alloca i32, align 4
  %first = alloca i32, align 4
  %nested = alloca [3 x i32], align 4
  %empty_arr = alloca i32, align 4
  %matrix_3x3 = alloca [3 x i32], align 4
  %matrix_2x2 = alloca [2 x i32], align 4
  %mixed = alloca [4 x ptr], align 8
  %strings = alloca [4 x ptr], align 8
  %floats = alloca [5 x ptr], align 8
  %numbers = alloca [5 x ptr], align 8
  %array = alloca [5 x i32], align 4
  store [5 x i32] [i32 1, i32 2, i32 3, i32 4, i32 5], ptr %array, align 4
  store ptr %array, ptr %numbers, align 8
  %array1 = alloca [5 x double], align 8
  store [5 x double] [double 1.100000e+00, double 2.200000e+00, double 3.300000e+00, double 4.400000e+00, double 5.500000e+00], ptr %array1, align 8
  store ptr %array1, ptr %floats, align 8
  %array2 = alloca [4 x ptr], align 8
  store [4 x ptr] [ptr @.str.6, ptr @.str.7, ptr @.str.8, ptr @.str.9], ptr %array2, align 8
  store ptr %array2, ptr %strings, align 8
  %array3 = alloca [4 x i32], align 4
  store [4 x i32] [i32 1, i32 0, i32 0, i32 0], ptr %array3, align 4
  store ptr %array3, ptr %mixed, align 8
  %array4 = alloca [2 x i32], align 4
  store [2 x i32] [i32 1, i32 2], ptr %array4, align 4
  store i32 0, ptr %matrix_2x2, align 4
  %array5 = alloca [3 x double], align 8
  store [3 x double] [double 1.000000e+00, double 2.000000e+00, double 3.000000e+00], ptr %array5, align 8
  store i32 0, ptr %matrix_3x3, align 4
  store i32 0, ptr %empty_arr, align 4
  %array6 = alloca [3 x i32], align 4
  store [3 x i32] [i32 1, i32 2, i32 3], ptr %array6, align 4
  store i32 0, ptr %nested, align 4
  %numbers7 = load [5 x ptr], ptr %numbers, align 8
  store i32 0, ptr %first, align 4
  %numbers8 = load [5 x ptr], ptr %numbers, align 8
  store i32 0, ptr %last, align 4
  store i32 0, ptr %idx, align 4
  br label %forcond

forcond:                                          ; preds = %forinc, %entry
  %idx9 = load i32, ptr %idx, align 4
  %cmptmp = icmp slt i32 %idx9, 5
  br i1 %cmptmp, label %forbody, label %afterfor

forbody:                                          ; preds = %forcond
  %numbers10 = load [5 x ptr], ptr %numbers, align 8
  %idx11 = load i32, ptr %idx, align 4
  %0 = call i32 (ptr, ...) @printf(ptr @.str.11, i32 0)
  br label %forinc

forinc:                                           ; preds = %forbody
  %idx12 = load i32, ptr %idx, align 4
  %inc = add i32 %idx12, 1
  store i32 %inc, ptr %idx, align 4
  br label %forcond

afterfor:                                         ; preds = %forcond
  %1 = call i32 (ptr, ...) @printf(ptr @.str.13, ptr @.str.12)
  ret void
}

define void @vectorExamples() {
entry:
  %i = alloca i32, align 4
  %vec_size = alloca i32, align 4
  %second_float = alloca i32, align 4
  %first_int = alloca i32, align 4
  %char_vec = alloca ptr, align 8
  %str_vec = alloca ptr, align 8
  %float_vec = alloca ptr, align 8
  %int_vec = alloca ptr, align 8
  %int_vec1 = load ptr, ptr %int_vec, align 8
  store i32 0, ptr %first_int, align 4
  %float_vec2 = load ptr, ptr %float_vec, align 8
  store i32 0, ptr %second_float, align 4
  store i32 0, ptr %vec_size, align 4
  store i32 0, ptr %i, align 4
  br label %forcond

forcond:                                          ; preds = %forinc, %entry
  %i3 = load i32, ptr %i, align 4
  %cmptmp = icmp slt i32 %i3, 0
  br i1 %cmptmp, label %forbody, label %afterfor

forbody:                                          ; preds = %forcond
  %int_vec4 = load ptr, ptr %int_vec, align 8
  %i5 = load i32, ptr %i, align 4
  %0 = call i32 (ptr, ...) @printf(ptr @.str.14, i32 0)
  br label %forinc

forinc:                                           ; preds = %forbody
  %i6 = load i32, ptr %i, align 4
  %inc = add i32 %i6, 1
  store i32 %inc, ptr %i, align 4
  br label %forcond

afterfor:                                         ; preds = %forcond
  %1 = call i32 (ptr, ...) @printf(ptr @.str.16, ptr @.str.15)
  ret void
}

define void @operatorExamples() {
entry:
  %value = alloca i32, align 4
  %counter = alloca i32, align 4
  %not_result = alloca i1, align 1
  %or_result = alloca i1, align 1
  %and_result = alloca i1, align 1
  %greater_equal = alloca i1, align 1
  %less_equal = alloca i1, align 1
  %greater_than = alloca i1, align 1
  %less_than = alloca i1, align 1
  %not_equal = alloca i1, align 1
  %is_equal = alloca i1, align 1
  %mod_result = alloca i32, align 4
  %div_result = alloca i32, align 4
  %mul_result = alloca i32, align 4
  %sub_result = alloca i32, align 4
  %add_result = alloca i32, align 4
  store i32 15, ptr %add_result, align 4
  store i32 5, ptr %sub_result, align 4
  store i32 50, ptr %mul_result, align 4
  store i32 2, ptr %div_result, align 4
  store i32 1, ptr %mod_result, align 4
  store i1 true, ptr %is_equal, align 1
  store i1 true, ptr %not_equal, align 1
  store i1 true, ptr %less_than, align 1
  store i1 true, ptr %greater_than, align 1
  store i1 true, ptr %less_equal, align 1
  store i1 true, ptr %greater_equal, align 1
  store i1 true, ptr %and_result, align 1
  store i1 true, ptr %or_result, align 1
  store i1 true, ptr %not_result, align 1
  store i32 0, ptr %counter, align 4
  %counter1 = load i32, ptr %counter, align 4
  %inc = add i32 %counter1, 1
  store i32 %inc, ptr %counter, align 4
  %counter2 = load i32, ptr %counter, align 4
  %inc3 = add i32 %counter2, 1
  store i32 %inc3, ptr %counter, align 4
  %counter4 = load i32, ptr %counter, align 4
  %dec = sub i32 %counter4, 1
  store i32 %dec, ptr %counter, align 4
  store i32 10, ptr %value, align 4
  %0 = call i32 (ptr, ...) @printf(ptr @.str.18, ptr @.str.17)
  ret void
}

define void @controlFlow() {
entry:
  %y = alloca i32, align 4
  %x = alloca i32, align 4
  %value = alloca i32, align 4
  %counter = alloca i32, align 4
  %j = alloca i32, align 4
  %i = alloca i32, align 4
  store i32 0, ptr %i, align 4
  br label %forcond

forcond:                                          ; preds = %forinc, %entry
  %i1 = load i32, ptr %i, align 4
  %cmptmp = icmp slt i32 %i1, 10
  br i1 %cmptmp, label %forbody, label %afterfor

forbody:                                          ; preds = %forcond
  %i2 = load i32, ptr %i, align 4
  %0 = call i32 (ptr, ...) @printf(ptr @.str.19, i32 %i2)
  br label %forinc

forinc:                                           ; preds = %forbody
  %i3 = load i32, ptr %i, align 4
  %inc = add i32 %i3, 1
  store i32 %inc, ptr %i, align 4
  br label %forcond

afterfor:                                         ; preds = %forcond
  store i32 10, ptr %j, align 4
  br label %forcond4

forcond4:                                         ; preds = %forinc9, %afterfor
  %j5 = load i32, ptr %j, align 4
  %cmptmp6 = icmp sgt i32 %j5, 0
  br i1 %cmptmp6, label %forbody7, label %afterfor11

forbody7:                                         ; preds = %forcond4
  %j8 = load i32, ptr %j, align 4
  %1 = call i32 (ptr, ...) @printf(ptr @.str.20, i32 %j8)
  br label %forinc9

forinc9:                                          ; preds = %forbody7
  %j10 = load i32, ptr %j, align 4
  %dec = sub i32 %j10, 1
  store i32 %dec, ptr %j, align 4
  br label %forcond4

afterfor11:                                       ; preds = %forcond4
  store i32 0, ptr %counter, align 4
  br label %whilecond

whilecond:                                        ; preds = %whilebody, %afterfor11
  %counter12 = load i32, ptr %counter, align 4
  %cmptmp13 = icmp slt i32 %counter12, 5
  br i1 %cmptmp13, label %whilebody, label %afterwhile

whilebody:                                        ; preds = %whilecond
  %counter14 = load i32, ptr %counter, align 4
  %2 = call i32 (ptr, ...) @printf(ptr @.str.21, i32 %counter14)
  %counter15 = load i32, ptr %counter, align 4
  %inc16 = add i32 %counter15, 1
  store i32 %inc16, ptr %counter, align 4
  br label %whilecond

afterwhile:                                       ; preds = %whilecond
  store i32 42, ptr %value, align 4
  %value17 = load i32, ptr %value, align 4
  %cmptmp18 = icmp sgt i32 %value17, 50
  br i1 %cmptmp18, label %then, label %else

then:                                             ; preds = %afterwhile
  %3 = call i32 (ptr, ...) @printf(ptr @.str.23, ptr @.str.22)
  br label %ifcont

else:                                             ; preds = %afterwhile
  %4 = call i32 (ptr, ...) @printf(ptr @.str.25, ptr @.str.24)
  br label %ifcont

ifcont:                                           ; preds = %else, %then
  store i32 10, ptr %x, align 4
  store i32 20, ptr %y, align 4
  %x19 = load i32, ptr %x, align 4
  %y20 = load i32, ptr %y, align 4
  %cmptmp21 = icmp slt i32 %x19, %y20
  br i1 %cmptmp21, label %then22, label %ifcont27

then22:                                           ; preds = %ifcont
  %x23 = load i32, ptr %x, align 4
  %cmptmp24 = icmp sgt i32 %x23, 0
  br i1 %cmptmp24, label %then25, label %ifcont26

then25:                                           ; preds = %then22
  %5 = call i32 (ptr, ...) @printf(ptr @.str.27, ptr @.str.26)
  br label %ifcont26

ifcont26:                                         ; preds = %then25, %then22
  br label %ifcont27

ifcont27:                                         ; preds = %ifcont26, %ifcont
  %6 = call i32 (ptr, ...) @printf(ptr @.str.29, ptr @.str.28)
  ret void
}

define void @objectExamples() {
entry:
  %0 = call i32 (ptr, ...) @printf(ptr @.str.31, ptr @.str.30)
  ret void
}

define void @reservedWordsDemo() {
entry:
  %length = alloca i32, align 4
  %arr = alloca [3 x ptr], align 8
  %size = alloca i32, align 4
  %temp = alloca ptr, align 8
  %count = alloca i32, align 4
  %i = alloca i32, align 4
  store i32 0, ptr %i, align 4
  br label %forcond

forcond:                                          ; preds = %forinc, %entry
  %i1 = load i32, ptr %i, align 4
  %cmptmp = icmp slt i32 %i1, 5
  br i1 %cmptmp, label %forbody, label %afterfor

forbody:                                          ; preds = %forcond
  %i2 = load i32, ptr %i, align 4
  %cmptmp3 = icmp eq i32 %i2, 3
  br i1 %cmptmp3, label %then, label %else

then:                                             ; preds = %forbody
  %0 = call i32 (ptr, ...) @printf(ptr @.str.33, ptr @.str.32)
  br label %ifcont

else:                                             ; preds = %forbody
  %i4 = load i32, ptr %i, align 4
  %1 = call i32 (ptr, ...) @printf(ptr @.str.34, i32 %i4)
  br label %ifcont

ifcont:                                           ; preds = %else, %then
  br label %forinc

forinc:                                           ; preds = %ifcont
  %i5 = load i32, ptr %i, align 4
  %inc = add i32 %i5, 1
  store i32 %inc, ptr %i, align 4
  br label %forcond

afterfor:                                         ; preds = %forcond
  store i32 0, ptr %count, align 4
  br label %whilecond

whilecond:                                        ; preds = %whilebody, %afterfor
  %count6 = load i32, ptr %count, align 4
  %cmptmp7 = icmp slt i32 %count6, 3
  br i1 %cmptmp7, label %whilebody, label %afterwhile

whilebody:                                        ; preds = %whilecond
  %count8 = load i32, ptr %count, align 4
  %2 = call i32 (ptr, ...) @printf(ptr @.str.35, i32 %count8)
  %count9 = load i32, ptr %count, align 4
  %inc10 = add i32 %count9, 1
  store i32 %inc10, ptr %count, align 4
  br label %whilecond

afterwhile:                                       ; preds = %whilecond
  store i32 0, ptr %size, align 4
  %array = alloca [3 x i32], align 4
  store [3 x i32] [i32 1, i32 2, i32 3], ptr %array, align 4
  store ptr %array, ptr %arr, align 8
  store i32 3, ptr %length, align 4
  %3 = call i32 (ptr, ...) @printf(ptr @.str.37, ptr @.str.36)
  ret void
}

define void @edgeCases() {
entry:
  %x = alloca i32, align 4
  %complex = alloca [3 x i32], align 4
  %size = alloca i32, align 4
  %nums = alloca ptr, align 8
  %value = alloca double, align 8
  %empty = alloca i32, align 4
  %single = alloca [1 x ptr], align 8
  %result = alloca i32, align 4
  %c = alloca i32, align 4
  %b = alloca i32, align 4
  %a = alloca i32, align 4
  %large_float = alloca double, align 8
  %large_int = alloca i32, align 4
  store i32 123456789, ptr %large_int, align 4
  store double 0x412E240CA45A1CAC, ptr %large_float, align 8
  store i32 1, ptr %a, align 4
  store i32 2, ptr %b, align 4
  store i32 3, ptr %c, align 4
  store i32 27, ptr %result, align 4
  %array = alloca [1 x i32], align 4
  store [1 x i32] [i32 42], ptr %array, align 4
  store ptr %array, ptr %single, align 8
  store i32 0, ptr %empty, align 4
  store double 3.140000e+00, ptr %value, align 8
  store i32 0, ptr %size, align 4
  %array1 = alloca [2 x i32], align 4
  store [2 x i32] [i32 1, i32 2], ptr %array1, align 4
  store i32 0, ptr %complex, align 4
  store i32 10, ptr %x, align 4
  %x2 = load i32, ptr %x, align 4
  %inc = add i32 %x2, 1
  store i32 %inc, ptr %x, align 4
  %x3 = load i32, ptr %x, align 4
  %dec = sub i32 %x3, 1
  store i32 %dec, ptr %x, align 4
  %0 = call i32 (ptr, ...) @printf(ptr @.str.39, ptr @.str.38)
  ret void
}

define i32 @main() {
entry:
  %0 = call i32 (ptr, ...) @printf(ptr @.str.41, ptr @.str.40)
  %1 = call i32 (ptr, ...) @printf(ptr @.str.43, ptr @.str.42)
  %2 = call i32 (ptr, ...) @printf(ptr @.str.45, ptr @.str.44)
  call void @primitives()
  call void @arrayExamples()
  call void @vectorExamples()
  call void @operatorExamples()
  call void @controlFlow()
  call void @objectExamples()
  call void @reservedWordsDemo()
  call void @edgeCases()
  %3 = call i32 (ptr, ...) @printf(ptr @.str.47, ptr @.str.46)
  %4 = call i32 (ptr, ...) @printf(ptr @.str.49, ptr @.str.48)
  %5 = call i32 (ptr, ...) @printf(ptr @.str.51, ptr @.str.50)
  ret i32 0
}
