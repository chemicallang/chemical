// TODO: support #if (_MSC_VER >= 800) || defined(_STDCALL_SUPPORTED)
// TODO: @stdcall annotation should be behind the above if

@stdcall
@extern
public func RtlMoveMemory(Destination : *mut void, Source : *void, Length : SIZE_T);

@stdcall
@extern
public func RtlFillMemory(Destination : *mut void, Length : SIZE_T, Fill : uchar);

@stdcall
@extern
public func RtlZeroMemory(Destination : *mut void, Length : SIZE_T);

/* MoveMemory: overlap-safe move (maps to RtlMoveMemory / memmove) */
// TODO: make this comptime
public func MoveMemory(Destination: *mut void, Source : *void, Length: size_t): void {
    RtlMoveMemory(Destination, Source, Length as SIZE_T);
}

/* FillMemory: fill a block with a byte value (maps to RtlFillMemory / memset) */
// TODO: make this comptime
public func FillMemory(Destination: *mut void, Length: size_t, Fill: uchar): void {
    RtlFillMemory(Destination, Length as SIZE_T, Fill);
}

/* ZeroMemory: zero a block (maps to RtlZeroMemory / memset) */
// TODO: make this comptime
public func ZeroMemory(Destination: *mut void, Length: size_t): void {
    RtlZeroMemory(Destination, Length as SIZE_T);
}