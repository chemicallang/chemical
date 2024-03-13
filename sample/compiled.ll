; ModuleID = 'TodoName'
source_filename = "TodoName"

@0 = private unnamed_addr constant [11 x i8] c"check : %d\00", align 1
@1 = private unnamed_addr constant [17 x i8] c"hello world : %d\00", align 1

declare i32 @printf(ptr, ...)

define i32 @main() {
entry:
  %x = alloca i32, align 4
  store i32 1, ptr %x, align 4
  store i32 5, ptr %x, align 4
  %x1 = load ptr, ptr %x, align 8
  %0 = call i32 (ptr, ...) @printf(ptr @0, ptr %x1)
  %1 = call i32 (ptr, ...) @printf(ptr @1, i32 12)
  ret i32 0
}
