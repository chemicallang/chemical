; ModuleID = 'TodoName'
source_filename = "TodoName"

@0 = private unnamed_addr constant [17 x i8] c"hello world : %d\00", align 1

declare i32 @printf(ptr, ...)

define i32 @main() {
entry:
  %0 = call i32 (ptr, ...) @printf(ptr @0, i32 12)
  ret i32 0
}
