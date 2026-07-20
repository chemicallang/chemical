
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/span.ch **/
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/result.ch **/
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/panic.ch **/
struct std_Location;
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/mutex.ch **/
struct std_stdlock_guard;
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/alloc.ch **/
struct std_stdallocLayout;
struct std_stdallocAllocError;
struct std_stdallocAllocRecord;
struct std_stdallocDebugTracker;
struct std_stdallocGlobalAllocator;
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/serialization.ch **/
struct std_stdUnit;
struct std_stdSerializationError;
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/os_string.ch **/
struct std_stdOsString;
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/string_view.ch **/
struct std_stdstring_view;
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/unordered_map2.ch **/
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/deque.ch **/
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/hashing/fnv1.ch **/
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/hashing/compare_impl.ch **/
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/concurrency/threadpool.ch **/
struct std_stdconcurrentThread;
struct std_stdconcurrentTask;
struct std_stdconcurrentPoolData;
struct std_stdconcurrentThreadPool;
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/u16_string.ch **/
struct std_stdu16string;
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/hashing/hash.ch **/
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/std.ch **/
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/ordered_map.ch **/
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/hashing/hash_impl.ch **/
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/option.ch **/
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/string.ch **/
union std_stdu64_double_union;
struct std_stdstring;
struct std_stdStringStream;
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/utils.ch **/
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/time/types.ch **/
struct std_stdchronoDuration;
struct std_stdchronoInstant;
struct std_stdchronoSystemTime;
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/stream.ch **/
struct std_CommandLineStream;
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/function.ch **/
struct std_stddefault_function_instance;
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/env.ch **/
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/hashing/murmur.ch **/
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/pair.ch **/
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/vector.ch **/
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/posix/time.ch **/
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/posix/condvar.ch **/
struct std_stdCondVar;
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/posix/mutex.ch **/
struct std_stdmutex;
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/span.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/result.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/panic.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/mutex.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/alloc.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/serialization.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/os_string.ch **/
typedef char std_stdnative_char_t;
struct std_stdstring {
	union {
		struct {
			const char* data;
			size_t length;
		} constant;
		struct {
			char* data;
			size_t length;
			size_t capacity;
		} heap;
		struct {
			char buffer[16];
			unsigned char length;
		} sso;
	} storage;
	char state;
};
typedef struct std_stdstring std_stdnative_string_t;
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/string_view.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/unordered_map2.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/deque.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/hashing/fnv1.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/hashing/compare_impl.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/concurrency/threadpool.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/u16_string.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/hashing/hash.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/std.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/ordered_map.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/hashing/hash_impl.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/option.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/string.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/utils.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/time/types.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/stream.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/function.ch **/
typedef void(*std_stddestructor_type)(void* obj);
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/env.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/hashing/murmur.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/pair.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/vector.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/posix/time.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/posix/condvar.ch **/
struct std_stdCondVar {
	uint8_t storage[48];
};
typedef struct std_stdCondVar std_stdcondvar;
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/posix/mutex.ch **/
/** FwdDeclare:Generics std **/
struct std_stdResult__cgs__0;
struct std_stdResult__cgs__1;
struct std_stdResult__cgs__2;
struct std_stdResult__cgs__3;
struct std_stdvector__cgs__0;
struct std_stdvector__cgs__1;
struct std_stdOption__cgs__0;
struct std_stdResult__cgs__4;
struct std_stdResult__cgs__5;
struct std_stdResult__cgs__6;
struct std_stdResult__cgs__7;
struct std_stdResult__cgs__8;
struct std_stdResult__cgs__9;
struct std_stdResult__cgs__10;
struct std_stdResult__cgs__11;
struct std_stdResult__cgs__12;
struct std_stdResult__cgs__13;
struct std_stdResult__cgs__14;
struct std_stdResult__cgs__15;
struct std_stdvector__cgs__2;
struct std_stdResult__cgs__16;
struct std_stdResult__cgs__17;
struct std_stdResult__cgs__18;
struct std_stdResult__cgs__19;
struct std_stdResult__cgs__20;
struct std_stdspan__cgs__0;
struct std_stdvector__cgs__3;
struct std_stdResult__cgs__21;
struct std_stdResult__cgs__22;
struct std_stdResult__cgs__23;
struct std_stdResult__cgs__24;
struct std_stdResult__cgs__25;
struct std_stdpair__cgs__0;
struct std_stdResult__cgs__26;
struct std_stdpair__cgs__1;
struct std_stdResult__cgs__27;
struct std_stdOption__cgs__1;
/** Declare:Generics std **/
struct std_stdallocAllocError {
	int __chx__vt_621827;
	union {
		struct {
		} OutOfMemory;
	};
};
struct std_stdResult__cgs__0 {
	int __chx__vt_621827;
	union {
		struct {
			void* value;
		} Ok;
		struct {
			struct std_stdallocAllocError error;
		} Err;
	};
};
struct std_stdUnit {
};
struct std_stdSerializationError {
	int kind;
	struct std_stdstring message;
};
struct std_stdResult__cgs__1 {
	int __chx__vt_621827;
	union {
		struct {
			struct std_stdUnit value;
		} Ok;
		struct {
			struct std_stdSerializationError error;
		} Err;
	};
};
void std_stdResult__cgs__1delete(struct std_stdResult__cgs__1* self);
struct std_stdResult__cgs__2 {
	int __chx__vt_621827;
	union {
		struct {
			_Bool value;
		} Ok;
		struct {
			struct std_stdSerializationError error;
		} Err;
	};
};
void std_stdResult__cgs__2delete(struct std_stdResult__cgs__2* self);
struct std_stdstring_view {
	const char* _data;
	size_t _size;
};
struct std_stdResult__cgs__3 {
	int __chx__vt_621827;
	union {
		struct {
			int8_t value;
		} Ok;
		struct {
			struct std_stdstring_view error;
		} Err;
	};
};
struct std_stdvector__cgs__0 {
	struct std_stdconcurrentTask* data_ptr;
	size_t data_size;
	size_t data_cap;
};
void std_stdvector__cgs__0make(struct std_stdvector__cgs__0* this);
void std_stdvector__cgs__0make_with_capacity(struct std_stdvector__cgs__0* this, size_t init_cap);
void std_stdvector__cgs__0resize(struct std_stdvector__cgs__0* self, size_t new_size);
void std_stdvector__cgs__0reserve(struct std_stdvector__cgs__0* self, size_t new_cap);
void std_stdvector__cgs__0ensure_capacity_for_one_more(struct std_stdvector__cgs__0* self);
void std_stdvector__cgs__0push(struct std_stdvector__cgs__0* self, struct std_stdconcurrentTask* value);
void std_stdvector__cgs__0push_back(struct std_stdvector__cgs__0* self, struct std_stdconcurrentTask* value);
void std_stdvector__cgs__0get(struct std_stdconcurrentTask* __chx_struct_ret_param_xx, struct std_stdvector__cgs__0*const self, size_t index);
struct std_stdconcurrentTask* std_stdvector__cgs__0get_ptr(struct std_stdvector__cgs__0*const self, size_t index);
struct std_stdconcurrentTask* std_stdvector__cgs__0get_ref(struct std_stdvector__cgs__0*const self, size_t index);
void std_stdvector__cgs__0set(struct std_stdvector__cgs__0* self, size_t index, struct std_stdconcurrentTask* value);
size_t std_stdvector__cgs__0size(struct std_stdvector__cgs__0*const self);
size_t std_stdvector__cgs__0capacity(struct std_stdvector__cgs__0*const self);
const struct std_stdconcurrentTask* std_stdvector__cgs__0data(struct std_stdvector__cgs__0*const self);
struct std_stdconcurrentTask* std_stdvector__cgs__0last_ptr(struct std_stdvector__cgs__0*const self);
void std_stdvector__cgs__0remove(struct std_stdvector__cgs__0* self, size_t index);
void std_stdvector__cgs__0erase(struct std_stdvector__cgs__0* self, size_t index);
void std_stdvector__cgs__0remove_last(struct std_stdvector__cgs__0* self);
void std_stdvector__cgs__0pop_back(struct std_stdvector__cgs__0* self);
void std_stdvector__cgs__0take_last(struct std_stdconcurrentTask* __chx_struct_ret_param_xx, struct std_stdvector__cgs__0* self);
_Bool std_stdvector__cgs__0empty(struct std_stdvector__cgs__0*const self);
void std_stdvector__cgs__0clear(struct std_stdvector__cgs__0* self);
void std_stdvector__cgs__0set_len(struct std_stdvector__cgs__0* self, size_t new_size);
void std_stdvector__cgs__0delete(struct std_stdvector__cgs__0* self);
struct std_stdvector__cgs__1 {
	struct std_stdstring_view* data_ptr;
	size_t data_size;
	size_t data_cap;
};
void std_stdvector__cgs__1make(struct std_stdvector__cgs__1* this);
void std_stdvector__cgs__1make_with_capacity(struct std_stdvector__cgs__1* this, size_t init_cap);
void std_stdvector__cgs__1resize(struct std_stdvector__cgs__1* self, size_t new_size);
void std_stdvector__cgs__1reserve(struct std_stdvector__cgs__1* self, size_t new_cap);
void std_stdvector__cgs__1ensure_capacity_for_one_more(struct std_stdvector__cgs__1* self);
void std_stdvector__cgs__1push(struct std_stdvector__cgs__1* self, struct std_stdstring_view* value);
void std_stdvector__cgs__1push_back(struct std_stdvector__cgs__1* self, struct std_stdstring_view* value);
void std_stdvector__cgs__1get(struct std_stdstring_view* __chx_struct_ret_param_xx, struct std_stdvector__cgs__1*const self, size_t index);
struct std_stdstring_view* std_stdvector__cgs__1get_ptr(struct std_stdvector__cgs__1*const self, size_t index);
struct std_stdstring_view* std_stdvector__cgs__1get_ref(struct std_stdvector__cgs__1*const self, size_t index);
void std_stdvector__cgs__1set(struct std_stdvector__cgs__1* self, size_t index, struct std_stdstring_view* value);
size_t std_stdvector__cgs__1size(struct std_stdvector__cgs__1*const self);
size_t std_stdvector__cgs__1capacity(struct std_stdvector__cgs__1*const self);
const struct std_stdstring_view* std_stdvector__cgs__1data(struct std_stdvector__cgs__1*const self);
struct std_stdstring_view* std_stdvector__cgs__1last_ptr(struct std_stdvector__cgs__1*const self);
void std_stdvector__cgs__1remove(struct std_stdvector__cgs__1* self, size_t index);
void std_stdvector__cgs__1erase(struct std_stdvector__cgs__1* self, size_t index);
void std_stdvector__cgs__1remove_last(struct std_stdvector__cgs__1* self);
void std_stdvector__cgs__1pop_back(struct std_stdvector__cgs__1* self);
void std_stdvector__cgs__1take_last(struct std_stdstring_view* __chx_struct_ret_param_xx, struct std_stdvector__cgs__1* self);
_Bool std_stdvector__cgs__1empty(struct std_stdvector__cgs__1*const self);
void std_stdvector__cgs__1clear(struct std_stdvector__cgs__1* self);
void std_stdvector__cgs__1set_len(struct std_stdvector__cgs__1* self, size_t new_size);
void std_stdvector__cgs__1delete(struct std_stdvector__cgs__1* self);
struct std_stdOption__cgs__0 {
	int __chx__vt_621827;
	union {
		struct {
			struct std_stdstring value;
		} Some;
		struct {
		} None;
	};
};
void std_stdOption__cgs__0take(struct std_stdstring* __chx_struct_ret_param_xx, struct std_stdOption__cgs__0*const self);
void std_stdOption__cgs__0delete(struct std_stdOption__cgs__0* self);
struct std_stdResult__cgs__4 {
	int __chx__vt_621827;
	union {
		struct {
			char value;
		} Ok;
		struct {
			struct std_stdSerializationError error;
		} Err;
	};
};
void std_stdResult__cgs__4delete(struct std_stdResult__cgs__4* self);
struct std_stdResult__cgs__5 {
	int __chx__vt_621827;
	union {
		struct {
			int16_t value;
		} Ok;
		struct {
			struct std_stdstring_view error;
		} Err;
	};
};
typedef struct __chx_core_coreiterableLinear__cgs__0_vt_t {
	const struct std_stdstring_view*(*data)(void* self);uint64_t(*size)(void* self);
} __chx_core_coreiterableLinear__cgs__0_vt_t;
struct std_stdResult__cgs__6 {
	int __chx__vt_621827;
	union {
		struct {
			uint64_t value;
		} Ok;
		struct {
			struct std_stdSerializationError error;
		} Err;
	};
};
void std_stdResult__cgs__6delete(struct std_stdResult__cgs__6* self);
typedef struct __chx_core_coreiterableLinear__cgs__1_vt_t {
	const struct std_stdconcurrentTask*(*data)(void* self);uint64_t(*size)(void* self);
} __chx_core_coreiterableLinear__cgs__1_vt_t;
struct std_stdResult__cgs__7 {
	int __chx__vt_621827;
	union {
		struct {
			int32_t value;
		} Ok;
		struct {
			struct std_stdstring_view error;
		} Err;
	};
};
struct std_stdResult__cgs__8 {
	int __chx__vt_621827;
	union {
		struct {
			int64_t value;
		} Ok;
		struct {
			struct std_stdstring_view error;
		} Err;
	};
};
struct std_stdResult__cgs__9 {
	int __chx__vt_621827;
	union {
		struct {
			uint8_t value;
		} Ok;
		struct {
			struct std_stdstring_view error;
		} Err;
	};
};
struct std_stdResult__cgs__10 {
	int __chx__vt_621827;
	union {
		struct {
			int64_t value;
		} Ok;
		struct {
			struct std_stdSerializationError error;
		} Err;
	};
};
void std_stdResult__cgs__10delete(struct std_stdResult__cgs__10* self);
struct std_stdResult__cgs__11 {
	int __chx__vt_621827;
	union {
		struct {
			uint16_t value;
		} Ok;
		struct {
			struct std_stdstring_view error;
		} Err;
	};
};
struct std_stdResult__cgs__12 {
	int __chx__vt_621827;
	union {
		struct {
			uint32_t value;
		} Ok;
		struct {
			struct std_stdstring_view error;
		} Err;
	};
};
struct std_stdResult__cgs__13 {
	int __chx__vt_621827;
	union {
		struct {
			double value;
		} Ok;
		struct {
			struct std_stdSerializationError error;
		} Err;
	};
};
void std_stdResult__cgs__13delete(struct std_stdResult__cgs__13* self);
struct std_stdResult__cgs__14 {
	int __chx__vt_621827;
	union {
		struct {
			uint64_t value;
		} Ok;
		struct {
			struct std_stdstring_view error;
		} Err;
	};
};
struct std_stdResult__cgs__15 {
	int __chx__vt_621827;
	union {
		struct {
			int value;
		} Ok;
		struct {
			struct std_stdstring_view error;
		} Err;
	};
};
struct std_stdvector__cgs__2 {
	cstd_usize* data_ptr;
	size_t data_size;
	size_t data_cap;
};
void std_stdvector__cgs__2make(struct std_stdvector__cgs__2* this);
void std_stdvector__cgs__2make_with_capacity(struct std_stdvector__cgs__2* this, size_t init_cap);
void std_stdvector__cgs__2resize(struct std_stdvector__cgs__2* self, size_t new_size);
void std_stdvector__cgs__2reserve(struct std_stdvector__cgs__2* self, size_t new_cap);
void std_stdvector__cgs__2ensure_capacity_for_one_more(struct std_stdvector__cgs__2* self);
void std_stdvector__cgs__2push(struct std_stdvector__cgs__2* self, cstd_usize value);
void std_stdvector__cgs__2push_back(struct std_stdvector__cgs__2* self, cstd_usize value);
cstd_usize std_stdvector__cgs__2get(struct std_stdvector__cgs__2*const self, size_t index);
cstd_usize* std_stdvector__cgs__2get_ptr(struct std_stdvector__cgs__2*const self, size_t index);
cstd_usize* std_stdvector__cgs__2get_ref(struct std_stdvector__cgs__2*const self, size_t index);
void std_stdvector__cgs__2set(struct std_stdvector__cgs__2* self, size_t index, cstd_usize value);
size_t std_stdvector__cgs__2size(struct std_stdvector__cgs__2*const self);
size_t std_stdvector__cgs__2capacity(struct std_stdvector__cgs__2*const self);
const cstd_usize* std_stdvector__cgs__2data(struct std_stdvector__cgs__2*const self);
cstd_usize* std_stdvector__cgs__2last_ptr(struct std_stdvector__cgs__2*const self);
void std_stdvector__cgs__2remove(struct std_stdvector__cgs__2* self, size_t index);
void std_stdvector__cgs__2erase(struct std_stdvector__cgs__2* self, size_t index);
void std_stdvector__cgs__2remove_last(struct std_stdvector__cgs__2* self);
void std_stdvector__cgs__2pop_back(struct std_stdvector__cgs__2* self);
cstd_usize std_stdvector__cgs__2take_last(struct std_stdvector__cgs__2* self);
_Bool std_stdvector__cgs__2empty(struct std_stdvector__cgs__2*const self);
void std_stdvector__cgs__2clear(struct std_stdvector__cgs__2* self);
void std_stdvector__cgs__2set_len(struct std_stdvector__cgs__2* self, size_t new_size);
void std_stdvector__cgs__2delete(struct std_stdvector__cgs__2* self);
struct std_stdResult__cgs__16 {
	int __chx__vt_621827;
	union {
		struct {
			unsigned int value;
		} Ok;
		struct {
			struct std_stdstring_view error;
		} Err;
	};
};
struct std_stdResult__cgs__17 {
	int __chx__vt_621827;
	union {
		struct {
			float value;
		} Ok;
		struct {
			struct std_stdstring_view error;
		} Err;
	};
};
struct std_stdResult__cgs__18 {
	int __chx__vt_621827;
	union {
		struct {
			float value;
		} Ok;
		struct {
			struct std_stdSerializationError error;
		} Err;
	};
};
void std_stdResult__cgs__18delete(struct std_stdResult__cgs__18* self);
struct std_stdResult__cgs__19 {
	int __chx__vt_621827;
	union {
		struct {
			struct std_stdstring_view value;
		} Ok;
		struct {
			struct std_stdSerializationError error;
		} Err;
	};
};
void std_stdResult__cgs__19delete(struct std_stdResult__cgs__19* self);
typedef struct __chx_core_coreiterableLinear__cgs__2_vt_t {
	const cstd_usize*(*data)(void* self);uint64_t(*size)(void* self);
} __chx_core_coreiterableLinear__cgs__2_vt_t;
struct std_stdResult__cgs__20 {
	int __chx__vt_621827;
	union {
		struct {
			double value;
		} Ok;
		struct {
			struct std_stdstring_view error;
		} Err;
	};
};
struct std_stdspan__cgs__0 {
	const uint8_t* _data;
	size_t _size;
};;;
void std_stdspan__cgs__0constructor(struct std_stdspan__cgs__0* this, const uint8_t* array_ptr, size_t array_size);
void std_stdspan__cgs__0empty_make(struct std_stdspan__cgs__0* this);
const uint8_t* std_stdspan__cgs__0data(struct std_stdspan__cgs__0*const self);
const uint8_t* std_stdspan__cgs__0get(struct std_stdspan__cgs__0*const self, size_t loc);
size_t std_stdspan__cgs__0size(struct std_stdspan__cgs__0*const self);
_Bool std_stdspan__cgs__0empty(struct std_stdspan__cgs__0*const self);
struct std_stdvector__cgs__3 {
	uint8_t* data_ptr;
	size_t data_size;
	size_t data_cap;
};
void std_stdvector__cgs__3make(struct std_stdvector__cgs__3* this);
void std_stdvector__cgs__3make_with_capacity(struct std_stdvector__cgs__3* this, size_t init_cap);
void std_stdvector__cgs__3resize(struct std_stdvector__cgs__3* self, size_t new_size);
void std_stdvector__cgs__3reserve(struct std_stdvector__cgs__3* self, size_t new_cap);
void std_stdvector__cgs__3ensure_capacity_for_one_more(struct std_stdvector__cgs__3* self);
void std_stdvector__cgs__3push(struct std_stdvector__cgs__3* self, uint8_t value);
void std_stdvector__cgs__3push_back(struct std_stdvector__cgs__3* self, uint8_t value);
uint8_t std_stdvector__cgs__3get(struct std_stdvector__cgs__3*const self, size_t index);
uint8_t* std_stdvector__cgs__3get_ptr(struct std_stdvector__cgs__3*const self, size_t index);
uint8_t* std_stdvector__cgs__3get_ref(struct std_stdvector__cgs__3*const self, size_t index);
void std_stdvector__cgs__3set(struct std_stdvector__cgs__3* self, size_t index, uint8_t value);
size_t std_stdvector__cgs__3size(struct std_stdvector__cgs__3*const self);
size_t std_stdvector__cgs__3capacity(struct std_stdvector__cgs__3*const self);
const uint8_t* std_stdvector__cgs__3data(struct std_stdvector__cgs__3*const self);
uint8_t* std_stdvector__cgs__3last_ptr(struct std_stdvector__cgs__3*const self);
void std_stdvector__cgs__3remove(struct std_stdvector__cgs__3* self, size_t index);
void std_stdvector__cgs__3erase(struct std_stdvector__cgs__3* self, size_t index);
void std_stdvector__cgs__3remove_last(struct std_stdvector__cgs__3* self);
void std_stdvector__cgs__3pop_back(struct std_stdvector__cgs__3* self);
uint8_t std_stdvector__cgs__3take_last(struct std_stdvector__cgs__3* self);
_Bool std_stdvector__cgs__3empty(struct std_stdvector__cgs__3*const self);
void std_stdvector__cgs__3clear(struct std_stdvector__cgs__3* self);
void std_stdvector__cgs__3set_len(struct std_stdvector__cgs__3* self, size_t new_size);
void std_stdvector__cgs__3delete(struct std_stdvector__cgs__3* self);
typedef struct __chx_core_coreiterableLinear__cgs__3_vt_t {
	const uint8_t*(*data)(void* self);uint64_t(*size)(void* self);
} __chx_core_coreiterableLinear__cgs__3_vt_t;
struct std_stdResult__cgs__21 {
	int __chx__vt_621827;
	union {
		struct {
			struct std_stdspan__cgs__0 value;
		} Ok;
		struct {
			struct std_stdSerializationError error;
		} Err;
	};
};
void std_stdResult__cgs__21delete(struct std_stdResult__cgs__21* self);
struct std_stdResult__cgs__22 {
	int __chx__vt_621827;
	union {
		struct {
			void* value;
		} Ok;
		struct {
			struct std_stdSerializationError error;
		} Err;
	};
};
void std_stdResult__cgs__22delete(struct std_stdResult__cgs__22* self);
struct std_stdResult__cgs__23 {
	int __chx__vt_621827;
	union {
		struct {
			void* value;
		} Ok;
		struct {
			struct std_stdSerializationError error;
		} Err;
	};
};
void std_stdResult__cgs__23delete(struct std_stdResult__cgs__23* self);
struct std_stdResult__cgs__24 {
	int __chx__vt_621827;
	union {
		struct {
			void* value;
		} Ok;
		struct {
			struct std_stdSerializationError error;
		} Err;
	};
};
void std_stdResult__cgs__24delete(struct std_stdResult__cgs__24* self);
struct std_stdResult__cgs__25 {
	int __chx__vt_621827;
	union {
		struct {
			void* value;
		} Ok;
		struct {
			struct std_stdSerializationError error;
		} Err;
	};
};
void std_stdResult__cgs__25delete(struct std_stdResult__cgs__25* self);
struct std_stdpair__cgs__0 {
	struct std_stdstring_view first;
	void* second;
};
struct std_stdResult__cgs__26 {
	int __chx__vt_621827;
	union {
		struct {
			struct std_stdpair__cgs__0 value;
		} Ok;
		struct {
			struct std_stdSerializationError error;
		} Err;
	};
};
void std_stdResult__cgs__26delete(struct std_stdResult__cgs__26* self);
struct std_stdpair__cgs__1 {
	void* first;
	void* second;
};
struct std_stdResult__cgs__27 {
	int __chx__vt_621827;
	union {
		struct {
			struct std_stdpair__cgs__1 value;
		} Ok;
		struct {
			struct std_stdSerializationError error;
		} Err;
	};
};
void std_stdResult__cgs__27delete(struct std_stdResult__cgs__27* self);
struct std_stddefault_function_instance {
	void* fn_pointer;
	void* fn_data_ptr;
	char buffer[32];
	void(*dtor)(void* obj);
	_Bool is_heap;
};
typedef struct std_stddefault_function_instance std_stdfunction;
struct std_stdconcurrentTask {
	std_stdfunction f;
};
struct std_stdOption__cgs__1 {
	int __chx__vt_621827;
	union {
		struct {
			struct std_stdconcurrentTask value;
		} Some;
		struct {
		} None;
	};
};
void std_stdOption__cgs__1take(struct std_stdconcurrentTask* __chx_struct_ret_param_xx, struct std_stdOption__cgs__1*const self);
void std_stdreplace__cfg_0(struct std_stdconcurrentTask* __chx_struct_ret_param_xx, struct std_stdconcurrentTask* value, struct std_stdconcurrentTask* repl);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/span.ch **/
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/result.ch **/
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/panic.ch **/
struct std_Location {
	const char* filename;
	unsigned int line;
	unsigned int character;
};
void std_raw_panic(const char* message, struct std_Location* location);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/mutex.ch **/
struct std_stdlock_guard {
	struct std_stdmutex* m;
};
void std_stdlock_guardconstructor(struct std_stdlock_guard* this, struct std_stdmutex* mtx);
void std_stdlock_guarddelete(struct std_stdlock_guard* self);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/alloc.ch **/
struct std_stdallocLayout {
	size_t size;
	size_t align_;
};
void std_stdallocLayoutmake(struct std_stdallocLayout* this, size_t s, size_t a);
typedef struct __chx_std_stdallocAllocator_vt_t {
	void(*alloc)(void* self, struct std_stdResult__cgs__0* __chx_struct_ret_param_xx, struct std_stdallocLayout* layout);void(*dealloc)(void* self, void* ptr, struct std_stdallocLayout* layout);void(*grow)(void* self, struct std_stdResult__cgs__0* __chx_struct_ret_param_xx, void* old, struct std_stdallocLayout* old_layout, size_t new_size);void(*shrink)(void* self, struct std_stdResult__cgs__0* __chx_struct_ret_param_xx, void* old, struct std_stdallocLayout* old_layout, size_t new_size);
} __chx_std_stdallocAllocator_vt_t;
struct std_stdallocAllocRecord {
	void* ptr;
	size_t size;
	struct std_stdallocAllocRecord* next;
};
struct std_stdallocDebugTracker {
	struct std_stdallocAllocRecord* head;
	size_t count;
	size_t total_bytes;
};
static struct std_stdallocDebugTracker* std_stdallocdebug_tracker_create();
static void std_stdallocdebug_tracker_record_alloc(struct std_stdallocDebugTracker* tracker, void* ptr, size_t size);
static void std_stdallocdebug_tracker_record_dealloc(struct std_stdallocDebugTracker* tracker, void* ptr);
static void std_stdallocdebug_tracker_destroy(struct std_stdallocDebugTracker* tracker);
static void std_stdallocdebug_tracker_print_leaks(struct std_stdallocDebugTracker* tracker);
struct std_stdallocGlobalAllocator {
	void* _debug;
};
void std_stdallocGlobalAllocatormake(struct std_stdallocGlobalAllocator* this);
void std_stdallocGlobalAllocatoralloc(struct std_stdResult__cgs__0* __chx_struct_ret_param_xx, struct std_stdallocGlobalAllocator*const self, struct std_stdallocLayout* layout);
void std_stdallocGlobalAllocatordealloc(struct std_stdallocGlobalAllocator*const self, void* ptr, struct std_stdallocLayout* layout);
void std_stdallocGlobalAllocatorgrow(struct std_stdResult__cgs__0* __chx_struct_ret_param_xx, struct std_stdallocGlobalAllocator*const self, void* old, struct std_stdallocLayout* old_layout, size_t new_size);
void std_stdallocGlobalAllocatorshrink(struct std_stdResult__cgs__0* __chx_struct_ret_param_xx, struct std_stdallocGlobalAllocator*const self, void* old, struct std_stdallocLayout* old_layout, size_t new_size);
void std_stdallocGlobalAllocatoralloc_zeroed(struct std_stdResult__cgs__0* __chx_struct_ret_param_xx, struct std_stdallocGlobalAllocator*const self, struct std_stdallocLayout* layout);
void std_stdallocGlobalAllocatorcheck_leaks(struct std_stdallocGlobalAllocator*const self);
void std_stdallocGlobalAllocatordestroy(struct std_stdallocGlobalAllocator*const self);
void std_stdallocalloc_bytes(struct std_stdResult__cgs__0* __chx_struct_ret_param_xx, size_t size);
void std_stdallocdealloc_bytes(void* ptr);
void std_stdallocalloc_zeroed_bytes(struct std_stdResult__cgs__0* __chx_struct_ret_param_xx, size_t size);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/serialization.ch **/
void std_stdUnitmake(struct std_stdUnit* this);
void std_stdSerializationErrormake(struct std_stdSerializationError* this);
void std_stdSerializationErrordelete(struct std_stdSerializationError* self);
typedef struct __chx_std_stdDecoder_vt_t {
	void(*decode_null)(void* self, struct std_stdResult__cgs__1* __chx_struct_ret_param_xx);void(*decode_bool)(void* self, struct std_stdResult__cgs__2* __chx_struct_ret_param_xx);void(*decode_char)(void* self, struct std_stdResult__cgs__4* __chx_struct_ret_param_xx);void(*decode_u64)(void* self, struct std_stdResult__cgs__6* __chx_struct_ret_param_xx);void(*decode_i64)(void* self, struct std_stdResult__cgs__10* __chx_struct_ret_param_xx);void(*decode_double)(void* self, struct std_stdResult__cgs__13* __chx_struct_ret_param_xx);void(*decode_float)(void* self, struct std_stdResult__cgs__18* __chx_struct_ret_param_xx);void(*decode_str)(void* self, struct std_stdResult__cgs__19* __chx_struct_ret_param_xx);void(*decode_bytes)(void* self, struct std_stdResult__cgs__21* __chx_struct_ret_param_xx);void(*array)(void* self, struct std_stdResult__cgs__22* __chx_struct_ret_param_xx);void(*object)(void* self, struct std_stdResult__cgs__23* __chx_struct_ret_param_xx);void(*map)(void* self, struct std_stdResult__cgs__24* __chx_struct_ret_param_xx);
} __chx_std_stdDecoder_vt_t;
typedef struct __chx_std_stdArrayDecoder_vt_t {
	void(*item_decoder)(void* self, struct std_stdResult__cgs__25* __chx_struct_ret_param_xx);uint64_t(*total)(void* self);uint64_t(*remaining)(void* self);
} __chx_std_stdArrayDecoder_vt_t;
typedef struct __chx_std_stdObjectDecoder_vt_t {
	void(*item_decoder)(void* self, struct std_stdResult__cgs__26* __chx_struct_ret_param_xx);uint64_t(*total)(void* self);
} __chx_std_stdObjectDecoder_vt_t;
typedef struct __chx_std_stdMapDecoder_vt_t {
	void(*item_decoder)(void* self, struct std_stdResult__cgs__27* __chx_struct_ret_param_xx);uint64_t(*total)(void* self);
} __chx_std_stdMapDecoder_vt_t;
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/os_string.ch **/


typedef char std_stdnative_char_t;
typedef struct std_stdstring std_stdnative_string_t;
typedef struct std_stdstring std_stdnative_string_t;
struct std_stdOsString {
	std_stdnative_string_t data_;
};
static void std_stdOsStringmake(struct std_stdOsString* this, std_stdnative_string_t* str);
static void std_stdOsStringmake2(struct std_stdOsString* this, struct std_stdstring*const utf8);
static void std_stdOsStringfrom_utf8(struct std_stdOsString* __chx_struct_ret_param_xx, struct std_stdstring*const utf8);
static size_t std_stdOsStringsize(struct std_stdOsString*const self);
static _Bool std_stdOsStringempty(struct std_stdOsString*const self);
static std_stdnative_string_t*const std_stdOsStringnative(struct std_stdOsString*const self);
static const std_stdnative_char_t* std_stdOsStringc_str_native(struct std_stdOsString*const self);
static void std_stdOsStringclear(struct std_stdOsString* self);
static void std_stdOsStringreserve(struct std_stdOsString* self, size_t n);
static void std_stdOsStringappend_utf8(struct std_stdOsString* self, const char* data, size_t len);
static void std_stdOsStringappend_utf8_str(struct std_stdOsString* self, struct std_stdstring*const utf8);
static void std_stdOsStringappend_utf8_view(struct std_stdOsString* self, struct std_stdstring_view*const utf8);
static void std_stdOsStringappend_native(struct std_stdOsString* self, std_stdnative_string_t*const native);
static void std_stdOsStringto_utf8(struct std_stdstring* __chx_struct_ret_param_xx, struct std_stdOsString*const self);
static void std_stdOsStringdelete(struct std_stdOsString* self);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/string_view.ch **/;
void std_stdstring_viewempty_make(struct std_stdstring_view* this);
void std_stdstring_viewconstructor(struct std_stdstring_view* this, const char* value, size_t length);
void std_stdstring_viewref_make(struct std_stdstring_view* this, char*const value, size_t length);
void std_stdstring_viewmake_view(struct std_stdstring_view* this, struct std_stdstring*const str);
void std_stdstring_viewmake_no_len(struct std_stdstring_view* this, const char* value);
const char* std_stdstring_viewdata(struct std_stdstring_view*const self);
size_t std_stdstring_viewsize(struct std_stdstring_view*const self);
_Bool std_stdstring_viewempty(struct std_stdstring_view*const self);
char std_stdstring_viewget(struct std_stdstring_view*const self, size_t index);
void std_stdstring_viewsubview(struct std_stdstring_view* __chx_struct_ret_param_xx, struct std_stdstring_view*const self, size_t start, size_t end);
void std_stdstring_viewskip(struct std_stdstring_view* __chx_struct_ret_param_xx, struct std_stdstring_view*const self, size_t count);
size_t std_stdstring_viewfind(struct std_stdstring_view*const self, struct std_stdstring_view*const needle);
size_t std_stdstring_viewfind_last(struct std_stdstring_view*const self, struct std_stdstring_view*const needle);
_Bool std_stdstring_viewcontains(struct std_stdstring_view*const self, struct std_stdstring_view*const needle);
_Bool std_stdstring_viewends_with(struct std_stdstring_view*const self, struct std_stdstring_view*const other);
_Bool std_stdstring_viewstarts_with(struct std_stdstring_view*const self, struct std_stdstring_view*const other);
void std_stdstring_viewtrim(struct std_stdstring_view* __chx_struct_ret_param_xx, struct std_stdstring_view*const self);
void std_stdstring_viewto_i8(struct std_stdResult__cgs__3* __chx_struct_ret_param_xx, struct std_stdstring_view*const self);
void std_stdstring_viewto_i16(struct std_stdResult__cgs__5* __chx_struct_ret_param_xx, struct std_stdstring_view*const self);
void std_stdstring_viewto_i32(struct std_stdResult__cgs__7* __chx_struct_ret_param_xx, struct std_stdstring_view*const self);
void std_stdstring_viewto_i64(struct std_stdResult__cgs__8* __chx_struct_ret_param_xx, struct std_stdstring_view*const self);
void std_stdstring_viewto_u8(struct std_stdResult__cgs__9* __chx_struct_ret_param_xx, struct std_stdstring_view*const self);
void std_stdstring_viewto_u16(struct std_stdResult__cgs__11* __chx_struct_ret_param_xx, struct std_stdstring_view*const self);
void std_stdstring_viewto_u32(struct std_stdResult__cgs__12* __chx_struct_ret_param_xx, struct std_stdstring_view*const self);
void std_stdstring_viewto_u64(struct std_stdResult__cgs__14* __chx_struct_ret_param_xx, struct std_stdstring_view*const self);
void std_stdstring_viewto_int(struct std_stdResult__cgs__15* __chx_struct_ret_param_xx, struct std_stdstring_view*const self);
void std_stdstring_viewto_uint(struct std_stdResult__cgs__16* __chx_struct_ret_param_xx, struct std_stdstring_view*const self);
void std_stdstring_viewto_float(struct std_stdResult__cgs__17* __chx_struct_ret_param_xx, struct std_stdstring_view*const self);
void std_stdstring_viewto_double(struct std_stdResult__cgs__20* __chx_struct_ret_param_xx, struct std_stdstring_view*const self);
void std_stdstring_viewsplit(struct std_stdvector__cgs__1* __chx_struct_ret_param_xx, struct std_stdstring_view*const self, char delim);
void std_stdstring_viewto_string(struct std_stdstring* __chx_struct_ret_param_xx, struct std_stdstring_view*const self);
void std_stdstring_viewstream(struct std_stdstring_view*const self, struct std_stdStringStream* s);
static unsigned int std_Hashable_string_view_hash(struct std_stdstring_view*const self);
typedef struct __chx_std_Hashable_vt_t {
	unsigned int(*hash)(void* self);
} __chx_std_Hashable_vt_t;
extern const __chx_std_Hashable_vt_t std_Hashablestd_stdstring_view;
static _Bool std_Eq_string_view_equals(struct std_stdstring_view*const self, struct std_stdstring_view*const other);
typedef struct __chx_std_Eq_vt_t {
	_Bool(*equals)(void* self, void**const other);
} __chx_std_Eq_vt_t;
extern const __chx_std_Eq_vt_t std_Eqstd_stdstring_view;
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/unordered_map2.ch **/

/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/deque.ch **/

/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/hashing/fnv1.ch **/
unsigned int std_fnv1a_hash_32(const char* str);
size_t std_fnv1_hash_view(struct std_stdstring_view*const view);
size_t std_fnv1_hash(const char* s);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/hashing/compare_impl.ch **/
_Bool std_char_equals(char*const self, char*const other);
extern const __chx_std_Eq_vt_t std_Eq_char;
_Bool std_uchar_equals(unsigned char*const self, unsigned char*const other);
extern const __chx_std_Eq_vt_t std_Eq_uchar;
_Bool std_short_equals(short*const self, short*const other);
extern const __chx_std_Eq_vt_t std_Eq_short;
_Bool std_ushort_equals(unsigned short*const self, unsigned short*const other);
extern const __chx_std_Eq_vt_t std_Eq_ushort;
_Bool std_int_equals(int*const self, int*const other);
extern const __chx_std_Eq_vt_t std_Eq_int;
_Bool std_uint_equals(unsigned int*const self, unsigned int*const other);
extern const __chx_std_Eq_vt_t std_Eq_uint;
_Bool std_long_equals(long*const self, long*const other);
extern const __chx_std_Eq_vt_t std_Eq_long;
_Bool std_ulong_equals(unsigned long*const self, unsigned long*const other);
extern const __chx_std_Eq_vt_t std_Eq_ulong;
_Bool std_i64_equals(int64_t*const self, int64_t*const other);
extern const __chx_std_Eq_vt_t std_Eq_i64;
_Bool std_u64_equals(uint64_t*const self, uint64_t*const other);
extern const __chx_std_Eq_vt_t std_Eq_u64;
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/concurrency/threadpool.ch **/
extern int pthread_create(const cstd_usize* thread_out, const void* attr, const void* start_routine, const void* arg);
extern int pthread_join(cstd_usize thread, const void** retval);
extern int usleep(int usec);
extern long sysconf(int name);
static int std__SC_NPROCESSORS_ONLN;
cstd_usize std_stdconcurrenthardware_threads();
void std_stdconcurrentsleep_ms(unsigned long ms);
static cstd_usize std_stdconcurrentspawn_native(const void*(*entry)(const void* arg), const void* arg);
static void std_stdconcurrentjoin_native(cstd_usize h);
typedef uint64_t cstd_usize;
struct std_stdconcurrentThread {
	cstd_usize handle;
};
void std_stdconcurrentThreadjoin(struct std_stdconcurrentThread*const self);
void std_stdconcurrentspawn(struct std_stdconcurrentThread* __chx_struct_ret_param_xx, const void*(*entry)(const void* arg), const void* arg);
static const void* std_stdconcurrentworker_main(const void* arg);
struct std_stdmutex {
	uint8_t storage[40];
};
typedef struct std_stdCondVar std_stdcondvar;
struct std_stdconcurrentPoolData {
	struct std_stdmutex m;
	std_stdcondvar cv;
	struct std_stdvector__cgs__0 q;
	struct std_stdvector__cgs__2 workers;
	_Bool run;
};
void std_stdconcurrentPoolDatasubmit_void(struct std_stdconcurrentPoolData* self, std_stdfunction* f);
void std_stdconcurrentPoolDatadelete(struct std_stdconcurrentPoolData* self);
struct std_stdconcurrentThreadPool {
	struct std_stdconcurrentPoolData* data;
};
void std_stdconcurrentThreadPoolsubmit_void(struct std_stdconcurrentThreadPool*const self, std_stdfunction* f);
void std_stdconcurrentThreadPooldelete(struct std_stdconcurrentThreadPool*const self);
void std_stdconcurrentcreate_pool(struct std_stdconcurrentThreadPool* __chx_struct_ret_param_xx, unsigned int n);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/u16_string.ch **/

static uint16_t std_stdEMPTY_U16;
size_t std_stdu16_strlen(const uint16_t* ptr);
struct std_stdu16string {
	union {
		struct {
			const uint16_t* data;
			size_t length;
		} constant;
		struct {
			uint16_t* data;
			size_t length;
			size_t capacity;
		} heap;
		struct {
			uint16_t buffer[8];
			unsigned char length;
		} sso;
	} storage;
	char state;
};
void std_stdu16stringconstructor(struct std_stdu16string* this, const uint16_t* value, size_t length);
void std_stdu16stringconstructor2(struct std_stdu16string* this, const uint16_t* value, size_t length, _Bool ensure);
void std_stdu16stringempty_str(struct std_stdu16string* this);
void std_stdu16stringmake_no_len(struct std_stdu16string* this, const uint16_t* value);
size_t std_stdu16stringsize(struct std_stdu16string*const self);
_Bool std_stdu16stringempty(struct std_stdu16string*const self);
void std_stdu16stringmove_const_to_buffer(struct std_stdu16string* self);
void std_stdu16stringmove_data_to_heap(struct std_stdu16string* self, const uint16_t* from_data, size_t length, size_t capacity);
void std_stdu16stringresize_heap(struct std_stdu16string* self, size_t new_capacity);
void std_stdu16stringensure_mut(struct std_stdu16string* self, size_t length);
void std_stdu16stringreserve(struct std_stdu16string* self, size_t new_capacity);
void std_stdu16stringset(struct std_stdu16string* self, size_t index, uint16_t value);
uint16_t std_stdu16stringget(struct std_stdu16string*const self, size_t index);
void std_stdu16stringappend_with_len(struct std_stdu16string* self, const uint16_t* value, size_t len);
void std_stdu16stringappend_u16_unit(struct std_stdu16string* self, uint16_t u);
void std_stdu16stringappend_char(struct std_stdu16string* self, char c);
void std_stdu16stringappend_codepoint(struct std_stdu16string* self, uint32_t cp);
void std_stdu16stringappend_utf8_view(struct std_stdu16string* self, const char* ptr, size_t len);
void std_stdu16stringsubstr(struct std_stdu16string* __chx_struct_ret_param_xx, struct std_stdu16string*const self, size_t start, size_t len);
void std_stdu16stringto_utf8(struct std_stdstring* __chx_struct_ret_param_xx, struct std_stdu16string*const self);
int std_stdu16stringfind(struct std_stdu16string*const self, struct std_stdu16string* sub);
_Bool std_stdu16stringstarts_with(struct std_stdu16string*const self, struct std_stdu16string* prefix);
_Bool std_stdu16stringends_with(struct std_stdu16string*const self, struct std_stdu16string* suffix);
void std_stdu16stringpush_back(struct std_stdu16string* self, uint16_t u);
void std_stdu16stringpop_back(struct std_stdu16string* self);
size_t std_stdu16stringcapacity(struct std_stdu16string* self);
const uint16_t* std_stdu16stringdata(struct std_stdu16string*const self);
uint16_t* std_stdu16stringmutable_data(struct std_stdu16string* self);
void std_stdu16stringclear(struct std_stdu16string* self);
void std_stdu16stringdelete(struct std_stdu16string* self);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/hashing/hash.ch **/
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/std.ch **/
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/ordered_map.ch **/

/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/hashing/hash_impl.ch **/

size_t std_char_hash(char*const self);
extern const __chx_std_Hashable_vt_t std_Hashable_char;
size_t std_uchar_hash(unsigned char*const self);
extern const __chx_std_Hashable_vt_t std_Hashable_uchar;
size_t std_short_hash(short*const self);
extern const __chx_std_Hashable_vt_t std_Hashable_short;
size_t std_ushort_hash(unsigned short*const self);
extern const __chx_std_Hashable_vt_t std_Hashable_ushort;
size_t std_int_hash(int*const self);
extern const __chx_std_Hashable_vt_t std_Hashable_int;
size_t std_uint_hash(unsigned int*const self);
extern const __chx_std_Hashable_vt_t std_Hashable_uint;
size_t std_long_hash(long*const self);
extern const __chx_std_Hashable_vt_t std_Hashable_long;
size_t std_ulong_hash(unsigned long*const self);
extern const __chx_std_Hashable_vt_t std_Hashable_ulong;
size_t std_i64_hash(int64_t*const self);
extern const __chx_std_Hashable_vt_t std_Hashable_i64;
size_t std_u64_hash(uint64_t*const self);
extern const __chx_std_Hashable_vt_t std_Hashable_u64;
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/option.ch **/
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/string.ch **/

union std_stdu64_double_union {
	uint64_t u;
	double d;
};
static uint64_t std_stddbl_bits(double x);
static double std_stddbl_from_bits(uint64_t b);



static _Bool std_stddbl_is_nan(double x);
static _Bool std_stddbl_is_inf(double x);
static _Bool std_stddbl_is_neg(double x);
static unsigned int std_Hashable_string_hash(struct std_stdstring*const self);
extern const __chx_std_Hashable_vt_t std_Hashablestd_stdstring;
static _Bool std_Eq_string_equals(struct std_stdstring*const self, struct std_stdstring*const other);
extern const __chx_std_Eq_vt_t std_Eqstd_stdstring;;
void std_stdstringconstructor(struct std_stdstring* this, const char* value, size_t length);
void std_stdstringview_make(struct std_stdstring* this, struct std_stdstring_view*const value);
void std_stdstringview_make2(struct std_stdstring* this, struct std_stdstring_view* value);
void std_stdstringconstructor2(struct std_stdstring* this, const char* value, size_t length, _Bool ensure);
void std_stdstringempty_str(struct std_stdstring* this);
void std_stdstringmake_no_len(struct std_stdstring* this, const char* value);
void std_stdstringmake_with_char(struct std_stdstring* this, char value);
size_t std_stdstringsize(struct std_stdstring*const self);
void std_stdstringresize(struct std_stdstring* self, size_t value);
_Bool std_stdstringempty(struct std_stdstring*const self);
_Bool std_stdstringequals_with_len(struct std_stdstring*const self, const char* d, size_t l);
_Bool std_stdstringequals_view(struct std_stdstring*const self, struct std_stdstring_view*const other);
void std_stdstringmove_const_to_buffer(struct std_stdstring* self);
void std_stdstringmove_data_to_heap(struct std_stdstring* self, const char* from_data, size_t length, size_t capacity);
_Bool std_stdstringresize_heap(struct std_stdstring* self, size_t new_capacity);
void std_stdstringensure_mut(struct std_stdstring* self, size_t length);
void std_stdstringreserve(struct std_stdstring* self, size_t new_capacity);
void std_stdstringset(struct std_stdstring* self, size_t index, char value);
char std_stdstringget(struct std_stdstring*const self, size_t index);
void std_stdstringappend_with_len(struct std_stdstring* self, const char* value, size_t len);;
void std_stdstringappend_char_ptr(struct std_stdstring* self, const char* value);
void std_stdstringappend_string(struct std_stdstring* self, struct std_stdstring*const value);
void std_stdstringappend_str(struct std_stdstring* self, const struct std_stdstring* value);
void std_stdstringappend_view(struct std_stdstring* self, struct std_stdstring_view*const value);
void std_stdstringappend_uinteger(struct std_stdstring* self, uint64_t value);
void std_stdstringappend_integer(struct std_stdstring* self, int64_t value);
void std_stdstringappend_double(struct std_stdstring* self, double value, int precision);
void std_stdstringappend_float(struct std_stdstring* self, float value, int precision);
void std_stdstringcopy(struct std_stdstring* __chx_struct_ret_param_xx, struct std_stdstring*const self);
void std_stdstringsubstring(struct std_stdstring* __chx_struct_ret_param_xx, struct std_stdstring*const self, size_t start, size_t end);
void std_stdstringappend(struct std_stdstring* self, char value);
size_t std_stdstringfind(struct std_stdstring*const self, struct std_stdstring_view*const needle);
size_t std_stdstringfind_last(struct std_stdstring*const self, struct std_stdstring_view*const needle);
_Bool std_stdstringcontains(struct std_stdstring*const self, struct std_stdstring_view*const needle);
_Bool std_stdstringstarts_with(struct std_stdstring*const self, struct std_stdstring_view*const other);
void std_stdstringtrim(struct std_stdstring_view* __chx_struct_ret_param_xx, struct std_stdstring*const self);
void std_stdstringsplit(struct std_stdvector__cgs__1* __chx_struct_ret_param_xx, struct std_stdstring*const self, char delim);
_Bool std_stdstringends_with(struct std_stdstring*const self, struct std_stdstring_view*const other);
void std_stdstringerase(struct std_stdstring* self, size_t start, size_t len);
size_t std_stdstringcapacity(struct std_stdstring* self);
const char* std_stdstringdata(struct std_stdstring*const self);
const char* std_stdstringc_str(struct std_stdstring*const self);
char* std_stdstringmutable_data(struct std_stdstring* self);
void std_stdstringclear(struct std_stdstring* self);
void std_stdstringto_view(struct std_stdstring_view* __chx_struct_ret_param_xx, struct std_stdstring*const self);
void std_stdstringto_i8(struct std_stdResult__cgs__3* __chx_struct_ret_param_xx, struct std_stdstring*const self);
void std_stdstringto_i16(struct std_stdResult__cgs__5* __chx_struct_ret_param_xx, struct std_stdstring*const self);
void std_stdstringto_i32(struct std_stdResult__cgs__7* __chx_struct_ret_param_xx, struct std_stdstring*const self);
void std_stdstringto_i64(struct std_stdResult__cgs__8* __chx_struct_ret_param_xx, struct std_stdstring*const self);
void std_stdstringto_u8(struct std_stdResult__cgs__9* __chx_struct_ret_param_xx, struct std_stdstring*const self);
void std_stdstringto_u16(struct std_stdResult__cgs__11* __chx_struct_ret_param_xx, struct std_stdstring*const self);
void std_stdstringto_u32(struct std_stdResult__cgs__12* __chx_struct_ret_param_xx, struct std_stdstring*const self);
void std_stdstringto_u64(struct std_stdResult__cgs__14* __chx_struct_ret_param_xx, struct std_stdstring*const self);
void std_stdstringto_int(struct std_stdResult__cgs__15* __chx_struct_ret_param_xx, struct std_stdstring*const self);
void std_stdstringto_uint(struct std_stdResult__cgs__16* __chx_struct_ret_param_xx, struct std_stdstring*const self);
void std_stdstringto_float(struct std_stdResult__cgs__17* __chx_struct_ret_param_xx, struct std_stdstring*const self);
void std_stdstringto_double(struct std_stdResult__cgs__20* __chx_struct_ret_param_xx, struct std_stdstring*const self);
void std_stdstringstream(struct std_stdstring*const self, struct std_stdStringStream* s);
void std_stdstringdelete(struct std_stdstring* self);
struct std_stdStringStream {
	struct std_stdstring* str;
};
static void core_corestreamStream_StringStream_writeChar(struct std_stdStringStream* self, char value);
static void core_corestreamStream_StringStream_writeUChar(struct std_stdStringStream* self, unsigned char value);
static void core_corestreamStream_StringStream_writeSigned(struct std_stdStringStream* self, int64_t value);
static void core_corestreamStream_StringStream_writeUnsigned(struct std_stdStringStream* self, uint64_t value);
static void core_corestreamStream_StringStream_writeStr(struct std_stdStringStream* self, const char* value, uint64_t length);
static void core_corestreamStream_StringStream_writeStrNoLen(struct std_stdStringStream* self, const char* value);
static void core_corestreamStream_StringStream_writeFloat(struct std_stdStringStream* self, float value);
static void core_corestreamStream_StringStream_writeDouble(struct std_stdStringStream* self, double value);
extern const __chx_core_corestreamStream_vt_t core_corestreamStreamstd_stdStringStream;
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/utils.ch **/

static size_t std_stdinternal_view_find(struct std_stdstring_view*const me, struct std_stdstring_view*const needle);
static size_t std_stdinternal_view_find_last(struct std_stdstring_view*const me, struct std_stdstring_view*const needle);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/time/types.ch **/





struct std_stdchronoDuration {
	int64_t secs;
	int64_t nanos;
};
void std_stdchronoDurationinit(struct std_stdchronoDuration* this);
void std_stdchronoDurationfrom_parts(struct std_stdchronoDuration* this, int64_t s, int64_t n);
void std_stdchronoDurationfrom_secs(struct std_stdchronoDuration* __chx_struct_ret_param_xx, int64_t secs);
void std_stdchronoDurationfrom_millis(struct std_stdchronoDuration* __chx_struct_ret_param_xx, int64_t ms);
void std_stdchronoDurationfrom_micros(struct std_stdchronoDuration* __chx_struct_ret_param_xx, int64_t us);
void std_stdchronoDurationfrom_nanos(struct std_stdchronoDuration* __chx_struct_ret_param_xx, int64_t ns);
int64_t std_stdchronoDurationas_secs(struct std_stdchronoDuration*const self);
int64_t std_stdchronoDurationsubsec_nanos(struct std_stdchronoDuration*const self);
int64_t std_stdchronoDurationas_millis(struct std_stdchronoDuration*const self);
int64_t std_stdchronoDurationas_micros(struct std_stdchronoDuration*const self);
int64_t std_stdchronoDurationas_nanos(struct std_stdchronoDuration*const self);
void std_stdchronoDurationnormalize(struct std_stdchronoDuration* self);
_Bool std_stdchronoDurationequals(struct std_stdchronoDuration*const self, struct std_stdchronoDuration*const other);
int std_stdchronoDurationcmp(struct std_stdchronoDuration*const self, struct std_stdchronoDuration*const other);
void std_stdchronoDurationadd(struct std_stdchronoDuration* __chx_struct_ret_param_xx, struct std_stdchronoDuration*const self, struct std_stdchronoDuration*const other);
void std_stdchronoDurationsub(struct std_stdchronoDuration* __chx_struct_ret_param_xx, struct std_stdchronoDuration*const self, struct std_stdchronoDuration*const other);
void std_stdchronoDurationneg(struct std_stdchronoDuration* __chx_struct_ret_param_xx, struct std_stdchronoDuration*const self);
void std_stdchronoDurationmul_i64(struct std_stdchronoDuration* __chx_struct_ret_param_xx, struct std_stdchronoDuration*const self, int64_t scalar);
void std_stdchronoDurationdiv_i64(struct std_stdchronoDuration* __chx_struct_ret_param_xx, struct std_stdchronoDuration*const self, int64_t scalar);
void std_stdchronoDurationabs(struct std_stdchronoDuration* __chx_struct_ret_param_xx, struct std_stdchronoDuration*const self);
_Bool std_stdchronoDurationis_zero(struct std_stdchronoDuration*const self);
_Bool std_stdchronoDurationis_positive(struct std_stdchronoDuration*const self);
_Bool std_stdchronoDurationis_negative(struct std_stdchronoDuration*const self);









struct std_stdchronoInstant {
	int64_t secs;
	int64_t nanos;
};
void std_stdchronoInstantinit(struct std_stdchronoInstant* this);
void std_stdchronoInstantnow(struct std_stdchronoInstant* __chx_struct_ret_param_xx);
void std_stdchronoInstantduration_since(struct std_stdchronoDuration* __chx_struct_ret_param_xx, struct std_stdchronoInstant*const self, struct std_stdchronoInstant*const earlier);
void std_stdchronoInstantelapsed(struct std_stdchronoDuration* __chx_struct_ret_param_xx, struct std_stdchronoInstant*const self);
void std_stdchronoInstantduration_to(struct std_stdchronoDuration* __chx_struct_ret_param_xx, struct std_stdchronoInstant*const self, struct std_stdchronoInstant*const later);
void std_stdchronoInstantadd_duration(struct std_stdchronoInstant* __chx_struct_ret_param_xx, struct std_stdchronoInstant*const self, struct std_stdchronoDuration*const dur);
void std_stdchronoInstantsub_duration(struct std_stdchronoInstant* __chx_struct_ret_param_xx, struct std_stdchronoInstant*const self, struct std_stdchronoDuration*const dur);
_Bool std_stdchronoInstantequals(struct std_stdchronoInstant*const self, struct std_stdchronoInstant*const other);
int std_stdchronoInstantcmp(struct std_stdchronoInstant*const self, struct std_stdchronoInstant*const other);
struct std_stdchronoSystemTime {
	int64_t secs;
	int64_t nanos;
};
void std_stdchronoSystemTimeinit(struct std_stdchronoSystemTime* this);
void std_stdchronoSystemTimenow(struct std_stdchronoSystemTime* __chx_struct_ret_param_xx);
void std_stdchronoSystemTimefrom_unix_epoch(struct std_stdchronoSystemTime* __chx_struct_ret_param_xx, int64_t secs);
void std_stdchronoSystemTimefrom_unix_epoch_nanos(struct std_stdchronoSystemTime* __chx_struct_ret_param_xx, int64_t secs, int64_t nanos);
int64_t std_stdchronoSystemTimeas_unix_epoch_secs(struct std_stdchronoSystemTime*const self);
int64_t std_stdchronoSystemTimeas_unix_epoch_nanos(struct std_stdchronoSystemTime*const self);
void std_stdchronoSystemTimeduration_since(struct std_stdchronoDuration* __chx_struct_ret_param_xx, struct std_stdchronoSystemTime*const self, struct std_stdchronoSystemTime*const earlier);
void std_stdchronoSystemTimeelapsed(struct std_stdchronoDuration* __chx_struct_ret_param_xx, struct std_stdchronoSystemTime*const self);
void std_stdchronoSystemTimeadd_duration(struct std_stdchronoSystemTime* __chx_struct_ret_param_xx, struct std_stdchronoSystemTime*const self, struct std_stdchronoDuration*const dur);
void std_stdchronoSystemTimesub_duration(struct std_stdchronoSystemTime* __chx_struct_ret_param_xx, struct std_stdchronoSystemTime*const self, struct std_stdchronoDuration*const dur);
_Bool std_stdchronoSystemTimeequals(struct std_stdchronoSystemTime*const self, struct std_stdchronoSystemTime*const other);

/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/stream.ch **/
static void core_corestreamStream_CommandLineStream_writeChar(struct std_CommandLineStream*const self, char value);
static void core_corestreamStream_CommandLineStream_writeUChar(struct std_CommandLineStream*const self, unsigned char value);
static void core_corestreamStream_CommandLineStream_writeSigned(struct std_CommandLineStream*const self, int64_t value);
static void core_corestreamStream_CommandLineStream_writeUnsigned(struct std_CommandLineStream*const self, uint64_t value);
static void core_corestreamStream_CommandLineStream_writeStr(struct std_CommandLineStream*const self, const char* value, uint64_t length);
static void core_corestreamStream_CommandLineStream_writeStrNoLen(struct std_CommandLineStream*const self, const char* value);
static void core_corestreamStream_CommandLineStream_writeFloat(struct std_CommandLineStream*const self, float value);
static void core_corestreamStream_CommandLineStream_writeDouble(struct std_CommandLineStream*const self, double value);
extern const __chx_core_corestreamStream_vt_t core_corestreamStreamstd_CommandLineStream;
struct std_CommandLineStream {
};
size_t std_CommandLineStreamu64_to_chars(struct std_CommandLineStream*const self, char* out_buf, uint64_t value);
size_t std_CommandLineStreami64_to_chars(struct std_CommandLineStream*const self, char* out_buf, int64_t value);
size_t std_CommandLineStreamdouble_to_chars(struct std_CommandLineStream*const self, char* out_buf, double value, int precision);
size_t std_CommandLineStreamfloat_to_chars(struct std_CommandLineStream*const self, char* out_buf, float value, int precision);
void std_CommandLineStreammake(struct std_CommandLineStream* this);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/function.ch **/
typedef void(*std_stddestructor_type)(void* obj);;
void std_stddefault_function_instancemake2(struct std_stddefault_function_instance* this, void* ptr, void* cap, std_stddestructor_type destr, size_t size_data, size_t align_data);
void* std_stddefault_function_instanceget_fn_ptr(struct std_stddefault_function_instance*const self);
void* std_stddefault_function_instanceget_data_ptr(struct std_stddefault_function_instance*const self);
void std_stddefault_function_instancedelete(struct std_stddefault_function_instance*const self);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/env.ch **/
void std_stdget_env(struct std_stdOption__cgs__0* __chx_struct_ret_param_xx, struct std_stdstring_view*const name);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/hashing/murmur.ch **/
uint32_t std_murmurhash(const char* key, uint32_t len, uint32_t seed);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/pair.ch **/
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/vector.ch **/
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/posix/time.ch **/
int64_t std_stdnow_milli();
static int std_stdclock_id_realtime();
static int std_stdclock_id_monotonic();
static void std_stdnow_clock(int64_t* secs, int64_t* nanos, int clock_id);
static void std_stdnow_monotonic(int64_t* secs, int64_t* nanos);
static void std_stdnow_realtime(int64_t* secs, int64_t* nanos);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/posix/condvar.ch **/
extern int pthread_cond_init(uint8_t* cond, const void* attr);
extern int pthread_cond_destroy(uint8_t* cond);
extern int pthread_cond_wait(uint8_t* cond, uint8_t* mutex);
extern int pthread_cond_timedwait(uint8_t* cond, uint8_t* mutex, const struct timespec* abstime);
extern int pthread_cond_signal(uint8_t* cond);
extern int pthread_cond_broadcast(uint8_t* cond);
extern int clock_gettime(int clk_id, struct timespec* ts);



static void std_stdcompute_abstime_ms(struct timespec* out, unsigned long timeout_ms);
void std_stdCondVarconstructor(struct std_stdCondVar* this);
void std_stdCondVarwait(struct std_stdCondVar* self, struct std_stdmutex* mutex);
_Bool std_stdCondVartimed_wait(struct std_stdCondVar* self, struct std_stdmutex* mutex, unsigned long timeout_ms);
void std_stdCondVarnotify_one(struct std_stdCondVar* self);
void std_stdCondVarnotify_all(struct std_stdCondVar* self);
void std_stdCondVardelete(struct std_stdCondVar* self);
typedef struct std_stdCondVar std_stdcondvar;
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/std/posix/mutex.ch **/
extern int pthread_mutex_init(uint8_t* m, const void* attr);
extern int pthread_mutex_lock(uint8_t* m);
extern int pthread_mutex_trylock(uint8_t* m);
extern int pthread_mutex_unlock(uint8_t* m);
extern int pthread_mutex_destroy(uint8_t* m);
void std_stdmutexconstructor(struct std_stdmutex* this);
void std_stdmutexlock(struct std_stdmutex* self);
_Bool std_stdmutextry_lock(struct std_stdmutex* self);
void std_stdmutexunlock(struct std_stdmutex* self);
void std_stdmutexdelete(struct std_stdmutex* self);
/** Implement:Generics std **/
void std_stdResult__cgs__1delete(struct std_stdResult__cgs__1* self){
	__chx__dstctr_clnup_blk__:{
		switch(self->__chx__vt_621827) {
			case 0:
			break;
			case 1:
			std_stdSerializationErrordelete(&self->Err.error);
			break;
		}
	}
}
void std_stdResult__cgs__2delete(struct std_stdResult__cgs__2* self){
	__chx__dstctr_clnup_blk__:{
		switch(self->__chx__vt_621827) {
			case 0:
			break;
			case 1:
			std_stdSerializationErrordelete(&self->Err.error);
			break;
		}
	}
}
void std_stdvector__cgs__0make(struct std_stdvector__cgs__0* this){
	*this = (struct std_stdvector__cgs__0){ 
		.data_ptr = NULL, 
		.data_size = 0, 
		.data_cap = 0
	};
	return;
}
void std_stdvector__cgs__0make_with_capacity(struct std_stdvector__cgs__0* this, size_t init_cap){
	*this = (struct std_stdvector__cgs__0){ 
		.data_ptr = ((struct std_stdconcurrentTask*) malloc((sizeof(struct std_stdconcurrentTask) * init_cap))), 
		.data_size = 0, 
		.data_cap = init_cap
	};
	return;
}
void std_stdvector__cgs__0resize(struct std_stdvector__cgs__0* self, size_t new_size){
	if((new_size == self->data_size)){
		return;
	}
	if((new_size < self->data_size)){
		unsigned long to_destruct = (self->data_size - new_size);
		struct std_stdconcurrentTask* start = (self->data_ptr + new_size);
		
		self->data_size = new_size;
		return;
	}
	if((new_size > self->data_cap)){
		size_t new_cap = ((size_t) 2);
		if((self->data_cap != 0)){
			new_cap = self->data_cap;
		}
		while((new_cap < new_size)) {
			new_cap = (new_cap * 2);
		}
		std_stdvector__cgs__0reserve(self, new_cap);
	}
	size_t i = self->data_size;
	while((i < new_size)) {
		self->data_ptr[i] = ((struct std_stdconcurrentTask){0});
		i = (i + 1);
	}
	self->data_size = new_size;
}
void std_stdvector__cgs__0reserve(struct std_stdvector__cgs__0* self, size_t new_cap){
	if((new_cap <= self->data_cap)){
		return;
	}
	struct std_stdconcurrentTask* new_data = ((struct std_stdconcurrentTask*) realloc(self->data_ptr, (sizeof(struct std_stdconcurrentTask) * new_cap)));
	
	if((new_data != NULL)){
		self->data_ptr = new_data;
		self->data_cap = new_cap;
	} else {
		std_raw_panic("failed to resize vector", &(struct std_Location){ 
			.filename = "/home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/vector.ch", 
			.line = 68, 
			.character = 21
		});
	}
}
void std_stdvector__cgs__0ensure_capacity_for_one_more(struct std_stdvector__cgs__0* self){
	if((self->data_cap == 0)){
		std_stdvector__cgs__0reserve(self, 2);
	} else {
		std_stdvector__cgs__0reserve(self, (self->data_cap * 2));
	}
}
void std_stdvector__cgs__0push(struct std_stdvector__cgs__0* self, struct std_stdconcurrentTask* value){
	size_t s = self->data_size;
	if((s >= self->data_cap)){
		std_stdvector__cgs__0ensure_capacity_for_one_more(self);
	}
	memcpy(&self->data_ptr[s], value, sizeof(struct std_stdconcurrentTask));
	0;
	self->data_size = (s + 1);
}
void std_stdvector__cgs__0push_back(struct std_stdvector__cgs__0* self, struct std_stdconcurrentTask* value){
	std_stdvector__cgs__0push(self, *({  &value; }));
}
void std_stdvector__cgs__0get(struct std_stdconcurrentTask* __chx_struct_ret_param_xx, struct std_stdvector__cgs__0*const self, size_t index){
	*__chx_struct_ret_param_xx = self->data_ptr[index];
	return;
}
struct std_stdconcurrentTask* std_stdvector__cgs__0get_ptr(struct std_stdvector__cgs__0*const self, size_t index){
	return &self->data_ptr[index];
}
struct std_stdconcurrentTask* std_stdvector__cgs__0get_ref(struct std_stdvector__cgs__0*const self, size_t index){
	return &self->data_ptr[index];
}
void std_stdvector__cgs__0set(struct std_stdvector__cgs__0* self, size_t index, struct std_stdconcurrentTask* value){
	self->data_ptr[index] = ({  *value; });
}
size_t std_stdvector__cgs__0size(struct std_stdvector__cgs__0*const self){
	return self->data_size;
}
size_t std_stdvector__cgs__0capacity(struct std_stdvector__cgs__0*const self){
	return self->data_cap;
}
const struct std_stdconcurrentTask* std_stdvector__cgs__0data(struct std_stdvector__cgs__0*const self){
	return self->data_ptr;
}
struct std_stdconcurrentTask* std_stdvector__cgs__0last_ptr(struct std_stdvector__cgs__0*const self){
	return (self->data_ptr + (self->data_size - 1));
}
void std_stdvector__cgs__0remove(struct std_stdvector__cgs__0* self, size_t index){
	size_t s = self->data_size;
	unsigned long last = (s - 1);
	
	if((index == last)){
		self->data_size = last;
	} else {
		memmove(&self->data_ptr[index], &self->data_ptr[(index + 1)], (sizeof(struct std_stdconcurrentTask) * (last - index)));
		self->data_size = last;
	}
}
void std_stdvector__cgs__0erase(struct std_stdvector__cgs__0* self, size_t index){
	std_stdvector__cgs__0remove(self, index);
}
void std_stdvector__cgs__0remove_last(struct std_stdvector__cgs__0* self){
	size_t s = self->data_size;
	unsigned long last = (s - 1);
	struct std_stdconcurrentTask* ptr = std_stdvector__cgs__0get_ptr(self, last);
	
	self->data_size = last;
}
void std_stdvector__cgs__0pop_back(struct std_stdvector__cgs__0* self){
	std_stdvector__cgs__0remove_last(self);
}
void std_stdvector__cgs__0take_last(struct std_stdconcurrentTask* __chx_struct_ret_param_xx, struct std_stdvector__cgs__0* self){
	unsigned long last = (self->data_size - 1);
	self->data_size = last;
	
	*__chx_struct_ret_param_xx = *std_stdvector__cgs__0get_ptr(self, last);
	return;
}
_Bool std_stdvector__cgs__0empty(struct std_stdvector__cgs__0*const self){
	return (self->data_size == 0);
}
void std_stdvector__cgs__0clear(struct std_stdvector__cgs__0* self){
	
	self->data_size = 0;
}
void std_stdvector__cgs__0set_len(struct std_stdvector__cgs__0* self, size_t new_size){
	self->data_size = new_size;
}
void std_stdvector__cgs__0delete(struct std_stdvector__cgs__0* self){
	if((self->data_ptr != NULL)){
		
		free(self->data_ptr);
	}
	__chx__dstctr_clnup_blk__:{
	}
}
const struct std_stdconcurrentTask* core_coreiterableLinear__cgs__1_vector__cgs__0_data(struct std_stdvector__cgs__0*const self){
	return self->data_ptr;
}
size_t core_coreiterableLinear__cgs__1_vector__cgs__0_size(struct std_stdvector__cgs__0*const self){
	return self->data_size;
}
const __chx_core_coreiterableLinear__cgs__1_vt_t core_coreiterableLinear__cgs__1std_stdvector__cgs__0 = {
	(const struct std_stdconcurrentTask*(*)(void* self)) core_coreiterableLinear__cgs__1_vector__cgs__0_data,
	(uint64_t(*)(void* self)) core_coreiterableLinear__cgs__1_vector__cgs__0_size,
};
void std_stdvector__cgs__1make(struct std_stdvector__cgs__1* this){
	*this = (struct std_stdvector__cgs__1){ 
		.data_ptr = NULL, 
		.data_size = 0, 
		.data_cap = 0
	};
	return;
}
void std_stdvector__cgs__1make_with_capacity(struct std_stdvector__cgs__1* this, size_t init_cap){
	*this = (struct std_stdvector__cgs__1){ 
		.data_ptr = ((struct std_stdstring_view*) malloc((sizeof(struct std_stdstring_view) * init_cap))), 
		.data_size = 0, 
		.data_cap = init_cap
	};
	return;
}
void std_stdvector__cgs__1resize(struct std_stdvector__cgs__1* self, size_t new_size){
	if((new_size == self->data_size)){
		return;
	}
	if((new_size < self->data_size)){
		unsigned long to_destruct = (self->data_size - new_size);
		struct std_stdstring_view* start = (self->data_ptr + new_size);
		
		self->data_size = new_size;
		return;
	}
	if((new_size > self->data_cap)){
		size_t new_cap = ((size_t) 2);
		if((self->data_cap != 0)){
			new_cap = self->data_cap;
		}
		while((new_cap < new_size)) {
			new_cap = (new_cap * 2);
		}
		std_stdvector__cgs__1reserve(self, new_cap);
	}
	size_t i = self->data_size;
	while((i < new_size)) {
		self->data_ptr[i] = ((struct std_stdstring_view){0});
		i = (i + 1);
	}
	self->data_size = new_size;
}
void std_stdvector__cgs__1reserve(struct std_stdvector__cgs__1* self, size_t new_cap){
	if((new_cap <= self->data_cap)){
		return;
	}
	struct std_stdstring_view* new_data = ((struct std_stdstring_view*) realloc(self->data_ptr, (sizeof(struct std_stdstring_view) * new_cap)));
	
	if((new_data != NULL)){
		self->data_ptr = new_data;
		self->data_cap = new_cap;
	} else {
		std_raw_panic("failed to resize vector", &(struct std_Location){ 
			.filename = "/home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/vector.ch", 
			.line = 68, 
			.character = 21
		});
	}
}
void std_stdvector__cgs__1ensure_capacity_for_one_more(struct std_stdvector__cgs__1* self){
	if((self->data_cap == 0)){
		std_stdvector__cgs__1reserve(self, 2);
	} else {
		std_stdvector__cgs__1reserve(self, (self->data_cap * 2));
	}
}
void std_stdvector__cgs__1push(struct std_stdvector__cgs__1* self, struct std_stdstring_view* value){
	size_t s = self->data_size;
	if((s >= self->data_cap)){
		std_stdvector__cgs__1ensure_capacity_for_one_more(self);
	}
	memcpy(&self->data_ptr[s], value, sizeof(struct std_stdstring_view));
	0;
	self->data_size = (s + 1);
}
void std_stdvector__cgs__1push_back(struct std_stdvector__cgs__1* self, struct std_stdstring_view* value){
	std_stdvector__cgs__1push(self, *({  &value; }));
}
void std_stdvector__cgs__1get(struct std_stdstring_view* __chx_struct_ret_param_xx, struct std_stdvector__cgs__1*const self, size_t index){
	*__chx_struct_ret_param_xx = self->data_ptr[index];
	return;
}
struct std_stdstring_view* std_stdvector__cgs__1get_ptr(struct std_stdvector__cgs__1*const self, size_t index){
	return &self->data_ptr[index];
}
struct std_stdstring_view* std_stdvector__cgs__1get_ref(struct std_stdvector__cgs__1*const self, size_t index){
	return &self->data_ptr[index];
}
void std_stdvector__cgs__1set(struct std_stdvector__cgs__1* self, size_t index, struct std_stdstring_view* value){
	self->data_ptr[index] = ({  *value; });
}
size_t std_stdvector__cgs__1size(struct std_stdvector__cgs__1*const self){
	return self->data_size;
}
size_t std_stdvector__cgs__1capacity(struct std_stdvector__cgs__1*const self){
	return self->data_cap;
}
const struct std_stdstring_view* std_stdvector__cgs__1data(struct std_stdvector__cgs__1*const self){
	return self->data_ptr;
}
struct std_stdstring_view* std_stdvector__cgs__1last_ptr(struct std_stdvector__cgs__1*const self){
	return (self->data_ptr + (self->data_size - 1));
}
void std_stdvector__cgs__1remove(struct std_stdvector__cgs__1* self, size_t index){
	size_t s = self->data_size;
	unsigned long last = (s - 1);
	
	if((index == last)){
		self->data_size = last;
	} else {
		memmove(&self->data_ptr[index], &self->data_ptr[(index + 1)], (sizeof(struct std_stdstring_view) * (last - index)));
		self->data_size = last;
	}
}
void std_stdvector__cgs__1erase(struct std_stdvector__cgs__1* self, size_t index){
	std_stdvector__cgs__1remove(self, index);
}
void std_stdvector__cgs__1remove_last(struct std_stdvector__cgs__1* self){
	size_t s = self->data_size;
	unsigned long last = (s - 1);
	struct std_stdstring_view* ptr = std_stdvector__cgs__1get_ptr(self, last);
	
	self->data_size = last;
}
void std_stdvector__cgs__1pop_back(struct std_stdvector__cgs__1* self){
	std_stdvector__cgs__1remove_last(self);
}
void std_stdvector__cgs__1take_last(struct std_stdstring_view* __chx_struct_ret_param_xx, struct std_stdvector__cgs__1* self){
	unsigned long last = (self->data_size - 1);
	self->data_size = last;
	
	*__chx_struct_ret_param_xx = *std_stdvector__cgs__1get_ptr(self, last);
	return;
}
_Bool std_stdvector__cgs__1empty(struct std_stdvector__cgs__1*const self){
	return (self->data_size == 0);
}
void std_stdvector__cgs__1clear(struct std_stdvector__cgs__1* self){
	
	self->data_size = 0;
}
void std_stdvector__cgs__1set_len(struct std_stdvector__cgs__1* self, size_t new_size){
	self->data_size = new_size;
}
void std_stdvector__cgs__1delete(struct std_stdvector__cgs__1* self){
	if((self->data_ptr != NULL)){
		
		free(self->data_ptr);
	}
	__chx__dstctr_clnup_blk__:{
	}
}
const struct std_stdstring_view* core_coreiterableLinear__cgs__0_vector__cgs__1_data(struct std_stdvector__cgs__1*const self){
	return self->data_ptr;
}
size_t core_coreiterableLinear__cgs__0_vector__cgs__1_size(struct std_stdvector__cgs__1*const self){
	return self->data_size;
}
const __chx_core_coreiterableLinear__cgs__0_vt_t core_coreiterableLinear__cgs__0std_stdvector__cgs__1 = {
	(const struct std_stdstring_view*(*)(void* self)) core_coreiterableLinear__cgs__0_vector__cgs__1_data,
	(uint64_t(*)(void* self)) core_coreiterableLinear__cgs__0_vector__cgs__1_size,
};
void std_stdOption__cgs__0take(struct std_stdstring* __chx_struct_ret_param_xx, struct std_stdOption__cgs__0*const self){
	if((self->__chx__vt_621827 == 1)){
		std_raw_panic("cannot take on a option that\'s none", &(struct std_Location){ 
			.filename = "/home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/option.ch", 
			.line = 8, 
			.character = 17
		});
	}
	struct std_stdOption__cgs__0*const __chx__lv__0 = self;
	struct std_stdstring temp;
	_Bool __chx__lv__1 = true;
	memcpy(&temp, &__chx__lv__0->Some.value, sizeof(struct std_stdstring));
	({ struct std_stdOption__cgs__0* __chx__lv__2 = self; *__chx__lv__2 = (struct std_stdOption__cgs__0) { .__chx__vt_621827 = 1, 
	}; __chx__lv__2; });
	*__chx_struct_ret_param_xx = temp;
	return;
}
void std_stdOption__cgs__0delete(struct std_stdOption__cgs__0* self){
	__chx__dstctr_clnup_blk__:{
		switch(self->__chx__vt_621827) {
			case 0:
			std_stdstringdelete(&self->Some.value);
			break;
			case 1:
			break;
		}
	}
}
void std_stdResult__cgs__4delete(struct std_stdResult__cgs__4* self){
	__chx__dstctr_clnup_blk__:{
		switch(self->__chx__vt_621827) {
			case 0:
			break;
			case 1:
			std_stdSerializationErrordelete(&self->Err.error);
			break;
		}
	}
}
void std_stdResult__cgs__6delete(struct std_stdResult__cgs__6* self){
	__chx__dstctr_clnup_blk__:{
		switch(self->__chx__vt_621827) {
			case 0:
			break;
			case 1:
			std_stdSerializationErrordelete(&self->Err.error);
			break;
		}
	}
}
void std_stdResult__cgs__10delete(struct std_stdResult__cgs__10* self){
	__chx__dstctr_clnup_blk__:{
		switch(self->__chx__vt_621827) {
			case 0:
			break;
			case 1:
			std_stdSerializationErrordelete(&self->Err.error);
			break;
		}
	}
}
void std_stdResult__cgs__13delete(struct std_stdResult__cgs__13* self){
	__chx__dstctr_clnup_blk__:{
		switch(self->__chx__vt_621827) {
			case 0:
			break;
			case 1:
			std_stdSerializationErrordelete(&self->Err.error);
			break;
		}
	}
}
void std_stdvector__cgs__2make(struct std_stdvector__cgs__2* this){
	*this = (struct std_stdvector__cgs__2){ 
		.data_ptr = NULL, 
		.data_size = 0, 
		.data_cap = 0
	};
	return;
}
void std_stdvector__cgs__2make_with_capacity(struct std_stdvector__cgs__2* this, size_t init_cap){
	*this = (struct std_stdvector__cgs__2){ 
		.data_ptr = ((cstd_usize*) malloc((sizeof(cstd_usize) * init_cap))), 
		.data_size = 0, 
		.data_cap = init_cap
	};
	return;
}
void std_stdvector__cgs__2resize(struct std_stdvector__cgs__2* self, size_t new_size){
	if((new_size == self->data_size)){
		return;
	}
	if((new_size < self->data_size)){
		unsigned long to_destruct = (self->data_size - new_size);
		cstd_usize* start = (self->data_ptr + new_size);
		
		self->data_size = new_size;
		return;
	}
	if((new_size > self->data_cap)){
		size_t new_cap = ((size_t) 2);
		if((self->data_cap != 0)){
			new_cap = self->data_cap;
		}
		while((new_cap < new_size)) {
			new_cap = (new_cap * 2);
		}
		std_stdvector__cgs__2reserve(self, new_cap);
	}
	size_t i = self->data_size;
	while((i < new_size)) {
		self->data_ptr[i] = ((cstd_usize){0});
		i = (i + 1);
	}
	self->data_size = new_size;
}
void std_stdvector__cgs__2reserve(struct std_stdvector__cgs__2* self, size_t new_cap){
	if((new_cap <= self->data_cap)){
		return;
	}
	cstd_usize* new_data = ((cstd_usize*) realloc(self->data_ptr, (sizeof(cstd_usize) * new_cap)));
	
	if((new_data != NULL)){
		self->data_ptr = new_data;
		self->data_cap = new_cap;
	} else {
		std_raw_panic("failed to resize vector", &(struct std_Location){ 
			.filename = "/home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/vector.ch", 
			.line = 68, 
			.character = 21
		});
	}
}
void std_stdvector__cgs__2ensure_capacity_for_one_more(struct std_stdvector__cgs__2* self){
	if((self->data_cap == 0)){
		std_stdvector__cgs__2reserve(self, 2);
	} else {
		std_stdvector__cgs__2reserve(self, (self->data_cap * 2));
	}
}
void std_stdvector__cgs__2push(struct std_stdvector__cgs__2* self, cstd_usize value){
	size_t s = self->data_size;
	if((s >= self->data_cap)){
		std_stdvector__cgs__2ensure_capacity_for_one_more(self);
	}
	memcpy(&self->data_ptr[s], &value, sizeof(cstd_usize));
	0;
	self->data_size = (s + 1);
}
void std_stdvector__cgs__2push_back(struct std_stdvector__cgs__2* self, cstd_usize value){
	std_stdvector__cgs__2push(self, value);
}
cstd_usize std_stdvector__cgs__2get(struct std_stdvector__cgs__2*const self, size_t index){
	return self->data_ptr[index];
}
cstd_usize* std_stdvector__cgs__2get_ptr(struct std_stdvector__cgs__2*const self, size_t index){
	return &self->data_ptr[index];
}
cstd_usize* std_stdvector__cgs__2get_ref(struct std_stdvector__cgs__2*const self, size_t index){
	return &self->data_ptr[index];
}
void std_stdvector__cgs__2set(struct std_stdvector__cgs__2* self, size_t index, cstd_usize value){
	self->data_ptr[index] = ({  value; });
}
size_t std_stdvector__cgs__2size(struct std_stdvector__cgs__2*const self){
	return self->data_size;
}
size_t std_stdvector__cgs__2capacity(struct std_stdvector__cgs__2*const self){
	return self->data_cap;
}
const cstd_usize* std_stdvector__cgs__2data(struct std_stdvector__cgs__2*const self){
	return self->data_ptr;
}
cstd_usize* std_stdvector__cgs__2last_ptr(struct std_stdvector__cgs__2*const self){
	return (self->data_ptr + (self->data_size - 1));
}
void std_stdvector__cgs__2remove(struct std_stdvector__cgs__2* self, size_t index){
	size_t s = self->data_size;
	unsigned long last = (s - 1);
	
	if((index == last)){
		self->data_size = last;
	} else {
		memmove(&self->data_ptr[index], &self->data_ptr[(index + 1)], (sizeof(cstd_usize) * (last - index)));
		self->data_size = last;
	}
}
void std_stdvector__cgs__2erase(struct std_stdvector__cgs__2* self, size_t index){
	std_stdvector__cgs__2remove(self, index);
}
void std_stdvector__cgs__2remove_last(struct std_stdvector__cgs__2* self){
	size_t s = self->data_size;
	unsigned long last = (s - 1);
	cstd_usize* ptr = std_stdvector__cgs__2get_ptr(self, last);
	
	self->data_size = last;
}
void std_stdvector__cgs__2pop_back(struct std_stdvector__cgs__2* self){
	std_stdvector__cgs__2remove_last(self);
}
cstd_usize std_stdvector__cgs__2take_last(struct std_stdvector__cgs__2* self){
	unsigned long last = (self->data_size - 1);
	self->data_size = last;
	
	return *std_stdvector__cgs__2get_ptr(self, last);
}
_Bool std_stdvector__cgs__2empty(struct std_stdvector__cgs__2*const self){
	return (self->data_size == 0);
}
void std_stdvector__cgs__2clear(struct std_stdvector__cgs__2* self){
	
	self->data_size = 0;
}
void std_stdvector__cgs__2set_len(struct std_stdvector__cgs__2* self, size_t new_size){
	self->data_size = new_size;
}
void std_stdvector__cgs__2delete(struct std_stdvector__cgs__2* self){
	if((self->data_ptr != NULL)){
		
		free(self->data_ptr);
	}
	__chx__dstctr_clnup_blk__:{
	}
}
const cstd_usize* core_coreiterableLinear__cgs__2_vector__cgs__2_data(struct std_stdvector__cgs__2*const self){
	return self->data_ptr;
}
size_t core_coreiterableLinear__cgs__2_vector__cgs__2_size(struct std_stdvector__cgs__2*const self){
	return self->data_size;
}
const __chx_core_coreiterableLinear__cgs__2_vt_t core_coreiterableLinear__cgs__2std_stdvector__cgs__2 = {
	(const cstd_usize*(*)(void* self)) core_coreiterableLinear__cgs__2_vector__cgs__2_data,
	(uint64_t(*)(void* self)) core_coreiterableLinear__cgs__2_vector__cgs__2_size,
};
void std_stdResult__cgs__18delete(struct std_stdResult__cgs__18* self){
	__chx__dstctr_clnup_blk__:{
		switch(self->__chx__vt_621827) {
			case 0:
			break;
			case 1:
			std_stdSerializationErrordelete(&self->Err.error);
			break;
		}
	}
}
void std_stdResult__cgs__19delete(struct std_stdResult__cgs__19* self){
	__chx__dstctr_clnup_blk__:{
		switch(self->__chx__vt_621827) {
			case 0:
			break;
			case 1:
			std_stdSerializationErrordelete(&self->Err.error);
			break;
		}
	}
}
void std_stdspan__cgs__0constructor(struct std_stdspan__cgs__0* this, const uint8_t* array_ptr, size_t array_size){
	*this = (struct std_stdspan__cgs__0){ 
		._data = array_ptr, 
		._size = array_size
	};
	return;
}
void std_stdspan__cgs__0empty_make(struct std_stdspan__cgs__0* this){
	*this = (struct std_stdspan__cgs__0){ 
		._data = NULL, 
		._size = 0
	};
	return;
}
const uint8_t* std_stdspan__cgs__0data(struct std_stdspan__cgs__0*const self){
	return self->_data;
}
const uint8_t* std_stdspan__cgs__0get(struct std_stdspan__cgs__0*const self, size_t loc){
	return (self->_data + loc);
}
size_t std_stdspan__cgs__0size(struct std_stdspan__cgs__0*const self){
	return self->_size;
}
_Bool std_stdspan__cgs__0empty(struct std_stdspan__cgs__0*const self){
	return (self->_size == 0);
}
const uint8_t* core_coreiterableLinear__cgs__3_span__cgs__0_data(struct std_stdspan__cgs__0*const self){
	return self->_data;
}
size_t core_coreiterableLinear__cgs__3_span__cgs__0_size(struct std_stdspan__cgs__0*const self){
	return self->_size;
}
const __chx_core_coreiterableLinear__cgs__3_vt_t core_coreiterableLinear__cgs__3std_stdspan__cgs__0 = {
	(const uint8_t*(*)(void* self)) core_coreiterableLinear__cgs__3_span__cgs__0_data,
	(uint64_t(*)(void* self)) core_coreiterableLinear__cgs__3_span__cgs__0_size,
};
void std_stdvector__cgs__3make(struct std_stdvector__cgs__3* this){
	*this = (struct std_stdvector__cgs__3){ 
		.data_ptr = NULL, 
		.data_size = 0, 
		.data_cap = 0
	};
	return;
}
void std_stdvector__cgs__3make_with_capacity(struct std_stdvector__cgs__3* this, size_t init_cap){
	*this = (struct std_stdvector__cgs__3){ 
		.data_ptr = ((uint8_t*) malloc((sizeof(uint8_t) * init_cap))), 
		.data_size = 0, 
		.data_cap = init_cap
	};
	return;
}
void std_stdvector__cgs__3resize(struct std_stdvector__cgs__3* self, size_t new_size){
	if((new_size == self->data_size)){
		return;
	}
	if((new_size < self->data_size)){
		unsigned long to_destruct = (self->data_size - new_size);
		uint8_t* start = (self->data_ptr + new_size);
		
		self->data_size = new_size;
		return;
	}
	if((new_size > self->data_cap)){
		size_t new_cap = ((size_t) 2);
		if((self->data_cap != 0)){
			new_cap = self->data_cap;
		}
		while((new_cap < new_size)) {
			new_cap = (new_cap * 2);
		}
		std_stdvector__cgs__3reserve(self, new_cap);
	}
	size_t i = self->data_size;
	while((i < new_size)) {
		self->data_ptr[i] = ((uint8_t){0});
		i = (i + 1);
	}
	self->data_size = new_size;
}
void std_stdvector__cgs__3reserve(struct std_stdvector__cgs__3* self, size_t new_cap){
	if((new_cap <= self->data_cap)){
		return;
	}
	uint8_t* new_data = ((uint8_t*) realloc(self->data_ptr, (sizeof(uint8_t) * new_cap)));
	
	if((new_data != NULL)){
		self->data_ptr = new_data;
		self->data_cap = new_cap;
	} else {
		std_raw_panic("failed to resize vector", &(struct std_Location){ 
			.filename = "/home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/vector.ch", 
			.line = 68, 
			.character = 21
		});
	}
}
void std_stdvector__cgs__3ensure_capacity_for_one_more(struct std_stdvector__cgs__3* self){
	if((self->data_cap == 0)){
		std_stdvector__cgs__3reserve(self, 2);
	} else {
		std_stdvector__cgs__3reserve(self, (self->data_cap * 2));
	}
}
void std_stdvector__cgs__3push(struct std_stdvector__cgs__3* self, uint8_t value){
	size_t s = self->data_size;
	if((s >= self->data_cap)){
		std_stdvector__cgs__3ensure_capacity_for_one_more(self);
	}
	memcpy(&self->data_ptr[s], &value, sizeof(uint8_t));
	0;
	self->data_size = (s + 1);
}
void std_stdvector__cgs__3push_back(struct std_stdvector__cgs__3* self, uint8_t value){
	std_stdvector__cgs__3push(self, value);
}
uint8_t std_stdvector__cgs__3get(struct std_stdvector__cgs__3*const self, size_t index){
	return self->data_ptr[index];
}
uint8_t* std_stdvector__cgs__3get_ptr(struct std_stdvector__cgs__3*const self, size_t index){
	return &self->data_ptr[index];
}
uint8_t* std_stdvector__cgs__3get_ref(struct std_stdvector__cgs__3*const self, size_t index){
	return &self->data_ptr[index];
}
void std_stdvector__cgs__3set(struct std_stdvector__cgs__3* self, size_t index, uint8_t value){
	self->data_ptr[index] = ({  value; });
}
size_t std_stdvector__cgs__3size(struct std_stdvector__cgs__3*const self){
	return self->data_size;
}
size_t std_stdvector__cgs__3capacity(struct std_stdvector__cgs__3*const self){
	return self->data_cap;
}
const uint8_t* std_stdvector__cgs__3data(struct std_stdvector__cgs__3*const self){
	return self->data_ptr;
}
uint8_t* std_stdvector__cgs__3last_ptr(struct std_stdvector__cgs__3*const self){
	return (self->data_ptr + (self->data_size - 1));
}
void std_stdvector__cgs__3remove(struct std_stdvector__cgs__3* self, size_t index){
	size_t s = self->data_size;
	unsigned long last = (s - 1);
	
	if((index == last)){
		self->data_size = last;
	} else {
		memmove(&self->data_ptr[index], &self->data_ptr[(index + 1)], (sizeof(uint8_t) * (last - index)));
		self->data_size = last;
	}
}
void std_stdvector__cgs__3erase(struct std_stdvector__cgs__3* self, size_t index){
	std_stdvector__cgs__3remove(self, index);
}
void std_stdvector__cgs__3remove_last(struct std_stdvector__cgs__3* self){
	size_t s = self->data_size;
	unsigned long last = (s - 1);
	uint8_t* ptr = std_stdvector__cgs__3get_ptr(self, last);
	
	self->data_size = last;
}
void std_stdvector__cgs__3pop_back(struct std_stdvector__cgs__3* self){
	std_stdvector__cgs__3remove_last(self);
}
uint8_t std_stdvector__cgs__3take_last(struct std_stdvector__cgs__3* self){
	unsigned long last = (self->data_size - 1);
	self->data_size = last;
	
	return *std_stdvector__cgs__3get_ptr(self, last);
}
_Bool std_stdvector__cgs__3empty(struct std_stdvector__cgs__3*const self){
	return (self->data_size == 0);
}
void std_stdvector__cgs__3clear(struct std_stdvector__cgs__3* self){
	
	self->data_size = 0;
}
void std_stdvector__cgs__3set_len(struct std_stdvector__cgs__3* self, size_t new_size){
	self->data_size = new_size;
}
void std_stdvector__cgs__3delete(struct std_stdvector__cgs__3* self){
	if((self->data_ptr != NULL)){
		
		free(self->data_ptr);
	}
	__chx__dstctr_clnup_blk__:{
	}
}
const uint8_t* core_coreiterableLinear__cgs__3_vector__cgs__3_data(struct std_stdvector__cgs__3*const self){
	return self->data_ptr;
}
size_t core_coreiterableLinear__cgs__3_vector__cgs__3_size(struct std_stdvector__cgs__3*const self){
	return self->data_size;
}
const __chx_core_coreiterableLinear__cgs__3_vt_t core_coreiterableLinear__cgs__3std_stdvector__cgs__3 = {
	(const uint8_t*(*)(void* self)) core_coreiterableLinear__cgs__3_vector__cgs__3_data,
	(uint64_t(*)(void* self)) core_coreiterableLinear__cgs__3_vector__cgs__3_size,
};
void std_stdResult__cgs__21delete(struct std_stdResult__cgs__21* self){
	__chx__dstctr_clnup_blk__:{
		switch(self->__chx__vt_621827) {
			case 0:
			break;
			case 1:
			std_stdSerializationErrordelete(&self->Err.error);
			break;
		}
	}
}
void std_stdResult__cgs__22delete(struct std_stdResult__cgs__22* self){
	__chx__dstctr_clnup_blk__:{
		switch(self->__chx__vt_621827) {
			case 0:
			break;
			case 1:
			std_stdSerializationErrordelete(&self->Err.error);
			break;
		}
	}
}
void std_stdResult__cgs__23delete(struct std_stdResult__cgs__23* self){
	__chx__dstctr_clnup_blk__:{
		switch(self->__chx__vt_621827) {
			case 0:
			break;
			case 1:
			std_stdSerializationErrordelete(&self->Err.error);
			break;
		}
	}
}
void std_stdResult__cgs__24delete(struct std_stdResult__cgs__24* self){
	__chx__dstctr_clnup_blk__:{
		switch(self->__chx__vt_621827) {
			case 0:
			break;
			case 1:
			std_stdSerializationErrordelete(&self->Err.error);
			break;
		}
	}
}
void std_stdResult__cgs__25delete(struct std_stdResult__cgs__25* self){
	__chx__dstctr_clnup_blk__:{
		switch(self->__chx__vt_621827) {
			case 0:
			break;
			case 1:
			std_stdSerializationErrordelete(&self->Err.error);
			break;
		}
	}
}
void std_stdResult__cgs__26delete(struct std_stdResult__cgs__26* self){
	__chx__dstctr_clnup_blk__:{
		switch(self->__chx__vt_621827) {
			case 0:
			break;
			case 1:
			std_stdSerializationErrordelete(&self->Err.error);
			break;
		}
	}
}
void std_stdResult__cgs__27delete(struct std_stdResult__cgs__27* self){
	__chx__dstctr_clnup_blk__:{
		switch(self->__chx__vt_621827) {
			case 0:
			break;
			case 1:
			std_stdSerializationErrordelete(&self->Err.error);
			break;
		}
	}
}
void std_stdOption__cgs__1take(struct std_stdconcurrentTask* __chx_struct_ret_param_xx, struct std_stdOption__cgs__1*const self){
	if((self->__chx__vt_621827 == 1)){
		std_raw_panic("cannot take on a option that\'s none", &(struct std_Location){ 
			.filename = "/home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/option.ch", 
			.line = 8, 
			.character = 17
		});
	}
	struct std_stdOption__cgs__1*const __chx__lv__3 = self;
	struct std_stdconcurrentTask temp;
	memcpy(&temp, &__chx__lv__3->Some.value, sizeof(struct std_stdconcurrentTask));
	({ struct std_stdOption__cgs__1* __chx__lv__4 = self; *__chx__lv__4 = (struct std_stdOption__cgs__1) { .__chx__vt_621827 = 1, 
	}; __chx__lv__4; });
	*__chx_struct_ret_param_xx = temp;
	return;
}
void std_stdreplace__cfg_0(struct std_stdconcurrentTask* __chx_struct_ret_param_xx, struct std_stdconcurrentTask* value, struct std_stdconcurrentTask* repl){
	struct std_stdconcurrentTask temp;
	memcpy(&temp, value, sizeof(struct std_stdconcurrentTask));
	memcpy(value, repl, sizeof(struct std_stdconcurrentTask));
	0;
	*__chx_struct_ret_param_xx = temp;
	return;
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/span.ch **/
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/result.ch **/
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/panic.ch **/
void std_raw_panic(const char* message, struct std_Location* location){
	printf("panic with message \'%s\' at \'%s:%d:%d\'\n", message, location->filename, location->line, location->character);
	abort();
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/mutex.ch **/
void std_stdlock_guardconstructor(struct std_stdlock_guard* this, struct std_stdmutex* mtx){
	std_stdmutexlock(mtx);
	*this = (struct std_stdlock_guard){ 
		.m = mtx
	};
	return;
}
void std_stdlock_guarddelete(struct std_stdlock_guard* self){
	std_stdmutexunlock(self->m);
	__chx__dstctr_clnup_blk__:{
	}
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/alloc.ch **/
void std_stdallocLayoutmake(struct std_stdallocLayout* this, size_t s, size_t a){
	*this = (struct std_stdallocLayout){ 
		.size = s, 
		.align_ = a
	};
	return;
}
static struct std_stdallocDebugTracker* std_stdallocdebug_tracker_create(){
	struct std_stdallocDebugTracker* tracker = ((struct std_stdallocDebugTracker*) malloc(sizeof(struct std_stdallocDebugTracker)));
	tracker->head = NULL;
	tracker->count = 0;
	tracker->total_bytes = 0;
	return tracker;
}
static void std_stdallocdebug_tracker_record_alloc(struct std_stdallocDebugTracker* tracker, void* ptr, size_t size){
	struct std_stdallocAllocRecord* record = ((struct std_stdallocAllocRecord*) malloc(sizeof(struct std_stdallocAllocRecord)));
	record->ptr = ptr;
	record->size = size;
	record->next = tracker->head;
	tracker->head = record;
	tracker->count++;
	tracker->total_bytes += size;
}
static void std_stdallocdebug_tracker_record_dealloc(struct std_stdallocDebugTracker* tracker, void* ptr){
	struct std_stdallocAllocRecord* prev = NULL;
	struct std_stdallocAllocRecord* curr = tracker->head;
	while((curr != NULL)) {
		if((curr->ptr == ptr)){
			if((prev != NULL)){
				prev->next = curr->next;
			} else {
				tracker->head = curr->next;
			}
			tracker->total_bytes -= curr->size;
			tracker->count--;
			free(((void*) curr));
			return;
		}
		prev = curr;
		curr = curr->next;
	}
}
static void std_stdallocdebug_tracker_destroy(struct std_stdallocDebugTracker* tracker){
	struct std_stdallocAllocRecord* curr = tracker->head;
	while((curr != NULL)) {
		struct std_stdallocAllocRecord* next_node = curr->next;
		free(((void*) curr));
		curr = next_node;
	}
	free(((void*) tracker));
}
static void std_stdallocdebug_tracker_print_leaks(struct std_stdallocDebugTracker* tracker){
	if((tracker->count > 0)){
		printf("[Allocator] %zu leaks detected (%zu bytes unfreed)\n", tracker->count, tracker->total_bytes);
	} else {
		printf("[Allocator] no leaks detected\n");
	}
}
void std_stdallocGlobalAllocatormake(struct std_stdallocGlobalAllocator* this){
	struct std_stdallocGlobalAllocator a = (struct std_stdallocGlobalAllocator){ 
		._debug = NULL
	};
	_Bool __chx__lv__5 = true;
	
	a._debug = ((void*) std_stdallocdebug_tracker_create());
	*this = a;
	return;
}
void std_stdallocGlobalAllocatoralloc(struct std_stdResult__cgs__0* __chx_struct_ret_param_xx, struct std_stdallocGlobalAllocator*const self, struct std_stdallocLayout* layout){
	void* ptr = malloc(layout->size);
	if((ptr == NULL)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__0) { .__chx__vt_621827 = 1, 
			.Err.error = (struct std_stdallocAllocError) { .__chx__vt_621827 = 0, 
			}
		};
		return;
	}
	
	std_stdallocdebug_tracker_record_alloc(((struct std_stdallocDebugTracker*) self->_debug), ptr, layout->size);
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__0) { .__chx__vt_621827 = 0, 
		.Ok.value = ptr
	};
	return;
}
void std_stdallocGlobalAllocatordealloc(struct std_stdallocGlobalAllocator*const self, void* ptr, struct std_stdallocLayout* layout){
	
	std_stdallocdebug_tracker_record_dealloc(((struct std_stdallocDebugTracker*) self->_debug), ptr);
	free(ptr);
}
void std_stdallocGlobalAllocatorgrow(struct std_stdResult__cgs__0* __chx_struct_ret_param_xx, struct std_stdallocGlobalAllocator*const self, void* old, struct std_stdallocLayout* old_layout, size_t new_size){
	void* ptr = realloc(old, new_size);
	if((ptr == NULL)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__0) { .__chx__vt_621827 = 1, 
			.Err.error = (struct std_stdallocAllocError) { .__chx__vt_621827 = 0, 
			}
		};
		return;
	}
	
	struct std_stdallocDebugTracker* tracker = ((struct std_stdallocDebugTracker*) self->_debug);
	std_stdallocdebug_tracker_record_dealloc(tracker, old);
	std_stdallocdebug_tracker_record_alloc(tracker, ptr, new_size);
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__0) { .__chx__vt_621827 = 0, 
		.Ok.value = ptr
	};
	return;
}
void std_stdallocGlobalAllocatorshrink(struct std_stdResult__cgs__0* __chx_struct_ret_param_xx, struct std_stdallocGlobalAllocator*const self, void* old, struct std_stdallocLayout* old_layout, size_t new_size){
	void* ptr = realloc(old, new_size);
	if((ptr == NULL)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__0) { .__chx__vt_621827 = 1, 
			.Err.error = (struct std_stdallocAllocError) { .__chx__vt_621827 = 0, 
			}
		};
		return;
	}
	
	struct std_stdallocDebugTracker* tracker = ((struct std_stdallocDebugTracker*) self->_debug);
	std_stdallocdebug_tracker_record_dealloc(tracker, old);
	std_stdallocdebug_tracker_record_alloc(tracker, ptr, new_size);
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__0) { .__chx__vt_621827 = 0, 
		.Ok.value = ptr
	};
	return;
}
void std_stdallocGlobalAllocatoralloc_zeroed(struct std_stdResult__cgs__0* __chx_struct_ret_param_xx, struct std_stdallocGlobalAllocator*const self, struct std_stdallocLayout* layout){
	struct std_stdResult__cgs__0 result = (*({ struct std_stdResult__cgs__0 __chx__lv__6; std_stdallocGlobalAllocatoralloc(&__chx__lv__6, self, ({ struct std_stdallocLayout __chx__lv__7 = *layout; &__chx__lv__7; })); &__chx__lv__6; }));
	if((result.__chx__vt_621827 == 1)){
		*__chx_struct_ret_param_xx = result;
		return;
	}
	struct std_stdResult__cgs__0* __chx__lv__8 = &result;
	memset(__chx__lv__8->Ok.value, 0, layout->size);
	*__chx_struct_ret_param_xx = result;
	return;
}
void std_stdallocGlobalAllocatorcheck_leaks(struct std_stdallocGlobalAllocator*const self){
	
	std_stdallocdebug_tracker_print_leaks(((struct std_stdallocDebugTracker*) self->_debug));
}
void std_stdallocGlobalAllocatordestroy(struct std_stdallocGlobalAllocator*const self){
	
	std_stdallocdebug_tracker_destroy(((struct std_stdallocDebugTracker*) self->_debug));
	__chx__dstctr_clnup_blk__:{
	}
}
void std_stdallocalloc_bytes(struct std_stdResult__cgs__0* __chx_struct_ret_param_xx, size_t size){
	void* ptr = malloc(size);
	if((ptr == NULL)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__0) { .__chx__vt_621827 = 1, 
			.Err.error = (struct std_stdallocAllocError) { .__chx__vt_621827 = 0, 
			}
		};
		return;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__0) { .__chx__vt_621827 = 0, 
		.Ok.value = ptr
	};
	return;
}
void std_stdallocdealloc_bytes(void* ptr){
	free(ptr);
}
void std_stdallocalloc_zeroed_bytes(struct std_stdResult__cgs__0* __chx_struct_ret_param_xx, size_t size){
	struct std_stdResult__cgs__0 result = (*({ struct std_stdResult__cgs__0 __chx__lv__9; std_stdallocalloc_bytes(&__chx__lv__9, size); &__chx__lv__9; }));
	if((result.__chx__vt_621827 == 1)){
		*__chx_struct_ret_param_xx = result;
		return;
	}
	struct std_stdResult__cgs__0* __chx__lv__10 = &result;
	memset(__chx__lv__10->Ok.value, 0, size);
	*__chx_struct_ret_param_xx = result;
	return;
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/serialization.ch **/
void std_stdUnitmake(struct std_stdUnit* this){
}
void std_stdSerializationErrormake(struct std_stdSerializationError* this){
	this->kind = 0;
	std_stdstringempty_str(&this->message);
}
void std_stdSerializationErrordelete(struct std_stdSerializationError* self){
	__chx__dstctr_clnup_blk__:{
		std_stdstringdelete(&self->message);
	}
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/os_string.ch **/




static void std_stdOsStringmake(struct std_stdOsString* this, std_stdnative_string_t* str){
	_Bool __chx__lv__11 = true;
	*this = (struct std_stdOsString){ 
		.data_ = ({ __chx__lv__11 = false; *str; })
	};
	if(__chx__lv__11) {
		std_stdstringdelete(str);
	}
	return;
}
static void std_stdOsStringmake2(struct std_stdOsString* this, struct std_stdstring*const utf8){
	
	*this = (struct std_stdOsString){ 
		.data_ = (*({ struct std_stdstring __chx__lv__12; std_stdstringcopy(&__chx__lv__12, utf8); &__chx__lv__12; }))
	};
	return;
}
static void std_stdOsStringfrom_utf8(struct std_stdOsString* __chx_struct_ret_param_xx, struct std_stdstring*const utf8){
	*__chx_struct_ret_param_xx = (*({ struct std_stdOsString __chx__lv__13; std_stdOsStringmake2(&__chx__lv__13, utf8); &__chx__lv__13; }));
	return;
}
static size_t std_stdOsStringsize(struct std_stdOsString*const self){
	return std_stdstringsize(&self->data_);
}
static _Bool std_stdOsStringempty(struct std_stdOsString*const self){
	return std_stdstringempty(&self->data_);
}
static std_stdnative_string_t*const std_stdOsStringnative(struct std_stdOsString*const self){
	return &self->data_;
}
static const std_stdnative_char_t* std_stdOsStringc_str_native(struct std_stdOsString*const self){
	
	return std_stdstringdata(&self->data_);
}
static void std_stdOsStringclear(struct std_stdOsString* self){
	std_stdstringclear(&self->data_);
}
static void std_stdOsStringreserve(struct std_stdOsString* self, size_t n){
	std_stdstringreserve(&self->data_, n);
}
static void std_stdOsStringappend_utf8(struct std_stdOsString* self, const char* data, size_t len){
	
	std_stdstringappend_with_len(&self->data_, data, len);
}
static void std_stdOsStringappend_utf8_str(struct std_stdOsString* self, struct std_stdstring*const utf8){
	std_stdOsStringappend_utf8(self, std_stdstringdata(utf8), std_stdstringsize(utf8));
}
static void std_stdOsStringappend_utf8_view(struct std_stdOsString* self, struct std_stdstring_view*const utf8){
	std_stdOsStringappend_utf8(self, std_stdstring_viewdata(utf8), std_stdstring_viewsize(utf8));
}
static void std_stdOsStringappend_native(struct std_stdOsString* self, std_stdnative_string_t*const native){
	std_stdstringappend_with_len(&self->data_, std_stdstringdata(native), std_stdstringsize(native));
}
static void std_stdOsStringto_utf8(struct std_stdstring* __chx_struct_ret_param_xx, struct std_stdOsString*const self){
	
	*__chx_struct_ret_param_xx = self->data_;
	return;
}
static void std_stdOsStringdelete(struct std_stdOsString* self){
	__chx__dstctr_clnup_blk__:{
		std_stdstringdelete(&self->data_);
	}
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/string_view.ch **/
void std_stdstring_viewempty_make(struct std_stdstring_view* this){
	*this = (struct std_stdstring_view){ 
		._data = "", 
		._size = 0
	};
	return;
}
void std_stdstring_viewconstructor(struct std_stdstring_view* this, const char* value, size_t length){
	*this = (struct std_stdstring_view){ 
		._data = value, 
		._size = length
	};
	return;
}
void std_stdstring_viewref_make(struct std_stdstring_view* this, char*const value, size_t length){
	*this = (struct std_stdstring_view){ 
		._data = value, 
		._size = length
	};
	return;
}
void std_stdstring_viewmake_view(struct std_stdstring_view* this, struct std_stdstring*const str){
	*this = (struct std_stdstring_view){ 
		._data = std_stdstringdata(str), 
		._size = std_stdstringsize(str)
	};
	return;
}
void std_stdstring_viewmake_no_len(struct std_stdstring_view* this, const char* value){
	*this = (struct std_stdstring_view){ 
		._data = value, 
		._size = strlen(value)
	};
	return;
}
const char* std_stdstring_viewdata(struct std_stdstring_view*const self){
	return self->_data;
}
size_t std_stdstring_viewsize(struct std_stdstring_view*const self){
	return self->_size;
}
_Bool std_stdstring_viewempty(struct std_stdstring_view*const self){
	return (self->_size == 0);
}
char std_stdstring_viewget(struct std_stdstring_view*const self, size_t index){
	return *(self->_data + index);
}
void std_stdstring_viewsubview(struct std_stdstring_view* __chx_struct_ret_param_xx, struct std_stdstring_view*const self, size_t start, size_t end){
	*__chx_struct_ret_param_xx = (*({ struct std_stdstring_view __chx__lv__14; std_stdstring_viewconstructor(&__chx__lv__14, (self->_data + start), (end - start)); &__chx__lv__14; }));
	return;
}
void std_stdstring_viewskip(struct std_stdstring_view* __chx_struct_ret_param_xx, struct std_stdstring_view*const self, size_t count){
	*__chx_struct_ret_param_xx = (*({ struct std_stdstring_view __chx__lv__15; std_stdstring_viewconstructor(&__chx__lv__15, (self->_data + count), (self->_size - count)); &__chx__lv__15; }));
	return;
}
size_t std_stdstring_viewfind(struct std_stdstring_view*const self, struct std_stdstring_view*const needle){
	return std_stdinternal_view_find(self, needle);
}
size_t std_stdstring_viewfind_last(struct std_stdstring_view*const self, struct std_stdstring_view*const needle){
	return std_stdinternal_view_find_last(self, needle);
}
_Bool std_stdstring_viewcontains(struct std_stdstring_view*const self, struct std_stdstring_view*const needle){
	return (std_stdstring_viewfind(self, needle) != (((size_t) 0) - 1));
}
_Bool std_stdstring_viewends_with(struct std_stdstring_view*const self, struct std_stdstring_view*const other){
	if((std_stdstring_viewsize(other) > std_stdstring_viewsize(self))){
		return 0;
	}
	return (memcmp(((std_stdstring_viewdata(self) + std_stdstring_viewsize(self)) - std_stdstring_viewsize(other)), std_stdstring_viewdata(other), std_stdstring_viewsize(other)) == 0);
}
_Bool std_stdstring_viewstarts_with(struct std_stdstring_view*const self, struct std_stdstring_view*const other){
	if((std_stdstring_viewsize(other) > std_stdstring_viewsize(self))){
		return 0;
	}
	return (memcmp(std_stdstring_viewdata(self), std_stdstring_viewdata(other), std_stdstring_viewsize(other)) == 0);
}
void std_stdstring_viewtrim(struct std_stdstring_view* __chx_struct_ret_param_xx, struct std_stdstring_view*const self){
	unsigned int s = 0;
	while(((s < self->_size) && ((((std_stdstring_viewget(self, s) == ' ') || (std_stdstring_viewget(self, s) == '\t')) || (std_stdstring_viewget(self, s) == '\n')) || (std_stdstring_viewget(self, s) == '\r')))) {
		s++;
	}
	if((s == self->_size)){
		*__chx_struct_ret_param_xx = (*({ struct std_stdstring_view __chx__lv__16; std_stdstring_viewconstructor(&__chx__lv__16, "", 0); &__chx__lv__16; }));
		return;
	}
	unsigned long e = (self->_size - 1);
	while(((e > s) && ((((std_stdstring_viewget(self, e) == ' ') || (std_stdstring_viewget(self, e) == '\t')) || (std_stdstring_viewget(self, e) == '\n')) || (std_stdstring_viewget(self, e) == '\r')))) {
		e--;
	}
	*__chx_struct_ret_param_xx = (*({ struct std_stdstring_view __chx__lv__17; std_stdstring_viewsubview(&__chx__lv__17, self, s, (e + 1)); &__chx__lv__17; }));
	return;
}
void std_stdstring_viewto_i8(struct std_stdResult__cgs__3* __chx_struct_ret_param_xx, struct std_stdstring_view*const self){
	switch(((*({ struct std_stdResult__cgs__8 __chx__lv__18; std_stdstring_viewto_i64(&__chx__lv__18, self); &__chx__lv__18; }))).__chx__vt_621827) {
		case 0:{
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__3) { .__chx__vt_621827 = 0, 
				.Ok.value = ((int8_t) ((*({ struct std_stdResult__cgs__8 __chx__lv__19; std_stdstring_viewto_i64(&__chx__lv__19, self); &__chx__lv__19; }))).Ok.value)
			};
			return;
			break;
		}
		case 1:{
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__3) { .__chx__vt_621827 = 1, 
				.Err.error = ((*({ struct std_stdResult__cgs__8 __chx__lv__20; std_stdstring_viewto_i64(&__chx__lv__20, self); &__chx__lv__20; }))).Err.error
			};
			return;
			break;
		}
	}
}
void std_stdstring_viewto_i16(struct std_stdResult__cgs__5* __chx_struct_ret_param_xx, struct std_stdstring_view*const self){
	switch(((*({ struct std_stdResult__cgs__8 __chx__lv__21; std_stdstring_viewto_i64(&__chx__lv__21, self); &__chx__lv__21; }))).__chx__vt_621827) {
		case 0:{
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__5) { .__chx__vt_621827 = 0, 
				.Ok.value = ((int16_t) ((*({ struct std_stdResult__cgs__8 __chx__lv__22; std_stdstring_viewto_i64(&__chx__lv__22, self); &__chx__lv__22; }))).Ok.value)
			};
			return;
			break;
		}
		case 1:{
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__5) { .__chx__vt_621827 = 1, 
				.Err.error = ((*({ struct std_stdResult__cgs__8 __chx__lv__23; std_stdstring_viewto_i64(&__chx__lv__23, self); &__chx__lv__23; }))).Err.error
			};
			return;
			break;
		}
	}
}
void std_stdstring_viewto_i32(struct std_stdResult__cgs__7* __chx_struct_ret_param_xx, struct std_stdstring_view*const self){
	switch(((*({ struct std_stdResult__cgs__8 __chx__lv__24; std_stdstring_viewto_i64(&__chx__lv__24, self); &__chx__lv__24; }))).__chx__vt_621827) {
		case 0:{
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__7) { .__chx__vt_621827 = 0, 
				.Ok.value = ((int32_t) ((*({ struct std_stdResult__cgs__8 __chx__lv__25; std_stdstring_viewto_i64(&__chx__lv__25, self); &__chx__lv__25; }))).Ok.value)
			};
			return;
			break;
		}
		case 1:{
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__7) { .__chx__vt_621827 = 1, 
				.Err.error = ((*({ struct std_stdResult__cgs__8 __chx__lv__26; std_stdstring_viewto_i64(&__chx__lv__26, self); &__chx__lv__26; }))).Err.error
			};
			return;
			break;
		}
	}
}
void std_stdstring_viewto_i64(struct std_stdResult__cgs__8* __chx_struct_ret_param_xx, struct std_stdstring_view*const self){
	int64_t res = 0;
	int64_t sign = 1;
	size_t i = 0;
	while(((i < self->_size) && isspace(self->_data[i]))) {
		i++;
	}
	if((i == self->_size)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__8) { .__chx__vt_621827 = 1, 
			.Err.error = (*({ struct std_stdstring_view __chx__lv__27; std_stdstring_viewconstructor(&__chx__lv__27, "empty string", 12); &__chx__lv__27; }))
		};
		return;
	}
	if((self->_data[i] == '-')){
		sign = -1;
		i++;
	}else if((self->_data[i] == '+')){
		i++;
	}
	if(((i == self->_size) || !isdigit(self->_data[i]))){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__8) { .__chx__vt_621827 = 1, 
			.Err.error = (*({ struct std_stdstring_view __chx__lv__28; std_stdstring_viewconstructor(&__chx__lv__28, "invalid format", 14); &__chx__lv__28; }))
		};
		return;
	}
	while(((i < self->_size) && isdigit(self->_data[i]))) {
		res = ((res * 10) + ((int64_t) (self->_data[i] - '0')));
		i++;
	}
	while(((i < self->_size) && isspace(self->_data[i]))) {
		i++;
	}
	if((i < self->_size)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__8) { .__chx__vt_621827 = 1, 
			.Err.error = (*({ struct std_stdstring_view __chx__lv__29; std_stdstring_viewconstructor(&__chx__lv__29, "trailing characters", 19); &__chx__lv__29; }))
		};
		return;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__8) { .__chx__vt_621827 = 0, 
		.Ok.value = (res * sign)
	};
	return;
}
void std_stdstring_viewto_u8(struct std_stdResult__cgs__9* __chx_struct_ret_param_xx, struct std_stdstring_view*const self){
	switch(((*({ struct std_stdResult__cgs__14 __chx__lv__30; std_stdstring_viewto_u64(&__chx__lv__30, self); &__chx__lv__30; }))).__chx__vt_621827) {
		case 0:{
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__9) { .__chx__vt_621827 = 0, 
				.Ok.value = ((uint8_t) ((*({ struct std_stdResult__cgs__14 __chx__lv__31; std_stdstring_viewto_u64(&__chx__lv__31, self); &__chx__lv__31; }))).Ok.value)
			};
			return;
			break;
		}
		case 1:{
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__9) { .__chx__vt_621827 = 1, 
				.Err.error = ((*({ struct std_stdResult__cgs__14 __chx__lv__32; std_stdstring_viewto_u64(&__chx__lv__32, self); &__chx__lv__32; }))).Err.error
			};
			return;
			break;
		}
	}
}
void std_stdstring_viewto_u16(struct std_stdResult__cgs__11* __chx_struct_ret_param_xx, struct std_stdstring_view*const self){
	switch(((*({ struct std_stdResult__cgs__14 __chx__lv__33; std_stdstring_viewto_u64(&__chx__lv__33, self); &__chx__lv__33; }))).__chx__vt_621827) {
		case 0:{
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__11) { .__chx__vt_621827 = 0, 
				.Ok.value = ((uint16_t) ((*({ struct std_stdResult__cgs__14 __chx__lv__34; std_stdstring_viewto_u64(&__chx__lv__34, self); &__chx__lv__34; }))).Ok.value)
			};
			return;
			break;
		}
		case 1:{
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__11) { .__chx__vt_621827 = 1, 
				.Err.error = ((*({ struct std_stdResult__cgs__14 __chx__lv__35; std_stdstring_viewto_u64(&__chx__lv__35, self); &__chx__lv__35; }))).Err.error
			};
			return;
			break;
		}
	}
}
void std_stdstring_viewto_u32(struct std_stdResult__cgs__12* __chx_struct_ret_param_xx, struct std_stdstring_view*const self){
	switch(((*({ struct std_stdResult__cgs__14 __chx__lv__36; std_stdstring_viewto_u64(&__chx__lv__36, self); &__chx__lv__36; }))).__chx__vt_621827) {
		case 0:{
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__12) { .__chx__vt_621827 = 0, 
				.Ok.value = ((uint32_t) ((*({ struct std_stdResult__cgs__14 __chx__lv__37; std_stdstring_viewto_u64(&__chx__lv__37, self); &__chx__lv__37; }))).Ok.value)
			};
			return;
			break;
		}
		case 1:{
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__12) { .__chx__vt_621827 = 1, 
				.Err.error = ((*({ struct std_stdResult__cgs__14 __chx__lv__38; std_stdstring_viewto_u64(&__chx__lv__38, self); &__chx__lv__38; }))).Err.error
			};
			return;
			break;
		}
	}
}
void std_stdstring_viewto_u64(struct std_stdResult__cgs__14* __chx_struct_ret_param_xx, struct std_stdstring_view*const self){
	uint64_t res = 0;
	size_t i = 0;
	while(((i < self->_size) && isspace(self->_data[i]))) {
		i++;
	}
	if((i == self->_size)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__14) { .__chx__vt_621827 = 1, 
			.Err.error = (*({ struct std_stdstring_view __chx__lv__39; std_stdstring_viewconstructor(&__chx__lv__39, "empty string", 12); &__chx__lv__39; }))
		};
		return;
	}
	if((self->_data[i] == '+')){
		i++;
	}
	if(((i == self->_size) || !isdigit(self->_data[i]))){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__14) { .__chx__vt_621827 = 1, 
			.Err.error = (*({ struct std_stdstring_view __chx__lv__40; std_stdstring_viewconstructor(&__chx__lv__40, "invalid format", 14); &__chx__lv__40; }))
		};
		return;
	}
	while(((i < self->_size) && isdigit(self->_data[i]))) {
		res = ((res * 10) + ((uint64_t) (self->_data[i] - '0')));
		i++;
	}
	while(((i < self->_size) && isspace(self->_data[i]))) {
		i++;
	}
	if((i < self->_size)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__14) { .__chx__vt_621827 = 1, 
			.Err.error = (*({ struct std_stdstring_view __chx__lv__41; std_stdstring_viewconstructor(&__chx__lv__41, "trailing characters", 19); &__chx__lv__41; }))
		};
		return;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__14) { .__chx__vt_621827 = 0, 
		.Ok.value = res
	};
	return;
}
void std_stdstring_viewto_int(struct std_stdResult__cgs__15* __chx_struct_ret_param_xx, struct std_stdstring_view*const self){
	switch(((*({ struct std_stdResult__cgs__7 __chx__lv__42; std_stdstring_viewto_i32(&__chx__lv__42, self); &__chx__lv__42; }))).__chx__vt_621827) {
		case 0:{
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__15) { .__chx__vt_621827 = 0, 
				.Ok.value = ((int) ((*({ struct std_stdResult__cgs__7 __chx__lv__43; std_stdstring_viewto_i32(&__chx__lv__43, self); &__chx__lv__43; }))).Ok.value)
			};
			return;
			break;
		}
		case 1:{
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__15) { .__chx__vt_621827 = 1, 
				.Err.error = ((*({ struct std_stdResult__cgs__7 __chx__lv__44; std_stdstring_viewto_i32(&__chx__lv__44, self); &__chx__lv__44; }))).Err.error
			};
			return;
			break;
		}
	}
}
void std_stdstring_viewto_uint(struct std_stdResult__cgs__16* __chx_struct_ret_param_xx, struct std_stdstring_view*const self){
	switch(((*({ struct std_stdResult__cgs__12 __chx__lv__45; std_stdstring_viewto_u32(&__chx__lv__45, self); &__chx__lv__45; }))).__chx__vt_621827) {
		case 0:{
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__16) { .__chx__vt_621827 = 0, 
				.Ok.value = ((unsigned int) ((*({ struct std_stdResult__cgs__12 __chx__lv__46; std_stdstring_viewto_u32(&__chx__lv__46, self); &__chx__lv__46; }))).Ok.value)
			};
			return;
			break;
		}
		case 1:{
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__16) { .__chx__vt_621827 = 1, 
				.Err.error = ((*({ struct std_stdResult__cgs__12 __chx__lv__47; std_stdstring_viewto_u32(&__chx__lv__47, self); &__chx__lv__47; }))).Err.error
			};
			return;
			break;
		}
	}
}
void std_stdstring_viewto_float(struct std_stdResult__cgs__17* __chx_struct_ret_param_xx, struct std_stdstring_view*const self){
	if((self->_size == 0)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__17) { .__chx__vt_621827 = 1, 
			.Err.error = (*({ struct std_stdstring_view __chx__lv__48; std_stdstring_viewconstructor(&__chx__lv__48, "empty string", 12); &__chx__lv__48; }))
		};
		return;
	}
	if((self->_size < 128)){
		char buf[128];
		
		memcpy(&buf[0], self->_data, self->_size);
		buf[self->_size] = '\0';
		char* end = NULL;
		float res = ((float) strtod(((char*) &buf[0]), &end));
		if((end == ((char*) &buf[0]))){
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__17) { .__chx__vt_621827 = 1, 
				.Err.error = (*({ struct std_stdstring_view __chx__lv__49; std_stdstring_viewconstructor(&__chx__lv__49, "invalid format", 14); &__chx__lv__49; }))
			};
			return;
		}
		while((((end != NULL) && (*end != '\0')) && isspace(((int) *end)))) {
			end++;
		}
		if(((end != NULL) && (*end != '\0'))){
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__17) { .__chx__vt_621827 = 1, 
				.Err.error = (*({ struct std_stdstring_view __chx__lv__50; std_stdstring_viewconstructor(&__chx__lv__50, "trailing characters", 19); &__chx__lv__50; }))
			};
			return;
		}
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__17) { .__chx__vt_621827 = 0, 
			.Ok.value = res
		};
		return;
	}
	struct std_stdstring s = (*({ struct std_stdstring __chx__lv__51; std_stdstringview_make(&__chx__lv__51, self); &__chx__lv__51; }));
	_Bool __chx__lv__52 = true;
	*__chx_struct_ret_param_xx = (*({ struct std_stdResult__cgs__17 __chx__lv__53; std_stdstringto_float(&__chx__lv__53, &s); &__chx__lv__53; }));
	if(__chx__lv__52) {
		std_stdstringdelete(&s);
	}
	return;
}
void std_stdstring_viewto_double(struct std_stdResult__cgs__20* __chx_struct_ret_param_xx, struct std_stdstring_view*const self){
	if((self->_size == 0)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__20) { .__chx__vt_621827 = 1, 
			.Err.error = (*({ struct std_stdstring_view __chx__lv__54; std_stdstring_viewconstructor(&__chx__lv__54, "empty string", 12); &__chx__lv__54; }))
		};
		return;
	}
	if((self->_size < 128)){
		char buf[128];
		
		memcpy(&buf[0], self->_data, self->_size);
		buf[self->_size] = '\0';
		char* end = NULL;
		double res = strtod(((char*) &buf[0]), &end);
		if((end == ((char*) &buf[0]))){
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__20) { .__chx__vt_621827 = 1, 
				.Err.error = (*({ struct std_stdstring_view __chx__lv__55; std_stdstring_viewconstructor(&__chx__lv__55, "invalid format", 14); &__chx__lv__55; }))
			};
			return;
		}
		while((((end != NULL) && (*end != '\0')) && isspace(((int) *end)))) {
			end++;
		}
		if(((end != NULL) && (*end != '\0'))){
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__20) { .__chx__vt_621827 = 1, 
				.Err.error = (*({ struct std_stdstring_view __chx__lv__56; std_stdstring_viewconstructor(&__chx__lv__56, "trailing characters", 19); &__chx__lv__56; }))
			};
			return;
		}
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__20) { .__chx__vt_621827 = 0, 
			.Ok.value = res
		};
		return;
	}
	struct std_stdstring s = (*({ struct std_stdstring __chx__lv__57; std_stdstringview_make(&__chx__lv__57, self); &__chx__lv__57; }));
	_Bool __chx__lv__58 = true;
	*__chx_struct_ret_param_xx = (*({ struct std_stdResult__cgs__20 __chx__lv__59; std_stdstringto_double(&__chx__lv__59, &s); &__chx__lv__59; }));
	if(__chx__lv__58) {
		std_stdstringdelete(&s);
	}
	return;
}
void std_stdstring_viewsplit(struct std_stdvector__cgs__1* __chx_struct_ret_param_xx, struct std_stdstring_view*const self, char delim){
	struct std_stdvector__cgs__1 res = (*({ struct std_stdvector__cgs__1 __chx__lv__60; std_stdvector__cgs__1make(&__chx__lv__60); &__chx__lv__60; }));
	_Bool __chx__lv__61 = true;
	unsigned int start = 0;
	for(unsigned int i = 0;(i < self->_size);i++){
		if((std_stdstring_viewget(self, i) == delim)){
			std_stdvector__cgs__1push(&res, &(*({ struct std_stdstring_view __chx__lv__62; std_stdstring_viewsubview(&__chx__lv__62, self, start, i); &__chx__lv__62; })));
			start = (i + 1);
		}
	}
	if((start <= self->_size)){
		std_stdvector__cgs__1push(&res, &(*({ struct std_stdstring_view __chx__lv__63; std_stdstring_viewsubview(&__chx__lv__63, self, start, self->_size); &__chx__lv__63; })));
	}
	*__chx_struct_ret_param_xx = res;
	return;
}
void std_stdstring_viewto_string(struct std_stdstring* __chx_struct_ret_param_xx, struct std_stdstring_view*const self){
	*__chx_struct_ret_param_xx = (*({ struct std_stdstring __chx__lv__64; std_stdstringview_make(&__chx__lv__64, self); &__chx__lv__64; }));
	return;
}
void std_stdstring_viewstream(struct std_stdstring_view*const self, struct std_stdStringStream* s){
	core_corestreamStream_StringStream_writeStr(s, self->_data, self->_size);
}
unsigned int std_Hashable_string_view_hash(struct std_stdstring_view*const self){
	return ((unsigned int) std_fnv1_hash_view(self));
}
const __chx_std_Hashable_vt_t std_Hashablestd_stdstring_view = {
	(unsigned int(*)(void* self)) std_Hashable_string_view_hash,
};
_Bool std_Eq_string_view_equals(struct std_stdstring_view*const self, struct std_stdstring_view*const other){
	size_t self_size = self->_size;
	return ((self_size == std_stdstring_viewsize(other)) && (strncmp(std_stdstring_viewdata(self), std_stdstring_viewdata(other), self_size) == 0));
}
const __chx_std_Eq_vt_t std_Eqstd_stdstring_view = {
	(_Bool(*)(void* self, void**const other)) std_Eq_string_view_equals,
};
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/unordered_map2.ch **/

/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/deque.ch **/

/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/hashing/fnv1.ch **/
unsigned int std_fnv1a_hash_32(const char* str){
	const char* ptr = str;
	unsigned int hash = 2166136261;
	while(1) {
		char c = *ptr;
		if((c == '\0')){
			return hash;
		} else {
			hash ^= ((unsigned char) c);
			hash *= 16777619;
			ptr++;
		}
	}
	return hash;
}
size_t std_fnv1_hash_view(struct std_stdstring_view*const view){
	const char* ptr = std_stdstring_viewdata(view);
	const char* end = (ptr + std_stdstring_viewsize(view));
	size_t hash = ((size_t) 14695981039346656037);
	while((ptr != end)) {
		char c = *ptr;
		hash ^= ((size_t) c);
		hash *= 1099511628211;
		ptr++;
	}
	return hash;
}
size_t std_fnv1_hash(const char* s){
	const char* ptr = s;
	size_t hash = ((size_t) 14695981039346656037);
	while(1) {
		char c = *ptr;
		if((c == '\0')){
			return hash;
		} else {
			hash ^= ((size_t) c);
			hash *= 1099511628211;
			ptr++;
		}
	}
	return hash;
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/hashing/compare_impl.ch **/
_Bool std_char_equals(char*const self, char*const other){
	return (*self == *other);
}
const __chx_std_Eq_vt_t std_Eq_char = {
	(_Bool(*)(void* self, void**const other)) std_char_equals,
};
_Bool std_uchar_equals(unsigned char*const self, unsigned char*const other){
	return (*self == *other);
}
const __chx_std_Eq_vt_t std_Eq_uchar = {
	(_Bool(*)(void* self, void**const other)) std_uchar_equals,
};
_Bool std_short_equals(short*const self, short*const other){
	return (*self == *other);
}
const __chx_std_Eq_vt_t std_Eq_short = {
	(_Bool(*)(void* self, void**const other)) std_short_equals,
};
_Bool std_ushort_equals(unsigned short*const self, unsigned short*const other){
	return (*self == *other);
}
const __chx_std_Eq_vt_t std_Eq_ushort = {
	(_Bool(*)(void* self, void**const other)) std_ushort_equals,
};
_Bool std_int_equals(int*const self, int*const other){
	return (*self == *other);
}
const __chx_std_Eq_vt_t std_Eq_int = {
	(_Bool(*)(void* self, void**const other)) std_int_equals,
};
_Bool std_uint_equals(unsigned int*const self, unsigned int*const other){
	return (*self == *other);
}
const __chx_std_Eq_vt_t std_Eq_uint = {
	(_Bool(*)(void* self, void**const other)) std_uint_equals,
};
_Bool std_long_equals(long*const self, long*const other){
	return (*self == *other);
}
const __chx_std_Eq_vt_t std_Eq_long = {
	(_Bool(*)(void* self, void**const other)) std_long_equals,
};
_Bool std_ulong_equals(unsigned long*const self, unsigned long*const other){
	return (*self == *other);
}
const __chx_std_Eq_vt_t std_Eq_ulong = {
	(_Bool(*)(void* self, void**const other)) std_ulong_equals,
};
_Bool std_i64_equals(int64_t*const self, int64_t*const other){
	return (*self == *other);
}
const __chx_std_Eq_vt_t std_Eq_i64 = {
	(_Bool(*)(void* self, void**const other)) std_i64_equals,
};
_Bool std_u64_equals(uint64_t*const self, uint64_t*const other){
	return (*self == *other);
}
const __chx_std_Eq_vt_t std_Eq_u64 = {
	(_Bool(*)(void* self, void**const other)) std_u64_equals,
};
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/concurrency/threadpool.ch **/





static int std__SC_NPROCESSORS_ONLN = 84;
static void __chemda_668_0(void* this){
}
cstd_usize std_stdconcurrenthardware_threads(){
	
	long n = sysconf(std__SC_NPROCESSORS_ONLN);
	return (n < 1) ? ({ 
	((cstd_usize) 1); }) : ({ 
	((cstd_usize) n); });
}
void std_stdconcurrentsleep_ms(unsigned long ms){
	
	usleep(((int) (ms * 1000)));
}
static cstd_usize std_stdconcurrentspawn_native(const void*(*entry)(const void* arg), const void* arg){
	
	cstd_usize th = 0;
	if((pthread_create(&th, NULL, entry, arg) != 0)){
		std_raw_panic("pthread_create", &(struct std_Location){ 
			.filename = "/home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/concurrency/threadpool.ch", 
			.line = 62, 
			.character = 25
		});
	}
	return th;
}
static void std_stdconcurrentjoin_native(cstd_usize h){
	
	pthread_join(h, NULL);
}
void std_stdconcurrentThreadjoin(struct std_stdconcurrentThread*const self){
	std_stdconcurrentjoin_native(self->handle);
}
void std_stdconcurrentspawn(struct std_stdconcurrentThread* __chx_struct_ret_param_xx, const void*(*entry)(const void* arg), const void* arg){
	*__chx_struct_ret_param_xx = (struct std_stdconcurrentThread){ 
		.handle = std_stdconcurrentspawn_native(entry, arg)
	};
	return;
}
static const void* std_stdconcurrentworker_main(const void* arg){
	struct std_stdconcurrentPoolData* P = ((struct std_stdconcurrentPoolData*) arg);
	while(1) {
		struct std_stdOption__cgs__1 opt = (struct std_stdOption__cgs__1) { .__chx__vt_621827 = 1, 
		};
		std_stdmutexlock(&P->m);
		while(((std_stdvector__cgs__0size(&P->q) == 0) && P->run)) {
			std_stdCondVarwait(&P->cv, &P->m);
		}
		if((!P->run && (std_stdvector__cgs__0size(&P->q) == 0))){
			std_stdmutexunlock(&P->m);
			break;
		}
		if((std_stdvector__cgs__0size(&P->q) > 0)){
			struct std_stdconcurrentTask* first_ele = std_stdvector__cgs__0get_ptr(&P->q, 0);
			struct std_stdconcurrentTask t = (*({ struct std_stdconcurrentTask __chx__lv__65; std_stdreplace__cfg_0(&__chx__lv__65, &*first_ele, &(struct std_stdconcurrentTask){ 
				.f = *({ __chemical_fat_pointer__* __chemda_668_0_pair = &(__chemical_fat_pointer__){__chemda_668_0,NULL}; &(*({ struct std_stddefault_function_instance __chx__lv__66; std_stddefault_function_instancemake2(&__chx__lv__66, __chemda_668_0_pair->first, __chemda_668_0_pair->second, ((std_stddestructor_type) NULL), 0, 0); &__chx__lv__66; })); })
			}); &__chx__lv__65; }));
			std_stdvector__cgs__0remove(&P->q, 0);
			opt = (struct std_stdOption__cgs__1) { .__chx__vt_621827 = 0, 
				.Some.value = t
			};
		}
		std_stdmutexunlock(&P->m);
		if((opt.__chx__vt_621827 == 0)){
			struct std_stdOption__cgs__1* __chx__lv__67 = &opt;
			({ struct std_stddefault_function_instance* __chx__lv__68 = &__chx__lv__67->Some.value.f; ((void(*)(void*))std_stddefault_function_instanceget_fn_ptr(__chx__lv__68))(std_stddefault_function_instanceget_data_ptr(__chx__lv__68)); });
		}
	}
	return NULL;
}
void std_stdconcurrentPoolDatasubmit_void(struct std_stdconcurrentPoolData* self, std_stdfunction* f){
	_Bool __chx__lv__69 = true;
	std_stdmutexlock(&self->m);
	std_stdvector__cgs__0push_back(&self->q, &(struct std_stdconcurrentTask){ 
		.f = ({ __chx__lv__69 = false; *f; })
	});
	std_stdCondVarnotify_one(&self->cv);
	std_stdmutexunlock(&self->m);
	if(__chx__lv__69) {
		std_stddefault_function_instancedelete(f);
	}
}
void std_stdconcurrentPoolDatadelete(struct std_stdconcurrentPoolData* self){
	std_stdmutexlock(&self->m);
	self->run = 0;
	std_stdCondVarnotify_all(&self->cv);
	std_stdmutexunlock(&self->m);
	unsigned int i = 0;
	while((i < std_stdvector__cgs__2size(&self->workers))) {
		std_stdconcurrentjoin_native(*std_stdvector__cgs__2get_ptr(&self->workers, i));
		i = (i + 1);
	}
	std_stdvector__cgs__2clear(&self->workers);
	std_stdvector__cgs__0clear(&self->q);
	__chx__dstctr_clnup_blk__:{
		std_stdmutexdelete(&self->m);
		std_stdCondVardelete(&self->cv);
		std_stdvector__cgs__0delete(&self->q);
		std_stdvector__cgs__2delete(&self->workers);
	}
}
void std_stdconcurrentThreadPoolsubmit_void(struct std_stdconcurrentThreadPool*const self, std_stdfunction* f){
	_Bool __chx__lv__70 = true;
	std_stdconcurrentPoolDatasubmit_void(self->data, ({ __chx__lv__70 = false; &*f; }));
	if(__chx__lv__70) {
		std_stddefault_function_instancedelete(f);
	}
}
void std_stdconcurrentThreadPooldelete(struct std_stdconcurrentThreadPool*const self){
	struct std_stdconcurrentPoolData*__chx__lv__71 = self->data;
	
	std_stdconcurrentPoolDatadelete(__chx__lv__71);
	free(__chx__lv__71);
	__chx__dstctr_clnup_blk__:{
	}
}
void std_stdconcurrentcreate_pool(struct std_stdconcurrentThreadPool* __chx_struct_ret_param_xx, unsigned int n){
	if((n == 0)){
		std_raw_panic("n==0", &(struct std_Location){ 
			.filename = "/home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/concurrency/threadpool.ch", 
			.line = 267, 
			.character = 28
		});
	}
	struct std_stdconcurrentPoolData* p = ((struct std_stdconcurrentPoolData*) malloc(sizeof(struct std_stdconcurrentPoolData)));
	({ struct std_stdconcurrentPoolData* __chx__lv__72 = p; *__chx__lv__72 = (struct std_stdconcurrentPoolData){ 
		.m = (*({ struct std_stdmutex __chx__lv__73; std_stdmutexconstructor(&__chx__lv__73); &__chx__lv__73; })), 
		.cv = (*({ struct std_stdCondVar __chx__lv__74; std_stdCondVarconstructor(&__chx__lv__74); &__chx__lv__74; })), 
		.q = (*({ struct std_stdvector__cgs__0 __chx__lv__75; std_stdvector__cgs__0make(&__chx__lv__75); &__chx__lv__75; })), 
		.workers = (*({ struct std_stdvector__cgs__2 __chx__lv__76; std_stdvector__cgs__2make(&__chx__lv__76); &__chx__lv__76; })), 
		.run = 1
	}; __chx__lv__72; });
	unsigned int i = 0;
	while((i < n)) {
		cstd_usize h = std_stdconcurrentspawn_native(std_stdconcurrentworker_main, ((const void*) p));
		std_stdvector__cgs__2push_back(&p->workers, h);
		i = (i + 1);
	}
	*__chx_struct_ret_param_xx = (struct std_stdconcurrentThreadPool){ 
		.data = p
	};
	return;
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/u16_string.ch **/

static uint16_t std_stdEMPTY_U16 = 0;
size_t std_stdu16_strlen(const uint16_t* ptr){
	const uint16_t* start = ptr;
	const uint16_t* current = ptr;
	while((*current != 0)) {
		current++;
	}
	return (current - start);
}
void std_stdu16stringconstructor(struct std_stdu16string* this, const uint16_t* value, size_t length){
	struct std_stdu16string s = (struct std_stdu16string){ 
		.storage = { 
			.constant = { 
				.data = value, 
				.length = length
			}
		}, 
		.state = '0'
	};
	_Bool __chx__lv__77 = true;
	std_stdu16stringensure_mut(&s, length);
	*this = s;
	return;
}
void std_stdu16stringconstructor2(struct std_stdu16string* this, const uint16_t* value, size_t length, _Bool ensure){
	struct std_stdu16string s = (struct std_stdu16string){ 
		.storage = { 
			.constant = { 
				.data = value, 
				.length = length
			}
		}, 
		.state = '0'
	};
	_Bool __chx__lv__78 = true;
	if(ensure){
		std_stdu16stringensure_mut(&s, length);
	}
	*this = s;
	return;
}
void std_stdu16stringempty_str(struct std_stdu16string* this){
	*this = (struct std_stdu16string){ 
		.storage = { 
			.constant = { 
				.data = &std_stdEMPTY_U16, 
				.length = 0
			}
		}, 
		.state = '0'
	};
	return;
}
void std_stdu16stringmake_no_len(struct std_stdu16string* this, const uint16_t* value){
	size_t length = std_stdu16_strlen(value);
	struct std_stdu16string s = (struct std_stdu16string){ 
		.storage = { 
			.constant = { 
				.data = value, 
				.length = length
			}
		}, 
		.state = '0'
	};
	_Bool __chx__lv__79 = true;
	std_stdu16stringensure_mut(&s, length);
	*this = s;
	return;
}
size_t std_stdu16stringsize(struct std_stdu16string*const self){
	switch(self->state) {
		case '0':{
			return self->storage.constant.length;
			break;
		}
		case '1':{
			return self->storage.sso.length;
			break;
		}
		case '2':{
			return self->storage.heap.length;
			break;
		}
		
		default:{
			return 0;
			break;
		}
	}
}
_Bool std_stdu16stringempty(struct std_stdu16string*const self){
	return (std_stdu16stringsize(self) == 0);
}
void std_stdu16stringmove_const_to_buffer(struct std_stdu16string* self){
	const uint16_t* d = self->storage.constant.data;
	size_t length = self->storage.constant.length;
	
	for(int i = 0;(i < length);i++){
		self->storage.sso.buffer[i] = d[i];
	}
	self->storage.sso.buffer[length] = 0;
	self->storage.sso.length = ((unsigned char) length);
	self->state = '1';
}
void std_stdu16stringmove_data_to_heap(struct std_stdu16string* self, const uint16_t* from_data, size_t length, size_t capacity){
	uint16_t* d = ((uint16_t*) malloc(((capacity * 2) + 2)));
	memcpy(d, from_data, (length * 2));
	d[length] = 0;
	self->storage.heap.data = d;
	self->storage.heap.length = length;
	self->storage.heap.capacity = capacity;
	self->state = '2';
}
void std_stdu16stringresize_heap(struct std_stdu16string* self, size_t new_capacity){
	uint16_t* d = ((uint16_t*) realloc(self->storage.heap.data, ((new_capacity * 2) + 2)));
	d[self->storage.heap.length] = 0;
	self->storage.heap.data = d;
	self->storage.heap.capacity = new_capacity;
}
void std_stdu16stringensure_mut(struct std_stdu16string* self, size_t length){
	if((((self->state == '0') || (self->state == '1')) && (length < 8))){
		if((self->state == '0')){
			std_stdu16stringmove_const_to_buffer(self);
		}
	} else {
		if((self->state == '0')){
			std_stdu16stringmove_data_to_heap(self, self->storage.constant.data, self->storage.constant.length, length);
		}else if((self->state == '1')){
			std_stdu16stringmove_data_to_heap(self, &self->storage.sso.buffer[0], self->storage.sso.length, length);
		}else if((self->storage.heap.capacity <= length)){
			std_stdu16stringresize_heap(self, length);
		}
	}
}
void std_stdu16stringreserve(struct std_stdu16string* self, size_t new_capacity){
	switch(self->state) {
		case '0':{
			if(((new_capacity < 8) && (self->storage.constant.length < 8))){
				std_stdu16stringmove_const_to_buffer(self);
			} else {
				size_t len = self->storage.constant.length;
				if((new_capacity > len)){
					std_stdu16stringmove_data_to_heap(self, self->storage.constant.data, len, new_capacity);
				} else {
					std_stdu16stringmove_data_to_heap(self, self->storage.constant.data, len, len);
				}
			}
			break;
		}
		case '1':{
			if((new_capacity >= 8)){
				std_stdu16stringmove_data_to_heap(self, &self->storage.sso.buffer[0], self->storage.sso.length, new_capacity);
			}
			break;
		}
		case '2':{
			if((new_capacity > self->storage.heap.capacity)){
				std_stdu16stringresize_heap(self, new_capacity);
			}
			break;
		}
	}
}
void std_stdu16stringset(struct std_stdu16string* self, size_t index, uint16_t value){
	switch(self->state) {
		case '0':{
			std_stdu16stringmove_const_to_buffer(self);
			self->storage.sso.buffer[index] = value;
			break;
		}
		case '1':{
			self->storage.sso.buffer[index] = value;
			break;
		}
		case '2':{
			self->storage.heap.data[index] = value;
			break;
		}
	}
}
uint16_t std_stdu16stringget(struct std_stdu16string*const self, size_t index){
	switch(self->state) {
		case '0':{
			return self->storage.constant.data[index];
			break;
		}
		case '1':{
			return self->storage.sso.buffer[index];
			break;
		}
		case '2':{
			return self->storage.heap.data[index];
			break;
		}
		
		default:{
			return 0;
			break;
		}
	}
}
void std_stdu16stringappend_with_len(struct std_stdu16string* self, const uint16_t* value, size_t len){
	size_t offset = std_stdu16stringsize(self);
	unsigned long new_size = (offset + len);
	std_stdu16stringensure_mut(self, (new_size + 1));
	if((self->state == '1')){
		memcpy(&self->storage.sso.buffer[offset], value, (len * 2));
		self->storage.sso.buffer[new_size] = 0;
		self->storage.sso.length = new_size;
	} else {
		memcpy(&self->storage.heap.data[offset], value, (len * 2));
		self->storage.heap.data[new_size] = 0;
		self->storage.heap.length = new_size;
	}
}
void std_stdu16stringappend_u16_unit(struct std_stdu16string* self, uint16_t u){
	size_t offset = std_stdu16stringsize(self);
	unsigned long new_size = (offset + 1);
	std_stdu16stringensure_mut(self, (new_size + 1));
	if((self->state == '1')){
		self->storage.sso.buffer[offset] = u;
		self->storage.sso.buffer[new_size] = 0;
		self->storage.sso.length = new_size;
	} else {
		self->storage.heap.data[offset] = u;
		self->storage.heap.data[new_size] = 0;
		self->storage.heap.length = new_size;
	}
}
void std_stdu16stringappend_char(struct std_stdu16string* self, char c){
	char tmp = c;
	std_stdu16stringappend_utf8_view(self, &tmp, 1);
}
void std_stdu16stringappend_codepoint(struct std_stdu16string* self, uint32_t cp){
	if((cp <= 65535)){
		if(((cp >= 55296) && (cp <= 57343))){
			std_stdu16stringappend_u16_unit(self, 65533);
		} else {
			std_stdu16stringappend_u16_unit(self, ((uint16_t) cp));
		}
	}else if((cp <= 1114111)){
		uint32_t v = (cp - 65536);
		uint16_t high = ((uint16_t) (55296 + (v >> 10)));
		uint16_t low = ((uint16_t) (56320 + (v & 1023)));
		size_t offset = std_stdu16stringsize(self);
		unsigned long new_size = (offset + 2);
		std_stdu16stringensure_mut(self, (new_size + 1));
		if((self->state == '1')){
			self->storage.sso.buffer[offset] = high;
			self->storage.sso.buffer[(offset + 1)] = low;
			self->storage.sso.buffer[new_size] = 0;
			self->storage.sso.length = new_size;
		} else {
			self->storage.heap.data[offset] = high;
			self->storage.heap.data[(offset + 1)] = low;
			self->storage.heap.data[new_size] = 0;
			self->storage.heap.length = new_size;
		}
	} else {
		std_stdu16stringappend_u16_unit(self, 65533);
	}
}
void std_stdu16stringappend_utf8_view(struct std_stdu16string* self, const char* ptr, size_t len){
	if((len == 0)){
		return;
	}
	unsigned long out_cap = (len + 1);
	uint16_t* out = ((uint16_t*) malloc(((out_cap * 2) + 2)));
	size_t in_i = 0;
	size_t out_i = 0;
	while((in_i < len)) {
		unsigned char b0 = (((unsigned char) ptr[in_i]) & 'ÿ');
		uint32_t cp = 0;
		if(((b0 & '€') == 0)){
			cp = ((uint32_t) b0);
			in_i = (in_i + 1);
		}else if((((b0 & 'à') == 192) && ((in_i + 1) < len))){
			unsigned char b1 = (((unsigned char) ptr[(in_i + 1)]) & 'ÿ');
			if(((b1 & 'À') != 128)){
				cp = 65533;
				in_i = (in_i + 1);
			} else {
				cp = ((((uint32_t) (b0 & '')) << 6) | ((uint32_t) (b1 & '\?')));
				if((cp < 128)){
					cp = 65533;
				}
				in_i = (in_i + 2);
			}
		}else if((((b0 & 'ð') == 224) && ((in_i + 2) < len))){
			unsigned char b1 = (((unsigned char) ptr[(in_i + 1)]) & 'ÿ');
			unsigned char b2 = (((unsigned char) ptr[(in_i + 2)]) & 'ÿ');
			if((((b1 & 'À') != 128) || ((b2 & 'À') != 128))){
				cp = 65533;
				in_i = (in_i + 1);
			} else {
				cp = (((((uint32_t) (b0 & '')) << 12) | (((uint32_t) (b1 & '\?')) << 6)) | ((uint32_t) (b2 & '\?')));
				if(((cp < 2048) || ((cp >= 55296) && (cp <= 57343)))){
					cp = 65533;
				}
				in_i = (in_i + 3);
			}
		}else if((((b0 & 'ø') == 240) && ((in_i + 3) < len))){
			unsigned char b1 = (((unsigned char) ptr[(in_i + 1)]) & 'ÿ');
			unsigned char b2 = (((unsigned char) ptr[(in_i + 2)]) & 'ÿ');
			unsigned char b3 = (((unsigned char) ptr[(in_i + 3)]) & 'ÿ');
			if(((((b1 & 'À') != 128) || ((b2 & 'À') != 128)) || ((b3 & 'À') != 128))){
				cp = 65533;
				in_i = (in_i + 1);
			} else {
				cp = ((((((uint32_t) (b0 & '\a')) << 18) | (((uint32_t) (b1 & '\?')) << 12)) | (((uint32_t) (b2 & '\?')) << 6)) | ((uint32_t) (b3 & '\?')));
				if(((cp < 65536) || (cp > 1114111))){
					cp = 65533;
				}
				in_i = (in_i + 4);
			}
		} else {
			cp = 65533;
			in_i = (in_i + 1);
		}
		if((cp <= 65535)){
			out[out_i] = ((uint16_t) cp);
			out_i = (out_i + 1);
		} else {
			uint32_t v = (cp - 65536);
			uint16_t high = ((uint16_t) (55296 + (v >> 10)));
			uint16_t low = ((uint16_t) (56320 + (v & 1023)));
			out[out_i] = high;
			out_i = (out_i + 1);
			out[out_i] = low;
			out_i = (out_i + 1);
		}
	}
	std_stdu16stringappend_with_len(self, out, out_i);
	free(out);
}
void std_stdu16stringsubstr(struct std_stdu16string* __chx_struct_ret_param_xx, struct std_stdu16string*const self, size_t start, size_t len){
	struct std_stdu16string result = (*({ struct std_stdu16string __chx__lv__80; std_stdu16stringempty_str(&__chx__lv__80); &__chx__lv__80; }));
	_Bool __chx__lv__81 = true;
	size_t my_size = std_stdu16stringsize(self);
	if((start >= my_size)){
		*__chx_struct_ret_param_xx = result;
		return;
	}
	size_t real_len = len;
	if(((start + real_len) > my_size)){
		real_len = (my_size - start);
	}
	if((real_len == 0)){
		*__chx_struct_ret_param_xx = result;
		return;
	}
	const uint16_t* src = std_stdu16stringdata(self);
	uint16_t* buf = ((uint16_t*) malloc(((real_len * 2) + 2)));
	memcpy(buf, &src[start], (real_len * 2));
	buf[real_len] = 0;
	struct std_stdu16string __chx__lv__82 = (*({ struct std_stdu16string __chx__lv__83; std_stdu16stringconstructor(&__chx__lv__83, buf, real_len); &__chx__lv__83; }));
	*__chx_struct_ret_param_xx = result;
	std_stdu16stringdelete(&__chx__lv__82);
	return;
}
void std_stdu16stringto_utf8(struct std_stdstring* __chx_struct_ret_param_xx, struct std_stdu16string*const self){
	size_t n = std_stdu16stringsize(self);
	if((n == 0)){
		*__chx_struct_ret_param_xx = (*({ struct std_stdstring __chx__lv__84; std_stdstringempty_str(&__chx__lv__84); &__chx__lv__84; }));
		return;
	}
	unsigned long out_cap = ((n * 3) + 1);
	struct std_stdstring out = (*({ struct std_stdstring __chx__lv__85; std_stdstringempty_str(&__chx__lv__85); &__chx__lv__85; }));
	_Bool __chx__lv__86 = true;
	std_stdstringreserve(&out, (out_cap + 1));
	size_t i = 0;
	const uint16_t* src = std_stdu16stringdata(self);
	while((i < n)) {
		uint32_t w = ((uint32_t) src[i]);
		if((((w >= 55296) && (w <= 56319)) && ((i + 1) < n))){
			uint32_t w2 = ((uint32_t) src[(i + 1)]);
			if(((w2 >= 56320) && (w2 <= 57343))){
				uint32_t cp = ((((w - 55296) << 10) | (w2 - 56320)) + 65536);
				std_stdstringappend(&out, ((char) (240 | ((cp >> 18) & 7))));
				std_stdstringappend(&out, ((char) (128 | ((cp >> 12) & 63))));
				std_stdstringappend(&out, ((char) (128 | ((cp >> 6) & 63))));
				std_stdstringappend(&out, ((char) (128 | (cp & 63))));
				i = (i + 2);
				continue;
			}
		}
		if((w <= 127)){
			std_stdstringappend(&out, ((char) (w & 127)));
		}else if((w <= 2047)){
			std_stdstringappend(&out, ((char) (192 | ((w >> 6) & 31))));
			std_stdstringappend(&out, ((char) (128 | (w & 63))));
		} else {
			if(((w >= 55296) && (w <= 57343))){
				w = 65533;
			}
			std_stdstringappend(&out, ((char) (224 | ((w >> 12) & 15))));
			std_stdstringappend(&out, ((char) (128 | ((w >> 6) & 63))));
			std_stdstringappend(&out, ((char) (128 | (w & 63))));
		}
		i = (i + 1);
	}
	*__chx_struct_ret_param_xx = out;
	return;
}
int std_stdu16stringfind(struct std_stdu16string*const self, struct std_stdu16string* sub){
	_Bool __chx__lv__87 = true;
	size_t n = std_stdu16stringsize(self);
	size_t m = std_stdu16stringsize(sub);
	if((m == 0)){
		const int __chx__lv__88 = 0;
		if(__chx__lv__87) {
			std_stdu16stringdelete(sub);
		}
		return __chx__lv__88;
	}
	if((m > n)){
		const int __chx__lv__89 = -1;
		if(__chx__lv__87) {
			std_stdu16stringdelete(sub);
		}
		return __chx__lv__89;
	}
	const uint16_t* src = std_stdu16stringdata(self);
	const uint16_t* pat = std_stdu16stringdata(sub);
	size_t i = 0;
	while(((i + m) <= n)) {
		size_t j = 0;
		while(((j < m) && (src[(i + j)] == pat[j]))) {
			j = (j + 1);
		}
		if((j == m)){
			const int __chx__lv__90 = ((int) i);
			if(__chx__lv__87) {
				std_stdu16stringdelete(sub);
			}
			return __chx__lv__90;
		}
		i = (i + 1);
	}
	const int __chx__lv__91 = -1;
	if(__chx__lv__87) {
		std_stdu16stringdelete(sub);
	}
	return __chx__lv__91;
}
_Bool std_stdu16stringstarts_with(struct std_stdu16string*const self, struct std_stdu16string* prefix){
	_Bool __chx__lv__92 = true;
	size_t p_len = std_stdu16stringsize(prefix);
	size_t my_len = std_stdu16stringsize(self);
	if((p_len > my_len)){
		const _Bool __chx__lv__93 = 0;
		if(__chx__lv__92) {
			std_stdu16stringdelete(prefix);
		}
		return __chx__lv__93;
	}
	const uint16_t* src = std_stdu16stringdata(self);
	const uint16_t* pre = std_stdu16stringdata(prefix);
	size_t i = 0;
	while((i < p_len)) {
		if((src[i] != pre[i])){
			const _Bool __chx__lv__94 = 0;
			if(__chx__lv__92) {
				std_stdu16stringdelete(prefix);
			}
			return __chx__lv__94;
		}
		i = (i + 1);
	}
	const _Bool __chx__lv__95 = 1;
	if(__chx__lv__92) {
		std_stdu16stringdelete(prefix);
	}
	return __chx__lv__95;
}
_Bool std_stdu16stringends_with(struct std_stdu16string*const self, struct std_stdu16string* suffix){
	_Bool __chx__lv__96 = true;
	size_t s_len = std_stdu16stringsize(suffix);
	size_t my_len = std_stdu16stringsize(self);
	if((s_len > my_len)){
		const _Bool __chx__lv__97 = 0;
		if(__chx__lv__96) {
			std_stdu16stringdelete(suffix);
		}
		return __chx__lv__97;
	}
	const uint16_t* src = std_stdu16stringdata(self);
	const uint16_t* suf = std_stdu16stringdata(suffix);
	size_t i = 0;
	while((i < s_len)) {
		if((src[((my_len - s_len) + i)] != suf[i])){
			const _Bool __chx__lv__98 = 0;
			if(__chx__lv__96) {
				std_stdu16stringdelete(suffix);
			}
			return __chx__lv__98;
		}
		i = (i + 1);
	}
	const _Bool __chx__lv__99 = 1;
	if(__chx__lv__96) {
		std_stdu16stringdelete(suffix);
	}
	return __chx__lv__99;
}
void std_stdu16stringpush_back(struct std_stdu16string* self, uint16_t u){
	std_stdu16stringappend_u16_unit(self, u);
}
void std_stdu16stringpop_back(struct std_stdu16string* self){
	size_t sz = std_stdu16stringsize(self);
	if((sz == 0)){
		return;
	}
	if((self->state == '1')){
		self->storage.sso.length = ((unsigned char) (sz - 1));
		self->storage.sso.buffer[(sz - 1)] = 0;
	}else if((self->state == '2')){
		self->storage.heap.length = (sz - 1);
		self->storage.heap.data[(sz - 1)] = 0;
	} else {
		std_stdu16stringmove_const_to_buffer(self);
		self->storage.sso.length = ((unsigned char) (sz - 1));
		self->storage.sso.buffer[(sz - 1)] = 0;
	}
}
size_t std_stdu16stringcapacity(struct std_stdu16string* self){
	switch(self->state) {
		case '0':{
			return self->storage.constant.length;
			break;
		}
		case '1':{
			return ((size_t) 8);
			break;
		}
		case '2':{
			return self->storage.heap.capacity;
			break;
		}
		
		default:{
			return 0;
			break;
		}
	}
}
const uint16_t* std_stdu16stringdata(struct std_stdu16string*const self){
	switch(self->state) {
		case '0':{
			return self->storage.constant.data;
			break;
		}
		case '1':{
			return &self->storage.sso.buffer[0];
			break;
		}
		case '2':{
			return self->storage.heap.data;
			break;
		}
		
		default:{
			return &std_stdEMPTY_U16;
			break;
		}
	}
}
uint16_t* std_stdu16stringmutable_data(struct std_stdu16string* self){
	switch(self->state) {
		case '0':{
			std_stdu16stringmove_const_to_buffer(self);
			return &self->storage.sso.buffer[0];
			break;
		}
		case '1':{
			return &self->storage.sso.buffer[0];
			break;
		}
		case '2':{
			return self->storage.heap.data;
			break;
		}
		
		default:{
			return NULL;
			break;
		}
	}
}
void std_stdu16stringclear(struct std_stdu16string* self){
	switch(self->state) {
		case '0':{
			
			self->storage.constant.data = &std_stdEMPTY_U16;
			self->storage.constant.length = 0;
			break;
		}
		case '1':{
			self->storage.sso.buffer[0] = 0;
			self->storage.sso.length = 0;
			break;
		}
		case '2':{
			self->storage.heap.data[0] = 0;
			self->storage.heap.length = 0;
			break;
		}
		
		default:{
			break;
		}
	}
}
void std_stdu16stringdelete(struct std_stdu16string* self){
	if((self->state == '2')){
		free(self->storage.heap.data);
	}
	__chx__dstctr_clnup_blk__:{
	}
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/hashing/hash.ch **/
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/std.ch **/
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/ordered_map.ch **/

/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/hashing/hash_impl.ch **/

size_t std_char_hash(char*const self){
	return ((size_t) *self);
}
const __chx_std_Hashable_vt_t std_Hashable_char = {
	(unsigned int(*)(void* self)) std_char_hash,
};
size_t std_uchar_hash(unsigned char*const self){
	return *self;
}
const __chx_std_Hashable_vt_t std_Hashable_uchar = {
	(unsigned int(*)(void* self)) std_uchar_hash,
};
size_t std_short_hash(short*const self){
	return ((size_t) (*self * 2654435769));
}
const __chx_std_Hashable_vt_t std_Hashable_short = {
	(unsigned int(*)(void* self)) std_short_hash,
};
size_t std_ushort_hash(unsigned short*const self){
	return (*self * 2654435769);
}
const __chx_std_Hashable_vt_t std_Hashable_ushort = {
	(unsigned int(*)(void* self)) std_ushort_hash,
};
size_t std_int_hash(int*const self){
	return std_murmurhash(((const char*) self), sizeof(int), 0);
}
const __chx_std_Hashable_vt_t std_Hashable_int = {
	(unsigned int(*)(void* self)) std_int_hash,
};
size_t std_uint_hash(unsigned int*const self){
	return std_murmurhash(((const char*) self), sizeof(unsigned int), 0);
}
const __chx_std_Hashable_vt_t std_Hashable_uint = {
	(unsigned int(*)(void* self)) std_uint_hash,
};
size_t std_long_hash(long*const self){
	return std_murmurhash(((const char*) self), sizeof(long), 0);
}
const __chx_std_Hashable_vt_t std_Hashable_long = {
	(unsigned int(*)(void* self)) std_long_hash,
};
size_t std_ulong_hash(unsigned long*const self){
	return std_murmurhash(((const char*) self), sizeof(unsigned long), 0);
}
const __chx_std_Hashable_vt_t std_Hashable_ulong = {
	(unsigned int(*)(void* self)) std_ulong_hash,
};
size_t std_i64_hash(int64_t*const self){
	return std_murmurhash(((const char*) self), sizeof(int64_t), 0);
}
const __chx_std_Hashable_vt_t std_Hashable_i64 = {
	(unsigned int(*)(void* self)) std_i64_hash,
};
size_t std_u64_hash(uint64_t*const self){
	return std_murmurhash(((const char*) self), sizeof(uint64_t), 0);
}
const __chx_std_Hashable_vt_t std_Hashable_u64 = {
	(unsigned int(*)(void* self)) std_u64_hash,
};
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/option.ch **/
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/string.ch **/

static uint64_t std_stddbl_bits(double x){
	union std_stdu64_double_union u;
	u.d = x;
	return u.u;
}
static double std_stddbl_from_bits(uint64_t b){
	union std_stdu64_double_union u;
	u.u = b;
	return u.d;
}



static _Bool std_stddbl_is_nan(double x){
	uint64_t b = std_stddbl_bits(x);
	return (((b & 9218868437227405312) == 9218868437227405312) && ((b & 4503599627370495) != 0));
}
static _Bool std_stddbl_is_inf(double x){
	uint64_t b = std_stddbl_bits(x);
	return (((b & 9218868437227405312) == 9218868437227405312) && ((b & 4503599627370495) == 0));
}
static _Bool std_stddbl_is_neg(double x){
	return ((std_stddbl_bits(x) & 9223372036854775808) != 0);
}
unsigned int std_Hashable_string_hash(struct std_stdstring*const self){
	return std_fnv1a_hash_32(std_stdstringdata(self));
}
const __chx_std_Hashable_vt_t std_Hashablestd_stdstring = {
	(unsigned int(*)(void* self)) std_Hashable_string_hash,
};
_Bool std_Eq_string_equals(struct std_stdstring*const self, struct std_stdstring*const other){
	return std_stdstringequals_with_len(self, std_stdstringdata(other), std_stdstringsize(other));
}
const __chx_std_Eq_vt_t std_Eqstd_stdstring = {
	(_Bool(*)(void* self, void**const other)) std_Eq_string_equals,
};
void std_stdstringconstructor(struct std_stdstring* this, const char* value, size_t length){
	struct std_stdstring s = (struct std_stdstring){ 
		.storage = { 
			.constant = { 
				.data = value, 
				.length = length
			}
		}, 
		.state = '0'
	};
	_Bool __chx__lv__100 = true;
	std_stdstringensure_mut(&s, length);
	*this = s;
	return;
}
void std_stdstringview_make(struct std_stdstring* this, struct std_stdstring_view*const value){
	struct std_stdstring s = (struct std_stdstring){ 
		.storage = { 
			.constant = { 
				.data = std_stdstring_viewdata(value), 
				.length = std_stdstring_viewsize(value)
			}
		}, 
		.state = '0'
	};
	_Bool __chx__lv__101 = true;
	std_stdstringensure_mut(&s, std_stdstring_viewsize(value));
	*this = s;
	return;
}
void std_stdstringview_make2(struct std_stdstring* this, struct std_stdstring_view* value){
	*this = (*({ struct std_stdstring __chx__lv__102; std_stdstringview_make(&__chx__lv__102, value); &__chx__lv__102; }));
	return;
}
void std_stdstringconstructor2(struct std_stdstring* this, const char* value, size_t length, _Bool ensure){
	struct std_stdstring s = (struct std_stdstring){ 
		.storage = { 
			.constant = { 
				.data = value, 
				.length = length
			}
		}, 
		.state = '0'
	};
	_Bool __chx__lv__103 = true;
	if(ensure){
		std_stdstringensure_mut(&s, length);
	}
	*this = s;
	return;
}
void std_stdstringempty_str(struct std_stdstring* this){
	*this = (struct std_stdstring){ 
		.storage = { 
			.constant = { 
				.data = "", 
				.length = 0
			}
		}, 
		.state = '0'
	};
	return;
}
void std_stdstringmake_no_len(struct std_stdstring* this, const char* value){
	size_t length = strlen(value);
	struct std_stdstring s = (struct std_stdstring){ 
		.storage = { 
			.constant = { 
				.data = value, 
				.length = length
			}
		}, 
		.state = '0'
	};
	_Bool __chx__lv__104 = true;
	std_stdstringensure_mut(&s, length);
	*this = s;
	return;
}
void std_stdstringmake_with_char(struct std_stdstring* this, char value){
	struct std_stdstring s = (struct std_stdstring){ 
		.storage = { 
			.sso = { 
				.buffer = {}, 
				.length = 1
			}
		}, 
		.state = '1'
	};
	_Bool __chx__lv__105 = true;
	s.storage.sso.buffer[0] = value;
	s.storage.sso.buffer[1] = '\0';
	*this = s;
	return;
}
size_t std_stdstringsize(struct std_stdstring*const self){
	switch(self->state) {
		case '0':{
			return self->storage.constant.length;
			break;
		}
		case '1':{
			return self->storage.sso.length;
			break;
		}
		case '2':{
			return self->storage.heap.length;
			break;
		}
		
		default:{
			return 0;
			break;
		}
	}
}
void std_stdstringresize(struct std_stdstring* self, size_t value){
	std_stdstringensure_mut(self, (value + 1));
	switch(self->state) {
		case '1':{
			self->storage.sso.length = ((unsigned char) value);
			self->storage.sso.buffer[value] = '\0';
			break;
		}
		case '2':{
			self->storage.heap.length = value;
			self->storage.heap.data[value] = '\0';
			break;
		}
		
		default:{
			return;
			break;
		}
	}
}
_Bool std_stdstringempty(struct std_stdstring*const self){
	return (std_stdstringsize(self) == 0);
}
_Bool std_stdstringequals_with_len(struct std_stdstring*const self, const char* d, size_t l){
	size_t self_size = std_stdstringsize(self);
	return ((self_size == l) && (strncmp(std_stdstringdata(self), d, self_size) == 0));
}
_Bool std_stdstringequals_view(struct std_stdstring*const self, struct std_stdstring_view*const other){
	return std_stdstringequals_with_len(self, std_stdstring_viewdata(other), std_stdstring_viewsize(other));
}
void std_stdstringmove_const_to_buffer(struct std_stdstring* self){
	const char* d = self->storage.constant.data;
	size_t length = self->storage.constant.length;
	
	if((d != NULL)){
		for(int i = 0;(i < length);i++){
			self->storage.sso.buffer[i] = d[i];
		}
	}
	self->storage.sso.buffer[length] = '\0';
	self->storage.sso.length = ((unsigned char) length);
	self->state = '1';
}
void std_stdstringmove_data_to_heap(struct std_stdstring* self, const char* from_data, size_t length, size_t capacity){
	char* d = ((char*) malloc((capacity + 1)));
	memcpy(d, from_data, length);
	d[length] = '\0';
	self->storage.heap.data = d;
	self->storage.heap.length = length;
	self->storage.heap.capacity = capacity;
	self->state = '2';
}
_Bool std_stdstringresize_heap(struct std_stdstring* self, size_t new_capacity){
	char* d = ((char*) realloc(self->storage.heap.data, (new_capacity + 1)));
	if((d == NULL)){
		std_raw_panic("couldn\'t realloc in std::string -> resize_heap", &(struct std_Location){ 
			.filename = "/home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/string.ch", 
			.line = 254, 
			.character = 17
		});
		return 0;
	} else {
		d[self->storage.heap.length] = '\0';
		self->storage.heap.data = d;
		self->storage.heap.capacity = new_capacity;
		return 1;
	}
}
void std_stdstringensure_mut(struct std_stdstring* self, size_t length){
	if((((self->state == '0') || (self->state == '1')) && (length < 16))){
		if((self->state == '0')){
			std_stdstringmove_const_to_buffer(self);
		}
	} else {
		if((self->state == '0')){
			std_stdstringmove_data_to_heap(self, self->storage.constant.data, self->storage.constant.length, length);
		}else if((self->state == '1')){
			unsigned int new_cap = (length < (16 * 2)) ? ({ 
			(16 * 2); }) : ({ 
			(length * 2); });
			std_stdstringmove_data_to_heap(self, &self->storage.sso.buffer[0], self->storage.sso.length, new_cap);
		}else if((self->storage.heap.capacity <= length)){
			unsigned long new_cap = (length < (self->storage.heap.capacity * 2)) ? ({ 
			(self->storage.heap.capacity * 2); }) : ({ 
			length; });
			if((new_cap < (length + 16))){
				new_cap = (length + 64);
			}
			std_stdstringresize_heap(self, new_cap);
		}
	}
}
void std_stdstringreserve(struct std_stdstring* self, size_t new_capacity){
	switch(self->state) {
		case '0':{
			if(((new_capacity < 16) && (self->storage.constant.length < 16))){
				std_stdstringmove_const_to_buffer(self);
			} else {
				size_t len = self->storage.constant.length;
				if((new_capacity > len)){
					std_stdstringmove_data_to_heap(self, self->storage.constant.data, len, new_capacity);
				} else {
					std_stdstringmove_data_to_heap(self, self->storage.constant.data, len, len);
				}
			}
			break;
		}
		case '1':{
			if((new_capacity >= 16)){
				std_stdstringmove_data_to_heap(self, &self->storage.sso.buffer[0], self->storage.sso.length, new_capacity);
			}
			break;
		}
		case '2':{
			if((new_capacity > self->storage.heap.capacity)){
				std_stdstringresize_heap(self, new_capacity);
			}
			break;
		}
	}
}
void std_stdstringset(struct std_stdstring* self, size_t index, char value){
	switch(self->state) {
		case '0':{
			std_stdstringmove_const_to_buffer(self);
			self->storage.sso.buffer[index] = value;
			break;
		}
		case '1':{
			self->storage.sso.buffer[index] = value;
			break;
		}
		case '2':{
			self->storage.heap.data[index] = value;
			break;
		}
	}
}
char std_stdstringget(struct std_stdstring*const self, size_t index){
	switch(self->state) {
		case '0':{
			return self->storage.constant.data[index];
			break;
		}
		case '1':{
			return self->storage.sso.buffer[index];
			break;
		}
		case '2':{
			return self->storage.heap.data[index];
			break;
		}
		
		default:{
			return '\0';
			break;
		}
	}
}
void std_stdstringappend_with_len(struct std_stdstring* self, const char* value, size_t len){
	size_t offset = std_stdstringsize(self);
	unsigned long new_size = (offset + len);
	std_stdstringensure_mut(self, (new_size + 1));
	if((self->state == '1')){
		memcpy(&self->storage.sso.buffer[offset], value, len);
		self->storage.sso.buffer[new_size] = '\0';
		self->storage.sso.length = new_size;
	} else {
		memcpy(&self->storage.heap.data[offset], value, len);
		self->storage.heap.data[new_size] = '\0';
		self->storage.heap.length = new_size;
	}
}
void std_stdstringappend_char_ptr(struct std_stdstring* self, const char* value){
	std_stdstringappend_with_len(self, value, strlen(value));
}
void std_stdstringappend_string(struct std_stdstring* self, struct std_stdstring*const value){
	std_stdstringappend_with_len(self, std_stdstringdata(value), std_stdstringsize(value));
}
void std_stdstringappend_str(struct std_stdstring* self, const struct std_stdstring* value){
	std_stdstringappend_with_len(self, std_stdstringdata(value), std_stdstringsize(value));
}
void std_stdstringappend_view(struct std_stdstring* self, struct std_stdstring_view*const value){
	std_stdstringappend_with_len(self, std_stdstring_viewdata(value), std_stdstring_viewsize(value));
}
void std_stdstringappend_uinteger(struct std_stdstring* self, uint64_t value){
	if((value == 0)){
		std_stdstringappend(self, '0');
		return;
	}
	char buf[20];
	int bi = 0;
	while((value != 0)) {
		unsigned int digit = ((unsigned int) (value % 10));
		buf[bi] = ((char) (((int) '0') + ((int) digit)));
		bi = (bi + 1);
		value = (value / 10);
	}
	size_t old_len = std_stdstringsize(self);
	size_t add = ((size_t) bi);
	std_stdstringensure_mut(self, ((old_len + add) + 1));
	char* p = std_stdstringmutable_data(self);
	size_t i = 0;
	while((i < add)) {
		p[(old_len + i)] = buf[((add - 1) - i)];
		i = (i + 1);
	}
	p[(old_len + add)] = '\0';
	if((self->state == '1')){
		self->storage.sso.length = ((unsigned char) (old_len + add));
	} else {
		self->storage.heap.length = (old_len + add);
	}
}
void std_stdstringappend_integer(struct std_stdstring* self, int64_t value){
	if((value < 0)){
		std_stdstringappend(self, '-');
		int64_t tmp = (value + 1);
		uint64_t uv = ((uint64_t) 0);
		if((tmp < 0)){
			uv = ((uint64_t) -tmp);
			uv = (uv + 1);
		} else {
			uv = 1;
		}
		std_stdstringappend_uinteger(self, uv);
	} else {
		std_stdstringappend_uinteger(self, ((uint64_t) value));
	}
}
void std_stdstringappend_double(struct std_stdstring* self, double value, int precision){
	if((precision < 0)){
		precision = 6;
	}else if((precision > 18)){
		precision = 18;
	}
	if(std_stddbl_is_nan(value)){
		std_stdstringappend_view(self, &(*({ struct std_stdstring_view __chx__lv__106; std_stdstring_viewconstructor(&__chx__lv__106, "nan", 3); &__chx__lv__106; })));
		return;
	}
	if(std_stddbl_is_inf(value)){
		if(std_stddbl_is_neg(value)){
			std_stdstringappend_view(self, &(*({ struct std_stdstring_view __chx__lv__107; std_stdstring_viewconstructor(&__chx__lv__107, "-inf", 4); &__chx__lv__107; })));
		} else {
			std_stdstringappend_view(self, &(*({ struct std_stdstring_view __chx__lv__108; std_stdstring_viewconstructor(&__chx__lv__108, "inf", 3); &__chx__lv__108; })));
		}
		return;
	}
	double v = value;
	if((v < 0.0)){
		std_stdstringappend(self, '-');
		v = -v;
	}
	int64_t int_part = ((int64_t) v);
	std_stdstringappend_integer(self, int_part);
	if((precision == 0)){
		return;
	}
	std_stdstringappend(self, '.');
	double frac = (v - ((double) int_part));
	double pow10_d = 1.0;
	uint64_t pow10_u = 1;
	int pi = 0;
	while((pi < precision)) {
		pow10_d = (pow10_d * 10.0);
		pow10_u = (pow10_u * 10);
		pi = (pi + 1);
	}
	uint64_t scaled = ((uint64_t) ((frac * pow10_d) + 0.5));
	if((scaled >= pow10_u)){
		int64_t new_int = (int_part + 1);
		size_t dot_pos = std_stdstringsize(self);
		size_t si = 0;
		if((self->state == '1')){
			si = 0;
			size_t idx = dot_pos;
			while((((idx > 0) && (std_stdstringget(self, (idx - 1)) >= '0')) && (std_stdstringget(self, (idx - 1)) <= '9'))) {
				idx = (idx - 1);
			}
			si = idx;
		} else {
			si = 0;
			size_t idx2 = dot_pos;
			while((((idx2 > 0) && (std_stdstringget(self, (idx2 - 1)) >= '0')) && (std_stdstringget(self, (idx2 - 1)) <= '9'))) {
				idx2 = (idx2 - 1);
			}
			si = idx2;
		}
		if((self->state == '1')){
			self->storage.sso.buffer[si] = '\0';
			self->storage.sso.length = ((unsigned char) si);
		} else {
			self->storage.heap.data[si] = '\0';
			self->storage.heap.length = si;
		}
		std_stdstringappend_integer(self, new_int);
		std_stdstringappend(self, '.');
		scaled = 0;
	}
	char frac_buf[20];
	int fbi = 0;
	if((scaled == 0)){
		int z = 0;
		while((z < precision)) {
			std_stdstringappend(self, '0');
			z = (z + 1);
		}
		return;
	}
	uint64_t tmp = scaled;
	while((tmp != 0)) {
		uint64_t d = ((uint64_t) (tmp % 10));
		frac_buf[fbi] = ((char) (((int) '0') + ((int) d)));
		fbi = (fbi + 1);
		tmp = (tmp / 10);
	}
	int leading = (precision - fbi);
	int zi = 0;
	while((zi < leading)) {
		std_stdstringappend(self, '0');
		zi = (zi + 1);
	}
	int fj = 0;
	while((fj < fbi)) {
		std_stdstringappend(self, frac_buf[((fbi - 1) - fj)]);
		fj = (fj + 1);
	}
}
void std_stdstringappend_float(struct std_stdstring* self, float value, int precision){
	std_stdstringappend_double(self, ((double) value), precision);
}
void std_stdstringcopy(struct std_stdstring* __chx_struct_ret_param_xx, struct std_stdstring*const self){
	*__chx_struct_ret_param_xx = (*({ struct std_stdstring __chx__lv__109; std_stdstringsubstring(&__chx__lv__109, self, 0, std_stdstringsize(self)); &__chx__lv__109; }));
	return;
}
void std_stdstringsubstring(struct std_stdstring* __chx_struct_ret_param_xx, struct std_stdstring*const self, size_t start, size_t end){
	struct std_stdstring s;
	_Bool __chx__lv__110 = true;
	size_t actual_len = (end - start);
	if((actual_len < 16)){
		s.state = '1';
		s.storage.sso.length = ((unsigned char) actual_len);
		const char* d = std_stdstringdata(self);
		for(int i = 0;(i < actual_len);i++){
			s.storage.sso.buffer[i] = d[(start + i)];
		}
		s.storage.sso.buffer[actual_len] = '\0';
	} else {
		s.state = '2';
		unsigned long new_cap = (actual_len * 2);
		char* new_heap = ((char*) malloc(new_cap));
		const char* d = std_stdstringdata(self);
		for(int i = 0;(i < actual_len);i++){
			new_heap[i] = d[(start + i)];
		}
		s.storage.heap.data = new_heap;
		s.storage.heap.data[actual_len] = '\0';
		s.storage.heap.length = actual_len;
		s.storage.heap.capacity = new_cap;
	}
	*__chx_struct_ret_param_xx = s;
	return;
}
void std_stdstringappend(struct std_stdstring* self, char value){
	size_t length = std_stdstringsize(self);
	if((((self->state == '0') || (self->state == '1')) && (length < (16 - 1)))){
		if((self->state == '0')){
			std_stdstringmove_const_to_buffer(self);
		}
		self->storage.sso.buffer[length] = value;
		self->storage.sso.buffer[(length + 1)] = '\0';
		self->storage.sso.length = (length + 1);
	} else {
		if((self->state == '0')){
			std_stdstringmove_data_to_heap(self, self->storage.constant.data, length, (length * 2));
		}else if((self->state == '1')){
			std_stdstringmove_data_to_heap(self, &self->storage.sso.buffer[0], length, (length * 2));
		}else if((self->storage.heap.capacity <= (length + 2))){
			std_stdstringresize_heap(self, (self->storage.heap.capacity * 2));
		}
		self->storage.heap.data[length] = value;
		self->storage.heap.data[(length + 1)] = '\0';
		self->storage.heap.length = (length + 1);
	}
}
size_t std_stdstringfind(struct std_stdstring*const self, struct std_stdstring_view*const needle){
	struct std_stdstring_view view = (*({ struct std_stdstring_view __chx__lv__111; std_stdstring_viewconstructor(&__chx__lv__111, std_stdstringdata(self), std_stdstringsize(self)); &__chx__lv__111; }));
	return std_stdinternal_view_find(&view, needle);
}
size_t std_stdstringfind_last(struct std_stdstring*const self, struct std_stdstring_view*const needle){
	struct std_stdstring_view view = (*({ struct std_stdstring_view __chx__lv__112; std_stdstring_viewconstructor(&__chx__lv__112, std_stdstringdata(self), std_stdstringsize(self)); &__chx__lv__112; }));
	return std_stdinternal_view_find_last(&view, needle);
}
_Bool std_stdstringcontains(struct std_stdstring*const self, struct std_stdstring_view*const needle){
	return (std_stdstringfind(self, needle) != (((size_t) 0) - 1));
}
_Bool std_stdstringstarts_with(struct std_stdstring*const self, struct std_stdstring_view*const other){
	if((std_stdstring_viewsize(other) > std_stdstringsize(self))){
		return 0;
	}
	return (memcmp(std_stdstringdata(self), std_stdstring_viewdata(other), std_stdstring_viewsize(other)) == 0);
}
void std_stdstringtrim(struct std_stdstring_view* __chx_struct_ret_param_xx, struct std_stdstring*const self){
	*__chx_struct_ret_param_xx = (*({ struct std_stdstring_view __chx__lv__113; std_stdstring_viewtrim(&__chx__lv__113, &(*({ struct std_stdstring_view __chx__lv__114; std_stdstringto_view(&__chx__lv__114, self); &__chx__lv__114; }))); &__chx__lv__113; }));
	return;
}
void std_stdstringsplit(struct std_stdvector__cgs__1* __chx_struct_ret_param_xx, struct std_stdstring*const self, char delim){
	*__chx_struct_ret_param_xx = (*({ struct std_stdvector__cgs__1 __chx__lv__115; std_stdstring_viewsplit(&__chx__lv__115, &(*({ struct std_stdstring_view __chx__lv__116; std_stdstringto_view(&__chx__lv__116, self); &__chx__lv__116; })), delim); &__chx__lv__115; }));
	return;
}
_Bool std_stdstringends_with(struct std_stdstring*const self, struct std_stdstring_view*const other){
	if((std_stdstring_viewsize(other) > std_stdstringsize(self))){
		return 0;
	}
	return (memcmp(((std_stdstringdata(self) + std_stdstringsize(self)) - std_stdstring_viewsize(other)), std_stdstring_viewdata(other), std_stdstring_viewsize(other)) == 0);
}
void std_stdstringerase(struct std_stdstring* self, size_t start, size_t len){
	size_t sz = std_stdstringsize(self);
	if(((start >= sz) || (len == 0))){
		return;
	}
	size_t erase_len = len;
	if(((start + erase_len) > sz)){
		erase_len = (sz - start);
	}
	unsigned long tail_start = (start + erase_len);
	unsigned long tail_len = (sz - tail_start);
	std_stdstringensure_mut(self, (sz + 1));
	if((self->state == '1')){
		if((tail_len > 0)){
			memmove(&self->storage.sso.buffer[start], &self->storage.sso.buffer[tail_start], tail_len);
		}
		self->storage.sso.length = ((unsigned char) (sz - erase_len));
		self->storage.sso.buffer[self->storage.sso.length] = '\0';
	} else {
		if((tail_len > 0)){
			memmove(&self->storage.heap.data[start], &self->storage.heap.data[tail_start], tail_len);
		}
		self->storage.heap.length = (sz - erase_len);
		self->storage.heap.data[self->storage.heap.length] = '\0';
	}
}
size_t std_stdstringcapacity(struct std_stdstring* self){
	switch(self->state) {
		case '0':{
			return self->storage.constant.length;
			break;
		}
		case '1':{
			return ((size_t) 16);
			break;
		}
		case '2':{
			return self->storage.heap.capacity;
			break;
		}
		
		default:{
			return 0;
			break;
		}
	}
}
const char* std_stdstringdata(struct std_stdstring*const self){
	switch(self->state) {
		case '0':{
			return self->storage.constant.data;
			break;
		}
		case '1':{
			return &self->storage.sso.buffer[0];
			break;
		}
		case '2':{
			return self->storage.heap.data;
			break;
		}
		
		default:{
			return "";
			break;
		}
	}
}
const char* std_stdstringc_str(struct std_stdstring*const self){
	return std_stdstringdata(self);
}
char* std_stdstringmutable_data(struct std_stdstring* self){
	switch(self->state) {
		case '0':{
			if((self->storage.constant.length < 16)){
				std_stdstringmove_const_to_buffer(self);
				return &self->storage.sso.buffer[0];
			} else {
				std_stdstringmove_data_to_heap(self, self->storage.constant.data, self->storage.constant.length, (self->storage.constant.length * 2));
				return self->storage.heap.data;
			}
			break;
		}
		case '1':{
			return &self->storage.sso.buffer[0];
			break;
		}
		case '2':{
			return self->storage.heap.data;
			break;
		}
		
		default:{
			return NULL;
			break;
		}
	}
}
void std_stdstringclear(struct std_stdstring* self){
	switch(self->state) {
		case '0':{
			
			self->storage.constant.data = "";
			self->storage.constant.length = 0;
			break;
		}
		case '1':{
			self->storage.sso.buffer[0] = '\0';
			self->storage.sso.length = 0;
			break;
		}
		case '2':{
			self->storage.heap.data[0] = '\0';
			self->storage.heap.length = 0;
			break;
		}
		
		default:{
			break;
		}
	}
}
void std_stdstringto_view(struct std_stdstring_view* __chx_struct_ret_param_xx, struct std_stdstring*const self){
	*__chx_struct_ret_param_xx = (*({ struct std_stdstring_view __chx__lv__117; std_stdstring_viewconstructor(&__chx__lv__117, std_stdstringdata(self), std_stdstringsize(self)); &__chx__lv__117; }));
	return;
}
void std_stdstringto_i8(struct std_stdResult__cgs__3* __chx_struct_ret_param_xx, struct std_stdstring*const self){
	*__chx_struct_ret_param_xx = (*({ struct std_stdResult__cgs__3 __chx__lv__118; std_stdstring_viewto_i8(&__chx__lv__118, &(*({ struct std_stdstring_view __chx__lv__119; std_stdstringto_view(&__chx__lv__119, self); &__chx__lv__119; }))); &__chx__lv__118; }));
	return;
}
void std_stdstringto_i16(struct std_stdResult__cgs__5* __chx_struct_ret_param_xx, struct std_stdstring*const self){
	*__chx_struct_ret_param_xx = (*({ struct std_stdResult__cgs__5 __chx__lv__120; std_stdstring_viewto_i16(&__chx__lv__120, &(*({ struct std_stdstring_view __chx__lv__121; std_stdstringto_view(&__chx__lv__121, self); &__chx__lv__121; }))); &__chx__lv__120; }));
	return;
}
void std_stdstringto_i32(struct std_stdResult__cgs__7* __chx_struct_ret_param_xx, struct std_stdstring*const self){
	*__chx_struct_ret_param_xx = (*({ struct std_stdResult__cgs__7 __chx__lv__122; std_stdstring_viewto_i32(&__chx__lv__122, &(*({ struct std_stdstring_view __chx__lv__123; std_stdstringto_view(&__chx__lv__123, self); &__chx__lv__123; }))); &__chx__lv__122; }));
	return;
}
void std_stdstringto_i64(struct std_stdResult__cgs__8* __chx_struct_ret_param_xx, struct std_stdstring*const self){
	*__chx_struct_ret_param_xx = (*({ struct std_stdResult__cgs__8 __chx__lv__124; std_stdstring_viewto_i64(&__chx__lv__124, &(*({ struct std_stdstring_view __chx__lv__125; std_stdstringto_view(&__chx__lv__125, self); &__chx__lv__125; }))); &__chx__lv__124; }));
	return;
}
void std_stdstringto_u8(struct std_stdResult__cgs__9* __chx_struct_ret_param_xx, struct std_stdstring*const self){
	*__chx_struct_ret_param_xx = (*({ struct std_stdResult__cgs__9 __chx__lv__126; std_stdstring_viewto_u8(&__chx__lv__126, &(*({ struct std_stdstring_view __chx__lv__127; std_stdstringto_view(&__chx__lv__127, self); &__chx__lv__127; }))); &__chx__lv__126; }));
	return;
}
void std_stdstringto_u16(struct std_stdResult__cgs__11* __chx_struct_ret_param_xx, struct std_stdstring*const self){
	*__chx_struct_ret_param_xx = (*({ struct std_stdResult__cgs__11 __chx__lv__128; std_stdstring_viewto_u16(&__chx__lv__128, &(*({ struct std_stdstring_view __chx__lv__129; std_stdstringto_view(&__chx__lv__129, self); &__chx__lv__129; }))); &__chx__lv__128; }));
	return;
}
void std_stdstringto_u32(struct std_stdResult__cgs__12* __chx_struct_ret_param_xx, struct std_stdstring*const self){
	*__chx_struct_ret_param_xx = (*({ struct std_stdResult__cgs__12 __chx__lv__130; std_stdstring_viewto_u32(&__chx__lv__130, &(*({ struct std_stdstring_view __chx__lv__131; std_stdstringto_view(&__chx__lv__131, self); &__chx__lv__131; }))); &__chx__lv__130; }));
	return;
}
void std_stdstringto_u64(struct std_stdResult__cgs__14* __chx_struct_ret_param_xx, struct std_stdstring*const self){
	*__chx_struct_ret_param_xx = (*({ struct std_stdResult__cgs__14 __chx__lv__132; std_stdstring_viewto_u64(&__chx__lv__132, &(*({ struct std_stdstring_view __chx__lv__133; std_stdstringto_view(&__chx__lv__133, self); &__chx__lv__133; }))); &__chx__lv__132; }));
	return;
}
void std_stdstringto_int(struct std_stdResult__cgs__15* __chx_struct_ret_param_xx, struct std_stdstring*const self){
	*__chx_struct_ret_param_xx = (*({ struct std_stdResult__cgs__15 __chx__lv__134; std_stdstring_viewto_int(&__chx__lv__134, &(*({ struct std_stdstring_view __chx__lv__135; std_stdstringto_view(&__chx__lv__135, self); &__chx__lv__135; }))); &__chx__lv__134; }));
	return;
}
void std_stdstringto_uint(struct std_stdResult__cgs__16* __chx_struct_ret_param_xx, struct std_stdstring*const self){
	*__chx_struct_ret_param_xx = (*({ struct std_stdResult__cgs__16 __chx__lv__136; std_stdstring_viewto_uint(&__chx__lv__136, &(*({ struct std_stdstring_view __chx__lv__137; std_stdstringto_view(&__chx__lv__137, self); &__chx__lv__137; }))); &__chx__lv__136; }));
	return;
}
void std_stdstringto_float(struct std_stdResult__cgs__17* __chx_struct_ret_param_xx, struct std_stdstring*const self){
	char* end = NULL;
	float res = ((float) strtod(((char*) std_stdstringdata(self)), &end));
	if((end == std_stdstringdata(self))){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__17) { .__chx__vt_621827 = 1, 
			.Err.error = (*({ struct std_stdstring_view __chx__lv__138; std_stdstring_viewconstructor(&__chx__lv__138, "invalid format", 14); &__chx__lv__138; }))
		};
		return;
	}
	while((((end != NULL) && (*end != '\0')) && isspace(((int) *end)))) {
		end++;
	}
	if(((end != NULL) && (*end != '\0'))){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__17) { .__chx__vt_621827 = 1, 
			.Err.error = (*({ struct std_stdstring_view __chx__lv__139; std_stdstring_viewconstructor(&__chx__lv__139, "trailing characters", 19); &__chx__lv__139; }))
		};
		return;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__17) { .__chx__vt_621827 = 0, 
		.Ok.value = res
	};
	return;
}
void std_stdstringto_double(struct std_stdResult__cgs__20* __chx_struct_ret_param_xx, struct std_stdstring*const self){
	char* end = NULL;
	double res = strtod(((char*) std_stdstringdata(self)), &end);
	if((end == std_stdstringdata(self))){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__20) { .__chx__vt_621827 = 1, 
			.Err.error = (*({ struct std_stdstring_view __chx__lv__140; std_stdstring_viewconstructor(&__chx__lv__140, "invalid format", 14); &__chx__lv__140; }))
		};
		return;
	}
	while((((end != NULL) && (*end != '\0')) && isspace(((int) *end)))) {
		end++;
	}
	if(((end != NULL) && (*end != '\0'))){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__20) { .__chx__vt_621827 = 1, 
			.Err.error = (*({ struct std_stdstring_view __chx__lv__141; std_stdstring_viewconstructor(&__chx__lv__141, "trailing characters", 19); &__chx__lv__141; }))
		};
		return;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__20) { .__chx__vt_621827 = 0, 
		.Ok.value = res
	};
	return;
}
void std_stdstringstream(struct std_stdstring*const self, struct std_stdStringStream* s){
	core_corestreamStream_StringStream_writeStr(s, std_stdstringdata(self), std_stdstringsize(self));
}
void std_stdstringdelete(struct std_stdstring* self){
	if((self->state == '2')){
		free(self->storage.heap.data);
	}
	__chx__dstctr_clnup_blk__:{
	}
}
void core_corestreamStream_StringStream_writeChar(struct std_stdStringStream* self, char value){
	std_stdstringappend(self->str, value);
}
void core_corestreamStream_StringStream_writeUChar(struct std_stdStringStream* self, unsigned char value){
	std_stdstringappend(self->str, ((char) value));
}
void core_corestreamStream_StringStream_writeSigned(struct std_stdStringStream* self, int64_t value){
	std_stdstringappend_integer(self->str, value);
}
void core_corestreamStream_StringStream_writeUnsigned(struct std_stdStringStream* self, uint64_t value){
	std_stdstringappend_uinteger(self->str, value);
}
void core_corestreamStream_StringStream_writeStr(struct std_stdStringStream* self, const char* value, uint64_t length){
	std_stdstringappend_with_len(self->str, value, length);
}
void core_corestreamStream_StringStream_writeStrNoLen(struct std_stdStringStream* self, const char* value){
	std_stdstringappend_with_len(self->str, value, strlen(value));
}
void core_corestreamStream_StringStream_writeFloat(struct std_stdStringStream* self, float value){
	std_stdstringappend_double(self->str, ((double) value), 3);
}
void core_corestreamStream_StringStream_writeDouble(struct std_stdStringStream* self, double value){
	std_stdstringappend_double(self->str, ((double) value), 3);
}
const __chx_core_corestreamStream_vt_t core_corestreamStreamstd_stdStringStream = {
	(void(*)(void* self, const char* value, uint64_t length)) core_corestreamStream_StringStream_writeStr,
	(void(*)(void* self, const char* value)) core_corestreamStream_StringStream_writeStrNoLen,
	(void(*)(void* self, int64_t value)) core_corestreamStream_StringStream_writeSigned,
	(void(*)(void* self, uint64_t value)) core_corestreamStream_StringStream_writeUnsigned,
	(void(*)(void* self, char value)) core_corestreamStream_StringStream_writeChar,
	(void(*)(void* self, unsigned char value)) core_corestreamStream_StringStream_writeUChar,
	(void(*)(void* self, float value)) core_corestreamStream_StringStream_writeFloat,
	(void(*)(void* self, double value)) core_corestreamStream_StringStream_writeDouble,
};
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/utils.ch **/

static size_t std_stdinternal_view_find(struct std_stdstring_view*const me, struct std_stdstring_view*const needle){
	const char* hay = std_stdstring_viewdata(me);
	size_t hay_len = std_stdstring_viewsize(me);
	const char* nd = std_stdstring_viewdata(needle);
	size_t nlen = std_stdstring_viewsize(needle);
	if((nlen == 0)){
		return 0;
	}
	if((nlen > hay_len)){
		return (((size_t) 0) - 1);
	}
	if((nlen == 1)){
		char c = nd[0];
		size_t i = 0;
		while((i < hay_len)) {
			if((hay[i] == c)){
				return i;
			}
			i = (i + 1);
		}
		return (((size_t) 0) - 1);
	}
	unsigned char skip[256];
	int i = 0;
	while((i < 256)) {
		skip[i] = ((unsigned char) nlen);
		i = (i + 1);
	}
	size_t j = 0;
	while((j < (nlen - 1))) {
		skip[((unsigned int) nd[j])] = ((unsigned char) ((nlen - 1) - j));
		j = (j + 1);
	}
	size_t pos = 0;
	unsigned long last = (nlen - 1);
	while((pos <= (hay_len - nlen))) {
		size_t k = last;
		while((hay[(pos + k)] == nd[k])) {
			if((k == 0)){
				return pos;
			}
			k = (k - 1);
		}
		pos = (pos + skip[((unsigned int) hay[(pos + last)])]);
	}
	return (((size_t) 0) - 1);
}
static size_t std_stdinternal_view_find_last(struct std_stdstring_view*const me, struct std_stdstring_view*const needle){
	const char* hay = std_stdstring_viewdata(me);
	size_t hay_len = std_stdstring_viewsize(me);
	const char* nd = std_stdstring_viewdata(needle);
	size_t nlen = std_stdstring_viewsize(needle);
	if((nlen == 0)){
		return hay_len;
	}
	if((nlen > hay_len)){
		return (((size_t) 0) - 1);
	}
	int i = ((int) (hay_len - nlen));
	while((i >= 0)) {
		_Bool match = 1;
		size_t j = 0;
		while((j < nlen)) {
			if((hay[(((size_t) i) + j)] != nd[j])){
				match = 0;
				break;
			}
			j = (j + 1);
		}
		if(match){
			return ((size_t) i);
		}
		i = (i - 1);
	}
	return (((size_t) 0) - 1);
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/time/types.ch **/





void std_stdchronoDurationinit(struct std_stdchronoDuration* this){
	*this = (struct std_stdchronoDuration){ 
		.secs = 0, 
		.nanos = 0
	};
	return;
}
void std_stdchronoDurationfrom_parts(struct std_stdchronoDuration* this, int64_t s, int64_t n){
	struct std_stdchronoDuration d = (struct std_stdchronoDuration){ 
		.secs = s, 
		.nanos = n
	};
	std_stdchronoDurationnormalize(&d);
	*this = d;
	return;
}
void std_stdchronoDurationfrom_secs(struct std_stdchronoDuration* __chx_struct_ret_param_xx, int64_t secs){
	*__chx_struct_ret_param_xx = (struct std_stdchronoDuration){ 
		.secs = secs, 
		.nanos = 0
	};
	return;
}
void std_stdchronoDurationfrom_millis(struct std_stdchronoDuration* __chx_struct_ret_param_xx, int64_t ms){
	struct std_stdchronoDuration d = (struct std_stdchronoDuration){ 
		.secs = (ms / 1000), 
		.nanos = ((ms % 1000) * 1000000)
	};
	std_stdchronoDurationnormalize(&d);
	*__chx_struct_ret_param_xx = d;
	return;
}
void std_stdchronoDurationfrom_micros(struct std_stdchronoDuration* __chx_struct_ret_param_xx, int64_t us){
	struct std_stdchronoDuration d = (struct std_stdchronoDuration){ 
		.secs = (us / 1000000), 
		.nanos = ((us % 1000000) * 1000)
	};
	std_stdchronoDurationnormalize(&d);
	*__chx_struct_ret_param_xx = d;
	return;
}
void std_stdchronoDurationfrom_nanos(struct std_stdchronoDuration* __chx_struct_ret_param_xx, int64_t ns){
	struct std_stdchronoDuration d = (struct std_stdchronoDuration){ 
		.secs = (ns / 1000000000), 
		.nanos = (ns % 1000000000)
	};
	std_stdchronoDurationnormalize(&d);
	*__chx_struct_ret_param_xx = d;
	return;
}
int64_t std_stdchronoDurationas_secs(struct std_stdchronoDuration*const self){
	return self->secs;
}
int64_t std_stdchronoDurationsubsec_nanos(struct std_stdchronoDuration*const self){
	return self->nanos;
}
int64_t std_stdchronoDurationas_millis(struct std_stdchronoDuration*const self){
	return ((self->secs * 1000) + (self->nanos / 1000000));
}
int64_t std_stdchronoDurationas_micros(struct std_stdchronoDuration*const self){
	if(((self->secs < 0) && (self->nanos > 0))){
		return (((self->secs + 1) * 1000000) - ((1000000000 - self->nanos) / 1000));
	}
	return ((self->secs * 1000000) + (self->nanos / 1000));
}
int64_t std_stdchronoDurationas_nanos(struct std_stdchronoDuration*const self){
	if(((self->secs < 0) && (self->nanos > 0))){
		return (((self->secs + 1) * 1000000000) - (1000000000 - self->nanos));
	}
	return ((self->secs * 1000000000) + self->nanos);
}
void std_stdchronoDurationnormalize(struct std_stdchronoDuration* self){
	if(((self->nanos >= 1000000000) || (self->nanos <= -1000000000))){
		int64_t extra = (self->nanos / 1000000000);
		self->secs = (self->secs + extra);
		self->nanos = (self->nanos - (extra * 1000000000));
	}
	if(((self->secs > 0) && (self->nanos < 0))){
		self->secs = (self->secs - 1);
		self->nanos = (self->nanos + 1000000000);
	}else if(((self->secs < 0) && (self->nanos > 0))){
		self->secs = (self->secs + 1);
		self->nanos = (self->nanos - 1000000000);
	}
}
_Bool std_stdchronoDurationequals(struct std_stdchronoDuration*const self, struct std_stdchronoDuration*const other){
	return ((self->secs == other->secs) && (self->nanos == other->nanos));
}
int std_stdchronoDurationcmp(struct std_stdchronoDuration*const self, struct std_stdchronoDuration*const other){
	if((self->secs < other->secs)){
		return -1;
	}
	if((self->secs > other->secs)){
		return 1;
	}
	if((self->nanos < other->nanos)){
		return -1;
	}
	if((self->nanos > other->nanos)){
		return 1;
	}
	return 0;
}
void std_stdchronoDurationadd(struct std_stdchronoDuration* __chx_struct_ret_param_xx, struct std_stdchronoDuration*const self, struct std_stdchronoDuration*const other){
	struct std_stdchronoDuration d = (struct std_stdchronoDuration){ 
		.secs = (self->secs + other->secs), 
		.nanos = (self->nanos + other->nanos)
	};
	std_stdchronoDurationnormalize(&d);
	*__chx_struct_ret_param_xx = d;
	return;
}
void std_stdchronoDurationsub(struct std_stdchronoDuration* __chx_struct_ret_param_xx, struct std_stdchronoDuration*const self, struct std_stdchronoDuration*const other){
	struct std_stdchronoDuration d = (struct std_stdchronoDuration){ 
		.secs = (self->secs - other->secs), 
		.nanos = (self->nanos - other->nanos)
	};
	std_stdchronoDurationnormalize(&d);
	*__chx_struct_ret_param_xx = d;
	return;
}
void std_stdchronoDurationneg(struct std_stdchronoDuration* __chx_struct_ret_param_xx, struct std_stdchronoDuration*const self){
	struct std_stdchronoDuration d = (struct std_stdchronoDuration){ 
		.secs = -self->secs, 
		.nanos = -self->nanos
	};
	std_stdchronoDurationnormalize(&d);
	*__chx_struct_ret_param_xx = d;
	return;
}
void std_stdchronoDurationmul_i64(struct std_stdchronoDuration* __chx_struct_ret_param_xx, struct std_stdchronoDuration*const self, int64_t scalar){
	if((scalar == 0)){
		*__chx_struct_ret_param_xx = (struct std_stdchronoDuration){ 
			.secs = 0, 
			.nanos = 0
		};
		return;
	}
	int64_t total_nanos = std_stdchronoDurationas_nanos(self);
	int64_t result_nanos = (total_nanos * scalar);
	*__chx_struct_ret_param_xx = (*({ struct std_stdchronoDuration __chx__lv__142; std_stdchronoDurationfrom_nanos(&__chx__lv__142, result_nanos); &__chx__lv__142; }));
	return;
}
void std_stdchronoDurationdiv_i64(struct std_stdchronoDuration* __chx_struct_ret_param_xx, struct std_stdchronoDuration*const self, int64_t scalar){
	if((scalar == 0)){
		std_raw_panic("Duration::div_i64: division by zero", &(struct std_Location){ 
			.filename = "/home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/time/types.ch", 
			.line = 144, 
			.character = 25
		});
	}
	int64_t total_nanos = std_stdchronoDurationas_nanos(self);
	int64_t result_nanos = (total_nanos / scalar);
	*__chx_struct_ret_param_xx = (*({ struct std_stdchronoDuration __chx__lv__143; std_stdchronoDurationfrom_nanos(&__chx__lv__143, result_nanos); &__chx__lv__143; }));
	return;
}
void std_stdchronoDurationabs(struct std_stdchronoDuration* __chx_struct_ret_param_xx, struct std_stdchronoDuration*const self){
	if((self->secs < 0)){
		*__chx_struct_ret_param_xx = (*({ struct std_stdchronoDuration __chx__lv__144; std_stdchronoDurationneg(&__chx__lv__144, self); &__chx__lv__144; }));
		return;
	}
	*__chx_struct_ret_param_xx = (struct std_stdchronoDuration){ 
		.secs = self->secs, 
		.nanos = self->nanos
	};
	return;
}
_Bool std_stdchronoDurationis_zero(struct std_stdchronoDuration*const self){
	return ((self->secs == 0) && (self->nanos == 0));
}
_Bool std_stdchronoDurationis_positive(struct std_stdchronoDuration*const self){
	return ((self->secs > 0) || ((self->secs == 0) && (self->nanos > 0)));
}
_Bool std_stdchronoDurationis_negative(struct std_stdchronoDuration*const self){
	return ((self->secs < 0) || ((self->secs == 0) && (self->nanos < 0)));
}









void std_stdchronoInstantinit(struct std_stdchronoInstant* this){
	*this = (struct std_stdchronoInstant){ 
		.secs = 0, 
		.nanos = 0
	};
	return;
}
void std_stdchronoInstantnow(struct std_stdchronoInstant* __chx_struct_ret_param_xx){
	int64_t s = 0;
	int64_t n = 0;
	std_stdnow_monotonic(&s, &n);
	*__chx_struct_ret_param_xx = (struct std_stdchronoInstant){ 
		.secs = s, 
		.nanos = n
	};
	return;
}
void std_stdchronoInstantduration_since(struct std_stdchronoDuration* __chx_struct_ret_param_xx, struct std_stdchronoInstant*const self, struct std_stdchronoInstant*const earlier){
	struct std_stdchronoDuration d = (*({ struct std_stdchronoDuration __chx__lv__145; std_stdchronoDurationfrom_parts(&__chx__lv__145, (self->secs - earlier->secs), (self->nanos - earlier->nanos)); &__chx__lv__145; }));
	*__chx_struct_ret_param_xx = d;
	return;
}
void std_stdchronoInstantelapsed(struct std_stdchronoDuration* __chx_struct_ret_param_xx, struct std_stdchronoInstant*const self){
	struct std_stdchronoInstant now_inst = (*({ struct std_stdchronoInstant __chx__lv__146; std_stdchronoInstantnow(&__chx__lv__146); &__chx__lv__146; }));
	*__chx_struct_ret_param_xx = (*({ struct std_stdchronoDuration __chx__lv__147; std_stdchronoInstantduration_since(&__chx__lv__147, &now_inst, self); &__chx__lv__147; }));
	return;
}
void std_stdchronoInstantduration_to(struct std_stdchronoDuration* __chx_struct_ret_param_xx, struct std_stdchronoInstant*const self, struct std_stdchronoInstant*const later){
	*__chx_struct_ret_param_xx = (*({ struct std_stdchronoDuration __chx__lv__148; std_stdchronoInstantduration_since(&__chx__lv__148, later, self); &__chx__lv__148; }));
	return;
}
void std_stdchronoInstantadd_duration(struct std_stdchronoInstant* __chx_struct_ret_param_xx, struct std_stdchronoInstant*const self, struct std_stdchronoDuration*const dur){
	struct std_stdchronoDuration d = (*({ struct std_stdchronoDuration __chx__lv__149; std_stdchronoDurationfrom_parts(&__chx__lv__149, self->secs, self->nanos); &__chx__lv__149; }));
	struct std_stdchronoDuration sum = (*({ struct std_stdchronoDuration __chx__lv__150; std_stdchronoDurationadd(&__chx__lv__150, &d, dur); &__chx__lv__150; }));
	*__chx_struct_ret_param_xx = (struct std_stdchronoInstant){ 
		.secs = std_stdchronoDurationas_secs(&sum), 
		.nanos = std_stdchronoDurationsubsec_nanos(&sum)
	};
	return;
}
void std_stdchronoInstantsub_duration(struct std_stdchronoInstant* __chx_struct_ret_param_xx, struct std_stdchronoInstant*const self, struct std_stdchronoDuration*const dur){
	struct std_stdchronoDuration d = (*({ struct std_stdchronoDuration __chx__lv__151; std_stdchronoDurationfrom_parts(&__chx__lv__151, self->secs, self->nanos); &__chx__lv__151; }));
	struct std_stdchronoDuration diff = (*({ struct std_stdchronoDuration __chx__lv__152; std_stdchronoDurationsub(&__chx__lv__152, &d, dur); &__chx__lv__152; }));
	*__chx_struct_ret_param_xx = (struct std_stdchronoInstant){ 
		.secs = std_stdchronoDurationas_secs(&diff), 
		.nanos = std_stdchronoDurationsubsec_nanos(&diff)
	};
	return;
}
_Bool std_stdchronoInstantequals(struct std_stdchronoInstant*const self, struct std_stdchronoInstant*const other){
	return ((self->secs == other->secs) && (self->nanos == other->nanos));
}
int std_stdchronoInstantcmp(struct std_stdchronoInstant*const self, struct std_stdchronoInstant*const other){
	if((self->secs < other->secs)){
		return -1;
	}
	if((self->secs > other->secs)){
		return 1;
	}
	if((self->nanos < other->nanos)){
		return -1;
	}
	if((self->nanos > other->nanos)){
		return 1;
	}
	return 0;
}
void std_stdchronoSystemTimeinit(struct std_stdchronoSystemTime* this){
	*this = (struct std_stdchronoSystemTime){ 
		.secs = 0, 
		.nanos = 0
	};
	return;
}
void std_stdchronoSystemTimenow(struct std_stdchronoSystemTime* __chx_struct_ret_param_xx){
	int64_t s = 0;
	int64_t n = 0;
	std_stdnow_realtime(&s, &n);
	*__chx_struct_ret_param_xx = (struct std_stdchronoSystemTime){ 
		.secs = s, 
		.nanos = n
	};
	return;
}
void std_stdchronoSystemTimefrom_unix_epoch(struct std_stdchronoSystemTime* __chx_struct_ret_param_xx, int64_t secs){
	*__chx_struct_ret_param_xx = (struct std_stdchronoSystemTime){ 
		.secs = secs, 
		.nanos = 0
	};
	return;
}
void std_stdchronoSystemTimefrom_unix_epoch_nanos(struct std_stdchronoSystemTime* __chx_struct_ret_param_xx, int64_t secs, int64_t nanos){
	struct std_stdchronoDuration d = (*({ struct std_stdchronoDuration __chx__lv__153; std_stdchronoDurationfrom_parts(&__chx__lv__153, secs, nanos); &__chx__lv__153; }));
	*__chx_struct_ret_param_xx = (struct std_stdchronoSystemTime){ 
		.secs = std_stdchronoDurationas_secs(&d), 
		.nanos = std_stdchronoDurationsubsec_nanos(&d)
	};
	return;
}
int64_t std_stdchronoSystemTimeas_unix_epoch_secs(struct std_stdchronoSystemTime*const self){
	return self->secs;
}
int64_t std_stdchronoSystemTimeas_unix_epoch_nanos(struct std_stdchronoSystemTime*const self){
	return ((self->secs * 1000000000) + self->nanos);
}
void std_stdchronoSystemTimeduration_since(struct std_stdchronoDuration* __chx_struct_ret_param_xx, struct std_stdchronoSystemTime*const self, struct std_stdchronoSystemTime*const earlier){
	struct std_stdchronoDuration d = (*({ struct std_stdchronoDuration __chx__lv__154; std_stdchronoDurationfrom_parts(&__chx__lv__154, (self->secs - earlier->secs), (self->nanos - earlier->nanos)); &__chx__lv__154; }));
	*__chx_struct_ret_param_xx = d;
	return;
}
void std_stdchronoSystemTimeelapsed(struct std_stdchronoDuration* __chx_struct_ret_param_xx, struct std_stdchronoSystemTime*const self){
	struct std_stdchronoSystemTime now_st = (*({ struct std_stdchronoSystemTime __chx__lv__155; std_stdchronoSystemTimenow(&__chx__lv__155); &__chx__lv__155; }));
	*__chx_struct_ret_param_xx = (*({ struct std_stdchronoDuration __chx__lv__156; std_stdchronoSystemTimeduration_since(&__chx__lv__156, &now_st, self); &__chx__lv__156; }));
	return;
}
void std_stdchronoSystemTimeadd_duration(struct std_stdchronoSystemTime* __chx_struct_ret_param_xx, struct std_stdchronoSystemTime*const self, struct std_stdchronoDuration*const dur){
	struct std_stdchronoDuration d = (*({ struct std_stdchronoDuration __chx__lv__157; std_stdchronoDurationfrom_parts(&__chx__lv__157, self->secs, self->nanos); &__chx__lv__157; }));
	struct std_stdchronoDuration sum = (*({ struct std_stdchronoDuration __chx__lv__158; std_stdchronoDurationadd(&__chx__lv__158, &d, dur); &__chx__lv__158; }));
	*__chx_struct_ret_param_xx = (struct std_stdchronoSystemTime){ 
		.secs = std_stdchronoDurationas_secs(&sum), 
		.nanos = std_stdchronoDurationsubsec_nanos(&sum)
	};
	return;
}
void std_stdchronoSystemTimesub_duration(struct std_stdchronoSystemTime* __chx_struct_ret_param_xx, struct std_stdchronoSystemTime*const self, struct std_stdchronoDuration*const dur){
	struct std_stdchronoDuration d = (*({ struct std_stdchronoDuration __chx__lv__159; std_stdchronoDurationfrom_parts(&__chx__lv__159, self->secs, self->nanos); &__chx__lv__159; }));
	struct std_stdchronoDuration diff = (*({ struct std_stdchronoDuration __chx__lv__160; std_stdchronoDurationsub(&__chx__lv__160, &d, dur); &__chx__lv__160; }));
	*__chx_struct_ret_param_xx = (struct std_stdchronoSystemTime){ 
		.secs = std_stdchronoDurationas_secs(&diff), 
		.nanos = std_stdchronoDurationsubsec_nanos(&diff)
	};
	return;
}
_Bool std_stdchronoSystemTimeequals(struct std_stdchronoSystemTime*const self, struct std_stdchronoSystemTime*const other){
	return ((self->secs == other->secs) && (self->nanos == other->nanos));
}

/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/stream.ch **/
void core_corestreamStream_CommandLineStream_writeChar(struct std_CommandLineStream*const self, char value){
	fwrite(&value, 1, 1, cstd_get_stdout());
}
void core_corestreamStream_CommandLineStream_writeUChar(struct std_CommandLineStream*const self, unsigned char value){
	fwrite(&value, 1, 1, cstd_get_stdout());
}
void core_corestreamStream_CommandLineStream_writeSigned(struct std_CommandLineStream*const self, int64_t value){
	char buf[64];
	size_t len = std_CommandLineStreami64_to_chars(self, &buf[0], value);
	fwrite(&buf[0], 1, len, cstd_get_stdout());
}
void core_corestreamStream_CommandLineStream_writeUnsigned(struct std_CommandLineStream*const self, uint64_t value){
	char buf[64];
	size_t len = std_CommandLineStreamu64_to_chars(self, &buf[0], value);
	fwrite(&buf[0], 1, len, cstd_get_stdout());
}
void core_corestreamStream_CommandLineStream_writeStr(struct std_CommandLineStream*const self, const char* value, uint64_t length){
	fwrite(value, 1, length, cstd_get_stdout());
}
void core_corestreamStream_CommandLineStream_writeStrNoLen(struct std_CommandLineStream*const self, const char* value){
	fwrite(value, 1, strlen(value), cstd_get_stdout());
}
void core_corestreamStream_CommandLineStream_writeFloat(struct std_CommandLineStream*const self, float value){
	char buf[128];
	size_t len = std_CommandLineStreamfloat_to_chars(self, &buf[0], value, 6);
	fwrite(&buf[0], 1, len, cstd_get_stdout());
}
void core_corestreamStream_CommandLineStream_writeDouble(struct std_CommandLineStream*const self, double value){
	char buf[256];
	size_t len = std_CommandLineStreamdouble_to_chars(self, &buf[0], value, 6);
	fwrite(&buf[0], 1, len, cstd_get_stdout());
}
const __chx_core_corestreamStream_vt_t core_corestreamStreamstd_CommandLineStream = {
	(void(*)(void* self, const char* value, uint64_t length)) core_corestreamStream_CommandLineStream_writeStr,
	(void(*)(void* self, const char* value)) core_corestreamStream_CommandLineStream_writeStrNoLen,
	(void(*)(void* self, int64_t value)) core_corestreamStream_CommandLineStream_writeSigned,
	(void(*)(void* self, uint64_t value)) core_corestreamStream_CommandLineStream_writeUnsigned,
	(void(*)(void* self, char value)) core_corestreamStream_CommandLineStream_writeChar,
	(void(*)(void* self, unsigned char value)) core_corestreamStream_CommandLineStream_writeUChar,
	(void(*)(void* self, float value)) core_corestreamStream_CommandLineStream_writeFloat,
	(void(*)(void* self, double value)) core_corestreamStream_CommandLineStream_writeDouble,
};
size_t std_CommandLineStreamu64_to_chars(struct std_CommandLineStream*const self, char* out_buf, uint64_t value){
	if((value == 0)){
		out_buf[0] = '0';
		return 1;
	}
	char rev[20];
	int ri = 0;
	while((value != 0)) {
		unsigned int d = ((unsigned int) (value % 10));
		rev[ri] = ((char) (((int) '0') + ((int) d)));
		ri = (ri + 1);
		value = (value / 10);
	}
	size_t i = 0;
	while((i < ((size_t) ri))) {
		out_buf[i] = rev[((ri - 1) - ((int) i))];
		i = (i + 1);
	}
	return ((size_t) ri);
}
size_t std_CommandLineStreami64_to_chars(struct std_CommandLineStream*const self, char* out_buf, int64_t value){
	size_t pos = 0;
	if((value < 0)){
		out_buf[0] = '-';
		pos = 1;
		int64_t tmp = (value + 1);
		uint64_t uv = 0;
		if((tmp < 0)){
			uv = ((uint64_t) -tmp);
			uv = (uv + 1);
		} else {
			uv = 1;
		}
		size_t written = std_CommandLineStreamu64_to_chars(self, (out_buf + 1), uv);
		return (pos + written);
	} else {
		size_t w = std_CommandLineStreamu64_to_chars(self, out_buf, ((uint64_t) value));
		return w;
	}
}
size_t std_CommandLineStreamdouble_to_chars(struct std_CommandLineStream*const self, char* out_buf, double value, int precision){
	if((precision < 0)){
		precision = 6;
	}else if((precision > 18)){
		precision = 18;
	}
	if(std_stddbl_is_nan(value)){
		out_buf[0] = 'n';
		out_buf[1] = 'a';
		out_buf[2] = 'n';
		return 3;
	}
	if(std_stddbl_is_inf(value)){
		if(std_stddbl_is_neg(value)){
			out_buf[0] = '-';
			out_buf[1] = 'i';
			out_buf[2] = 'n';
			out_buf[3] = 'f';
			return 4;
		} else {
			out_buf[0] = 'i';
			out_buf[1] = 'n';
			out_buf[2] = 'f';
			return 3;
		}
	}
	size_t dst_i = 0;
	double v = value;
	if((v < 0.0)){
		out_buf[dst_i] = '-';
		dst_i = (dst_i + 1);
		v = -v;
	}
	int64_t int_part = ((int64_t) v);
	char tmp_int[32];
	size_t int_len = std_CommandLineStreami64_to_chars(self, &tmp_int[0], int_part);
	size_t ci = 0;
	while((ci < int_len)) {
		out_buf[(dst_i + ci)] = tmp_int[ci];
		ci = (ci + 1);
	}
	dst_i = (dst_i + int_len);
	if((precision == 0)){
		return dst_i;
	}
	out_buf[dst_i] = '.';
	dst_i = (dst_i + 1);
	double frac = (v - ((double) int_part));
	double pow10_d = 1.0;
	uint64_t pow10_u = 1;
	int pi = 0;
	while((pi < precision)) {
		pow10_d = (pow10_d * 10.0);
		pow10_u = (pow10_u * 10);
		pi = (pi + 1);
	}
	uint64_t scaled = ((uint64_t) ((frac * pow10_d) + 0.5));
	if((scaled >= pow10_u)){
		int64_t new_int = (int_part + 1);
		size_t new_int_len = std_CommandLineStreami64_to_chars(self, &tmp_int[0], new_int);
		int offset_sign = 0;
		if((value < 0.0)){
			offset_sign = 1;
		}
		size_t k = 0;
		while((k < new_int_len)) {
			out_buf[(offset_sign + k)] = tmp_int[k];
			k = (k + 1);
		}
		dst_i = ((size_t) (offset_sign + new_int_len));
		out_buf[dst_i] = '.';
		dst_i = (dst_i + 1);
		scaled = 0;
		int_len = new_int_len;
	}
	if((scaled == 0)){
		int z = 0;
		while((z < precision)) {
			out_buf[dst_i] = '0';
			dst_i = (dst_i + 1);
			z = (z + 1);
		}
		return dst_i;
	}
	char revf[20];
	int rfi = 0;
	uint64_t tmpf = scaled;
	while((tmpf != 0)) {
		unsigned int d = ((unsigned int) (tmpf % 10));
		revf[rfi] = ((char) (((int) '0') + ((int) d)));
		rfi = (rfi + 1);
		tmpf = (tmpf / 10);
	}
	int leading = (precision - rfi);
	int zz = 0;
	while((zz < leading)) {
		out_buf[dst_i] = '0';
		dst_i = (dst_i + 1);
		zz = (zz + 1);
	}
	int fj = 0;
	while((fj < rfi)) {
		out_buf[dst_i] = revf[((rfi - 1) - fj)];
		dst_i = (dst_i + 1);
		fj = (fj + 1);
	}
	return dst_i;
}
size_t std_CommandLineStreamfloat_to_chars(struct std_CommandLineStream*const self, char* out_buf, float value, int precision){
	return std_CommandLineStreamdouble_to_chars(self, out_buf, ((double) value), precision);
}
void std_CommandLineStreammake(struct std_CommandLineStream* this){
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/function.ch **/
void std_stddefault_function_instancemake2(struct std_stddefault_function_instance* this, void* ptr, void* cap, std_stddestructor_type destr, size_t size_data, size_t align_data){
	void* captured = cap;
	if((captured == NULL)){
		*this = (struct std_stddefault_function_instance){ 
			.fn_pointer = ptr, 
			.fn_data_ptr = NULL, 
			.is_heap = 0, 
			.dtor = NULL, 
			.buffer = {}
		};
		return;
	}
	if((size_data >= 32)){
		char* allocated = ((char*) malloc(size_data));
		memcpy(allocated, captured, size_data);
		*this = (struct std_stddefault_function_instance){ 
			.fn_pointer = ptr, 
			.fn_data_ptr = allocated, 
			.is_heap = 1, 
			.dtor = destr, 
			.buffer = {}
		};
		return;
	} else {
		struct std_stddefault_function_instance i = (struct std_stddefault_function_instance){ 
			.fn_pointer = ptr, 
			.fn_data_ptr = NULL, 
			.is_heap = 0, 
			.dtor = destr, 
			.buffer = {}
		};
		_Bool __chx__lv__161 = true;
		memcpy(&i.buffer[0], captured, size_data);
		i.fn_data_ptr = &i.buffer[0];
		*this = i;
		return;
	}
}
void* std_stddefault_function_instanceget_fn_ptr(struct std_stddefault_function_instance*const self){
	return self->fn_pointer;
}
void* std_stddefault_function_instanceget_data_ptr(struct std_stddefault_function_instance*const self){
	if(self->is_heap){
		return self->fn_data_ptr;
	} else {
		return ((void*) &self->buffer[0]);
	}
}
void std_stddefault_function_instancedelete(struct std_stddefault_function_instance*const self){
	if((self->dtor != NULL)){
		self->dtor(std_stddefault_function_instanceget_data_ptr(self));
	}
	if(self->is_heap){
		free(self->fn_data_ptr);
	}
	__chx__dstctr_clnup_blk__:{
	}
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/env.ch **/
void std_stdget_env(struct std_stdOption__cgs__0* __chx_struct_ret_param_xx, struct std_stdstring_view*const name){
	char* ptr = getenv(std_stdstring_viewdata(name));
	if((ptr == NULL)){
		*__chx_struct_ret_param_xx = (struct std_stdOption__cgs__0) { .__chx__vt_621827 = 1, 
		};
		return;
	}
	*__chx_struct_ret_param_xx = (struct std_stdOption__cgs__0) { .__chx__vt_621827 = 0, 
		.Some.value = (*({ struct std_stdstring __chx__lv__162; std_stdstringmake_no_len(&__chx__lv__162, ptr); &__chx__lv__162; }))
	};
	return;
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/hashing/murmur.ch **/
uint32_t std_murmurhash(const char* key, uint32_t len, uint32_t seed){
	cstd_uint32_t c1 = ((cstd_uint32_t) 3432918353);
	cstd_uint32_t c2 = ((cstd_uint32_t) 461845907);
	cstd_uint32_t r1 = ((cstd_uint32_t) 15);
	cstd_uint32_t r2 = ((cstd_uint32_t) 13);
	cstd_uint32_t m = ((cstd_uint32_t) 5);
	cstd_uint32_t n = ((cstd_uint32_t) 3864292196);
	cstd_uint32_t h = ((cstd_uint32_t) 0);
	cstd_uint32_t k = ((cstd_uint32_t) 0);
	const cstd_uint8_t* d = ((const cstd_uint8_t*) key);
	const cstd_uint32_t* chunks = NULL;
	const cstd_uint8_t* tail = NULL;
	int i = 0;
	uint32_t l = (len / 4);
	h = seed;
	chunks = ((const cstd_uint32_t*) (d + (l * 4)));
	tail = ((const cstd_uint8_t*) (d + (l * 4)));
	for(i = -l;(i != 0);++i){
		
		k = chunks[i];
		k *= c1;
		k = ((k << r1) | (k >> (32 - r1)));
		k *= c2;
		h ^= k;
		h = ((h << r2) | (h >> (32 - r2)));
		h = ((h * m) + n);
	}
	k = 0;
	switch((len & 3)) {
		case 3:{
			k ^= (tail[2] << 16);
			k ^= (tail[1] << 8);
			k ^= tail[0];
			k *= c1;
			k = ((k << r1) | (k >> (32 - r1)));
			k *= c2;
			h ^= k;
			break;
		}
		case 2:{
			k ^= (tail[1] << 8);
			k ^= tail[0];
			k *= c1;
			k = ((k << r1) | (k >> (32 - r1)));
			k *= c2;
			h ^= k;
			break;
		}
		case 1:{
			k ^= tail[0];
			k *= c1;
			k = ((k << r1) | (k >> (32 - r1)));
			k *= c2;
			h ^= k;
			break;
		}
	}
	h ^= len;
	h ^= (h >> 16);
	h *= 2246822507;
	h ^= (h >> 13);
	h *= 3266489909;
	h ^= (h >> 16);
	return h;
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/pair.ch **/
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/std/src/vector.ch **/
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/std/posix/time.ch **/
int64_t std_stdnow_milli(){
	struct timespec ts = ((struct timespec){0});
	clock_gettime(0, &ts);
	return ((((int64_t) ts.tv_sec) * 1000) + (((int64_t) ts.tv_nsec) / 1000000));
}
static int std_stdclock_id_realtime(){
	return 0;
}
static int std_stdclock_id_monotonic(){
	
	return 1;
}
static void std_stdnow_clock(int64_t* secs, int64_t* nanos, int clock_id){
	struct timespec ts = ((struct timespec){0});
	int rc = clock_gettime(clock_id, &ts);
	if((rc != 0)){
		std_raw_panic("clock_gettime failed", &(struct std_Location){ 
			.filename = "/home/wakaztahir/work/Chemical/chemical/lang/libs/std/posix/time.ch", 
			.line = 28, 
			.character = 17
		});
	}
	*secs = ((int64_t) ts.tv_sec);
	*nanos = ((int64_t) ts.tv_nsec);
}
static void std_stdnow_monotonic(int64_t* secs, int64_t* nanos){
	std_stdnow_clock(secs, nanos, std_stdclock_id_monotonic());
}
static void std_stdnow_realtime(int64_t* secs, int64_t* nanos){
	std_stdnow_clock(secs, nanos, std_stdclock_id_realtime());
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/std/posix/condvar.ch **/



static void std_stdcompute_abstime_ms(struct timespec* out, unsigned long timeout_ms){
	struct timespec now;
	int rc = clock_gettime(0, &now);
	if((rc != 0)){
		std_raw_panic("clock_gettime failed", &(struct std_Location){ 
			.filename = "/home/wakaztahir/work/Chemical/chemical/lang/libs/std/posix/condvar.ch", 
			.line = 36, 
			.character = 17
		});
	}
	int64_t add_s = ((int64_t) (timeout_ms / 1000));
	int64_t add_ns = ((int64_t) ((timeout_ms % 1000) * 1000000));
	out->tv_sec = (now.tv_sec + add_s);
	out->tv_nsec = (now.tv_nsec + add_ns);
	if((out->tv_nsec >= 1000000000)){
		out->tv_sec = (out->tv_sec + 1);
		out->tv_nsec = (out->tv_nsec - 1000000000);
	}
}
void std_stdCondVarconstructor(struct std_stdCondVar* this){
	struct std_stdCondVar c = (struct std_stdCondVar){ 
		.storage = {}
	};
	_Bool __chx__lv__163 = true;
	int res = pthread_cond_init(&c.storage[0], NULL);
	if((res != 0)){
		std_raw_panic("pthread_cond_init failed", &(struct std_Location){ 
			.filename = "/home/wakaztahir/work/Chemical/chemical/lang/libs/std/posix/condvar.ch", 
			.line = 58, 
			.character = 21
		});
	}
	*this = c;
	return;
}
void std_stdCondVarwait(struct std_stdCondVar* self, struct std_stdmutex* mutex){
	int r = pthread_cond_wait(&self->storage[0], &mutex->storage[0]);
	if((r != 0)){
		std_raw_panic("pthread_cond_wait failed", &(struct std_Location){ 
			.filename = "/home/wakaztahir/work/Chemical/chemical/lang/libs/std/posix/condvar.ch", 
			.line = 68, 
			.character = 21
		});
	}
}
_Bool std_stdCondVartimed_wait(struct std_stdCondVar* self, struct std_stdmutex* mutex, unsigned long timeout_ms){
	struct timespec ts;
	std_stdcompute_abstime_ms(&ts, timeout_ms);
	int r = pthread_cond_timedwait(&self->storage[0], &mutex->storage[0], &ts);
	return (r == 0);
}
void std_stdCondVarnotify_one(struct std_stdCondVar* self){
	int r = pthread_cond_signal(&self->storage[0]);
	if((r != 0)){
		std_raw_panic("pthread_cond_signal failed", &(struct std_Location){ 
			.filename = "/home/wakaztahir/work/Chemical/chemical/lang/libs/std/posix/condvar.ch", 
			.line = 85, 
			.character = 21
		});
	}
}
void std_stdCondVarnotify_all(struct std_stdCondVar* self){
	int r = pthread_cond_broadcast(&self->storage[0]);
	if((r != 0)){
		std_raw_panic("pthread_cond_broadcast failed", &(struct std_Location){ 
			.filename = "/home/wakaztahir/work/Chemical/chemical/lang/libs/std/posix/condvar.ch", 
			.line = 92, 
			.character = 21
		});
	}
}
void std_stdCondVardelete(struct std_stdCondVar* self){
	int r = pthread_cond_destroy(&self->storage[0]);
	__chx__dstctr_clnup_blk__:{
	}
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/std/posix/mutex.ch **/
void std_stdmutexconstructor(struct std_stdmutex* this){
	struct std_stdmutex m = (struct std_stdmutex){ 
		.storage = {}
	};
	_Bool __chx__lv__164 = true;
	int res = pthread_mutex_init(&m.storage[0], NULL);
	if((res != 0)){
		std_raw_panic("pthread_mutex_init failed", &(struct std_Location){ 
			.filename = "/home/wakaztahir/work/Chemical/chemical/lang/libs/std/posix/mutex.ch", 
			.line = 31, 
			.character = 21
		});
	}
	*this = m;
	return;
}
void std_stdmutexlock(struct std_stdmutex* self){
	int res = pthread_mutex_lock(&self->storage[0]);
	if((res != 0)){
		std_raw_panic("pthread_mutex_lock failed", &(struct std_Location){ 
			.filename = "/home/wakaztahir/work/Chemical/chemical/lang/libs/std/posix/mutex.ch", 
			.line = 40, 
			.character = 21
		});
	}
}
_Bool std_stdmutextry_lock(struct std_stdmutex* self){
	int res = pthread_mutex_trylock(&self->storage[0]);
	if((res == 0)){
		return 1;
	} else {
		return 0;
	}
}
void std_stdmutexunlock(struct std_stdmutex* self){
	int res = pthread_mutex_unlock(&self->storage[0]);
	if((res != 0)){
		std_raw_panic("pthread_mutex_unlock failed", &(struct std_Location){ 
			.filename = "/home/wakaztahir/work/Chemical/chemical/lang/libs/std/posix/mutex.ch", 
			.line = 60, 
			.character = 21
		});
	}
}
void std_stdmutexdelete(struct std_stdmutex* self){
	int res = pthread_mutex_destroy(&self->storage[0]);
	if((res != 0)){
	}
	__chx__dstctr_clnup_blk__:{
	}
}