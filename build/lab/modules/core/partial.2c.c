
/** FwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\core\ops.ch **/
/** TypeAliasDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\core\ops.ch **/
/** Declare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\core\ops.ch **/
typedef struct __chx_core_coreopsIncrement_vt_t {
	void**const(*inc_pre)(void* self);void(*inc_post)(void* self, void** __chx_struct_ret_param_xx);
} __chx_core_coreopsIncrement_vt_t;
typedef struct __chx_core_coreopsDecrement_vt_t {
	void**const(*dec_pre)(void* self);void(*dec_post)(void* self, void** __chx_struct_ret_param_xx);
} __chx_core_coreopsDecrement_vt_t;
typedef struct __chx_core_coreopsPartialEq__cgs__0_vt_t {
	_Bool(*eq)(void* self, void** other);_Bool(*ne)(void* self, void** other);
} __chx_core_coreopsPartialEq__cgs__0_vt_t;
typedef struct __chx_core_coreopsEq_vt_t {
	_Bool(*eq)(void* self, void** other);_Bool(*ne)(void* self, void** other);
} __chx_core_coreopsEq_vt_t;
typedef struct __chx_core_coreopsOrd_vt_t {
	int(*cmp)(void* self, void**const other);_Bool(*lt)(void* self, void**const other);_Bool(*lte)(void* self, void**const other);_Bool(*gt)(void* self, void**const other);_Bool(*gte)(void* self, void**const other);
} __chx_core_coreopsOrd_vt_t;
/** Translate D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\core\ops.ch **/