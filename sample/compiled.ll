; ModuleID = 'TodoName'
source_filename = "TodoName"

@0 = private unnamed_addr constant [26 x i8] c"number of arguments : %d\0A\00", align 1
@1 = private unnamed_addr constant [14 x i8] c"current : %d\0A\00", align 1

declare i32 @printf(ptr, ...)

define i32 @main(i32 %0) {
entry:
  %1 = call i32 (ptr, ...) @printf(ptr @0, i32 %0)
  %arr = alloca [5 x i32], align 4
  %2 = getelementptr [5 x i32], ptr %arr, i32 0, i32 0
  store i32 2, ptr %2, align 4
  %3 = getelementptr [5 x i32], ptr %arr, i32 0, i32 1
  store i32 4, ptr %3, align 4
  %4 = getelementptr [5 x i32], ptr %arr, i32 0, i32 2
  store i32 6, ptr %4, align 4
  %5 = getelementptr [5 x i32], ptr %arr, i32 0, i32 3
  store i32 8, ptr %5, align 4
  %6 = getelementptr [5 x i32], ptr %arr, i32 0, i32 4
  store i32 10, ptr %6, align 4
  %i = alloca i32, align 4
  store i32 0, ptr %i, align 4
  br label %loopcond

loopcond:                                         ; preds = %ifend, %entry
  %i1 = load i32, ptr %i, align 4
  %7 = icmp slt i32 %i1, 5
  br i1 %7, label %loopthen, label %loopexit

loopthen:                                         ; preds = %loopcond
  %i2 = load i32, ptr %i, align 4
  %8 = icmp eq i32 %i2, 4
  br i1 %8, label %ifthen, label %ifend

loopexit:                                         ; preds = %ifthen, %loopcond
  ret i32 0

ifthen:                                           ; preds = %loopthen
  br label %loopexit

ifend:                                            ; preds = %loopthen
  %i3 = load i32, ptr %i, align 4
  %9 = getelementptr [5 x i32], ptr %arr, i32 0, i32 %i3
  %arr0 = load i32, ptr %9, align 4
  %10 = call i32 (ptr, ...) @printf(ptr @1, i32 %arr0)
  %i4 = load i32, ptr %i, align 4
  %11 = add i32 %i4, 1
  store i32 %11, ptr %i, align 4
  br label %loopcond
}
