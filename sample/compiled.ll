; ModuleID = 'TodoName'
source_filename = "TodoName"

@0 = private unnamed_addr constant [26 x i8] c"number of arguments : %d\0A\00", align 1
@1 = private unnamed_addr constant [19 x i8] c"function sum : %d\0A\00", align 1
@2 = private unnamed_addr constant [22 x i8] c"check this char '%c'\0A\00", align 1
@3 = private unnamed_addr constant [23 x i8] c"check this float : %f\0A\00", align 1
@4 = private unnamed_addr constant [14 x i8] c"current : %d\0A\00", align 1

declare i32 @printf(ptr, ...)

define i32 @add(i32 %0, i32 %1) {
entry:
  %2 = add i32 %0, %1
  ret i32 %2
}

define i32 @main(i32 %0) {
entry:
  %1 = call i32 (ptr, ...) @printf(ptr @0, i32 %0)
  %2 = call i32 @add(i32 5, i32 4)
  %3 = call i32 (ptr, ...) @printf(ptr @1, i32 %2)
  %4 = call i32 (ptr, ...) @printf(ptr @2, i8 120)
  %5 = call i32 (ptr, ...) @printf(ptr @3, double 1.123450e+00)
  %arr = alloca [5 x i32], align 4
  %6 = getelementptr [5 x i32], ptr %arr, i32 0, i32 0
  store i32 2, ptr %6, align 4
  %7 = getelementptr [5 x i32], ptr %arr, i32 0, i32 1
  store i32 4, ptr %7, align 4
  %8 = getelementptr [5 x i32], ptr %arr, i32 0, i32 2
  store i32 6, ptr %8, align 4
  %9 = getelementptr [5 x i32], ptr %arr, i32 0, i32 3
  store i32 8, ptr %9, align 4
  %10 = getelementptr [5 x i32], ptr %arr, i32 0, i32 4
  store i32 10, ptr %10, align 4
  %i = alloca i32, align 4
  store i32 0, ptr %i, align 4
  br label %loopcond

loopcond:                                         ; preds = %ifend, %entry
  %i1 = load i32, ptr %i, align 4
  %11 = icmp slt i32 %i1, 5
  br i1 %11, label %loopthen, label %loopexit

loopthen:                                         ; preds = %loopcond
  %i2 = load i32, ptr %i, align 4
  %12 = icmp eq i32 %i2, 4
  br i1 %12, label %ifthen, label %ifend

loopexit:                                         ; preds = %ifthen, %loopcond
  ret i32 0

ifthen:                                           ; preds = %loopthen
  br label %loopexit

ifend:                                            ; preds = %loopthen
  %i3 = load i32, ptr %i, align 4
  %13 = getelementptr [5 x i32], ptr %arr, i32 0, i32 %i3
  %arr0 = load i32, ptr %13, align 4
  %14 = call i32 (ptr, ...) @printf(ptr @4, i32 %arr0)
  %i4 = load i32, ptr %i, align 4
  %15 = add i32 %i4, 1
  store i32 %15, ptr %i, align 4
  br label %loopcond
}
