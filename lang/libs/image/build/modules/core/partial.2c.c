
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/core/interfaces.ch **/
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/core/ops.ch **/
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/core/iterable.ch **/
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/core/stream.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/core/interfaces.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/core/ops.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/core/iterable.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/core/stream.ch **/
/** FwdDeclare:Generics core **/
/** Declare:Generics core **/
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/core/interfaces.ch **/
typedef struct __chx_core_Copy_vt_t {
	
} __chx_core_Copy_vt_t;
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/core/ops.ch **/
typedef struct __chx_core_coreopsEq_vt_t {
	
} __chx_core_coreopsEq_vt_t;
typedef struct __chx_core_coreopsOrd_vt_t {
	int(*cmp)(void* self, void**const other);_Bool(*lt)(void* self, void**const other);_Bool(*lte)(void* self, void**const other);_Bool(*gt)(void* self, void**const other);_Bool(*gte)(void* self, void**const other);
} __chx_core_coreopsOrd_vt_t;
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/core/iterable.ch **/
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/core/stream.ch **/
typedef struct __chx_core_corestreamStream_vt_t {
	void(*writeStr)(void* self, const char* value, uint64_t length);void(*writeStrNoLen)(void* self, const char* value);void(*writeSigned)(void* self, int64_t value);void(*writeUnsigned)(void* self, uint64_t value);void(*writeChar)(void* self, char value);void(*writeUChar)(void* self, unsigned char value);void(*writeFloat)(void* self, float value);void(*writeDouble)(void* self, double value);
} __chx_core_corestreamStream_vt_t;
/** Implement:Generics core **/
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/core/interfaces.ch **/
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/core/ops.ch **/
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/core/iterable.ch **/
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/core/stream.ch **/