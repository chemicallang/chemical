; ModuleID = 'TodoName'
source_filename = "TodoName"

@0 = private unnamed_addr constant [11 x i8] c"check : %d\00", align 1
@1 = private unnamed_addr constant [17 x i8] c"hello world : %d\00", align 1

declare i32 @printf(ptr, ...)

define i32 @main() {
entry:
  %x = alloca i32, align 4
  store i32 1, ptr %x, align 4
  %x1 = load i32, ptr %x, align 4
  %0 = add i32 %x1, 5
  store i32 %0, ptr %x, align 4
  %x2 = load i32, ptr %x, align 4
  %1 = call i32 (ptr, ...) @printf(ptr @0, i32 %x2)
  %2 = call i32 (ptr, ...) @printf(ptr @1, i32 12)
  ret i32 0
}
