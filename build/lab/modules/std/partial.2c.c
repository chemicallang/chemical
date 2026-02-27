
/** FwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\concurrency\threadpool.ch **/
struct std_stdconcurrentThread;
struct std_stdconcurrentTask;
struct std_stdconcurrentPoolData;
struct std_stdconcurrentThreadPool;
/** FwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\fs.ch **/
/** FwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\function.ch **/
struct std_stddefault_function_instance;
/** FwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\hashing\compare_impl.ch **/
/** FwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\hashing\fnv1.ch **/
/** FwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\hashing\hash.ch **/
/** FwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\hashing\hash_impl.ch **/
/** FwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\hashing\murmur.ch **/
/** FwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\mutex.ch **/
struct std_stdlock_guard;
/** FwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\option.ch **/
struct std_stdOption__cgs__0;
/** FwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\os_string.ch **/
struct std_stdOsString;
/** FwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\pair.ch **/
/** FwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\panic.ch **/
struct std_Location;
/** FwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\result.ch **/
/** FwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\span.ch **/
/** FwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\std.ch **/
/** FwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\stream.ch **/
struct std_CommandLineStream;
/** FwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\string.ch **/
union std_stdu64_double_union;
struct std_stdstring;
struct std_stdStringStream;
/** FwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\string_view.ch **/
struct std_stdstring_view;
/** FwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\u16_string.ch **/
struct std_stdu16string;
/** FwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\unordered_map2.ch **/
/** FwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\utils.ch **/
/** FwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\vector.ch **/
struct std_stdvector__cgs__0;
struct std_stdvector__cgs__1;
/** FwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\win\condvar.ch **/
struct std_stdCondVar;
/** FwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\win\mutex.ch **/
struct std_stdmutex;
/** TypeAliasDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\concurrency\threadpool.ch **/
/** TypeAliasDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\fs.ch **/
/** TypeAliasDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\function.ch **/
typedef void(*std_stddestructor_type)(void* obj);
struct std_stddefault_function_instance {
	void* fn_pointer;
	void* fn_data_ptr;
	char buffer[32];
	void(*dtor)(void* obj);
	_Bool is_heap;
};
typedef struct std_stddefault_function_instance std_stdfunction;
/** TypeAliasDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\hashing\compare_impl.ch **/
/** TypeAliasDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\hashing\fnv1.ch **/
/** TypeAliasDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\hashing\hash.ch **/
/** TypeAliasDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\hashing\hash_impl.ch **/
/** TypeAliasDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\hashing\murmur.ch **/
/** TypeAliasDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\mutex.ch **/
/** TypeAliasDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\option.ch **/
/** TypeAliasDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\os_string.ch **/
typedef uint16_t std_stdnative_char_t;
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
typedef struct std_stdu16string std_stdnative_string_t;
/** TypeAliasDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\pair.ch **/
/** TypeAliasDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\panic.ch **/
/** TypeAliasDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\result.ch **/
/** TypeAliasDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\span.ch **/
/** TypeAliasDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\std.ch **/
/** TypeAliasDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\stream.ch **/
/** TypeAliasDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\string.ch **/
/** TypeAliasDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\string_view.ch **/
/** TypeAliasDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\u16_string.ch **/
/** TypeAliasDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\unordered_map2.ch **/
/** TypeAliasDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\utils.ch **/
/** TypeAliasDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\vector.ch **/
/** TypeAliasDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\win\condvar.ch **/
struct std_stdCondVar {
	uint8_t storage[8];
};
typedef struct std_stdCondVar std_stdcondvar;
/** TypeAliasDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\win\mutex.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\char\ctype.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\char\sys_types.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\char\uchar.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\char\wchar.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\char\wctype.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\common\arg_types.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\common\atomic_types.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\common\char_types.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\common\integer_types.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\common\io_types.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\common\multibyte_char_types.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\common\std_types.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\common\time_types.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\common\wchar_types.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\concurrency\stdatomic.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\concurrency\threads.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\core\float.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\core\inttypes.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\core\limits.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\core\stddef.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\core\stdint.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\diagnostics\assert.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\diagnostics\errno.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\diagnostics\setjmp.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\diagnostics\signal.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\io\stdarg.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\io\stdio.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\io\stdlib.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\io\string.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\io\sys_stat.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\locale\iso646.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\locale\locale.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\math\complex.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\math\fenv.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\math\math.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\math\tgmath.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\time\time.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\windows\diagnostics\signal.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\windows\errno.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\windows\io\stdio.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\windows\io\sys_stat.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\windows\minwinbase.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\windows\processthreadsapi.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\windows\stringapiset.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\windows\synchapi.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\windows\winbase.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\windows\windows.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\windows\winerror.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\core\ops.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\char\ctype.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\char\sys_types.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\char\uchar.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\char\wchar.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\char\wctype.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\common\arg_types.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\common\atomic_types.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\common\char_types.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\common\integer_types.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\common\io_types.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\common\multibyte_char_types.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\common\std_types.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\common\time_types.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\common\wchar_types.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\concurrency\stdatomic.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\concurrency\threads.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\core\float.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\core\inttypes.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\core\limits.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\core\stddef.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\core\stdint.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\diagnostics\assert.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\diagnostics\errno.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\diagnostics\setjmp.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\diagnostics\signal.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\io\stdarg.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\io\stdio.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\io\stdlib.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\io\string.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\io\sys_stat.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\locale\iso646.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\locale\locale.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\math\complex.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\math\fenv.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\math\math.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\math\tgmath.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\time\time.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\windows\diagnostics\signal.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\windows\errno.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\windows\io\stdio.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\windows\io\sys_stat.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\windows\minwinbase.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\windows\processthreadsapi.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\windows\stringapiset.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\windows\synchapi.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\windows\winbase.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\windows\windows.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\windows\winerror.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\core\ops.ch **/
/** Declare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\concurrency\threadpool.ch **/
extern __chem_dllimport cstd_HANDLE __chem_stdcall CreateThread(const void* lpThreadAttributes, unsigned long dwStackSize, const void* lpStartAddress, const void* lpParameter, unsigned long dwCreationFlags, unsigned long* lpThreadId);
struct std_SYSTEM_INFO {
	uint32_t dwOemId;
	uint32_t dwPageSize;
	const void* lpMinimumApplicationAddress;
	const void* lpMaximumApplicationAddress;
	cstd_usize dwActiveProcessorMask;
	uint32_t dwNumberOfProcessors;
	uint32_t dwProcessorType;
	uint32_t dwAllocationGranularity;
	uint16_t wProcessorLevel;
	uint16_t wProcessorRevision;
};
extern __chem_dllimport void __chem_stdcall GetSystemInfo(const struct std_SYSTEM_INFO* lpSystemInfo);
cstd_usize std_stdconcurrenthardware_threads();
void std_stdconcurrentsleep_ms(unsigned long ms);
static cstd_usize std_stdconcurrentspawn_native(const void*(*entry)(const void* arg), const void* arg);
static void std_stdconcurrentjoin_native(cstd_usize h);
struct std_stdconcurrentThread {
	cstd_usize handle;
};
void std_stdconcurrentThreadjoin(struct std_stdconcurrentThread*const self);
void std_stdconcurrentspawn(struct std_stdconcurrentThread* __chx_struct_ret_param_xx, const void*(*entry)(const void* arg), const void* arg);
typedef struct std_stddefault_function_instance std_stdfunction;
struct std_stdconcurrentTask {
	std_stdfunction f;
};
static const void* std_stdconcurrentworker_main(const void* arg);
struct std_stdmutex {
	uint8_t storage[40];
};
typedef struct std_stdCondVar std_stdcondvar;
struct std_stdvector__cgs__0 {
	struct std_stdconcurrentTask* data_ptr;
	size_t data_size;
	size_t data_cap;
};
struct std_stdvector__cgs__1 {
	cstd_usize* data_ptr;
	size_t data_size;
	size_t data_cap;
};
struct std_stdconcurrentPoolData {
	struct std_stdmutex m;
	std_stdcondvar cv;
	struct std_stdvector__cgs__0 q;
	struct std_stdvector__cgs__1 workers;
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
/** Declare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\fs.ch **/
int std_fsmkdir(const char* pathname);
static int std_fsfind_last_pos_of_or(const char* str, size_t len, char value, char value2);
void std_fsparent_path(struct std_stdstring* __chx_struct_ret_param_xx, struct std_stdstring_view* str);
int std_fswrite_to_file(const char* filepath, const char* data);




_Bool std_fspath_exists(const char* path);
_Bool std_fsis_directory(const char* path);
int std_fscopy_file(const char* src, const char* dest, int options);
int std_fscopy_directory(const char* src_dir, const char* dest_dir, int options);
/** Declare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\function.ch **/
typedef void(*std_stddestructor_type)(void* obj);;
void std_stddefault_function_instancemake2(struct std_stddefault_function_instance* this, void* ptr, void* cap, std_stddestructor_type destr, size_t size_data, size_t align_data);
void* std_stddefault_function_instanceget_fn_ptr(struct std_stddefault_function_instance*const self);
void* std_stddefault_function_instanceget_data_ptr(struct std_stddefault_function_instance*const self);
void std_stddefault_function_instancedelete(struct std_stddefault_function_instance*const self);
typedef struct std_stddefault_function_instance std_stdfunction;
/** Declare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\hashing\compare_impl.ch **/
_Bool std_char_equals(char*const self, char*const other);
_Bool std_uchar_equals(unsigned char*const self, unsigned char*const other);
_Bool std_short_equals(short*const self, short*const other);
_Bool std_ushort_equals(unsigned short*const self, unsigned short*const other);
_Bool std_int_equals(int*const self, int*const other);
_Bool std_uint_equals(unsigned int*const self, unsigned int*const other);
_Bool std_long_equals(long*const self, long*const other);
_Bool std_ulong_equals(unsigned long*const self, unsigned long*const other);
_Bool std_i64_equals(int64_t*const self, int64_t*const other);
_Bool std_u64_equals(uint64_t*const self, uint64_t*const other);
/** Declare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\hashing\fnv1.ch **/
unsigned int std_fnv1a_hash_32(const char* str);
size_t std_fnv1_hash_view(struct std_stdstring_view*const view);
size_t std_fnv1_hash(const char* s);
/** Declare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\hashing\hash.ch **/
typedef struct __chx_std_Eq_vt_t {
	_Bool(*equals)(void* self, void**const other);
} __chx_std_Eq_vt_t;
const __chx_std_Eq_vt_t std_Eq_char = {
	(_Bool(*)(void* self, void**const other)) std_char_equals,
};
const __chx_std_Eq_vt_t std_Eq_uchar = {
	(_Bool(*)(void* self, void**const other)) std_uchar_equals,
};
const __chx_std_Eq_vt_t std_Eq_short = {
	(_Bool(*)(void* self, void**const other)) std_short_equals,
};
const __chx_std_Eq_vt_t std_Eq_ushort = {
	(_Bool(*)(void* self, void**const other)) std_ushort_equals,
};
const __chx_std_Eq_vt_t std_Eq_int = {
	(_Bool(*)(void* self, void**const other)) std_int_equals,
};
const __chx_std_Eq_vt_t std_Eq_uint = {
	(_Bool(*)(void* self, void**const other)) std_uint_equals,
};
const __chx_std_Eq_vt_t std_Eq_long = {
	(_Bool(*)(void* self, void**const other)) std_long_equals,
};
const __chx_std_Eq_vt_t std_Eq_ulong = {
	(_Bool(*)(void* self, void**const other)) std_ulong_equals,
};
const __chx_std_Eq_vt_t std_Eq_i64 = {
	(_Bool(*)(void* self, void**const other)) std_i64_equals,
};
const __chx_std_Eq_vt_t std_Eq_u64 = {
	(_Bool(*)(void* self, void**const other)) std_u64_equals,
};
_Bool std_stdstringequals(struct std_stdstring*const self, struct std_stdstring*const other);
const __chx_std_Eq_vt_t std_Eqstd_stdstring = {
	(_Bool(*)(void* self, void**const other)) std_stdstringequals,
};
_Bool std_stdstring_viewequals(struct std_stdstring_view*const self, struct std_stdstring_view*const other);
const __chx_std_Eq_vt_t std_Eqstd_stdstring_view = {
	(_Bool(*)(void* self, void**const other)) std_stdstring_viewequals,
};
typedef struct __chx_std_Hashable_vt_t {
	unsigned int(*hash)(void* self);
} __chx_std_Hashable_vt_t;
unsigned int std_stdstringhash(struct std_stdstring*const self);
const __chx_std_Hashable_vt_t std_Hashablestd_stdstring = {
	(unsigned int(*)(void* self)) std_stdstringhash,
};
unsigned int std_stdstring_viewhash(struct std_stdstring_view*const self);
const __chx_std_Hashable_vt_t std_Hashablestd_stdstring_view = {
	(unsigned int(*)(void* self)) std_stdstring_viewhash,
};
/** Declare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\hashing\hash_impl.ch **/

size_t std_char_hash(char*const self);
const __chx_std_Hashable_vt_t std_Hashable_char = {
	(unsigned int(*)(void* self)) std_char_hash,
};
size_t std_uchar_hash(unsigned char*const self);
const __chx_std_Hashable_vt_t std_Hashable_uchar = {
	(unsigned int(*)(void* self)) std_uchar_hash,
};
size_t std_short_hash(short*const self);
const __chx_std_Hashable_vt_t std_Hashable_short = {
	(unsigned int(*)(void* self)) std_short_hash,
};
size_t std_ushort_hash(unsigned short*const self);
const __chx_std_Hashable_vt_t std_Hashable_ushort = {
	(unsigned int(*)(void* self)) std_ushort_hash,
};
size_t std_int_hash(int*const self);
const __chx_std_Hashable_vt_t std_Hashable_int = {
	(unsigned int(*)(void* self)) std_int_hash,
};
size_t std_uint_hash(unsigned int*const self);
const __chx_std_Hashable_vt_t std_Hashable_uint = {
	(unsigned int(*)(void* self)) std_uint_hash,
};
size_t std_long_hash(long*const self);
const __chx_std_Hashable_vt_t std_Hashable_long = {
	(unsigned int(*)(void* self)) std_long_hash,
};
size_t std_ulong_hash(unsigned long*const self);
const __chx_std_Hashable_vt_t std_Hashable_ulong = {
	(unsigned int(*)(void* self)) std_ulong_hash,
};
size_t std_i64_hash(int64_t*const self);
const __chx_std_Hashable_vt_t std_Hashable_i64 = {
	(unsigned int(*)(void* self)) std_i64_hash,
};
size_t std_u64_hash(uint64_t*const self);
const __chx_std_Hashable_vt_t std_Hashable_u64 = {
	(unsigned int(*)(void* self)) std_u64_hash,
};
/** Declare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\hashing\murmur.ch **/
uint32_t std_murmurhash(const char* key, uint32_t len, uint32_t seed);
/** Declare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\mutex.ch **/
struct std_stdlock_guard {
	struct std_stdmutex* m;
};
void std_stdlock_guardconstructor(struct std_stdlock_guard* this, struct std_stdmutex* mtx);
void std_stdlock_guarddelete(struct std_stdlock_guard* self);
/** Declare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\option.ch **/
struct std_stdOption__cgs__0 {
	int __chx__vt_621827;
	union {
		struct {
			struct std_stdconcurrentTask value;
		} Some;
		struct {
		} None;
	};
};
void std_stdOption__cgs__0take(struct std_stdconcurrentTask* __chx_struct_ret_param_xx, struct std_stdOption__cgs__0*const self);
/** Declare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\os_string.ch **/


typedef uint16_t std_stdnative_char_t;
typedef struct std_stdu16string std_stdnative_string_t;
typedef struct std_stdu16string std_stdnative_string_t;
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
static void std_stdOsStringutf8_to_u16(struct std_stdu16string* __chx_struct_ret_param_xx, struct std_stdstring*const utf8);
static void std_stdOsStringu16_to_utf8(struct std_stdstring* __chx_struct_ret_param_xx, struct std_stdu16string*const str);
static void std_stdOsStringdelete(struct std_stdOsString* self);
/** Declare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\pair.ch **/
/** Declare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\panic.ch **/
struct std_Location {
	const char* filename;
	unsigned int line;
	unsigned int character;
};
void std_raw_panic(const char* message, struct std_Location* location);
/** Declare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\result.ch **/
/** Declare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\span.ch **/
/** Declare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\std.ch **/
/** Declare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\stream.ch **/
typedef struct __chx_std_Stream_vt_t {
	void(*writeStr)(void* self, const char* value, uint64_t length);void(*writeStrNoLen)(void* self, const char* value);void(*writeI8)(void* self, int8_t value);void(*writeI16)(void* self, int16_t value);void(*writeI32)(void* self, int32_t value);void(*writeI64)(void* self, int64_t value);void(*writeU8)(void* self, uint8_t value);void(*writeU16)(void* self, uint16_t value);void(*writeU32)(void* self, uint32_t value);void(*writeU64)(void* self, uint64_t value);void(*writeChar)(void* self, char value);void(*writeUChar)(void* self, unsigned char value);void(*writeShort)(void* self, short value);void(*writeUShort)(void* self, unsigned short value);void(*writeInt)(void* self, int value);void(*writeUInt)(void* self, unsigned int value);void(*writeLong)(void* self, long value);void(*writeULong)(void* self, unsigned long value);void(*writeLongLong)(void* self, long long value);void(*writeULongLong)(void* self, unsigned long long value);void(*writeFloat)(void* self, float value);void(*writeDouble)(void* self, double value);
} __chx_std_Stream_vt_t;
void std_CommandLineStreamwriteStr(struct std_CommandLineStream*const self, const char* value, uint64_t length);
void std_CommandLineStreamwriteStrNoLen(struct std_CommandLineStream*const self, const char* value);
void std_CommandLineStreamwriteI8(struct std_CommandLineStream*const self, int8_t value);
void std_CommandLineStreamwriteI16(struct std_CommandLineStream*const self, int16_t value);
void std_CommandLineStreamwriteI32(struct std_CommandLineStream*const self, int32_t value);
void std_CommandLineStreamwriteI64(struct std_CommandLineStream*const self, int64_t value);
void std_CommandLineStreamwriteU8(struct std_CommandLineStream*const self, uint8_t value);
void std_CommandLineStreamwriteU16(struct std_CommandLineStream*const self, uint16_t value);
void std_CommandLineStreamwriteU32(struct std_CommandLineStream*const self, uint32_t value);
void std_CommandLineStreamwriteU64(struct std_CommandLineStream*const self, uint64_t value);
void std_CommandLineStreamwriteChar(struct std_CommandLineStream*const self, char value);
void std_CommandLineStreamwriteUChar(struct std_CommandLineStream*const self, unsigned char value);
void std_CommandLineStreamwriteShort(struct std_CommandLineStream*const self, short value);
void std_CommandLineStreamwriteUShort(struct std_CommandLineStream*const self, unsigned short value);
void std_CommandLineStreamwriteInt(struct std_CommandLineStream*const self, int value);
void std_CommandLineStreamwriteUInt(struct std_CommandLineStream*const self, unsigned int value);
void std_CommandLineStreamwriteLong(struct std_CommandLineStream*const self, long value);
void std_CommandLineStreamwriteULong(struct std_CommandLineStream*const self, unsigned long value);
void std_CommandLineStreamwriteLongLong(struct std_CommandLineStream*const self, long long value);
void std_CommandLineStreamwriteULongLong(struct std_CommandLineStream*const self, unsigned long long value);
void std_CommandLineStreamwriteFloat(struct std_CommandLineStream*const self, float value);
void std_CommandLineStreamwriteDouble(struct std_CommandLineStream*const self, double value);
const __chx_std_Stream_vt_t std_Streamstd_CommandLineStream = {
	(void(*)(void* self, const char* value, uint64_t length)) std_CommandLineStreamwriteStr,
	(void(*)(void* self, const char* value)) std_CommandLineStreamwriteStrNoLen,
	(void(*)(void* self, int8_t value)) std_CommandLineStreamwriteI8,
	(void(*)(void* self, int16_t value)) std_CommandLineStreamwriteI16,
	(void(*)(void* self, int32_t value)) std_CommandLineStreamwriteI32,
	(void(*)(void* self, int64_t value)) std_CommandLineStreamwriteI64,
	(void(*)(void* self, uint8_t value)) std_CommandLineStreamwriteU8,
	(void(*)(void* self, uint16_t value)) std_CommandLineStreamwriteU16,
	(void(*)(void* self, uint32_t value)) std_CommandLineStreamwriteU32,
	(void(*)(void* self, uint64_t value)) std_CommandLineStreamwriteU64,
	(void(*)(void* self, char value)) std_CommandLineStreamwriteChar,
	(void(*)(void* self, unsigned char value)) std_CommandLineStreamwriteUChar,
	(void(*)(void* self, short value)) std_CommandLineStreamwriteShort,
	(void(*)(void* self, unsigned short value)) std_CommandLineStreamwriteUShort,
	(void(*)(void* self, int value)) std_CommandLineStreamwriteInt,
	(void(*)(void* self, unsigned int value)) std_CommandLineStreamwriteUInt,
	(void(*)(void* self, long value)) std_CommandLineStreamwriteLong,
	(void(*)(void* self, unsigned long value)) std_CommandLineStreamwriteULong,
	(void(*)(void* self, long long value)) std_CommandLineStreamwriteLongLong,
	(void(*)(void* self, unsigned long long value)) std_CommandLineStreamwriteULongLong,
	(void(*)(void* self, float value)) std_CommandLineStreamwriteFloat,
	(void(*)(void* self, double value)) std_CommandLineStreamwriteDouble,
};
void std_stdStringStreamwriteStr(struct std_stdStringStream*const self, const char* value, uint64_t length);
void std_stdStringStreamwriteStrNoLen(struct std_stdStringStream*const self, const char* value);
void std_stdStringStreamwriteI8(struct std_stdStringStream*const self, int8_t value);
void std_stdStringStreamwriteI16(struct std_stdStringStream*const self, int16_t value);
void std_stdStringStreamwriteI32(struct std_stdStringStream*const self, int32_t value);
void std_stdStringStreamwriteI64(struct std_stdStringStream*const self, int64_t value);
void std_stdStringStreamwriteU8(struct std_stdStringStream*const self, uint8_t value);
void std_stdStringStreamwriteU16(struct std_stdStringStream*const self, uint16_t value);
void std_stdStringStreamwriteU32(struct std_stdStringStream*const self, uint32_t value);
void std_stdStringStreamwriteU64(struct std_stdStringStream*const self, uint64_t value);
void std_stdStringStreamwriteChar(struct std_stdStringStream*const self, char value);
void std_stdStringStreamwriteUChar(struct std_stdStringStream*const self, unsigned char value);
void std_stdStringStreamwriteShort(struct std_stdStringStream*const self, short value);
void std_stdStringStreamwriteUShort(struct std_stdStringStream*const self, unsigned short value);
void std_stdStringStreamwriteInt(struct std_stdStringStream*const self, int value);
void std_stdStringStreamwriteUInt(struct std_stdStringStream*const self, unsigned int value);
void std_stdStringStreamwriteLong(struct std_stdStringStream*const self, long value);
void std_stdStringStreamwriteULong(struct std_stdStringStream*const self, unsigned long value);
void std_stdStringStreamwriteLongLong(struct std_stdStringStream*const self, long long value);
void std_stdStringStreamwriteULongLong(struct std_stdStringStream*const self, unsigned long long value);
void std_stdStringStreamwriteFloat(struct std_stdStringStream*const self, float value);
void std_stdStringStreamwriteDouble(struct std_stdStringStream*const self, double value);
const __chx_std_Stream_vt_t std_Streamstd_stdStringStream = {
	(void(*)(void* self, const char* value, uint64_t length)) std_stdStringStreamwriteStr,
	(void(*)(void* self, const char* value)) std_stdStringStreamwriteStrNoLen,
	(void(*)(void* self, int8_t value)) std_stdStringStreamwriteI8,
	(void(*)(void* self, int16_t value)) std_stdStringStreamwriteI16,
	(void(*)(void* self, int32_t value)) std_stdStringStreamwriteI32,
	(void(*)(void* self, int64_t value)) std_stdStringStreamwriteI64,
	(void(*)(void* self, uint8_t value)) std_stdStringStreamwriteU8,
	(void(*)(void* self, uint16_t value)) std_stdStringStreamwriteU16,
	(void(*)(void* self, uint32_t value)) std_stdStringStreamwriteU32,
	(void(*)(void* self, uint64_t value)) std_stdStringStreamwriteU64,
	(void(*)(void* self, char value)) std_stdStringStreamwriteChar,
	(void(*)(void* self, unsigned char value)) std_stdStringStreamwriteUChar,
	(void(*)(void* self, short value)) std_stdStringStreamwriteShort,
	(void(*)(void* self, unsigned short value)) std_stdStringStreamwriteUShort,
	(void(*)(void* self, int value)) std_stdStringStreamwriteInt,
	(void(*)(void* self, unsigned int value)) std_stdStringStreamwriteUInt,
	(void(*)(void* self, long value)) std_stdStringStreamwriteLong,
	(void(*)(void* self, unsigned long value)) std_stdStringStreamwriteULong,
	(void(*)(void* self, long long value)) std_stdStringStreamwriteLongLong,
	(void(*)(void* self, unsigned long long value)) std_stdStringStreamwriteULongLong,
	(void(*)(void* self, float value)) std_stdStringStreamwriteFloat,
	(void(*)(void* self, double value)) std_stdStringStreamwriteDouble,
};
struct std_CommandLineStream {
};
size_t std_CommandLineStreamu64_to_chars(struct std_CommandLineStream*const self, char* out_buf, uint64_t value);
size_t std_CommandLineStreami64_to_chars(struct std_CommandLineStream*const self, char* out_buf, int64_t value);
size_t std_CommandLineStreamdouble_to_chars(struct std_CommandLineStream*const self, char* out_buf, double value, int precision);
size_t std_CommandLineStreamfloat_to_chars(struct std_CommandLineStream*const self, char* out_buf, float value, int precision);
/** Declare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\string.ch **/

union std_stdu64_double_union {
	uint64_t u;
	double d;
};
static uint64_t std_stddbl_bits(double x);
static double std_stddbl_from_bits(uint64_t b);



static _Bool std_stddbl_is_nan(double x);
static _Bool std_stddbl_is_inf(double x);
static _Bool std_stddbl_is_neg(double x);
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
};;
void std_stdstringconstructor(struct std_stdstring* this, const char* value, size_t length);
void std_stdstringview_make(struct std_stdstring* this, struct std_stdstring_view*const value);
void std_stdstringconstructor2(struct std_stdstring* this, const char* value, size_t length, _Bool ensure);
void std_stdstringempty_str(struct std_stdstring* this);
void std_stdstringmake_no_len(struct std_stdstring* this, const char* value);
void std_stdstringmake_with_char(struct std_stdstring* this, char value);
size_t std_stdstringsize(struct std_stdstring*const self);
void std_stdstringresize_unsafe(struct std_stdstring*const self, size_t value);
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
_Bool std_stdstringcontains(struct std_stdstring*const self, struct std_stdstring_view*const needle);
_Bool std_stdstringends_with(struct std_stdstring*const self, struct std_stdstring_view*const other);
void std_stdstringerase(struct std_stdstring* self, size_t start, size_t len);
size_t std_stdstringcapacity(struct std_stdstring* self);
const char* std_stdstringdata(struct std_stdstring*const self);
const char* std_stdstringc_str(struct std_stdstring*const self);
char* std_stdstringmutable_data(struct std_stdstring* self);
void std_stdstringclear(struct std_stdstring* self);
void std_stdstringto_view(struct std_stdstring_view* __chx_struct_ret_param_xx, struct std_stdstring*const self);
void std_stdstringdelete(struct std_stdstring* self);
struct std_stdStringStream {
	struct std_stdstring* str;
};
/** Declare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\string_view.ch **/
struct std_stdstring_view {
	const char* _data;
	size_t _size;
};;
void std_stdstring_viewempty_make(struct std_stdstring_view* this);
void std_stdstring_viewconstructor(struct std_stdstring_view* this, const char* value, size_t length);
void std_stdstring_viewmake_view(struct std_stdstring_view* this, struct std_stdstring*const str);
void std_stdstring_viewmake_no_len(struct std_stdstring_view* this, const char* value);
const char* std_stdstring_viewdata(struct std_stdstring_view*const self);
size_t std_stdstring_viewsize(struct std_stdstring_view*const self);
_Bool std_stdstring_viewempty(struct std_stdstring_view*const self);
char std_stdstring_viewget(struct std_stdstring_view*const self, size_t index);
void std_stdstring_viewsubview(struct std_stdstring_view* __chx_struct_ret_param_xx, struct std_stdstring_view*const self, size_t start, size_t end);
void std_stdstring_viewskip(struct std_stdstring_view* __chx_struct_ret_param_xx, struct std_stdstring_view*const self, size_t count);
size_t std_stdstring_viewfind(struct std_stdstring_view*const self, struct std_stdstring_view*const needle);
_Bool std_stdstring_viewcontains(struct std_stdstring_view*const self, struct std_stdstring_view*const needle);
_Bool std_stdstring_viewends_with(struct std_stdstring_view*const self, struct std_stdstring_view*const other);
/** Declare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\u16_string.ch **/

static uint16_t std_stdEMPTY_U16;
size_t std_stdu16_strlen(const uint16_t* ptr);
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
/** Declare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\unordered_map2.ch **/

/** Declare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\utils.ch **/
void std_stdreplace__cfg_0(struct std_stdconcurrentTask* __chx_struct_ret_param_xx, struct std_stdconcurrentTask* value, struct std_stdconcurrentTask* repl);

static size_t std_stdinternal_view_find(struct std_stdstring_view*const me, struct std_stdstring_view*const needle);
/** Declare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\vector.ch **/
void std_stdvector__cgs__0make(struct std_stdvector__cgs__0* this);
void std_stdvector__cgs__0make_with_capacity(struct std_stdvector__cgs__0* this, size_t init_cap);
void std_stdvector__cgs__0resize(struct std_stdvector__cgs__0* self, size_t new_cap);
void std_stdvector__cgs__0reserve(struct std_stdvector__cgs__0* self, size_t cap);
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
void std_stdvector__cgs__0resize_unsafe(struct std_stdvector__cgs__0* self, size_t new_size);
void std_stdvector__cgs__0delete(struct std_stdvector__cgs__0* self);
void std_stdvector__cgs__1make(struct std_stdvector__cgs__1* this);
void std_stdvector__cgs__1make_with_capacity(struct std_stdvector__cgs__1* this, size_t init_cap);
void std_stdvector__cgs__1resize(struct std_stdvector__cgs__1* self, size_t new_cap);
void std_stdvector__cgs__1reserve(struct std_stdvector__cgs__1* self, size_t cap);
void std_stdvector__cgs__1ensure_capacity_for_one_more(struct std_stdvector__cgs__1* self);
void std_stdvector__cgs__1push(struct std_stdvector__cgs__1* self, cstd_usize value);
void std_stdvector__cgs__1push_back(struct std_stdvector__cgs__1* self, cstd_usize value);
cstd_usize std_stdvector__cgs__1get(struct std_stdvector__cgs__1*const self, size_t index);
cstd_usize* std_stdvector__cgs__1get_ptr(struct std_stdvector__cgs__1*const self, size_t index);
cstd_usize* std_stdvector__cgs__1get_ref(struct std_stdvector__cgs__1*const self, size_t index);
void std_stdvector__cgs__1set(struct std_stdvector__cgs__1* self, size_t index, cstd_usize value);
size_t std_stdvector__cgs__1size(struct std_stdvector__cgs__1*const self);
size_t std_stdvector__cgs__1capacity(struct std_stdvector__cgs__1*const self);
const cstd_usize* std_stdvector__cgs__1data(struct std_stdvector__cgs__1*const self);
cstd_usize* std_stdvector__cgs__1last_ptr(struct std_stdvector__cgs__1*const self);
void std_stdvector__cgs__1remove(struct std_stdvector__cgs__1* self, size_t index);
void std_stdvector__cgs__1erase(struct std_stdvector__cgs__1* self, size_t index);
void std_stdvector__cgs__1remove_last(struct std_stdvector__cgs__1* self);
void std_stdvector__cgs__1pop_back(struct std_stdvector__cgs__1* self);
cstd_usize std_stdvector__cgs__1take_last(struct std_stdvector__cgs__1* self);
_Bool std_stdvector__cgs__1empty(struct std_stdvector__cgs__1*const self);
void std_stdvector__cgs__1clear(struct std_stdvector__cgs__1* self);
void std_stdvector__cgs__1resize_unsafe(struct std_stdvector__cgs__1* self, size_t new_size);
void std_stdvector__cgs__1delete(struct std_stdvector__cgs__1* self);
/** Declare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\win\condvar.ch **/
extern __chem_dllimport void __chem_stdcall InitializeConditionVariable(uint8_t* cond);
extern __chem_dllimport void __chem_stdcall WakeConditionVariable(uint8_t* cond);
extern __chem_dllimport void __chem_stdcall WakeAllConditionVariable(uint8_t* cond);
extern __chem_dllimport int __chem_stdcall SleepConditionVariableCS(uint8_t* cond, uint8_t* cs, unsigned long ms);

void std_stdCondVarconstructor(struct std_stdCondVar* this);
void std_stdCondVarwait(struct std_stdCondVar* self, struct std_stdmutex* mutex);
_Bool std_stdCondVartimed_wait(struct std_stdCondVar* self, struct std_stdmutex* mutex, unsigned long timeout_ms);
void std_stdCondVarnotify_one(struct std_stdCondVar* self);
void std_stdCondVarsignal(struct std_stdCondVar* self);
void std_stdCondVarnotify_all(struct std_stdCondVar* self);
void std_stdCondVardelete(struct std_stdCondVar* self);
typedef struct std_stdCondVar std_stdcondvar;
/** Declare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\win\mutex.ch **/
extern __chem_dllimport void __chem_stdcall InitializeCriticalSectionAndSpinCount(uint8_t* cs, unsigned long spin);
extern __chem_dllimport void __chem_stdcall EnterCriticalSection(uint8_t* cs);
extern __chem_dllimport int __chem_stdcall TryEnterCriticalSection(uint8_t* cs);
extern __chem_dllimport void __chem_stdcall LeaveCriticalSection(uint8_t* cs);
extern __chem_dllimport void __chem_stdcall DeleteCriticalSection(uint8_t* cs);

void std_stdmutexconstructor(struct std_stdmutex* this);
void std_stdmutexlock(struct std_stdmutex* self);
_Bool std_stdmutextry_lock(struct std_stdmutex* self);
void std_stdmutexunlock(struct std_stdmutex* self);
void std_stdmutexdelete(struct std_stdmutex* self);
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\char\ctype.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\char\sys_types.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\char\uchar.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\char\wchar.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\char\wctype.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\common\arg_types.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\common\atomic_types.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\common\char_types.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\common\integer_types.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\common\io_types.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\common\multibyte_char_types.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\common\std_types.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\common\time_types.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\common\wchar_types.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\concurrency\stdatomic.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\concurrency\threads.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\core\float.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\core\inttypes.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\core\limits.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\core\stddef.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\core\stdint.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\diagnostics\assert.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\diagnostics\errno.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\diagnostics\setjmp.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\diagnostics\signal.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\io\stdarg.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\io\stdio.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\io\stdlib.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\io\string.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\io\sys_stat.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\locale\iso646.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\locale\locale.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\math\complex.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\math\fenv.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\math\math.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\math\tgmath.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\src\time\time.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\windows\diagnostics\signal.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\windows\errno.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\windows\io\stdio.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\windows\io\sys_stat.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\windows\minwinbase.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\windows\processthreadsapi.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\windows\stringapiset.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\windows\synchapi.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\windows\winbase.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\windows\windows.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\cstd\windows\winerror.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\core\ops.ch **/
/** Translate D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\concurrency\threadpool.ch **/
static void __chemda_467_0(void* this){
}




cstd_usize std_stdconcurrenthardware_threads(){
	
	struct std_SYSTEM_INFO info;
	GetSystemInfo(&info);
	return ((cstd_usize) info.dwNumberOfProcessors);
}
void std_stdconcurrentsleep_ms(unsigned long ms){
	
	Sleep(((uint32_t) ms));
}
static cstd_usize std_stdconcurrentspawn_native(const void*(*entry)(const void* arg), const void* arg){
	
	unsigned long tid = 0;
	return ((cstd_usize) CreateThread(NULL, 0, entry, arg, 0, &tid));
}
static void std_stdconcurrentjoin_native(cstd_usize h){
	
	WaitForSingleObject(((cstd_HANDLE) h), 4294967295);
	CloseHandle(((cstd_HANDLE) h));
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
		struct std_stdOption__cgs__0 opt = (struct std_stdOption__cgs__0) { .__chx__vt_621827 = 1, 
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
			struct std_stdconcurrentTask __chx__lv__0;
			struct std_stdconcurrentTask t = (*({ std_stdreplace__cfg_0(&__chx__lv__0, &*first_ele, &(struct std_stdconcurrentTask){ 
				.f = *({ __chemical_fat_pointer__* __chemda_467_0_pair = &(__chemical_fat_pointer__){__chemda_467_0,NULL}; &(*({ struct std_stddefault_function_instance __chx__lv__1; std_stddefault_function_instancemake2(&__chx__lv__1, __chemda_467_0_pair->first, __chemda_467_0_pair->second, ((std_stddestructor_type) NULL), 0, 0); &__chx__lv__1; })); })
			}); &__chx__lv__0; }));
			std_stdvector__cgs__0remove(&P->q, 0);
			opt = (struct std_stdOption__cgs__0) { .__chx__vt_621827 = 0, 
				.Some.value = t
			};
		}
		std_stdmutexunlock(&P->m);
		if((opt.__chx__vt_621827 == 0)){
			struct std_stdOption__cgs__0* __chx__lv__2 = &opt;
			({ struct std_stddefault_function_instance* __chx__lv__3 = &__chx__lv__2->Some.value.f; ((void(*)(void*))std_stddefault_function_instanceget_fn_ptr(__chx__lv__3))(std_stddefault_function_instanceget_data_ptr(__chx__lv__3)); });
		}
	}
	return NULL;
}
void std_stdconcurrentPoolDatasubmit_void(struct std_stdconcurrentPoolData* self, std_stdfunction* f){
	_Bool __chx__lv__4 = true;
	std_stdmutexlock(&self->m);
	std_stdvector__cgs__0push_back(&self->q, &(struct std_stdconcurrentTask){ 
		.f = ({ __chx__lv__4 = false; *f; })
	});
	std_stdCondVarnotify_one(&self->cv);
	std_stdmutexunlock(&self->m);
	if(__chx__lv__4) {
		std_stddefault_function_instancedelete(f);
	}
}
void std_stdconcurrentPoolDatadelete(struct std_stdconcurrentPoolData* self){
	std_stdmutexlock(&self->m);
	self->run = 0;
	std_stdCondVarnotify_all(&self->cv);
	std_stdmutexunlock(&self->m);
	unsigned int i = 0;
	while((i < std_stdvector__cgs__1size(&self->workers))) {
		std_stdconcurrentjoin_native(*std_stdvector__cgs__1get_ptr(&self->workers, i));
		i = (i + 1);
	}
	std_stdvector__cgs__1clear(&self->workers);
	std_stdvector__cgs__0clear(&self->q);
	__chx__dstctr_clnup_blk__:{
		std_stdmutexdelete(&self->m);
		std_stdCondVardelete(&self->cv);
		std_stdvector__cgs__0delete(&self->q);
		std_stdvector__cgs__1delete(&self->workers);
	}
}
void std_stdconcurrentThreadPoolsubmit_void(struct std_stdconcurrentThreadPool*const self, std_stdfunction* f){
	_Bool __chx__lv__5 = true;
	std_stdconcurrentPoolDatasubmit_void(self->data, ({ __chx__lv__5 = false; &*f; }));
	if(__chx__lv__5) {
		std_stddefault_function_instancedelete(f);
	}
}
void std_stdconcurrentThreadPooldelete(struct std_stdconcurrentThreadPool*const self){
	struct std_stdconcurrentPoolData*__chx__lv__6 = self->data;
	
	std_stdconcurrentPoolDatadelete(__chx__lv__6);
	free(__chx__lv__6);
	__chx__dstctr_clnup_blk__:{
	}
}
void std_stdconcurrentcreate_pool(struct std_stdconcurrentThreadPool* __chx_struct_ret_param_xx, unsigned int n){
	if((n == 0)){
		std_raw_panic("n==0", &(struct std_Location){ 
			.character = 28, 
			.filename = "D:\\Programming\\Cpp\\zig-bootstrap\\chemical\\lang\\libs\\std\\src\\concurrency\\threadpool.ch", 
			.line = 266
		});
	}
	struct std_stdconcurrentPoolData* p = ((struct std_stdconcurrentPoolData*) malloc(sizeof(struct std_stdconcurrentPoolData)));
	struct std_stdmutex __chx__lv__7;
	struct std_stdvector__cgs__1 __chx__lv__8;
	struct std_stdvector__cgs__0 __chx__lv__9;
	struct std_stdCondVar __chx__lv__10;
	({ struct std_stdconcurrentPoolData* __chx__lv__11 = p; *__chx__lv__11 = (struct std_stdconcurrentPoolData){ 
		.m = (*({ std_stdmutexconstructor(&__chx__lv__7); &__chx__lv__7; })), 
		.workers = (*({ std_stdvector__cgs__1make(&__chx__lv__8); &__chx__lv__8; })), 
		.q = (*({ std_stdvector__cgs__0make(&__chx__lv__9); &__chx__lv__9; })), 
		.cv = (*({ std_stdCondVarconstructor(&__chx__lv__10); &__chx__lv__10; })), 
		.run = 1
	}; __chx__lv__11; });
	unsigned int i = 0;
	while((i < n)) {
		cstd_usize h = std_stdconcurrentspawn_native(std_stdconcurrentworker_main, ((const void*) p));
		std_stdvector__cgs__1push_back(&p->workers, h);
		i = (i + 1);
	}
	*__chx_struct_ret_param_xx = (struct std_stdconcurrentThreadPool){ 
		.data = p
	};
	return;
}
/** Translate D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\fs.ch **/
int std_fsmkdir(const char* pathname){
	
	return _mkdir(pathname);
}
static int std_fsfind_last_pos_of_or(const char* str, size_t len, char value, char value2){
	if((len == 0)){
		return -1;
	}
	int i = (((int) len) - 1);
	while((i >= 0)) {
		if(((str[i] == value) || (str[i] == value2))){
			return i;
		}
		i--;
	}
	return -1;
}
void std_fsparent_path(struct std_stdstring* __chx_struct_ret_param_xx, struct std_stdstring_view* str){
	struct std_stdstring __chx__lv__12;
	struct std_stdstring final = (*({ std_stdstringempty_str(&__chx__lv__12); &__chx__lv__12; }));
	_Bool __chx__lv__13 = true;
	int pos;
	
	pos = std_fsfind_last_pos_of_or(std_stdstring_viewdata(str), std_stdstring_viewsize(str), '\\', '/');
	if((pos > 0)){
		std_stdstringappend_with_len(&final, std_stdstring_viewdata(str), ((size_t) (pos + 1)));
	}
	*__chx_struct_ret_param_xx = final;
	return;
}
int std_fswrite_to_file(const char* filepath, const char* data){
	struct FILE* file = fopen(filepath, "w");
	if((file == NULL)){
		return -1;
	}
	if((fputs(data, file) == -1)){
		fclose(file);
		return -2;
	}
	if((fclose(file) != 0)){
		return -3;
	}
	return 0;
}




_Bool std_fspath_exists(const char* path){
	
	return (GetFileAttributesA(path) != ((cstd_DWORD) 4294967295));
}
_Bool std_fsis_directory(const char* path){
	
	cstd_DWORD attributes = GetFileAttributesA(path);
	return ((attributes != ((cstd_DWORD) 4294967295)) && (attributes & ((cstd_DWORD) 16)));
}
int std_fscopy_file(const char* src, const char* dest, int options){
	if((!(options & (1 << 1)) && std_fspath_exists(dest))){
		if((options & (1 << 2))){
			return 0;
		}
		fprintf(cstd_get_stderr(), "File exists: %s\n", dest);
		return -1;
	}
	
	if(!CopyFileA(src, dest, !(options & (1 << 1)))){
		fprintf(cstd_get_stderr(), "CopyFileA failed: %s -> %s\n", src, dest);
		return -1;
	}
	return 0;
}
int std_fscopy_directory(const char* src_dir, const char* dest_dir, int options){
	if(!std_fsis_directory(src_dir)){
		fprintf(cstd_get_stderr(), "Not a directory: %s\n", src_dir);
		return -1;
	}
	
	if(!std_fspath_exists(dest_dir)){
		if(!CreateDirectoryA(dest_dir, NULL)){
			fprintf(cstd_get_stderr(), "CreateDirectoryA failed: %s\n", dest_dir);
			return -1;
		}
	}else if(!std_fsis_directory(dest_dir)){
		fprintf(cstd_get_stderr(), "Destination exists but is not a directory: %s\n", dest_dir);
		return -1;
	}
	
	struct cstd_WIN32_FIND_DATAA find_data;
	char search_path[1024];
	snprintf(search_path, sizeof(search_path), "%s\\*", src_dir);
	cstd_HANDLE hFind = FindFirstFileA(search_path, &find_data);
	if((hFind == ((cstd_HANDLE) -1))){
		fprintf(cstd_get_stderr(), "FindFirstFileA failed: %s\n", src_dir);
		return -1;
	}
	do {
		const char* name = &find_data.cFileName[0];
		if(((strcmp(name, ".") == 0) || (strcmp(name, "..") == 0))){
			continue;
		}
		char src_path[1024];
		char dst_path[1024];
		snprintf(src_path, sizeof(src_path), "%s\\%s", src_dir, name);
		snprintf(dst_path, sizeof(dst_path), "%s\\%s", dest_dir, name);
		unsigned long is_dir = (find_data.dwFileAttributes & ((cstd_DWORD) 16));
		if(is_dir){
			if((options & (1 << 0))){
				if((std_fscopy_directory(src_path, dst_path, options) != 0)){
					FindClose(hFind);
					return -1;
				}
			}
		} else {
			if((std_fscopy_file(src_path, dst_path, options) != 0)){
				FindClose(hFind);
				return -1;
			}
		}
	} while((FindNextFileA(hFind, &find_data) != 0));
	cstd_DWORD last_error = GetLastError();
	if((last_error != 18)){
		fprintf(cstd_get_stderr(), "FindNextFileA failed\n");
		FindClose(hFind);
		return -1;
	}
	FindClose(hFind);
	return 0;
}
/** Translate D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\function.ch **/
void std_stddefault_function_instancemake2(struct std_stddefault_function_instance* this, void* ptr, void* cap, std_stddestructor_type destr, size_t size_data, size_t align_data){
	void* captured = cap;
	if((captured == NULL)){
		*this = (struct std_stddefault_function_instance){ 
			.fn_data_ptr = NULL, 
			.fn_pointer = ptr, 
			.dtor = NULL, 
			.is_heap = 0, 
			.buffer = {}
		};
		return;
	}
	if((size_data >= 32)){
		char* allocated = ((char*) malloc(size_data));
		memcpy(allocated, captured, size_data);
		*this = (struct std_stddefault_function_instance){ 
			.fn_data_ptr = allocated, 
			.fn_pointer = ptr, 
			.dtor = destr, 
			.is_heap = 1, 
			.buffer = {}
		};
		return;
	} else {
		struct std_stddefault_function_instance i = (struct std_stddefault_function_instance){ 
			.fn_data_ptr = NULL, 
			.fn_pointer = ptr, 
			.dtor = destr, 
			.is_heap = 0, 
			.buffer = {}
		};
		_Bool __chx__lv__14 = true;
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
		self->dtor(self->fn_data_ptr);
	}
	if(self->is_heap){
		free(self->fn_data_ptr);
	}
	__chx__dstctr_clnup_blk__:{
	}
}
/** Translate D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\hashing\compare_impl.ch **/
_Bool std_char_equals(char*const self, char*const other){
	return (*self == *other);
}
_Bool std_uchar_equals(unsigned char*const self, unsigned char*const other){
	return (*self == *other);
}
_Bool std_short_equals(short*const self, short*const other){
	return (*self == *other);
}
_Bool std_ushort_equals(unsigned short*const self, unsigned short*const other){
	return (*self == *other);
}
_Bool std_int_equals(int*const self, int*const other){
	return (*self == *other);
}
_Bool std_uint_equals(unsigned int*const self, unsigned int*const other){
	return (*self == *other);
}
_Bool std_long_equals(long*const self, long*const other){
	return (*self == *other);
}
_Bool std_ulong_equals(unsigned long*const self, unsigned long*const other){
	return (*self == *other);
}
_Bool std_i64_equals(int64_t*const self, int64_t*const other){
	return (*self == *other);
}
_Bool std_u64_equals(uint64_t*const self, uint64_t*const other){
	return (*self == *other);
}
/** Translate D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\hashing\fnv1.ch **/
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
/** Translate D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\hashing\hash.ch **/
/** Translate D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\hashing\hash_impl.ch **/

size_t std_char_hash(char*const self){
	return ((size_t) *self);
}
size_t std_uchar_hash(unsigned char*const self){
	return *self;
}
size_t std_short_hash(short*const self){
	return ((size_t) (*self * 2654435769));
}
size_t std_ushort_hash(unsigned short*const self){
	return (*self * 2654435769);
}
size_t std_int_hash(int*const self){
	return std_murmurhash(((const char*) self), sizeof(int), 0);
}
size_t std_uint_hash(unsigned int*const self){
	return std_murmurhash(((const char*) self), sizeof(unsigned int), 0);
}
size_t std_long_hash(long*const self){
	return std_murmurhash(((const char*) self), sizeof(long), 0);
}
size_t std_ulong_hash(unsigned long*const self){
	return std_murmurhash(((const char*) self), sizeof(unsigned long), 0);
}
size_t std_i64_hash(int64_t*const self){
	return std_murmurhash(((const char*) self), sizeof(int64_t), 0);
}
size_t std_u64_hash(uint64_t*const self){
	return std_murmurhash(((const char*) self), sizeof(uint64_t), 0);
}
/** Translate D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\hashing\murmur.ch **/
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
/** Translate D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\mutex.ch **/
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
/** Translate D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\option.ch **/
void std_stdOption__cgs__0take(struct std_stdconcurrentTask* __chx_struct_ret_param_xx, struct std_stdOption__cgs__0*const self){
	if((self->__chx__vt_621827 == 1)){
		std_raw_panic("cannot take on a option that\'s none", &(struct std_Location){ 
			.character = 17, 
			.filename = "D:\\Programming\\Cpp\\zig-bootstrap\\chemical\\lang\\libs\\std\\src\\option.ch", 
			.line = 8
		});
	}
	struct std_stdOption__cgs__0*const __chx__lv__15 = self;
	struct std_stdconcurrentTask temp = ({  __chx__lv__15->Some.value; });
	({ struct std_stdOption__cgs__0* __chx__lv__16 = self; *__chx__lv__16 = (struct std_stdOption__cgs__0) { .__chx__vt_621827 = 1, 
	}; __chx__lv__16; });
	*__chx_struct_ret_param_xx = temp;
	return;
}
/** Translate D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\os_string.ch **/




static void std_stdOsStringmake(struct std_stdOsString* this, std_stdnative_string_t* str){
	_Bool __chx__lv__17 = true;
	*this = (struct std_stdOsString){ 
		.data_ = ({ __chx__lv__17 = false; *str; })
	};
	if(__chx__lv__17) {
		std_stdu16stringdelete(str);
	}
	return;
}
static void std_stdOsStringmake2(struct std_stdOsString* this, struct std_stdstring*const utf8){
	
	struct std_stdu16string __chx__lv__18;
	*this = (struct std_stdOsString){ 
		.data_ = (*({ std_stdOsStringutf8_to_u16(&__chx__lv__18, utf8); &__chx__lv__18; }))
	};
	return;
}
static void std_stdOsStringfrom_utf8(struct std_stdOsString* __chx_struct_ret_param_xx, struct std_stdstring*const utf8){
	struct std_stdOsString __chx__lv__19;
	*__chx_struct_ret_param_xx = (*({ std_stdOsStringmake2(&__chx__lv__19, utf8); &__chx__lv__19; }));
	return;
}
static size_t std_stdOsStringsize(struct std_stdOsString*const self){
	return std_stdu16stringsize(&self->data_);
}
static _Bool std_stdOsStringempty(struct std_stdOsString*const self){
	return std_stdu16stringempty(&self->data_);
}
static std_stdnative_string_t*const std_stdOsStringnative(struct std_stdOsString*const self){
	return &self->data_;
}
static const std_stdnative_char_t* std_stdOsStringc_str_native(struct std_stdOsString*const self){
	
	if(std_stdu16stringempty(&self->data_)){
		return NULL;
	} else {
		return std_stdu16stringdata(&self->data_);
	}
}
static void std_stdOsStringclear(struct std_stdOsString* self){
	std_stdu16stringclear(&self->data_);
}
static void std_stdOsStringreserve(struct std_stdOsString* self, size_t n){
	std_stdu16stringreserve(&self->data_, n);
}
static void std_stdOsStringappend_utf8(struct std_stdOsString* self, const char* data, size_t len){
	
	std_stdu16stringappend_utf8_view(&self->data_, data, len);
}
static void std_stdOsStringappend_utf8_str(struct std_stdOsString* self, struct std_stdstring*const utf8){
	std_stdOsStringappend_utf8(self, std_stdstringdata(utf8), std_stdstringsize(utf8));
}
static void std_stdOsStringappend_utf8_view(struct std_stdOsString* self, struct std_stdstring_view*const utf8){
	std_stdOsStringappend_utf8(self, std_stdstring_viewdata(utf8), std_stdstring_viewsize(utf8));
}
static void std_stdOsStringappend_native(struct std_stdOsString* self, std_stdnative_string_t*const native){
	std_stdu16stringappend_with_len(&self->data_, std_stdu16stringdata(native), std_stdu16stringsize(native));
}
static void std_stdOsStringto_utf8(struct std_stdstring* __chx_struct_ret_param_xx, struct std_stdOsString*const self){
	
	struct std_stdstring __chx__lv__20;
	*__chx_struct_ret_param_xx = (*({ std_stdOsStringu16_to_utf8(&__chx__lv__20, &self->data_); &__chx__lv__20; }));
	return;
}
static void std_stdOsStringutf8_to_u16(struct std_stdu16string* __chx_struct_ret_param_xx, struct std_stdstring*const utf8){
	if(std_stdstringempty(utf8)){
		struct std_stdu16string __chx__lv__21;
		*__chx_struct_ret_param_xx = (*({ std_stdu16stringempty_str(&__chx__lv__21); &__chx__lv__21; }));
		return;
	}
	int req = MultiByteToWideChar(65001, 0, std_stdstringdata(utf8), ((int) std_stdstringsize(utf8)), NULL, 0);
	if((req == 0)){
		struct std_stdu16string __chx__lv__22;
		*__chx_struct_ret_param_xx = (*({ std_stdu16stringempty_str(&__chx__lv__22); &__chx__lv__22; }));
		return;
	}
	struct std_stdu16string __chx__lv__23;
	struct std_stdu16string tmp = (*({ std_stdu16stringempty_str(&__chx__lv__23); &__chx__lv__23; }));
	_Bool __chx__lv__24 = true;
	std_stdu16stringreserve(&tmp, ((unsigned int) req));
	int res = MultiByteToWideChar(65001, 0, std_stdstringdata(utf8), ((int) std_stdstringsize(utf8)), ((cstd_LPWSTR) std_stdu16stringdata(&tmp)), req);
	if((res == 0)){
		struct std_stdu16string __chx__lv__25;
		*__chx_struct_ret_param_xx = (*({ std_stdu16stringempty_str(&__chx__lv__25); &__chx__lv__25; }));
		if(__chx__lv__24) {
			std_stdu16stringdelete(&tmp);
		}
		return;
	}
	struct std_stdu16string __chx__lv__26;
	struct std_stdu16string out = (*({ std_stdu16stringempty_str(&__chx__lv__26); &__chx__lv__26; }));
	_Bool __chx__lv__27 = true;
	std_stdu16stringappend_with_len(&out, std_stdu16stringdata(&tmp), std_stdu16stringsize(&tmp));
	*__chx_struct_ret_param_xx = out;
	if(__chx__lv__24) {
		std_stdu16stringdelete(&tmp);
	}
	return;
}
static void std_stdOsStringu16_to_utf8(struct std_stdstring* __chx_struct_ret_param_xx, struct std_stdu16string*const str){
	if(std_stdu16stringempty(str)){
		struct std_stdstring __chx__lv__28;
		*__chx_struct_ret_param_xx = (*({ std_stdstringempty_str(&__chx__lv__28); &__chx__lv__28; }));
		return;
	}
	struct std_stdu16string __chx__lv__29;
	struct std_stdu16string tmp = (*({ std_stdu16stringempty_str(&__chx__lv__29); &__chx__lv__29; }));
	_Bool __chx__lv__30 = true;
	std_stdu16stringappend_with_len(&tmp, std_stdu16stringdata(str), std_stdu16stringsize(str));
	int req = WideCharToMultiByte(65001, 0, std_stdu16stringdata(&tmp), ((int) std_stdu16stringsize(&tmp)), NULL, 0, NULL, NULL);
	if((req == 0)){
		struct std_stdstring __chx__lv__31;
		*__chx_struct_ret_param_xx = (*({ std_stdstringempty_str(&__chx__lv__31); &__chx__lv__31; }));
		if(__chx__lv__30) {
			std_stdu16stringdelete(&tmp);
		}
		return;
	}
	struct std_stdstring __chx__lv__32;
	struct std_stdstring out = (*({ std_stdstringempty_str(&__chx__lv__32); &__chx__lv__32; }));
	_Bool __chx__lv__33 = true;
	std_stdstringreserve(&out, ((unsigned int) req));
	int res = WideCharToMultiByte(65001, 0, std_stdu16stringdata(&tmp), ((int) std_stdu16stringsize(&tmp)), ((cstd_LPSTR) std_stdstringdata(&out)), req, NULL, NULL);
	if((res == 0)){
		struct std_stdstring __chx__lv__34;
		*__chx_struct_ret_param_xx = (*({ std_stdstringempty_str(&__chx__lv__34); &__chx__lv__34; }));
		if(__chx__lv__33) {
			std_stdstringdelete(&out);
		}
		if(__chx__lv__30) {
			std_stdu16stringdelete(&tmp);
		}
		return;
	}
	*__chx_struct_ret_param_xx = out;
	if(__chx__lv__30) {
		std_stdu16stringdelete(&tmp);
	}
	return;
}
static void std_stdOsStringdelete(struct std_stdOsString* self){
	__chx__dstctr_clnup_blk__:{
		std_stdu16stringdelete(&self->data_);
	}
}
/** Translate D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\pair.ch **/
/** Translate D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\panic.ch **/
void std_raw_panic(const char* message, struct std_Location* location){
	printf("panic with message \'%s\' at \'%s:%d:%d\'\n", message, location->filename, location->line, location->character);
	abort();
}
/** Translate D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\result.ch **/
/** Translate D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\span.ch **/
/** Translate D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\std.ch **/
/** Translate D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\stream.ch **/
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
void std_CommandLineStreamwriteI8(struct std_CommandLineStream*const self, int8_t value){
	fwrite(&value, 1, 1, cstd_get_stdout());
}
void std_CommandLineStreamwriteI16(struct std_CommandLineStream*const self, int16_t value){
	char buf[32];
	size_t len = std_CommandLineStreami64_to_chars(self, &buf[0], ((int64_t) value));
	fwrite(&buf[0], 1, len, cstd_get_stdout());
}
void std_CommandLineStreamwriteI32(struct std_CommandLineStream*const self, int32_t value){
	char buf[32];
	size_t len = std_CommandLineStreami64_to_chars(self, &buf[0], ((int64_t) value));
	fwrite(&buf[0], 1, len, cstd_get_stdout());
}
void std_CommandLineStreamwriteI64(struct std_CommandLineStream*const self, int64_t value){
	char buf[64];
	size_t len = std_CommandLineStreami64_to_chars(self, &buf[0], value);
	fwrite(&buf[0], 1, len, cstd_get_stdout());
}
void std_CommandLineStreamwriteU8(struct std_CommandLineStream*const self, uint8_t value){
	fwrite(&value, 1, 1, cstd_get_stdout());
}
void std_CommandLineStreamwriteU16(struct std_CommandLineStream*const self, uint16_t value){
	char buf[32];
	size_t len = std_CommandLineStreamu64_to_chars(self, &buf[0], ((uint64_t) value));
	fwrite(&buf[0], 1, len, cstd_get_stdout());
}
void std_CommandLineStreamwriteU32(struct std_CommandLineStream*const self, uint32_t value){
	char buf[32];
	size_t len = std_CommandLineStreamu64_to_chars(self, &buf[0], ((uint64_t) value));
	fwrite(&buf[0], 1, len, cstd_get_stdout());
}
void std_CommandLineStreamwriteU64(struct std_CommandLineStream*const self, uint64_t value){
	char buf[64];
	size_t len = std_CommandLineStreamu64_to_chars(self, &buf[0], value);
	fwrite(&buf[0], 1, len, cstd_get_stdout());
}
void std_CommandLineStreamwriteStr(struct std_CommandLineStream*const self, const char* value, uint64_t length){
	fwrite(value, 1, length, cstd_get_stdout());
}
void std_CommandLineStreamwriteStrNoLen(struct std_CommandLineStream*const self, const char* value){
	fwrite(value, 1, strlen(value), cstd_get_stdout());
}
void std_CommandLineStreamwriteChar(struct std_CommandLineStream*const self, char value){
	fwrite(&value, 1, 1, cstd_get_stdout());
}
void std_CommandLineStreamwriteUChar(struct std_CommandLineStream*const self, unsigned char value){
	fwrite(&value, 1, 1, cstd_get_stdout());
}
void std_CommandLineStreamwriteShort(struct std_CommandLineStream*const self, short value){
	char buf[32];
	size_t len = std_CommandLineStreami64_to_chars(self, &buf[0], ((int64_t) value));
	fwrite(&buf[0], 1, len, cstd_get_stdout());
}
void std_CommandLineStreamwriteUShort(struct std_CommandLineStream*const self, unsigned short value){
	char buf[32];
	size_t len = std_CommandLineStreamu64_to_chars(self, &buf[0], ((uint64_t) value));
	fwrite(&buf[0], 1, len, cstd_get_stdout());
}
void std_CommandLineStreamwriteInt(struct std_CommandLineStream*const self, int value){
	char buf[32];
	size_t len = std_CommandLineStreami64_to_chars(self, &buf[0], ((int64_t) value));
	fwrite(&buf[0], 1, len, cstd_get_stdout());
}
void std_CommandLineStreamwriteUInt(struct std_CommandLineStream*const self, unsigned int value){
	char buf[32];
	size_t len = std_CommandLineStreamu64_to_chars(self, &buf[0], ((uint64_t) value));
	fwrite(&buf[0], 1, len, cstd_get_stdout());
}
void std_CommandLineStreamwriteLong(struct std_CommandLineStream*const self, long value){
	char buf[48];
	size_t len = std_CommandLineStreami64_to_chars(self, &buf[0], ((int64_t) value));
	fwrite(&buf[0], 1, len, cstd_get_stdout());
}
void std_CommandLineStreamwriteULong(struct std_CommandLineStream*const self, unsigned long value){
	char buf[48];
	size_t len = std_CommandLineStreamu64_to_chars(self, &buf[0], ((uint64_t) value));
	fwrite(&buf[0], 1, len, cstd_get_stdout());
}
void std_CommandLineStreamwriteLongLong(struct std_CommandLineStream*const self, long long value){
	char buf[64];
	size_t len = std_CommandLineStreami64_to_chars(self, &buf[0], value);
	fwrite(&buf[0], 1, len, cstd_get_stdout());
}
void std_CommandLineStreamwriteULongLong(struct std_CommandLineStream*const self, unsigned long long value){
	char buf[64];
	size_t len = std_CommandLineStreamu64_to_chars(self, &buf[0], value);
	fwrite(&buf[0], 1, len, cstd_get_stdout());
}
void std_CommandLineStreamwriteFloat(struct std_CommandLineStream*const self, float value){
	char buf[128];
	size_t len = std_CommandLineStreamfloat_to_chars(self, &buf[0], value, 6);
	fwrite(&buf[0], 1, len, cstd_get_stdout());
}
void std_CommandLineStreamwriteDouble(struct std_CommandLineStream*const self, double value){
	char buf[256];
	size_t len = std_CommandLineStreamdouble_to_chars(self, &buf[0], value, 6);
	fwrite(&buf[0], 1, len, cstd_get_stdout());
}
/** Translate D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\string.ch **/

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
void std_stdstringconstructor(struct std_stdstring* this, const char* value, size_t length){
	struct std_stdstring s = (struct std_stdstring){ 
		.state = '0', 
		.storage = { 
			.constant = { 
				.length = length, 
				.data = value
			}
		}
	};
	_Bool __chx__lv__35 = true;
	std_stdstringensure_mut(&s, length);
	*this = s;
	return;
}
void std_stdstringview_make(struct std_stdstring* this, struct std_stdstring_view*const value){
	struct std_stdstring s = (struct std_stdstring){ 
		.state = '0', 
		.storage = { 
			.constant = { 
				.length = std_stdstring_viewsize(value), 
				.data = std_stdstring_viewdata(value)
			}
		}
	};
	_Bool __chx__lv__36 = true;
	std_stdstringensure_mut(&s, std_stdstring_viewsize(value));
	*this = s;
	return;
}
void std_stdstringconstructor2(struct std_stdstring* this, const char* value, size_t length, _Bool ensure){
	struct std_stdstring s = (struct std_stdstring){ 
		.state = '0', 
		.storage = { 
			.constant = { 
				.length = length, 
				.data = value
			}
		}
	};
	_Bool __chx__lv__37 = true;
	if(ensure){
		std_stdstringensure_mut(&s, length);
	}
	*this = s;
	return;
}
void std_stdstringempty_str(struct std_stdstring* this){
	*this = (struct std_stdstring){ 
		.state = '0', 
		.storage = { 
			.constant = { 
				.length = 0, 
				.data = ""
			}
		}
	};
	return;
}
void std_stdstringmake_no_len(struct std_stdstring* this, const char* value){
	size_t length = strlen(value);
	struct std_stdstring s = (struct std_stdstring){ 
		.state = '0', 
		.storage = { 
			.constant = { 
				.length = length, 
				.data = value
			}
		}
	};
	_Bool __chx__lv__38 = true;
	std_stdstringensure_mut(&s, length);
	*this = s;
	return;
}
void std_stdstringmake_with_char(struct std_stdstring* this, char value){
	struct std_stdstring s = (struct std_stdstring){ 
		.state = '1', 
		.storage = { 
			.sso = { 
				.buffer = {}, 
				.length = 1
			}
		}
	};
	_Bool __chx__lv__39 = true;
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
void std_stdstringresize_unsafe(struct std_stdstring*const self, size_t value){
	switch(self->state) {
		case '0':{
			self->storage.constant.length = value;
			break;
		}
		case '1':{
			self->storage.sso.length = value;
			break;
		}
		case '2':{
			self->storage.heap.length = value;
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
_Bool std_stdstringequals(struct std_stdstring*const self, struct std_stdstring*const other){
	return std_stdstringequals_with_len(self, std_stdstringdata(other), std_stdstringsize(other));
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
			.character = 17, 
			.filename = "D:\\Programming\\Cpp\\zig-bootstrap\\chemical\\lang\\libs\\std\\src\\string.ch", 
			.line = 240
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
			uint64_t new_cap = (length < (self->storage.heap.capacity * 2)) ? ({ 
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
	uint64_t new_size = (offset + len);
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
		std_stdstringappend_view(self, &(*({ struct std_stdstring_view __chx__lv__40; std_stdstring_viewconstructor(&__chx__lv__40, "nan", 3); &__chx__lv__40; })));
		return;
	}
	if(std_stddbl_is_inf(value)){
		if(std_stddbl_is_neg(value)){
			std_stdstringappend_view(self, &(*({ struct std_stdstring_view __chx__lv__41; std_stdstring_viewconstructor(&__chx__lv__41, "-inf", 4); &__chx__lv__41; })));
		} else {
			std_stdstringappend_view(self, &(*({ struct std_stdstring_view __chx__lv__42; std_stdstring_viewconstructor(&__chx__lv__42, "inf", 3); &__chx__lv__42; })));
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
	struct std_stdstring __chx__lv__43;
	*__chx_struct_ret_param_xx = (*({ std_stdstringsubstring(&__chx__lv__43, self, 0, std_stdstringsize(self)); &__chx__lv__43; }));
	return;
}
void std_stdstringsubstring(struct std_stdstring* __chx_struct_ret_param_xx, struct std_stdstring*const self, size_t start, size_t end){
	struct std_stdstring s;
	_Bool __chx__lv__44 = true;
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
		uint64_t new_cap = (actual_len * 2);
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
	struct std_stdstring_view __chx__lv__45;
	return std_stdinternal_view_find(&(*({ std_stdstring_viewconstructor(&__chx__lv__45, std_stdstringdata(self), std_stdstringsize(self)); &__chx__lv__45; })), needle);
}
_Bool std_stdstringcontains(struct std_stdstring*const self, struct std_stdstring_view*const needle){
	return (std_stdstringfind(self, needle) != (((size_t) 0) - 1));
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
	uint64_t tail_start = (start + erase_len);
	uint64_t tail_len = (sz - tail_start);
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
			std_stdstringmove_const_to_buffer(self);
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
unsigned int std_stdstringhash(struct std_stdstring*const self){
	return std_fnv1a_hash_32(std_stdstringdata(self));
}
void std_stdstringto_view(struct std_stdstring_view* __chx_struct_ret_param_xx, struct std_stdstring*const self){
	struct std_stdstring_view __chx__lv__46;
	*__chx_struct_ret_param_xx = (*({ std_stdstring_viewconstructor(&__chx__lv__46, std_stdstringdata(self), std_stdstringsize(self)); &__chx__lv__46; }));
	return;
}
void std_stdstringdelete(struct std_stdstring* self){
	if((self->state == '2')){
		free(self->storage.heap.data);
	}
	__chx__dstctr_clnup_blk__:{
	}
}
void std_stdStringStreamwriteI8(struct std_stdStringStream* self, int8_t value){
	std_stdstringappend(self->str, value);
}
void std_stdStringStreamwriteI16(struct std_stdStringStream* self, int16_t value){
	std_stdstringappend_integer(self->str, value);
}
void std_stdStringStreamwriteI32(struct std_stdStringStream* self, int32_t value){
	std_stdstringappend_integer(self->str, value);
}
void std_stdStringStreamwriteI64(struct std_stdStringStream* self, int64_t value){
	std_stdstringappend_integer(self->str, value);
}
void std_stdStringStreamwriteU8(struct std_stdStringStream* self, uint8_t value){
	std_stdstringappend(self->str, ((char) value));
}
void std_stdStringStreamwriteU16(struct std_stdStringStream* self, uint16_t value){
	std_stdstringappend_uinteger(self->str, value);
}
void std_stdStringStreamwriteU32(struct std_stdStringStream* self, uint32_t value){
	std_stdstringappend_uinteger(self->str, value);
}
void std_stdStringStreamwriteU64(struct std_stdStringStream* self, uint64_t value){
	std_stdstringappend_uinteger(self->str, value);
}
void std_stdStringStreamwriteStr(struct std_stdStringStream* self, const char* value, uint64_t length){
	std_stdstringappend_with_len(self->str, value, length);
}
void std_stdStringStreamwriteStrNoLen(struct std_stdStringStream* self, const char* value){
	std_stdstringappend_with_len(self->str, value, strlen(value));
}
void std_stdStringStreamwriteChar(struct std_stdStringStream* self, char value){
	std_stdstringappend(self->str, value);
}
void std_stdStringStreamwriteUChar(struct std_stdStringStream* self, unsigned char value){
	std_stdstringappend(self->str, ((char) value));
}
void std_stdStringStreamwriteShort(struct std_stdStringStream* self, short value){
	std_stdstringappend_integer(self->str, value);
}
void std_stdStringStreamwriteUShort(struct std_stdStringStream* self, unsigned short value){
	std_stdstringappend_uinteger(self->str, value);
}
void std_stdStringStreamwriteInt(struct std_stdStringStream* self, int value){
	std_stdstringappend_integer(self->str, value);
}
void std_stdStringStreamwriteUInt(struct std_stdStringStream* self, unsigned int value){
	std_stdstringappend_uinteger(self->str, value);
}
void std_stdStringStreamwriteLong(struct std_stdStringStream* self, long value){
	std_stdstringappend_integer(self->str, value);
}
void std_stdStringStreamwriteULong(struct std_stdStringStream* self, unsigned long value){
	std_stdstringappend_uinteger(self->str, value);
}
void std_stdStringStreamwriteLongLong(struct std_stdStringStream* self, long long value){
	std_stdstringappend_integer(self->str, value);
}
void std_stdStringStreamwriteULongLong(struct std_stdStringStream* self, unsigned long long value){
	std_stdstringappend_uinteger(self->str, value);
}
void std_stdStringStreamwriteFloat(struct std_stdStringStream* self, float value){
	std_stdstringappend_double(self->str, ((double) value), 3);
}
void std_stdStringStreamwriteDouble(struct std_stdStringStream* self, double value){
	std_stdstringappend_double(self->str, ((double) value), 3);
}
/** Translate D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\string_view.ch **/
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
	struct std_stdstring_view __chx__lv__47;
	*__chx_struct_ret_param_xx = (*({ std_stdstring_viewconstructor(&__chx__lv__47, (self->_data + start), (end - start)); &__chx__lv__47; }));
	return;
}
void std_stdstring_viewskip(struct std_stdstring_view* __chx_struct_ret_param_xx, struct std_stdstring_view*const self, size_t count){
	struct std_stdstring_view __chx__lv__48;
	*__chx_struct_ret_param_xx = (*({ std_stdstring_viewconstructor(&__chx__lv__48, (self->_data + count), (self->_size - count)); &__chx__lv__48; }));
	return;
}
size_t std_stdstring_viewfind(struct std_stdstring_view*const self, struct std_stdstring_view*const needle){
	return std_stdinternal_view_find(self, needle);
}
_Bool std_stdstring_viewcontains(struct std_stdstring_view*const self, struct std_stdstring_view*const needle){
	return (std_stdstring_viewfind(self, needle) != (((size_t) 0) - 1));
}
_Bool std_stdstring_viewequals(struct std_stdstring_view*const self, struct std_stdstring_view*const other){
	size_t self_size = self->_size;
	return ((self_size == std_stdstring_viewsize(other)) && (strncmp(std_stdstring_viewdata(self), std_stdstring_viewdata(other), self_size) == 0));
}
_Bool std_stdstring_viewends_with(struct std_stdstring_view*const self, struct std_stdstring_view*const other){
	if((std_stdstring_viewsize(other) > std_stdstring_viewsize(self))){
		return 0;
	}
	return (memcmp(((std_stdstring_viewdata(self) + std_stdstring_viewsize(self)) - std_stdstring_viewsize(other)), std_stdstring_viewdata(other), std_stdstring_viewsize(other)) == 0);
}
unsigned int std_stdstring_viewhash(struct std_stdstring_view*const self){
	return ((unsigned int) std_fnv1_hash_view(self));
}
/** Translate D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\u16_string.ch **/

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
		.state = '0', 
		.storage = { 
			.constant = { 
				.length = length, 
				.data = value
			}
		}
	};
	_Bool __chx__lv__49 = true;
	std_stdu16stringensure_mut(&s, length);
	*this = s;
	return;
}
void std_stdu16stringconstructor2(struct std_stdu16string* this, const uint16_t* value, size_t length, _Bool ensure){
	struct std_stdu16string s = (struct std_stdu16string){ 
		.state = '0', 
		.storage = { 
			.constant = { 
				.length = length, 
				.data = value
			}
		}
	};
	_Bool __chx__lv__50 = true;
	if(ensure){
		std_stdu16stringensure_mut(&s, length);
	}
	*this = s;
	return;
}
void std_stdu16stringempty_str(struct std_stdu16string* this){
	*this = (struct std_stdu16string){ 
		.state = '0', 
		.storage = { 
			.constant = { 
				.length = 0, 
				.data = &std_stdEMPTY_U16
			}
		}
	};
	return;
}
void std_stdu16stringmake_no_len(struct std_stdu16string* this, const uint16_t* value){
	size_t length = std_stdu16_strlen(value);
	struct std_stdu16string s = (struct std_stdu16string){ 
		.state = '0', 
		.storage = { 
			.constant = { 
				.length = length, 
				.data = value
			}
		}
	};
	_Bool __chx__lv__51 = true;
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
	uint64_t new_size = (offset + len);
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
	uint64_t new_size = (offset + 1);
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
		uint64_t new_size = (offset + 2);
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
	uint64_t out_cap = (len + 1);
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
	struct std_stdu16string __chx__lv__52;
	struct std_stdu16string result = (*({ std_stdu16stringempty_str(&__chx__lv__52); &__chx__lv__52; }));
	_Bool __chx__lv__53 = true;
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
	struct std_stdu16string __chx__lv__54;
	struct std_stdu16string __chx__lv__55 = (*({ std_stdu16stringconstructor(&__chx__lv__54, buf, real_len); &__chx__lv__54; }));
	*__chx_struct_ret_param_xx = result;
	std_stdu16stringdelete(&__chx__lv__55);
	return;
}
void std_stdu16stringto_utf8(struct std_stdstring* __chx_struct_ret_param_xx, struct std_stdu16string*const self){
	size_t n = std_stdu16stringsize(self);
	if((n == 0)){
		struct std_stdstring __chx__lv__56;
		*__chx_struct_ret_param_xx = (*({ std_stdstringempty_str(&__chx__lv__56); &__chx__lv__56; }));
		return;
	}
	uint64_t out_cap = ((n * 3) + 1);
	struct std_stdstring __chx__lv__57;
	struct std_stdstring out = (*({ std_stdstringempty_str(&__chx__lv__57); &__chx__lv__57; }));
	_Bool __chx__lv__58 = true;
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
	_Bool __chx__lv__59 = true;
	size_t n = std_stdu16stringsize(self);
	size_t m = std_stdu16stringsize(sub);
	if((m == 0)){
		const int __chx__lv__60 = 0;
		if(__chx__lv__59) {
			std_stdu16stringdelete(sub);
		}
		return __chx__lv__60;
	}
	if((m > n)){
		const int __chx__lv__61 = -1;
		if(__chx__lv__59) {
			std_stdu16stringdelete(sub);
		}
		return __chx__lv__61;
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
			const int __chx__lv__62 = ((int) i);
			if(__chx__lv__59) {
				std_stdu16stringdelete(sub);
			}
			return __chx__lv__62;
		}
		i = (i + 1);
	}
	const int __chx__lv__63 = -1;
	if(__chx__lv__59) {
		std_stdu16stringdelete(sub);
	}
	return __chx__lv__63;
}
_Bool std_stdu16stringstarts_with(struct std_stdu16string*const self, struct std_stdu16string* prefix){
	_Bool __chx__lv__64 = true;
	size_t p_len = std_stdu16stringsize(prefix);
	size_t my_len = std_stdu16stringsize(self);
	if((p_len > my_len)){
		const _Bool __chx__lv__65 = 0;
		if(__chx__lv__64) {
			std_stdu16stringdelete(prefix);
		}
		return __chx__lv__65;
	}
	const uint16_t* src = std_stdu16stringdata(self);
	const uint16_t* pre = std_stdu16stringdata(prefix);
	size_t i = 0;
	while((i < p_len)) {
		if((src[i] != pre[i])){
			const _Bool __chx__lv__66 = 0;
			if(__chx__lv__64) {
				std_stdu16stringdelete(prefix);
			}
			return __chx__lv__66;
		}
		i = (i + 1);
	}
	const _Bool __chx__lv__67 = 1;
	if(__chx__lv__64) {
		std_stdu16stringdelete(prefix);
	}
	return __chx__lv__67;
}
_Bool std_stdu16stringends_with(struct std_stdu16string*const self, struct std_stdu16string* suffix){
	_Bool __chx__lv__68 = true;
	size_t s_len = std_stdu16stringsize(suffix);
	size_t my_len = std_stdu16stringsize(self);
	if((s_len > my_len)){
		const _Bool __chx__lv__69 = 0;
		if(__chx__lv__68) {
			std_stdu16stringdelete(suffix);
		}
		return __chx__lv__69;
	}
	const uint16_t* src = std_stdu16stringdata(self);
	const uint16_t* suf = std_stdu16stringdata(suffix);
	size_t i = 0;
	while((i < s_len)) {
		if((src[((my_len - s_len) + i)] != suf[i])){
			const _Bool __chx__lv__70 = 0;
			if(__chx__lv__68) {
				std_stdu16stringdelete(suffix);
			}
			return __chx__lv__70;
		}
		i = (i + 1);
	}
	const _Bool __chx__lv__71 = 1;
	if(__chx__lv__68) {
		std_stdu16stringdelete(suffix);
	}
	return __chx__lv__71;
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
/** Translate D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\unordered_map2.ch **/

/** Translate D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\utils.ch **/
void std_stdreplace__cfg_0(struct std_stdconcurrentTask* __chx_struct_ret_param_xx, struct std_stdconcurrentTask* value, struct std_stdconcurrentTask* repl){
	struct std_stdconcurrentTask temp;
	memcpy(&temp, value, sizeof(struct std_stdconcurrentTask));
	memcpy(value, repl, sizeof(struct std_stdconcurrentTask));
	0;
	*__chx_struct_ret_param_xx = temp;
	return;
}

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
	uint64_t last = (nlen - 1);
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
/** Translate D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\vector.ch **/
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
void std_stdvector__cgs__0resize(struct std_stdvector__cgs__0* self, size_t new_cap){
	struct std_stdconcurrentTask* new_data = ((struct std_stdconcurrentTask*) realloc(self->data_ptr, (sizeof(struct std_stdconcurrentTask) * new_cap)));
	
	if((new_data != NULL)){
		self->data_ptr = new_data;
		self->data_cap = new_cap;
	} else {
		std_raw_panic("failed to resize vector", &(struct std_Location){ 
			.character = 21, 
			.filename = "D:\\Programming\\Cpp\\zig-bootstrap\\chemical\\lang\\libs\\std\\src\\vector.ch", 
			.line = 33
		});
	}
}
void std_stdvector__cgs__0reserve(struct std_stdvector__cgs__0* self, size_t cap){
	if((cap <= self->data_cap)){
		return;
	}
	std_stdvector__cgs__0resize(self, cap);
}
void std_stdvector__cgs__0ensure_capacity_for_one_more(struct std_stdvector__cgs__0* self){
	if((self->data_cap == 0)){
		std_stdvector__cgs__0resize(self, 2);
	} else {
		std_stdvector__cgs__0resize(self, (self->data_cap * 2));
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
	uint64_t last = (s - 1);
	
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
	uint64_t last = (s - 1);
	struct std_stdconcurrentTask* ptr = std_stdvector__cgs__0get_ptr(self, last);
	
	self->data_size = last;
}
void std_stdvector__cgs__0pop_back(struct std_stdvector__cgs__0* self){
	std_stdvector__cgs__0remove_last(self);
}
void std_stdvector__cgs__0take_last(struct std_stdconcurrentTask* __chx_struct_ret_param_xx, struct std_stdvector__cgs__0* self){
	uint64_t last = (self->data_size - 1);
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
void std_stdvector__cgs__0resize_unsafe(struct std_stdvector__cgs__0* self, size_t new_size){
	self->data_size = new_size;
}
void std_stdvector__cgs__0delete(struct std_stdvector__cgs__0* self){
	if((self->data_ptr != NULL)){
		
		free(self->data_ptr);
	}
	__chx__dstctr_clnup_blk__:{
	}
}
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
		.data_ptr = ((cstd_usize*) malloc((sizeof(cstd_usize) * init_cap))), 
		.data_size = 0, 
		.data_cap = init_cap
	};
	return;
}
void std_stdvector__cgs__1resize(struct std_stdvector__cgs__1* self, size_t new_cap){
	cstd_usize* new_data = ((cstd_usize*) realloc(self->data_ptr, (sizeof(cstd_usize) * new_cap)));
	
	if((new_data != NULL)){
		self->data_ptr = new_data;
		self->data_cap = new_cap;
	} else {
		std_raw_panic("failed to resize vector", &(struct std_Location){ 
			.character = 21, 
			.filename = "D:\\Programming\\Cpp\\zig-bootstrap\\chemical\\lang\\libs\\std\\src\\vector.ch", 
			.line = 33
		});
	}
}
void std_stdvector__cgs__1reserve(struct std_stdvector__cgs__1* self, size_t cap){
	if((cap <= self->data_cap)){
		return;
	}
	std_stdvector__cgs__1resize(self, cap);
}
void std_stdvector__cgs__1ensure_capacity_for_one_more(struct std_stdvector__cgs__1* self){
	if((self->data_cap == 0)){
		std_stdvector__cgs__1resize(self, 2);
	} else {
		std_stdvector__cgs__1resize(self, (self->data_cap * 2));
	}
}
void std_stdvector__cgs__1push(struct std_stdvector__cgs__1* self, cstd_usize value){
	size_t s = self->data_size;
	if((s >= self->data_cap)){
		std_stdvector__cgs__1ensure_capacity_for_one_more(self);
	}
	memcpy(&self->data_ptr[s], &value, sizeof(cstd_usize));
	0;
	self->data_size = (s + 1);
}
void std_stdvector__cgs__1push_back(struct std_stdvector__cgs__1* self, cstd_usize value){
	std_stdvector__cgs__1push(self, value);
}
cstd_usize std_stdvector__cgs__1get(struct std_stdvector__cgs__1*const self, size_t index){
	return self->data_ptr[index];
}
cstd_usize* std_stdvector__cgs__1get_ptr(struct std_stdvector__cgs__1*const self, size_t index){
	return &self->data_ptr[index];
}
cstd_usize* std_stdvector__cgs__1get_ref(struct std_stdvector__cgs__1*const self, size_t index){
	return &self->data_ptr[index];
}
void std_stdvector__cgs__1set(struct std_stdvector__cgs__1* self, size_t index, cstd_usize value){
	self->data_ptr[index] = ({  value; });
}
size_t std_stdvector__cgs__1size(struct std_stdvector__cgs__1*const self){
	return self->data_size;
}
size_t std_stdvector__cgs__1capacity(struct std_stdvector__cgs__1*const self){
	return self->data_cap;
}
const cstd_usize* std_stdvector__cgs__1data(struct std_stdvector__cgs__1*const self){
	return self->data_ptr;
}
cstd_usize* std_stdvector__cgs__1last_ptr(struct std_stdvector__cgs__1*const self){
	return (self->data_ptr + (self->data_size - 1));
}
void std_stdvector__cgs__1remove(struct std_stdvector__cgs__1* self, size_t index){
	size_t s = self->data_size;
	uint64_t last = (s - 1);
	
	if((index == last)){
		self->data_size = last;
	} else {
		memmove(&self->data_ptr[index], &self->data_ptr[(index + 1)], (sizeof(cstd_usize) * (last - index)));
		self->data_size = last;
	}
}
void std_stdvector__cgs__1erase(struct std_stdvector__cgs__1* self, size_t index){
	std_stdvector__cgs__1remove(self, index);
}
void std_stdvector__cgs__1remove_last(struct std_stdvector__cgs__1* self){
	size_t s = self->data_size;
	uint64_t last = (s - 1);
	cstd_usize* ptr = std_stdvector__cgs__1get_ptr(self, last);
	
	self->data_size = last;
}
void std_stdvector__cgs__1pop_back(struct std_stdvector__cgs__1* self){
	std_stdvector__cgs__1remove_last(self);
}
cstd_usize std_stdvector__cgs__1take_last(struct std_stdvector__cgs__1* self){
	uint64_t last = (self->data_size - 1);
	self->data_size = last;
	return *std_stdvector__cgs__1get_ptr(self, last);
}
_Bool std_stdvector__cgs__1empty(struct std_stdvector__cgs__1*const self){
	return (self->data_size == 0);
}
void std_stdvector__cgs__1clear(struct std_stdvector__cgs__1* self){
	
	self->data_size = 0;
}
void std_stdvector__cgs__1resize_unsafe(struct std_stdvector__cgs__1* self, size_t new_size){
	self->data_size = new_size;
}
void std_stdvector__cgs__1delete(struct std_stdvector__cgs__1* self){
	if((self->data_ptr != NULL)){
		
		free(self->data_ptr);
	}
	__chx__dstctr_clnup_blk__:{
	}
}
/** Translate D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\win\condvar.ch **/

void std_stdCondVarconstructor(struct std_stdCondVar* this){
	struct std_stdCondVar c = (struct std_stdCondVar){ 
		.storage = {}
	};
	_Bool __chx__lv__72 = true;
	InitializeConditionVariable(&c.storage[0]);
	*this = c;
	return;
}
void std_stdCondVarwait(struct std_stdCondVar* self, struct std_stdmutex* mutex){
	int ok = SleepConditionVariableCS(&self->storage[0], &mutex->storage[0], 4294967295);
	if((ok == 0)){
		std_raw_panic("SleepConditionVariableCS failed in wait", &(struct std_Location){ 
			.character = 21, 
			.filename = "D:\\Programming\\Cpp\\zig-bootstrap\\chemical\\lang\\libs\\std\\win\\condvar.ch", 
			.line = 44
		});
	}
}
_Bool std_stdCondVartimed_wait(struct std_stdCondVar* self, struct std_stdmutex* mutex, unsigned long timeout_ms){
	int ok = SleepConditionVariableCS(&self->storage[0], &mutex->storage[0], timeout_ms);
	return (ok != 0);
}
void std_stdCondVarnotify_one(struct std_stdCondVar* self){
	WakeConditionVariable(&self->storage[0]);
}
void std_stdCondVarsignal(struct std_stdCondVar* self){
	std_stdCondVarnotify_one(self);
}
void std_stdCondVarnotify_all(struct std_stdCondVar* self){
	WakeAllConditionVariable(&self->storage[0]);
}
void std_stdCondVardelete(struct std_stdCondVar* self){
	__chx__dstctr_clnup_blk__:{
	}
}
/** Translate D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\win\mutex.ch **/

void std_stdmutexconstructor(struct std_stdmutex* this){
	struct std_stdmutex m = (struct std_stdmutex){ 
		.storage = {}
	};
	_Bool __chx__lv__73 = true;
	InitializeCriticalSectionAndSpinCount(&m.storage[0], 4000);
	*this = m;
	return;
}
void std_stdmutexlock(struct std_stdmutex* self){
	EnterCriticalSection(&self->storage[0]);
}
_Bool std_stdmutextry_lock(struct std_stdmutex* self){
	int r = TryEnterCriticalSection(&self->storage[0]);
	return (r != 0);
}
void std_stdmutexunlock(struct std_stdmutex* self){
	LeaveCriticalSection(&self->storage[0]);
}
void std_stdmutexdelete(struct std_stdmutex* self){
	DeleteCriticalSection(&self->storage[0]);
	__chx__dstctr_clnup_blk__:{
	}
}