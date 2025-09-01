func raw_panic(message : *char) {
    printf("panic with message '%s'\n", message);
    abort()
}

public comptime func panic(message : *char) {
    intrinsics::destruct_call_site()
    return (intrinsics::wrap(raw_panic(message)) as void);
}