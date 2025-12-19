; ModuleID = 'C-ACCEL-Module'
source_filename = "C-ACCEL-Module"

@.str = private constant [3 x i8] c"%d\00", align 1
@.str.1 = private constant [3 x i8] c"%c\00", align 1
@.str.2 = private constant [3 x i8] c"%d\00", align 1
@.str.3 = private constant [3 x i8] c"%c\00", align 1
@.str.4 = private constant [21 x i8] c"---Test Function---\0A\00", align 1
@.str.5 = private constant [3 x i8] c"%s\00", align 1
@.str.6 = private constant [27 x i8] c"---Array Test Function---\0A\00", align 1
@.str.7 = private constant [3 x i8] c"%s\00", align 1

declare i32 @printf(ptr, ...)

declare i32 @puts(ptr)

define void @test() {
entry:
  %z = alloca i32, align 4
  %y = alloca i32, align 4
  %x = alloca i32, align 4
  store i32 8, ptr %x, align 4
  store i32 9, ptr %y, align 4
  %x1 = load i32, ptr %x, align 4
  %y2 = load i32, ptr %y, align 4
  %addtmp = add i32 %x1, %y2
  store i32 %addtmp, ptr %z, align 4
  %z3 = load i32, ptr %z, align 4
  %0 = call i32 (ptr, ...) @printf(ptr @.str, i32 %z3)
  %1 = call i32 (ptr, ...) @printf(ptr @.str.1, i8 10)
  ret void
}

define void @ArrayTest() {
entry:
  %idx = alloca i32, align 4
  %b = alloca [5 x i32], align 4
  %a = alloca [5 x i32], align 4
  store [5 x i32] [i32 1, i32 2, i32 3, i32 4, i32 5], ptr %a, align 4
  store [5 x i32] [i32 5, i32 6, i32 7, i32 8, i32 9], ptr %b, align 4
  store i32 0, ptr %idx, align 4
  br label %forcond

forcond:                                          ; preds = %forinc, %entry
  %idx1 = load i32, ptr %idx, align 4
  %cmptmp = icmp slt i32 %idx1, 5
  br i1 %cmptmp, label %forbody, label %afterfor

forbody:                                          ; preds = %forcond
  %idx2 = load i32, ptr %idx, align 4
  %arrayelem = getelementptr inbounds [5 x i32], ptr %a, i32 0, i32 %idx2
  %arrayval = load i32, ptr %arrayelem, align 4
  %0 = call i32 (ptr, ...) @printf(ptr @.str.2, i32 %arrayval)
  %1 = call i32 (ptr, ...) @printf(ptr @.str.3, i8 10)
  br label %forinc

forinc:                                           ; preds = %forbody
  %idx3 = load i32, ptr %idx, align 4
  %inc = add i32 %idx3, 1
  store i32 %inc, ptr %idx, align 4
  br label %forcond

afterfor:                                         ; preds = %forcond
  ret void
}

define i32 @main() {
entry:
  %0 = call i32 (ptr, ...) @printf(ptr @.str.5, ptr @.str.4)
  call void @test()
  %1 = call i32 (ptr, ...) @printf(ptr @.str.7, ptr @.str.6)
  call void @ArrayTest()
  ret i32 0
}
