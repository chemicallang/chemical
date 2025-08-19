// TODO: support #if (_MSC_VER >= 800) || defined(_STDCALL_SUPPORTED)
// TODO: @stdcall annotation should be behind the above if

@stdcall
@extern
public func RtlMoveMemory(Destination : *mut void, Source : *void, Length : SIZE_T);

@stdcall
@extern
public func RtlCopyMemory(Destination : *mut void, Source : *void, Length : SIZE_T);

@stdcall
@extern
public func RtlFillMemory(Destination : *mut void, Length : SIZE_T, Fill : uchar);

@stdcall
@extern
public func RtlZeroMemory(Destination : *mut void, Length : SIZE_T);

/* MoveMemory: overlap-safe move (maps to RtlMoveMemory / memmove) */
public comptime func MoveMemory(Destination: *mut void, Source : *void, Length: size_t): void {
    return intrinsics::wrap(RtlMoveMemory(Destination, Source, Length as SIZE_T)) as void;
}

/* CopyMemory: not guaranteed to be overlap-safe (maps to RtlCopyMemory / memcpy) */
public comptime  func CopyMemory(Destination: *mut void, Source : *void, Length: size_t): void {
    return intrinsics::wrap(RtlCopyMemory(Destination, Source, Length as SIZE_T)) as void;
}

/* FillMemory: fill a block with a byte value (maps to RtlFillMemory / memset) */
public comptime  func FillMemory(Destination: *mut void, Length: size_t, Fill: uchar): void {
    return intrinsics::wrap(RtlFillMemory(Destination, Length as SIZE_T, Fill)) as void;
}

/* ZeroMemory: zero a block (maps to RtlZeroMemory / memset) */
public comptime  func ZeroMemory(Destination: *mut void, Length: size_t): void {
    return intrinsics::wrap(RtlZeroMemory(Destination, Length as SIZE_T)) as void;
}