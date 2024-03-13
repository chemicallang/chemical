; ModuleID = 'TodoName'
source_filename = "TodoName"

@0 = private unnamed_addr constant [22 x i8] c"congrats x == 6 : %d\0A\00", align 1
@1 = private unnamed_addr constant [13 x i8] c"nuts! x != 6\00", align 1
@2 = private unnamed_addr constant [17 x i8] c"hello world : %d\00", align 1

declare i32 @printf(ptr, ...)

define i32 @main() {
entry:
  %x = alloca i32, align 4
  store i32 1, ptr %x, align 4
  %x1 = load i32, ptr %x, align 4
  %0 = add i32 %x1, 5
  store i32 %0, ptr %x, align 4
  %x2 = load i32, ptr %x, align 4
  %1 = icmp eq i32 %x2, 6
  br i1 %1, label %then, label %else

then:                                             ; preds = %entry
  %x3 = load i32, ptr %x, align 4
  %2 = call i32 (ptr, ...) @printf(ptr @0, i32 %x3)
  br label %end

else:                                             ; preds = %entry
  %3 = call i32 (ptr, ...) @printf(ptr @1)
  br label %end

end:                                              ; preds = %else, %then
  %4 = call i32 (ptr, ...) @printf(ptr @2, i32 12)
  ret i32 0
}
