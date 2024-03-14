; ModuleID = 'TodoName'
source_filename = "TodoName"

@0 = private unnamed_addr constant [26 x i8] c"number of arguments : %d\0A\00", align 1
@1 = private unnamed_addr constant [7 x i8] c"i = %d\00", align 1

declare i32 @printf(ptr, ...)

define i32 @main(i32 %0) {
entry:
  %1 = call i32 (ptr, ...) @printf(ptr @0, i32 %0)
  %i = alloca i32, align 4
  store i32 0, ptr %i, align 4
  br label %loopthen

loopthen:                                         ; preds = %loopthen, %entry
  %i1 = load i32, ptr %i, align 4
  %2 = call i32 (ptr, ...) @printf(ptr @1, i32 %i1)
  %i2 = load i32, ptr %i, align 4
  %3 = add i32 %i2, 1
  store i32 %3, ptr %i, align 4
  %i3 = load i32, ptr %i, align 4
  %4 = icmp slt i32 %i3, 10
  br i1 %4, label %loopthen, label %loopexit

loopexit:                                         ; preds = %loopthen
  ret i32 0
}
