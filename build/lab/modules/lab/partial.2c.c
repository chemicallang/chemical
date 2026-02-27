
/** FwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\lab\src\AnnotationController.ch **/
/** FwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\lab\src\lab.ch **/
struct lab_LabJobCBI;
struct lab_DependencySymbolInfo;
struct lab_ModuleDependency;
struct lab_ImportSymbol;
struct lab_ImportRepo;
/** FwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\lab\src\target_data.ch **/
struct lab_TargetData;
/** TypeAliasDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\lab\src\AnnotationController.ch **/
/** TypeAliasDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\lab\src\lab.ch **/
/** TypeAliasDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\lab\src\target_data.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\concurrency\threadpool.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\fs.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\function.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\hashing\compare_impl.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\hashing\fnv1.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\hashing\hash.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\hashing\hash_impl.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\hashing\murmur.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\mutex.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\option.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\os_string.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\pair.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\panic.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\result.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\span.ch **/
struct std_stdspan__cgs__0;
struct std_stdspan__cgs__1;
struct std_stdspan__cgs__2;
struct std_stdspan__cgs__3;
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\std.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\stream.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\string.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\string_view.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\u16_string.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\unordered_map2.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\utils.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\vector.ch **/
struct std_stdvector__cgs__2;
struct std_stdvector__cgs__3;
struct std_stdvector__cgs__4;
struct std_stdvector__cgs__5;
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\win\condvar.ch **/
/** ExtFwdDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\win\mutex.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\concurrency\threadpool.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\fs.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\function.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\hashing\compare_impl.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\hashing\fnv1.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\hashing\hash.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\hashing\hash_impl.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\hashing\murmur.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\mutex.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\option.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\os_string.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\pair.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\panic.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\result.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\span.ch **/
struct std_stdspan__cgs__0 {
	const struct lab_ImportSymbol* _data;
	size_t _size;
};;;
void std_stdspan__cgs__0constructor(struct std_stdspan__cgs__0* this, const struct lab_ImportSymbol* array_ptr, size_t array_size);
void std_stdspan__cgs__0empty_make(struct std_stdspan__cgs__0* this);
const struct lab_ImportSymbol* std_stdspan__cgs__0data(struct std_stdspan__cgs__0*const self);
const struct lab_ImportSymbol* std_stdspan__cgs__0get(struct std_stdspan__cgs__0*const self, size_t loc);
size_t std_stdspan__cgs__0size(struct std_stdspan__cgs__0*const self);
_Bool std_stdspan__cgs__0empty(struct std_stdspan__cgs__0*const self);
struct std_stdspan__cgs__1 {
	const struct lab_ModuleDependency* _data;
	size_t _size;
};;;
void std_stdspan__cgs__1constructor(struct std_stdspan__cgs__1* this, const struct lab_ModuleDependency* array_ptr, size_t array_size);
void std_stdspan__cgs__1empty_make(struct std_stdspan__cgs__1* this);
const struct lab_ModuleDependency* std_stdspan__cgs__1data(struct std_stdspan__cgs__1*const self);
const struct lab_ModuleDependency* std_stdspan__cgs__1get(struct std_stdspan__cgs__1*const self, size_t loc);
size_t std_stdspan__cgs__1size(struct std_stdspan__cgs__1*const self);
_Bool std_stdspan__cgs__1empty(struct std_stdspan__cgs__1*const self);
struct std_stdspan__cgs__2 {
	const struct std_stdstring_view* _data;
	size_t _size;
};;;
void std_stdspan__cgs__2constructor(struct std_stdspan__cgs__2* this, const struct std_stdstring_view* array_ptr, size_t array_size);
void std_stdspan__cgs__2empty_make(struct std_stdspan__cgs__2* this);
const struct std_stdstring_view* std_stdspan__cgs__2data(struct std_stdspan__cgs__2*const self);
const struct std_stdstring_view* std_stdspan__cgs__2get(struct std_stdspan__cgs__2*const self, size_t loc);
size_t std_stdspan__cgs__2size(struct std_stdspan__cgs__2*const self);
_Bool std_stdspan__cgs__2empty(struct std_stdspan__cgs__2*const self);
struct std_stdspan__cgs__3 {
	const void*** _data;
	size_t _size;
};;;
void std_stdspan__cgs__3constructor(struct std_stdspan__cgs__3* this, const void*** array_ptr, size_t array_size);
void std_stdspan__cgs__3empty_make(struct std_stdspan__cgs__3* this);
const void*** std_stdspan__cgs__3data(struct std_stdspan__cgs__3*const self);
const void*** std_stdspan__cgs__3get(struct std_stdspan__cgs__3*const self, size_t loc);
size_t std_stdspan__cgs__3size(struct std_stdspan__cgs__3*const self);
_Bool std_stdspan__cgs__3empty(struct std_stdspan__cgs__3*const self);
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\std.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\stream.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\string.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\string_view.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\u16_string.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\unordered_map2.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\utils.ch **/
void std_stdreplace__cfg_0(struct std_stdconcurrentTask* __chx_struct_ret_param_xx, struct std_stdconcurrentTask* value, struct std_stdconcurrentTask* repl);
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\vector.ch **/
struct std_stdvector__cgs__2 {
	struct lab_ImportSymbol* data_ptr;
	size_t data_size;
	size_t data_cap;
};
void std_stdvector__cgs__2make(struct std_stdvector__cgs__2* this);
void std_stdvector__cgs__2make_with_capacity(struct std_stdvector__cgs__2* this, size_t init_cap);
void std_stdvector__cgs__2resize(struct std_stdvector__cgs__2* self, size_t new_cap);
void std_stdvector__cgs__2reserve(struct std_stdvector__cgs__2* self, size_t cap);
void std_stdvector__cgs__2ensure_capacity_for_one_more(struct std_stdvector__cgs__2* self);
void std_stdvector__cgs__2push(struct std_stdvector__cgs__2* self, struct lab_ImportSymbol* value);
void std_stdvector__cgs__2push_back(struct std_stdvector__cgs__2* self, struct lab_ImportSymbol* value);
void std_stdvector__cgs__2get(struct lab_ImportSymbol* __chx_struct_ret_param_xx, struct std_stdvector__cgs__2*const self, size_t index);
struct lab_ImportSymbol* std_stdvector__cgs__2get_ptr(struct std_stdvector__cgs__2*const self, size_t index);
struct lab_ImportSymbol* std_stdvector__cgs__2get_ref(struct std_stdvector__cgs__2*const self, size_t index);
void std_stdvector__cgs__2set(struct std_stdvector__cgs__2* self, size_t index, struct lab_ImportSymbol* value);
size_t std_stdvector__cgs__2size(struct std_stdvector__cgs__2*const self);
size_t std_stdvector__cgs__2capacity(struct std_stdvector__cgs__2*const self);
const struct lab_ImportSymbol* std_stdvector__cgs__2data(struct std_stdvector__cgs__2*const self);
struct lab_ImportSymbol* std_stdvector__cgs__2last_ptr(struct std_stdvector__cgs__2*const self);
void std_stdvector__cgs__2remove(struct std_stdvector__cgs__2* self, size_t index);
void std_stdvector__cgs__2erase(struct std_stdvector__cgs__2* self, size_t index);
void std_stdvector__cgs__2remove_last(struct std_stdvector__cgs__2* self);
void std_stdvector__cgs__2pop_back(struct std_stdvector__cgs__2* self);
void std_stdvector__cgs__2take_last(struct lab_ImportSymbol* __chx_struct_ret_param_xx, struct std_stdvector__cgs__2* self);
_Bool std_stdvector__cgs__2empty(struct std_stdvector__cgs__2*const self);
void std_stdvector__cgs__2clear(struct std_stdvector__cgs__2* self);
void std_stdvector__cgs__2resize_unsafe(struct std_stdvector__cgs__2* self, size_t new_size);
void std_stdvector__cgs__2delete(struct std_stdvector__cgs__2* self);
struct std_stdvector__cgs__3 {
	struct lab_ModuleDependency* data_ptr;
	size_t data_size;
	size_t data_cap;
};
void std_stdvector__cgs__3make(struct std_stdvector__cgs__3* this);
void std_stdvector__cgs__3make_with_capacity(struct std_stdvector__cgs__3* this, size_t init_cap);
void std_stdvector__cgs__3resize(struct std_stdvector__cgs__3* self, size_t new_cap);
void std_stdvector__cgs__3reserve(struct std_stdvector__cgs__3* self, size_t cap);
void std_stdvector__cgs__3ensure_capacity_for_one_more(struct std_stdvector__cgs__3* self);
void std_stdvector__cgs__3push(struct std_stdvector__cgs__3* self, struct lab_ModuleDependency* value);
void std_stdvector__cgs__3push_back(struct std_stdvector__cgs__3* self, struct lab_ModuleDependency* value);
void std_stdvector__cgs__3get(struct lab_ModuleDependency* __chx_struct_ret_param_xx, struct std_stdvector__cgs__3*const self, size_t index);
struct lab_ModuleDependency* std_stdvector__cgs__3get_ptr(struct std_stdvector__cgs__3*const self, size_t index);
struct lab_ModuleDependency* std_stdvector__cgs__3get_ref(struct std_stdvector__cgs__3*const self, size_t index);
void std_stdvector__cgs__3set(struct std_stdvector__cgs__3* self, size_t index, struct lab_ModuleDependency* value);
size_t std_stdvector__cgs__3size(struct std_stdvector__cgs__3*const self);
size_t std_stdvector__cgs__3capacity(struct std_stdvector__cgs__3*const self);
const struct lab_ModuleDependency* std_stdvector__cgs__3data(struct std_stdvector__cgs__3*const self);
struct lab_ModuleDependency* std_stdvector__cgs__3last_ptr(struct std_stdvector__cgs__3*const self);
void std_stdvector__cgs__3remove(struct std_stdvector__cgs__3* self, size_t index);
void std_stdvector__cgs__3erase(struct std_stdvector__cgs__3* self, size_t index);
void std_stdvector__cgs__3remove_last(struct std_stdvector__cgs__3* self);
void std_stdvector__cgs__3pop_back(struct std_stdvector__cgs__3* self);
void std_stdvector__cgs__3take_last(struct lab_ModuleDependency* __chx_struct_ret_param_xx, struct std_stdvector__cgs__3* self);
_Bool std_stdvector__cgs__3empty(struct std_stdvector__cgs__3*const self);
void std_stdvector__cgs__3clear(struct std_stdvector__cgs__3* self);
void std_stdvector__cgs__3resize_unsafe(struct std_stdvector__cgs__3* self, size_t new_size);
void std_stdvector__cgs__3delete(struct std_stdvector__cgs__3* self);
struct std_stdvector__cgs__4 {
	struct std_stdstring_view* data_ptr;
	size_t data_size;
	size_t data_cap;
};
void std_stdvector__cgs__4make(struct std_stdvector__cgs__4* this);
void std_stdvector__cgs__4make_with_capacity(struct std_stdvector__cgs__4* this, size_t init_cap);
void std_stdvector__cgs__4resize(struct std_stdvector__cgs__4* self, size_t new_cap);
void std_stdvector__cgs__4reserve(struct std_stdvector__cgs__4* self, size_t cap);
void std_stdvector__cgs__4ensure_capacity_for_one_more(struct std_stdvector__cgs__4* self);
void std_stdvector__cgs__4push(struct std_stdvector__cgs__4* self, struct std_stdstring_view* value);
void std_stdvector__cgs__4push_back(struct std_stdvector__cgs__4* self, struct std_stdstring_view* value);
void std_stdvector__cgs__4get(struct std_stdstring_view* __chx_struct_ret_param_xx, struct std_stdvector__cgs__4*const self, size_t index);
struct std_stdstring_view* std_stdvector__cgs__4get_ptr(struct std_stdvector__cgs__4*const self, size_t index);
struct std_stdstring_view* std_stdvector__cgs__4get_ref(struct std_stdvector__cgs__4*const self, size_t index);
void std_stdvector__cgs__4set(struct std_stdvector__cgs__4* self, size_t index, struct std_stdstring_view* value);
size_t std_stdvector__cgs__4size(struct std_stdvector__cgs__4*const self);
size_t std_stdvector__cgs__4capacity(struct std_stdvector__cgs__4*const self);
const struct std_stdstring_view* std_stdvector__cgs__4data(struct std_stdvector__cgs__4*const self);
struct std_stdstring_view* std_stdvector__cgs__4last_ptr(struct std_stdvector__cgs__4*const self);
void std_stdvector__cgs__4remove(struct std_stdvector__cgs__4* self, size_t index);
void std_stdvector__cgs__4erase(struct std_stdvector__cgs__4* self, size_t index);
void std_stdvector__cgs__4remove_last(struct std_stdvector__cgs__4* self);
void std_stdvector__cgs__4pop_back(struct std_stdvector__cgs__4* self);
void std_stdvector__cgs__4take_last(struct std_stdstring_view* __chx_struct_ret_param_xx, struct std_stdvector__cgs__4* self);
_Bool std_stdvector__cgs__4empty(struct std_stdvector__cgs__4*const self);
void std_stdvector__cgs__4clear(struct std_stdvector__cgs__4* self);
void std_stdvector__cgs__4resize_unsafe(struct std_stdvector__cgs__4* self, size_t new_size);
void std_stdvector__cgs__4delete(struct std_stdvector__cgs__4* self);
struct std_stdvector__cgs__5 {
	void*** data_ptr;
	size_t data_size;
	size_t data_cap;
};
void std_stdvector__cgs__5make(struct std_stdvector__cgs__5* this);
void std_stdvector__cgs__5make_with_capacity(struct std_stdvector__cgs__5* this, size_t init_cap);
void std_stdvector__cgs__5resize(struct std_stdvector__cgs__5* self, size_t new_cap);
void std_stdvector__cgs__5reserve(struct std_stdvector__cgs__5* self, size_t cap);
void std_stdvector__cgs__5ensure_capacity_for_one_more(struct std_stdvector__cgs__5* self);
void std_stdvector__cgs__5push(struct std_stdvector__cgs__5* self, void** value);
void std_stdvector__cgs__5push_back(struct std_stdvector__cgs__5* self, void** value);
void** std_stdvector__cgs__5get(struct std_stdvector__cgs__5*const self, size_t index);
void*** std_stdvector__cgs__5get_ptr(struct std_stdvector__cgs__5*const self, size_t index);
void*** std_stdvector__cgs__5get_ref(struct std_stdvector__cgs__5*const self, size_t index);
void std_stdvector__cgs__5set(struct std_stdvector__cgs__5* self, size_t index, void** value);
size_t std_stdvector__cgs__5size(struct std_stdvector__cgs__5*const self);
size_t std_stdvector__cgs__5capacity(struct std_stdvector__cgs__5*const self);
const void*** std_stdvector__cgs__5data(struct std_stdvector__cgs__5*const self);
void*** std_stdvector__cgs__5last_ptr(struct std_stdvector__cgs__5*const self);
void std_stdvector__cgs__5remove(struct std_stdvector__cgs__5* self, size_t index);
void std_stdvector__cgs__5erase(struct std_stdvector__cgs__5* self, size_t index);
void std_stdvector__cgs__5remove_last(struct std_stdvector__cgs__5* self);
void std_stdvector__cgs__5pop_back(struct std_stdvector__cgs__5* self);
void** std_stdvector__cgs__5take_last(struct std_stdvector__cgs__5* self);
_Bool std_stdvector__cgs__5empty(struct std_stdvector__cgs__5*const self);
void std_stdvector__cgs__5clear(struct std_stdvector__cgs__5* self);
void std_stdvector__cgs__5resize_unsafe(struct std_stdvector__cgs__5* self, size_t new_size);
void std_stdvector__cgs__5delete(struct std_stdvector__cgs__5* self);
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\win\condvar.ch **/
/** ExtDeclare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\win\mutex.ch **/
/** Declare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\lab\src\AnnotationController.ch **/
void lab_AnnotationControllercreateSingleMarkerAnnotation(void**const self, struct std_stdstring_view*const name, int policy);
void lab_AnnotationControllercreateMarkerAnnotation(void**const self, struct std_stdstring_view*const name);
void lab_AnnotationControllercreateCollectorAnnotation(void**const self, struct std_stdstring_view*const name, unsigned int expected_usage);
void lab_AnnotationControllercreateMarkerAndCollectorAnnotation(void**const self, struct std_stdstring_view*const name, unsigned int expected_usage);
/** Declare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\lab\src\lab.ch **/
int lab_ModulegetType(void**const self);
void lab_ModulegetScopeName(struct std_stdstring_view* __chx_struct_ret_param_xx, void**const self);
void lab_ModulegetName(struct std_stdstring_view* __chx_struct_ret_param_xx, void**const self);
void lab_ModulegetBitcodePath(struct std_stdstring_view* __chx_struct_ret_param_xx, void**const self);
void lab_ModulesetBitcodePath(void**const self, struct std_stdstring_view*const path);
void lab_ModulegetObjectPath(struct std_stdstring_view* __chx_struct_ret_param_xx, void**const self);
void lab_ModulesetObjectPath(void**const self, struct std_stdstring_view*const path);
void lab_ModulegetLlvmIrPath(struct std_stdstring_view* __chx_struct_ret_param_xx, void**const self);
void lab_ModulesetLlvmIrPath(void**const self, struct std_stdstring_view*const path);
void lab_ModulegetAsmPath(struct std_stdstring_view* __chx_struct_ret_param_xx, void**const self);
void lab_ModulesetAsmPath(void**const self, struct std_stdstring_view*const path);
int lab_LabJobgetType(void**const self);
void lab_LabJobgetName(struct std_stdstring_view* __chx_struct_ret_param_xx, void**const self);
void lab_LabJobgetAbsPath(struct std_stdstring_view* __chx_struct_ret_param_xx, void**const self);
void lab_LabJobgetBuildDir(struct std_stdstring_view* __chx_struct_ret_param_xx, void**const self);
int lab_LabJobgetStatus(void**const self);
void lab_LabJobgetTargetTriple(struct std_stdstring_view* __chx_struct_ret_param_xx, void**const self);
int lab_LabJobgetMode(void**const self);
struct lab_TargetData*const lab_LabJobgetTarget(void**const self);
struct lab_LabJobCBI {
};
struct lab_DependencySymbolInfo {
	struct std_stdspan__cgs__0 symbols;
	struct std_stdstring_view alias;
	uint64_t location;
};
struct lab_ModuleDependency {
	void** module;
	struct lab_DependencySymbolInfo* info;
};
void** lab_BuildContextgetAnnotationController(void**const self);
void** lab_BuildContextnew_package(void**const self, int type, int package_kind, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name, struct std_stdspan__cgs__1* dependencies);
void lab_BuildContextset_module_symbol_info(void**const self, void** module, unsigned int index, struct lab_DependencySymbolInfo*const info);
void** lab_BuildContextget_cached(void**const self, const void** job, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name);
void lab_BuildContextset_cached(void**const self, const void** job, void** module);
void lab_BuildContextadd_path(void**const self, void** module, struct std_stdstring_view*const path);
void lab_BuildContextadd_dependency(void**const self, void** job, void** module, struct lab_DependencySymbolInfo* info);
void lab_BuildContextadd_module(void**const self, void** job, void** module);
void lab_BuildContextput_job_before(void**const self, void** newJob, void** existingJob);
void lab_BuildContextlink_system_lib(void**const self, void** job, struct std_stdstring_view*const name, void** module);
_Bool lab_BuildContextadd_compiler_interface(void**const self, void** module, struct std_stdstring_view*const interface);
_Bool lab_BuildContextresolve_condition(void**const self, const void** job, struct std_stdstring_view*const condition);
void lab_BuildContextinclude_header(void**const self, void** module, struct std_stdstring_view*const header);
void** lab_BuildContexttranslate_to_chemical(void**const self, void** module, struct std_stdstring_view*const output_path);
void** lab_BuildContexttranslate_to_c(void**const self, struct std_stdstring_view*const name, struct std_stdstring_view*const output_path);
void** lab_BuildContextbuild_exe(void**const self, struct std_stdstring_view*const name);
void** lab_BuildContextrun_jit_exe(void**const self, struct std_stdstring_view*const name);
void** lab_BuildContextbuild_dynamic_lib(void**const self, struct std_stdstring_view*const name);
struct lab_LabJobCBI* lab_BuildContextbuild_cbi(void**const self, struct std_stdstring_view*const name);
void lab_BuildContextset_environment_testing(void**const self, void** job, _Bool value);
_Bool lab_BuildContextindex_cbi_fn(void**const self, struct lab_LabJobCBI* job, struct std_stdstring_view*const key, struct std_stdstring_view*const fn_name, int fn_type);
void lab_BuildContextadd_object(void**const self, const void** job, struct std_stdstring_view*const path);
void lab_BuildContextbuild_path(struct std_stdstring_view* __chx_struct_ret_param_xx, void**const self);
_Bool lab_BuildContexthas_arg(void**const self, struct std_stdstring_view*const name);
void lab_BuildContextget_arg(struct std_stdstring_view* __chx_struct_ret_param_xx, void**const self, struct std_stdstring_view*const name);
void lab_BuildContextremove_arg(void**const self, struct std_stdstring_view*const name);
_Bool lab_BuildContextdefine(void**const self, const void** job, struct std_stdstring_view*const name);
_Bool lab_BuildContextundefine(void**const self, const void** job, struct std_stdstring_view*const name);
int lab_BuildContextinvoke_dlltool(void**const self, struct std_stdspan__cgs__2* string_arr);
int lab_BuildContextinvoke_ranlib(void**const self, struct std_stdspan__cgs__2* string_arr);
int lab_BuildContextinvoke_lib(void**const self, struct std_stdspan__cgs__2* string_arr);
int lab_BuildContextinvoke_ar(void**const self, struct std_stdspan__cgs__2* string_arr);
void lab_BuildContextset_conflict_resolution_strategy(void**const self, void** job, int strategy);
_Bool lab_BuildContextfetch_job_dependency(void**const self, void** job, struct lab_ImportRepo*const dep, int strategy);
_Bool lab_BuildContextfetch_mod_dependency(void**const self, void** job, void** mod, struct lab_ImportRepo*const dep, int strategy);
void** lab_BuildContextnew_module(void**const ctx, int type, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name, struct std_stdspan__cgs__1* dependencies);
void** lab_BuildContextnew_app_module(void**const ctx, int type, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name, struct std_stdspan__cgs__1* dependencies);
void** lab_BuildContextdirectory_module(void**const ctx, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name, struct std_stdspan__cgs__1* dependencies);
static void lab_make_deps(struct std_stdvector__cgs__3* vec, struct std_stdspan__cgs__3* dependencies);
void** lab_BuildContextnew_module_with_deps(void**const ctx, int type, int package_kind, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name, struct std_stdspan__cgs__3* dependencies);
void** lab_BuildContextc_file_module(void**const ctx, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name, struct std_stdstring_view*const path, struct std_stdspan__cgs__3* dependencies);
void** lab_BuildContextcpp_file_module(void**const ctx, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name, struct std_stdstring_view*const path, struct std_stdspan__cgs__3* dependencies);
void** lab_BuildContextobject_module(void**const ctx, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name, struct std_stdstring_view*const path);
void** lab_BuildContextchemical_dir_module(void**const ctx, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name, struct std_stdstring_view*const path, struct std_stdspan__cgs__3* dependencies);
void** lab_BuildContextdirectory_app_module(void**const ctx, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name, struct std_stdstring_view*const path, struct std_stdspan__cgs__3* dependencies);
struct lab_ImportSymbol {
	struct std_stdspan__cgs__2 parts;
	struct std_stdstring_view alias;
};
void lab_ImportSymbolmake(struct lab_ImportSymbol* this);
struct lab_ImportRepo {
	struct std_stdstring_view scope;
	struct std_stdstring_view name;
	struct std_stdstring_view origin;
	struct std_stdstring_view from;
	struct std_stdstring_view subdir;
	struct std_stdstring_view version;
	struct std_stdstring_view branch;
	struct std_stdstring_view commit;
	struct std_stdstring_view alias;
	struct std_stdspan__cgs__0 symbols;
	uint64_t location;
};
int lab_AppBuildContextlaunch_executable(void**const self, struct std_stdstring_view*const path, _Bool same_window);
void lab_AppBuildContexton_finished(void**const self, void(*lambda)(const void* data), const void* data);
void lab_BuildContextadd_compiler_interfaces(void**const ctx, void** mod, struct std_stdspan__cgs__2* interfaces);
void** lab_BuildContextcreate_module(void**const ctx, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name, struct std_stdstring_view*const dir_path, struct std_stdspan__cgs__3* dependencies, struct std_stdspan__cgs__2* interfaces);
void** lab_BuildContextdefault_get(void**const ctx, _Bool* buildFlag, void*** cached, void**(*build)(const void** ctx));
void** lab_BuildContextfile_module(void**const ctx, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name, struct std_stdstring_view*const path, struct std_stdspan__cgs__3* dependencies);
void** lab_BuildContexttranslate_file_to_chemical(void**const ctx, struct std_stdstring_view*const c_path, struct std_stdstring_view*const output_path);
void lab_BuildContextinclude_headers(void**const ctx, void** module, struct std_stdspan__cgs__2* headers);
void lab_BuildContextindex_def_cbi_fn(void**const ctx, struct lab_LabJobCBI* job, struct std_stdstring_view*const name, int type);
void lab_BuildContextbuild_job_dir_path(struct std_stdstring* __chx_struct_ret_param_xx, void**const ctx, struct std_stdstring_view*const job_name);
void lab_BuildContextjob_dir_path(struct std_stdstring* __chx_struct_ret_param_xx, void**const ctx, void** job);
void lab_BuildContextbuild_mod_file_path(struct std_stdstring* __chx_struct_ret_param_xx, void**const ctx, struct std_stdstring_view*const job_name, struct std_stdstring_view*const mod_scope, struct std_stdstring_view*const mod_name, struct std_stdstring_view*const file);
void lab_BuildContextbuild_llvm_ir_path(struct std_stdstring* __chx_struct_ret_param_xx, void**const ctx, struct std_stdstring_view*const job_name, struct std_stdstring_view*const mod_scope, struct std_stdstring_view*const mod_name);
void lab_BuildContextbuild_asm_path(struct std_stdstring* __chx_struct_ret_param_xx, void**const ctx, struct std_stdstring_view*const job_name, struct std_stdstring_view*const mod_scope, struct std_stdstring_view*const mod_name);
void lab_BuildContextbuild_bitcode_path(struct std_stdstring* __chx_struct_ret_param_xx, void**const ctx, struct std_stdstring_view*const job_name, struct std_stdstring_view*const mod_scope, struct std_stdstring_view*const mod_name);
void lab_BuildContextllvm_ir_path(struct std_stdstring* __chx_struct_ret_param_xx, void**const ctx, void** job, void** mod);
void lab_BuildContextasm_path(struct std_stdstring* __chx_struct_ret_param_xx, void**const ctx, void** job, void** mod);
void lab_BuildContextbitcode_path(struct std_stdstring* __chx_struct_ret_param_xx, void**const ctx, void** job, void** mod);
void lab_labcurr_dir_of(struct std_stdstring* __chx_struct_ret_param_xx, const char* path, size_t len);
void lab_labappended_str(struct std_stdstring* __chx_struct_ret_param_xx, struct std_stdstring* str, const char* path);
/** Declare D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\lab\src\target_data.ch **/
struct lab_TargetData {
	_Bool tcc;
	_Bool clang;
	_Bool cbi;
	_Bool lsp;
	_Bool test;
	_Bool debug;
	_Bool debug_quick;
	_Bool debug_complete;
	_Bool release;
	_Bool release_safe;
	_Bool release_small;
	_Bool release_fast;
	_Bool posix;
	_Bool gnu;
	_Bool is64Bit;
	_Bool little_endian;
	_Bool big_endian;
	_Bool windows;
	_Bool win32;
	_Bool win64;
	_Bool linux;
	_Bool macos;
	_Bool freebsd;
	_Bool isUnix;
	_Bool android;
	_Bool cygwin;
	_Bool mingw32;
	_Bool mingw64;
	_Bool emscripten;
	_Bool x86_64;
	_Bool x86;
	_Bool i386;
	_Bool arm;
	_Bool aarch64;
	_Bool powerpc;
	_Bool powerpc64;
	_Bool riscv;
	_Bool s390x;
	_Bool wasm32;
	_Bool wasm64;
};
void lab_TargetDatamake(struct lab_TargetData* this);
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\concurrency\threadpool.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\fs.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\function.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\hashing\compare_impl.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\hashing\fnv1.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\hashing\hash.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\hashing\hash_impl.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\hashing\murmur.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\mutex.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\option.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\os_string.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\pair.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\panic.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\result.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\span.ch **/
void std_stdspan__cgs__0constructor(struct std_stdspan__cgs__0* this, const struct lab_ImportSymbol* array_ptr, size_t array_size){
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
const struct lab_ImportSymbol* std_stdspan__cgs__0data(struct std_stdspan__cgs__0*const self){
	return self->_data;
}
const struct lab_ImportSymbol* std_stdspan__cgs__0get(struct std_stdspan__cgs__0*const self, size_t loc){
	return (self->_data + loc);
}
size_t std_stdspan__cgs__0size(struct std_stdspan__cgs__0*const self){
	return self->_size;
}
_Bool std_stdspan__cgs__0empty(struct std_stdspan__cgs__0*const self){
	return (self->_size == 0);
}
void std_stdspan__cgs__1constructor(struct std_stdspan__cgs__1* this, const struct lab_ModuleDependency* array_ptr, size_t array_size){
	*this = (struct std_stdspan__cgs__1){ 
		._data = array_ptr, 
		._size = array_size
	};
	return;
}
void std_stdspan__cgs__1empty_make(struct std_stdspan__cgs__1* this){
	*this = (struct std_stdspan__cgs__1){ 
		._data = NULL, 
		._size = 0
	};
	return;
}
const struct lab_ModuleDependency* std_stdspan__cgs__1data(struct std_stdspan__cgs__1*const self){
	return self->_data;
}
const struct lab_ModuleDependency* std_stdspan__cgs__1get(struct std_stdspan__cgs__1*const self, size_t loc){
	return (self->_data + loc);
}
size_t std_stdspan__cgs__1size(struct std_stdspan__cgs__1*const self){
	return self->_size;
}
_Bool std_stdspan__cgs__1empty(struct std_stdspan__cgs__1*const self){
	return (self->_size == 0);
}
void std_stdspan__cgs__2constructor(struct std_stdspan__cgs__2* this, const struct std_stdstring_view* array_ptr, size_t array_size){
	*this = (struct std_stdspan__cgs__2){ 
		._data = array_ptr, 
		._size = array_size
	};
	return;
}
void std_stdspan__cgs__2empty_make(struct std_stdspan__cgs__2* this){
	*this = (struct std_stdspan__cgs__2){ 
		._data = NULL, 
		._size = 0
	};
	return;
}
const struct std_stdstring_view* std_stdspan__cgs__2data(struct std_stdspan__cgs__2*const self){
	return self->_data;
}
const struct std_stdstring_view* std_stdspan__cgs__2get(struct std_stdspan__cgs__2*const self, size_t loc){
	return (self->_data + loc);
}
size_t std_stdspan__cgs__2size(struct std_stdspan__cgs__2*const self){
	return self->_size;
}
_Bool std_stdspan__cgs__2empty(struct std_stdspan__cgs__2*const self){
	return (self->_size == 0);
}
void std_stdspan__cgs__3constructor(struct std_stdspan__cgs__3* this, const void*** array_ptr, size_t array_size){
	*this = (struct std_stdspan__cgs__3){ 
		._data = array_ptr, 
		._size = array_size
	};
	return;
}
void std_stdspan__cgs__3empty_make(struct std_stdspan__cgs__3* this){
	*this = (struct std_stdspan__cgs__3){ 
		._data = NULL, 
		._size = 0
	};
	return;
}
const void*** std_stdspan__cgs__3data(struct std_stdspan__cgs__3*const self){
	return self->_data;
}
const void*** std_stdspan__cgs__3get(struct std_stdspan__cgs__3*const self, size_t loc){
	return (self->_data + loc);
}
size_t std_stdspan__cgs__3size(struct std_stdspan__cgs__3*const self){
	return self->_size;
}
_Bool std_stdspan__cgs__3empty(struct std_stdspan__cgs__3*const self){
	return (self->_size == 0);
}
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\std.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\stream.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\string.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\string_view.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\u16_string.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\unordered_map2.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\utils.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\src\vector.ch **/
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
		.data_ptr = ((struct lab_ImportSymbol*) malloc((sizeof(struct lab_ImportSymbol) * init_cap))), 
		.data_size = 0, 
		.data_cap = init_cap
	};
	return;
}
void std_stdvector__cgs__2resize(struct std_stdvector__cgs__2* self, size_t new_cap){
	struct lab_ImportSymbol* new_data = ((struct lab_ImportSymbol*) realloc(self->data_ptr, (sizeof(struct lab_ImportSymbol) * new_cap)));
	
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
void std_stdvector__cgs__2reserve(struct std_stdvector__cgs__2* self, size_t cap){
	if((cap <= self->data_cap)){
		return;
	}
	std_stdvector__cgs__2resize(self, cap);
}
void std_stdvector__cgs__2ensure_capacity_for_one_more(struct std_stdvector__cgs__2* self){
	if((self->data_cap == 0)){
		std_stdvector__cgs__2resize(self, 2);
	} else {
		std_stdvector__cgs__2resize(self, (self->data_cap * 2));
	}
}
void std_stdvector__cgs__2push(struct std_stdvector__cgs__2* self, struct lab_ImportSymbol* value){
	size_t s = self->data_size;
	if((s >= self->data_cap)){
		std_stdvector__cgs__2ensure_capacity_for_one_more(self);
	}
	memcpy(&self->data_ptr[s], value, sizeof(struct lab_ImportSymbol));
	0;
	self->data_size = (s + 1);
}
void std_stdvector__cgs__2push_back(struct std_stdvector__cgs__2* self, struct lab_ImportSymbol* value){
	std_stdvector__cgs__2push(self, *({  &value; }));
}
void std_stdvector__cgs__2get(struct lab_ImportSymbol* __chx_struct_ret_param_xx, struct std_stdvector__cgs__2*const self, size_t index){
	*__chx_struct_ret_param_xx = self->data_ptr[index];
	return;
}
struct lab_ImportSymbol* std_stdvector__cgs__2get_ptr(struct std_stdvector__cgs__2*const self, size_t index){
	return &self->data_ptr[index];
}
struct lab_ImportSymbol* std_stdvector__cgs__2get_ref(struct std_stdvector__cgs__2*const self, size_t index){
	return &self->data_ptr[index];
}
void std_stdvector__cgs__2set(struct std_stdvector__cgs__2* self, size_t index, struct lab_ImportSymbol* value){
	self->data_ptr[index] = ({  *value; });
}
size_t std_stdvector__cgs__2size(struct std_stdvector__cgs__2*const self){
	return self->data_size;
}
size_t std_stdvector__cgs__2capacity(struct std_stdvector__cgs__2*const self){
	return self->data_cap;
}
const struct lab_ImportSymbol* std_stdvector__cgs__2data(struct std_stdvector__cgs__2*const self){
	return self->data_ptr;
}
struct lab_ImportSymbol* std_stdvector__cgs__2last_ptr(struct std_stdvector__cgs__2*const self){
	return (self->data_ptr + (self->data_size - 1));
}
void std_stdvector__cgs__2remove(struct std_stdvector__cgs__2* self, size_t index){
	size_t s = self->data_size;
	uint64_t last = (s - 1);
	
	if((index == last)){
		self->data_size = last;
	} else {
		memmove(&self->data_ptr[index], &self->data_ptr[(index + 1)], (sizeof(struct lab_ImportSymbol) * (last - index)));
		self->data_size = last;
	}
}
void std_stdvector__cgs__2erase(struct std_stdvector__cgs__2* self, size_t index){
	std_stdvector__cgs__2remove(self, index);
}
void std_stdvector__cgs__2remove_last(struct std_stdvector__cgs__2* self){
	size_t s = self->data_size;
	uint64_t last = (s - 1);
	struct lab_ImportSymbol* ptr = std_stdvector__cgs__2get_ptr(self, last);
	
	self->data_size = last;
}
void std_stdvector__cgs__2pop_back(struct std_stdvector__cgs__2* self){
	std_stdvector__cgs__2remove_last(self);
}
void std_stdvector__cgs__2take_last(struct lab_ImportSymbol* __chx_struct_ret_param_xx, struct std_stdvector__cgs__2* self){
	uint64_t last = (self->data_size - 1);
	self->data_size = last;
	*__chx_struct_ret_param_xx = *std_stdvector__cgs__2get_ptr(self, last);
	return;
}
_Bool std_stdvector__cgs__2empty(struct std_stdvector__cgs__2*const self){
	return (self->data_size == 0);
}
void std_stdvector__cgs__2clear(struct std_stdvector__cgs__2* self){
	
	self->data_size = 0;
}
void std_stdvector__cgs__2resize_unsafe(struct std_stdvector__cgs__2* self, size_t new_size){
	self->data_size = new_size;
}
void std_stdvector__cgs__2delete(struct std_stdvector__cgs__2* self){
	if((self->data_ptr != NULL)){
		
		free(self->data_ptr);
	}
	__chx__dstctr_clnup_blk__:{
	}
}
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
		.data_ptr = ((struct lab_ModuleDependency*) malloc((sizeof(struct lab_ModuleDependency) * init_cap))), 
		.data_size = 0, 
		.data_cap = init_cap
	};
	return;
}
void std_stdvector__cgs__3resize(struct std_stdvector__cgs__3* self, size_t new_cap){
	struct lab_ModuleDependency* new_data = ((struct lab_ModuleDependency*) realloc(self->data_ptr, (sizeof(struct lab_ModuleDependency) * new_cap)));
	
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
void std_stdvector__cgs__3reserve(struct std_stdvector__cgs__3* self, size_t cap){
	if((cap <= self->data_cap)){
		return;
	}
	std_stdvector__cgs__3resize(self, cap);
}
void std_stdvector__cgs__3ensure_capacity_for_one_more(struct std_stdvector__cgs__3* self){
	if((self->data_cap == 0)){
		std_stdvector__cgs__3resize(self, 2);
	} else {
		std_stdvector__cgs__3resize(self, (self->data_cap * 2));
	}
}
void std_stdvector__cgs__3push(struct std_stdvector__cgs__3* self, struct lab_ModuleDependency* value){
	size_t s = self->data_size;
	if((s >= self->data_cap)){
		std_stdvector__cgs__3ensure_capacity_for_one_more(self);
	}
	memcpy(&self->data_ptr[s], value, sizeof(struct lab_ModuleDependency));
	0;
	self->data_size = (s + 1);
}
void std_stdvector__cgs__3push_back(struct std_stdvector__cgs__3* self, struct lab_ModuleDependency* value){
	std_stdvector__cgs__3push(self, *({  &value; }));
}
void std_stdvector__cgs__3get(struct lab_ModuleDependency* __chx_struct_ret_param_xx, struct std_stdvector__cgs__3*const self, size_t index){
	*__chx_struct_ret_param_xx = self->data_ptr[index];
	return;
}
struct lab_ModuleDependency* std_stdvector__cgs__3get_ptr(struct std_stdvector__cgs__3*const self, size_t index){
	return &self->data_ptr[index];
}
struct lab_ModuleDependency* std_stdvector__cgs__3get_ref(struct std_stdvector__cgs__3*const self, size_t index){
	return &self->data_ptr[index];
}
void std_stdvector__cgs__3set(struct std_stdvector__cgs__3* self, size_t index, struct lab_ModuleDependency* value){
	self->data_ptr[index] = ({  *value; });
}
size_t std_stdvector__cgs__3size(struct std_stdvector__cgs__3*const self){
	return self->data_size;
}
size_t std_stdvector__cgs__3capacity(struct std_stdvector__cgs__3*const self){
	return self->data_cap;
}
const struct lab_ModuleDependency* std_stdvector__cgs__3data(struct std_stdvector__cgs__3*const self){
	return self->data_ptr;
}
struct lab_ModuleDependency* std_stdvector__cgs__3last_ptr(struct std_stdvector__cgs__3*const self){
	return (self->data_ptr + (self->data_size - 1));
}
void std_stdvector__cgs__3remove(struct std_stdvector__cgs__3* self, size_t index){
	size_t s = self->data_size;
	uint64_t last = (s - 1);
	
	if((index == last)){
		self->data_size = last;
	} else {
		memmove(&self->data_ptr[index], &self->data_ptr[(index + 1)], (sizeof(struct lab_ModuleDependency) * (last - index)));
		self->data_size = last;
	}
}
void std_stdvector__cgs__3erase(struct std_stdvector__cgs__3* self, size_t index){
	std_stdvector__cgs__3remove(self, index);
}
void std_stdvector__cgs__3remove_last(struct std_stdvector__cgs__3* self){
	size_t s = self->data_size;
	uint64_t last = (s - 1);
	struct lab_ModuleDependency* ptr = std_stdvector__cgs__3get_ptr(self, last);
	
	self->data_size = last;
}
void std_stdvector__cgs__3pop_back(struct std_stdvector__cgs__3* self){
	std_stdvector__cgs__3remove_last(self);
}
void std_stdvector__cgs__3take_last(struct lab_ModuleDependency* __chx_struct_ret_param_xx, struct std_stdvector__cgs__3* self){
	uint64_t last = (self->data_size - 1);
	self->data_size = last;
	*__chx_struct_ret_param_xx = *std_stdvector__cgs__3get_ptr(self, last);
	return;
}
_Bool std_stdvector__cgs__3empty(struct std_stdvector__cgs__3*const self){
	return (self->data_size == 0);
}
void std_stdvector__cgs__3clear(struct std_stdvector__cgs__3* self){
	
	self->data_size = 0;
}
void std_stdvector__cgs__3resize_unsafe(struct std_stdvector__cgs__3* self, size_t new_size){
	self->data_size = new_size;
}
void std_stdvector__cgs__3delete(struct std_stdvector__cgs__3* self){
	if((self->data_ptr != NULL)){
		
		free(self->data_ptr);
	}
	__chx__dstctr_clnup_blk__:{
	}
}
void std_stdvector__cgs__4make(struct std_stdvector__cgs__4* this){
	*this = (struct std_stdvector__cgs__4){ 
		.data_ptr = NULL, 
		.data_size = 0, 
		.data_cap = 0
	};
	return;
}
void std_stdvector__cgs__4make_with_capacity(struct std_stdvector__cgs__4* this, size_t init_cap){
	*this = (struct std_stdvector__cgs__4){ 
		.data_ptr = ((struct std_stdstring_view*) malloc((sizeof(struct std_stdstring_view) * init_cap))), 
		.data_size = 0, 
		.data_cap = init_cap
	};
	return;
}
void std_stdvector__cgs__4resize(struct std_stdvector__cgs__4* self, size_t new_cap){
	struct std_stdstring_view* new_data = ((struct std_stdstring_view*) realloc(self->data_ptr, (sizeof(struct std_stdstring_view) * new_cap)));
	
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
void std_stdvector__cgs__4reserve(struct std_stdvector__cgs__4* self, size_t cap){
	if((cap <= self->data_cap)){
		return;
	}
	std_stdvector__cgs__4resize(self, cap);
}
void std_stdvector__cgs__4ensure_capacity_for_one_more(struct std_stdvector__cgs__4* self){
	if((self->data_cap == 0)){
		std_stdvector__cgs__4resize(self, 2);
	} else {
		std_stdvector__cgs__4resize(self, (self->data_cap * 2));
	}
}
void std_stdvector__cgs__4push(struct std_stdvector__cgs__4* self, struct std_stdstring_view* value){
	size_t s = self->data_size;
	if((s >= self->data_cap)){
		std_stdvector__cgs__4ensure_capacity_for_one_more(self);
	}
	memcpy(&self->data_ptr[s], value, sizeof(struct std_stdstring_view));
	0;
	self->data_size = (s + 1);
}
void std_stdvector__cgs__4push_back(struct std_stdvector__cgs__4* self, struct std_stdstring_view* value){
	std_stdvector__cgs__4push(self, *({  &value; }));
}
void std_stdvector__cgs__4get(struct std_stdstring_view* __chx_struct_ret_param_xx, struct std_stdvector__cgs__4*const self, size_t index){
	*__chx_struct_ret_param_xx = self->data_ptr[index];
	return;
}
struct std_stdstring_view* std_stdvector__cgs__4get_ptr(struct std_stdvector__cgs__4*const self, size_t index){
	return &self->data_ptr[index];
}
struct std_stdstring_view* std_stdvector__cgs__4get_ref(struct std_stdvector__cgs__4*const self, size_t index){
	return &self->data_ptr[index];
}
void std_stdvector__cgs__4set(struct std_stdvector__cgs__4* self, size_t index, struct std_stdstring_view* value){
	self->data_ptr[index] = ({  *value; });
}
size_t std_stdvector__cgs__4size(struct std_stdvector__cgs__4*const self){
	return self->data_size;
}
size_t std_stdvector__cgs__4capacity(struct std_stdvector__cgs__4*const self){
	return self->data_cap;
}
const struct std_stdstring_view* std_stdvector__cgs__4data(struct std_stdvector__cgs__4*const self){
	return self->data_ptr;
}
struct std_stdstring_view* std_stdvector__cgs__4last_ptr(struct std_stdvector__cgs__4*const self){
	return (self->data_ptr + (self->data_size - 1));
}
void std_stdvector__cgs__4remove(struct std_stdvector__cgs__4* self, size_t index){
	size_t s = self->data_size;
	uint64_t last = (s - 1);
	
	if((index == last)){
		self->data_size = last;
	} else {
		memmove(&self->data_ptr[index], &self->data_ptr[(index + 1)], (sizeof(struct std_stdstring_view) * (last - index)));
		self->data_size = last;
	}
}
void std_stdvector__cgs__4erase(struct std_stdvector__cgs__4* self, size_t index){
	std_stdvector__cgs__4remove(self, index);
}
void std_stdvector__cgs__4remove_last(struct std_stdvector__cgs__4* self){
	size_t s = self->data_size;
	uint64_t last = (s - 1);
	struct std_stdstring_view* ptr = std_stdvector__cgs__4get_ptr(self, last);
	
	self->data_size = last;
}
void std_stdvector__cgs__4pop_back(struct std_stdvector__cgs__4* self){
	std_stdvector__cgs__4remove_last(self);
}
void std_stdvector__cgs__4take_last(struct std_stdstring_view* __chx_struct_ret_param_xx, struct std_stdvector__cgs__4* self){
	uint64_t last = (self->data_size - 1);
	self->data_size = last;
	*__chx_struct_ret_param_xx = *std_stdvector__cgs__4get_ptr(self, last);
	return;
}
_Bool std_stdvector__cgs__4empty(struct std_stdvector__cgs__4*const self){
	return (self->data_size == 0);
}
void std_stdvector__cgs__4clear(struct std_stdvector__cgs__4* self){
	
	self->data_size = 0;
}
void std_stdvector__cgs__4resize_unsafe(struct std_stdvector__cgs__4* self, size_t new_size){
	self->data_size = new_size;
}
void std_stdvector__cgs__4delete(struct std_stdvector__cgs__4* self){
	if((self->data_ptr != NULL)){
		
		free(self->data_ptr);
	}
	__chx__dstctr_clnup_blk__:{
	}
}
void std_stdvector__cgs__5make(struct std_stdvector__cgs__5* this){
	*this = (struct std_stdvector__cgs__5){ 
		.data_ptr = NULL, 
		.data_size = 0, 
		.data_cap = 0
	};
	return;
}
void std_stdvector__cgs__5make_with_capacity(struct std_stdvector__cgs__5* this, size_t init_cap){
	*this = (struct std_stdvector__cgs__5){ 
		.data_ptr = ((void***) malloc((sizeof(void**) * init_cap))), 
		.data_size = 0, 
		.data_cap = init_cap
	};
	return;
}
void std_stdvector__cgs__5resize(struct std_stdvector__cgs__5* self, size_t new_cap){
	void*** new_data = ((void***) realloc(self->data_ptr, (sizeof(void**) * new_cap)));
	
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
void std_stdvector__cgs__5reserve(struct std_stdvector__cgs__5* self, size_t cap){
	if((cap <= self->data_cap)){
		return;
	}
	std_stdvector__cgs__5resize(self, cap);
}
void std_stdvector__cgs__5ensure_capacity_for_one_more(struct std_stdvector__cgs__5* self){
	if((self->data_cap == 0)){
		std_stdvector__cgs__5resize(self, 2);
	} else {
		std_stdvector__cgs__5resize(self, (self->data_cap * 2));
	}
}
void std_stdvector__cgs__5push(struct std_stdvector__cgs__5* self, void** value){
	size_t s = self->data_size;
	if((s >= self->data_cap)){
		std_stdvector__cgs__5ensure_capacity_for_one_more(self);
	}
	memcpy(&self->data_ptr[s], &value, sizeof(void**));
	0;
	self->data_size = (s + 1);
}
void std_stdvector__cgs__5push_back(struct std_stdvector__cgs__5* self, void** value){
	std_stdvector__cgs__5push(self, value);
}
void** std_stdvector__cgs__5get(struct std_stdvector__cgs__5*const self, size_t index){
	return self->data_ptr[index];
}
void*** std_stdvector__cgs__5get_ptr(struct std_stdvector__cgs__5*const self, size_t index){
	return &self->data_ptr[index];
}
void*** std_stdvector__cgs__5get_ref(struct std_stdvector__cgs__5*const self, size_t index){
	return &self->data_ptr[index];
}
void std_stdvector__cgs__5set(struct std_stdvector__cgs__5* self, size_t index, void** value){
	self->data_ptr[index] = ({  value; });
}
size_t std_stdvector__cgs__5size(struct std_stdvector__cgs__5*const self){
	return self->data_size;
}
size_t std_stdvector__cgs__5capacity(struct std_stdvector__cgs__5*const self){
	return self->data_cap;
}
const void*** std_stdvector__cgs__5data(struct std_stdvector__cgs__5*const self){
	return self->data_ptr;
}
void*** std_stdvector__cgs__5last_ptr(struct std_stdvector__cgs__5*const self){
	return (self->data_ptr + (self->data_size - 1));
}
void std_stdvector__cgs__5remove(struct std_stdvector__cgs__5* self, size_t index){
	size_t s = self->data_size;
	uint64_t last = (s - 1);
	
	if((index == last)){
		self->data_size = last;
	} else {
		memmove(&self->data_ptr[index], &self->data_ptr[(index + 1)], (sizeof(void**) * (last - index)));
		self->data_size = last;
	}
}
void std_stdvector__cgs__5erase(struct std_stdvector__cgs__5* self, size_t index){
	std_stdvector__cgs__5remove(self, index);
}
void std_stdvector__cgs__5remove_last(struct std_stdvector__cgs__5* self){
	size_t s = self->data_size;
	uint64_t last = (s - 1);
	void*** ptr = std_stdvector__cgs__5get_ptr(self, last);
	
	self->data_size = last;
}
void std_stdvector__cgs__5pop_back(struct std_stdvector__cgs__5* self){
	std_stdvector__cgs__5remove_last(self);
}
void** std_stdvector__cgs__5take_last(struct std_stdvector__cgs__5* self){
	uint64_t last = (self->data_size - 1);
	self->data_size = last;
	return *std_stdvector__cgs__5get_ptr(self, last);
}
_Bool std_stdvector__cgs__5empty(struct std_stdvector__cgs__5*const self){
	return (self->data_size == 0);
}
void std_stdvector__cgs__5clear(struct std_stdvector__cgs__5* self){
	
	self->data_size = 0;
}
void std_stdvector__cgs__5resize_unsafe(struct std_stdvector__cgs__5* self, size_t new_size){
	self->data_size = new_size;
}
void std_stdvector__cgs__5delete(struct std_stdvector__cgs__5* self){
	if((self->data_ptr != NULL)){
		
		free(self->data_ptr);
	}
	__chx__dstctr_clnup_blk__:{
	}
}
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\win\condvar.ch **/
/** ExtImplement D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\std\win\mutex.ch **/
/** Translate D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\lab\src\AnnotationController.ch **/
/** Translate D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\lab\src\lab.ch **/
void** lab_BuildContextnew_module(void**const ctx, int type, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name, struct std_stdspan__cgs__1* dependencies){
	return lab_BuildContextnew_package(ctx, type, 0, scope_name, name, ({ struct std_stdspan__cgs__1 __chx__lv__0 = *dependencies; &__chx__lv__0; }));
}
void** lab_BuildContextnew_app_module(void**const ctx, int type, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name, struct std_stdspan__cgs__1* dependencies){
	return lab_BuildContextnew_package(ctx, type, 1, scope_name, name, ({ struct std_stdspan__cgs__1 __chx__lv__1 = *dependencies; &__chx__lv__1; }));
}
void** lab_BuildContextdirectory_module(void**const ctx, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name, struct std_stdspan__cgs__1* dependencies){
	return lab_BuildContextnew_package(ctx, 4, 0, scope_name, name, ({ struct std_stdspan__cgs__1 __chx__lv__2 = *dependencies; &__chx__lv__2; }));
}
static void lab_make_deps(struct std_stdvector__cgs__3* vec, struct std_stdspan__cgs__3* dependencies){
	std_stdvector__cgs__3reserve(vec, std_stdspan__cgs__3size(dependencies));
	const void*** start = std_stdspan__cgs__3data(dependencies);
	const void*** end = (start + std_stdspan__cgs__3size(dependencies));
	while((start != end)) {
		std_stdvector__cgs__3push(vec, &(struct lab_ModuleDependency){ 
			.info = NULL, 
			.module = ((void**) *start)
		});
		start++;
	}
}
void** lab_BuildContextnew_module_with_deps(void**const ctx, int type, int package_kind, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name, struct std_stdspan__cgs__3* dependencies){
	struct std_stdvector__cgs__3 __chx__lv__3;
	struct std_stdvector__cgs__3 vec = (*({ std_stdvector__cgs__3make(&__chx__lv__3); &__chx__lv__3; }));
	_Bool __chx__lv__4 = true;
	lab_make_deps(&vec, ({ struct std_stdspan__cgs__3 __chx__lv__5 = *dependencies; &__chx__lv__5; }));
	struct std_stdspan__cgs__1 __chx__lv__6;
	const void** __chx__lv__7 = lab_BuildContextnew_package(ctx, type, package_kind, scope_name, name, &(*({ std_stdspan__cgs__1constructor(&__chx__lv__6, std_stdvector__cgs__3data(&vec), std_stdvector__cgs__3size(&vec)); &__chx__lv__6; })));
	if(__chx__lv__4) {
		std_stdvector__cgs__3delete(&vec);
	}
	return __chx__lv__7;
}
void** lab_BuildContextc_file_module(void**const ctx, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name, struct std_stdstring_view*const path, struct std_stdspan__cgs__3* dependencies){
	struct std_stdvector__cgs__3 __chx__lv__8;
	struct std_stdvector__cgs__3 vec = (*({ std_stdvector__cgs__3make(&__chx__lv__8); &__chx__lv__8; }));
	_Bool __chx__lv__9 = true;
	lab_make_deps(&vec, ({ struct std_stdspan__cgs__3 __chx__lv__10 = *dependencies; &__chx__lv__10; }));
	struct std_stdspan__cgs__1 __chx__lv__11;
	void** mod = lab_BuildContextnew_package(ctx, 1, 0, scope_name, name, &(*({ std_stdspan__cgs__1constructor(&__chx__lv__11, std_stdvector__cgs__3data(&vec), std_stdvector__cgs__3size(&vec)); &__chx__lv__11; })));
	lab_BuildContextadd_path(ctx, mod, path);
	const void** __chx__lv__12 = mod;
	if(__chx__lv__9) {
		std_stdvector__cgs__3delete(&vec);
	}
	return __chx__lv__12;
}
void** lab_BuildContextcpp_file_module(void**const ctx, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name, struct std_stdstring_view*const path, struct std_stdspan__cgs__3* dependencies){
	struct std_stdvector__cgs__3 __chx__lv__13;
	struct std_stdvector__cgs__3 vec = (*({ std_stdvector__cgs__3make(&__chx__lv__13); &__chx__lv__13; }));
	_Bool __chx__lv__14 = true;
	lab_make_deps(&vec, ({ struct std_stdspan__cgs__3 __chx__lv__15 = *dependencies; &__chx__lv__15; }));
	struct std_stdspan__cgs__1 __chx__lv__16;
	void** mod = lab_BuildContextnew_package(ctx, 2, 0, scope_name, name, &(*({ std_stdspan__cgs__1constructor(&__chx__lv__16, std_stdvector__cgs__3data(&vec), std_stdvector__cgs__3size(&vec)); &__chx__lv__16; })));
	lab_BuildContextadd_path(ctx, mod, path);
	const void** __chx__lv__17 = mod;
	if(__chx__lv__14) {
		std_stdvector__cgs__3delete(&vec);
	}
	return __chx__lv__17;
}
void** lab_BuildContextobject_module(void**const ctx, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name, struct std_stdstring_view*const path){
	struct std_stdspan__cgs__1 __chx__lv__18;
	void** mod = lab_BuildContextnew_package(ctx, 3, 0, scope_name, name, &(*({ std_stdspan__cgs__1empty_make(&__chx__lv__18); &__chx__lv__18; })));
	lab_BuildContextadd_path(ctx, mod, path);
	return mod;
}
void** lab_BuildContextchemical_dir_module(void**const ctx, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name, struct std_stdstring_view*const path, struct std_stdspan__cgs__3* dependencies){
	void** mod = lab_BuildContextnew_module_with_deps(ctx, 4, 0, scope_name, name, ({ struct std_stdspan__cgs__3 __chx__lv__19 = *dependencies; &__chx__lv__19; }));
	lab_BuildContextadd_path(ctx, mod, path);
	return mod;
}
void** lab_BuildContextdirectory_app_module(void**const ctx, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name, struct std_stdstring_view*const path, struct std_stdspan__cgs__3* dependencies){
	void** mod = lab_BuildContextnew_module_with_deps(ctx, 4, 1, scope_name, name, ({ struct std_stdspan__cgs__3 __chx__lv__20 = *dependencies; &__chx__lv__20; }));
	lab_BuildContextadd_path(ctx, mod, path);
	return mod;
}
void lab_ImportSymbolmake(struct lab_ImportSymbol* this){
	std_stdspan__cgs__2empty_make(&this->parts);
	std_stdstring_viewempty_make(&this->alias);
}
void lab_BuildContextadd_compiler_interfaces(void**const ctx, void** mod, struct std_stdspan__cgs__2* interfaces){
	if(!std_stdspan__cgs__2empty(interfaces)){
		unsigned int i = 0;
		size_t s = std_stdspan__cgs__2size(interfaces);
		while((i < s)) {
			const struct std_stdstring_view* ci = std_stdspan__cgs__2get(interfaces, i);
			lab_BuildContextadd_compiler_interface(ctx, mod, &*ci);
			i++;
		}
	}
}
void** lab_BuildContextcreate_module(void**const ctx, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name, struct std_stdstring_view*const dir_path, struct std_stdspan__cgs__3* dependencies, struct std_stdspan__cgs__2* interfaces){
	void** mod = lab_BuildContextchemical_dir_module(ctx, scope_name, name, dir_path, ({ struct std_stdspan__cgs__3 __chx__lv__21 = *dependencies; &__chx__lv__21; }));
	lab_BuildContextadd_compiler_interfaces(ctx, mod, ({ struct std_stdspan__cgs__2 __chx__lv__22 = *interfaces; &__chx__lv__22; }));
	return mod;
}
void** lab_BuildContextdefault_get(void**const ctx, _Bool* buildFlag, void*** cached, void**(*build)(const void** ctx)){
	void** c = *cached;
	if(((c == NULL) && (*buildFlag == 1))){
		void** built = build(ctx);
		*cached = built;
		*buildFlag = 0;
		return built;
	} else {
		return c;
	}
}
void** lab_BuildContextfile_module(void**const ctx, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name, struct std_stdstring_view*const path, struct std_stdspan__cgs__3* dependencies){
	if(std_stdstring_viewends_with(path, &(*({ struct std_stdstring_view __chx__lv__23; std_stdstring_viewconstructor(&__chx__lv__23, ".c", 2); &__chx__lv__23; })))){
		void** mod = lab_BuildContextnew_module_with_deps(ctx, 1, 0, scope_name, name, ({ struct std_stdspan__cgs__3 __chx__lv__24 = *dependencies; &__chx__lv__24; }));
		lab_BuildContextadd_path(ctx, mod, path);
		return mod;
	}else if(std_stdstring_viewends_with(path, &(*({ struct std_stdstring_view __chx__lv__25; std_stdstring_viewconstructor(&__chx__lv__25, ".cpp", 4); &__chx__lv__25; })))){
		void** mod = lab_BuildContextnew_module_with_deps(ctx, 2, 0, scope_name, name, ({ struct std_stdspan__cgs__3 __chx__lv__26 = *dependencies; &__chx__lv__26; }));
		lab_BuildContextadd_path(ctx, mod, path);
		return mod;
	}else if(std_stdstring_viewends_with(path, &(*({ struct std_stdstring_view __chx__lv__27; std_stdstring_viewconstructor(&__chx__lv__27, ".o", 2); &__chx__lv__27; })))){
		void** mod = lab_BuildContextnew_module_with_deps(ctx, 3, 0, scope_name, name, ({ struct std_stdspan__cgs__3 __chx__lv__28 = *dependencies; &__chx__lv__28; }));
		lab_BuildContextadd_path(ctx, mod, path);
		return mod;
	} else {
		void** mod = lab_BuildContextnew_module_with_deps(ctx, 4, 0, scope_name, name, ({ struct std_stdspan__cgs__3 __chx__lv__29 = *dependencies; &__chx__lv__29; }));
		lab_BuildContextadd_path(ctx, mod, path);
		return mod;
	}
}
void** lab_BuildContexttranslate_file_to_chemical(void**const ctx, struct std_stdstring_view*const c_path, struct std_stdstring_view*const output_path){
	void** deps[] = {};
	void** mod = lab_BuildContextfile_module(ctx, &(*({ struct std_stdstring_view __chx__lv__30; std_stdstring_viewconstructor(&__chx__lv__30, "", 0); &__chx__lv__30; })), &(*({ struct std_stdstring_view __chx__lv__31; std_stdstring_viewconstructor(&__chx__lv__31, "CFile", 5); &__chx__lv__31; })), c_path, &(*({ struct std_stdspan__cgs__3 __chx__lv__32; std_stdspan__cgs__3constructor(&__chx__lv__32, (void**[]){}, 0); &__chx__lv__32; })));
	return lab_BuildContexttranslate_to_chemical(ctx, mod, output_path);
}
void lab_BuildContextinclude_headers(void**const ctx, void** module, struct std_stdspan__cgs__2* headers){
	int i = 0;
	size_t total = std_stdspan__cgs__2size(headers);
	while((i < total)) {
		const struct std_stdstring_view* ele = std_stdspan__cgs__2get(headers, ((size_t) i));
		lab_BuildContextinclude_header(ctx, module, &*ele);
		i++;
	}
}
void lab_BuildContextindex_def_cbi_fn(void**const ctx, struct lab_LabJobCBI* job, struct std_stdstring_view*const name, int type){
	struct std_stdstring_view __chx__lv__33;
	lab_BuildContextindex_cbi_fn(ctx, job, &(*({ lab_LabJobgetName(&__chx__lv__33, job); &__chx__lv__33; })), name, type);
}
void lab_BuildContextbuild_job_dir_path(struct std_stdstring* __chx_struct_ret_param_xx, void**const ctx, struct std_stdstring_view*const job_name){
	struct std_stdstring_view __chx__lv__34;
	struct std_stdstring __chx__lv__35;
	struct std_stdstring new_path = (*({ std_stdstringview_make(&__chx__lv__35, &(*({ lab_BuildContextbuild_path(&__chx__lv__34, ctx); &__chx__lv__34; }))); &__chx__lv__35; }));
	_Bool __chx__lv__36 = true;
	std_stdstringappend(&new_path, '/');
	std_stdstringappend_view(&new_path, job_name);
	std_stdstringappend_view(&new_path, &(*({ struct std_stdstring_view __chx__lv__37; std_stdstring_viewconstructor(&__chx__lv__37, ".dir", 4); &__chx__lv__37; })));
	std_stdstringappend(&new_path, '/');
	*__chx_struct_ret_param_xx = new_path;
	return;
}
void lab_BuildContextjob_dir_path(struct std_stdstring* __chx_struct_ret_param_xx, void**const ctx, void** job){
	struct std_stdstring_view __chx__lv__38;
	struct std_stdstring __chx__lv__39;
	*__chx_struct_ret_param_xx = (*({ lab_BuildContextbuild_job_dir_path(&__chx__lv__39, ctx, &(*({ lab_LabJobgetName(&__chx__lv__38, job); &__chx__lv__38; }))); &__chx__lv__39; }));
	return;
}
void lab_BuildContextbuild_mod_file_path(struct std_stdstring* __chx_struct_ret_param_xx, void**const ctx, struct std_stdstring_view*const job_name, struct std_stdstring_view*const mod_scope, struct std_stdstring_view*const mod_name, struct std_stdstring_view*const file){
	struct std_stdstring __chx__lv__40;
	struct std_stdstring str = (*({ lab_BuildContextbuild_job_dir_path(&__chx__lv__40, ctx, job_name); &__chx__lv__40; }));
	_Bool __chx__lv__41 = true;
	std_stdstringappend_view(&str, &(*({ struct std_stdstring_view __chx__lv__42; std_stdstring_viewconstructor(&__chx__lv__42, "modules/", 8); &__chx__lv__42; })));
	if(!std_stdstring_viewempty(mod_scope)){
		std_stdstringappend_view(&str, mod_scope);
		std_stdstringappend(&str, '.');
	}
	std_stdstringappend_view(&str, mod_name);
	std_stdstringappend(&str, '/');
	std_stdstringappend_view(&str, file);
	*__chx_struct_ret_param_xx = str;
	return;
}
void lab_BuildContextbuild_llvm_ir_path(struct std_stdstring* __chx_struct_ret_param_xx, void**const ctx, struct std_stdstring_view*const job_name, struct std_stdstring_view*const mod_scope, struct std_stdstring_view*const mod_name){
	struct std_stdstring __chx__lv__43;
	*__chx_struct_ret_param_xx = (*({ lab_BuildContextbuild_mod_file_path(&__chx__lv__43, ctx, job_name, mod_scope, mod_name, &(*({ struct std_stdstring_view __chx__lv__44; std_stdstring_viewconstructor(&__chx__lv__44, "llvm_ir.ll", 10); &__chx__lv__44; }))); &__chx__lv__43; }));
	return;
}
void lab_BuildContextbuild_asm_path(struct std_stdstring* __chx_struct_ret_param_xx, void**const ctx, struct std_stdstring_view*const job_name, struct std_stdstring_view*const mod_scope, struct std_stdstring_view*const mod_name){
	struct std_stdstring __chx__lv__45;
	*__chx_struct_ret_param_xx = (*({ lab_BuildContextbuild_mod_file_path(&__chx__lv__45, ctx, job_name, mod_scope, mod_name, &(*({ struct std_stdstring_view __chx__lv__46; std_stdstring_viewconstructor(&__chx__lv__46, "mod_asm.s", 9); &__chx__lv__46; }))); &__chx__lv__45; }));
	return;
}
void lab_BuildContextbuild_bitcode_path(struct std_stdstring* __chx_struct_ret_param_xx, void**const ctx, struct std_stdstring_view*const job_name, struct std_stdstring_view*const mod_scope, struct std_stdstring_view*const mod_name){
	struct std_stdstring __chx__lv__47;
	*__chx_struct_ret_param_xx = (*({ lab_BuildContextbuild_mod_file_path(&__chx__lv__47, ctx, job_name, mod_scope, mod_name, &(*({ struct std_stdstring_view __chx__lv__48; std_stdstring_viewconstructor(&__chx__lv__48, "mod_bitcode.bc", 14); &__chx__lv__48; }))); &__chx__lv__47; }));
	return;
}
void lab_BuildContextllvm_ir_path(struct std_stdstring* __chx_struct_ret_param_xx, void**const ctx, void** job, void** mod){
	struct std_stdstring_view __chx__lv__49;
	struct std_stdstring_view __chx__lv__50;
	struct std_stdstring_view __chx__lv__51;
	struct std_stdstring __chx__lv__52;
	*__chx_struct_ret_param_xx = (*({ lab_BuildContextbuild_llvm_ir_path(&__chx__lv__52, ctx, &(*({ lab_LabJobgetName(&__chx__lv__49, job); &__chx__lv__49; })), &(*({ lab_ModulegetScopeName(&__chx__lv__50, mod); &__chx__lv__50; })), &(*({ lab_ModulegetName(&__chx__lv__51, mod); &__chx__lv__51; }))); &__chx__lv__52; }));
	return;
}
void lab_BuildContextasm_path(struct std_stdstring* __chx_struct_ret_param_xx, void**const ctx, void** job, void** mod){
	struct std_stdstring_view __chx__lv__53;
	struct std_stdstring_view __chx__lv__54;
	struct std_stdstring_view __chx__lv__55;
	struct std_stdstring __chx__lv__56;
	*__chx_struct_ret_param_xx = (*({ lab_BuildContextbuild_asm_path(&__chx__lv__56, ctx, &(*({ lab_LabJobgetName(&__chx__lv__53, job); &__chx__lv__53; })), &(*({ lab_ModulegetScopeName(&__chx__lv__54, mod); &__chx__lv__54; })), &(*({ lab_ModulegetName(&__chx__lv__55, mod); &__chx__lv__55; }))); &__chx__lv__56; }));
	return;
}
void lab_BuildContextbitcode_path(struct std_stdstring* __chx_struct_ret_param_xx, void**const ctx, void** job, void** mod){
	struct std_stdstring_view __chx__lv__57;
	struct std_stdstring_view __chx__lv__58;
	struct std_stdstring_view __chx__lv__59;
	struct std_stdstring __chx__lv__60;
	*__chx_struct_ret_param_xx = (*({ lab_BuildContextbuild_bitcode_path(&__chx__lv__60, ctx, &(*({ lab_LabJobgetName(&__chx__lv__57, job); &__chx__lv__57; })), &(*({ lab_ModulegetScopeName(&__chx__lv__58, mod); &__chx__lv__58; })), &(*({ lab_ModulegetName(&__chx__lv__59, mod); &__chx__lv__59; }))); &__chx__lv__60; }));
	return;
}
void lab_labcurr_dir_of(struct std_stdstring* __chx_struct_ret_param_xx, const char* path, size_t len){
	struct std_stdstring_view __chx__lv__61;
	struct std_stdstring __chx__lv__62;
	*__chx_struct_ret_param_xx = (*({ std_fsparent_path(&__chx__lv__62, &(*({ std_stdstring_viewconstructor(&__chx__lv__61, path, len); &__chx__lv__61; }))); &__chx__lv__62; }));
	return;
}
void lab_labappended_str(struct std_stdstring* __chx_struct_ret_param_xx, struct std_stdstring* str, const char* path){
	_Bool __chx__lv__63 = true;
	std_stdstringappend_char_ptr(str, path);
	*__chx_struct_ret_param_xx = *str;
	return;
}
/** Translate D:\Programming\Cpp\zig-bootstrap\chemical\lang\libs\lab\src\target_data.ch **/
void lab_TargetDatamake(struct lab_TargetData* this){
	this->tcc = 0;
	this->clang = 0;
	this->cbi = 0;
	this->lsp = 0;
	this->test = 0;
	this->debug = 0;
	this->debug_quick = 0;
	this->debug_complete = 0;
	this->release = 0;
	this->release_safe = 0;
	this->release_small = 0;
	this->release_fast = 0;
	this->posix = 0;
	this->gnu = 0;
	this->is64Bit = 0;
	this->little_endian = 0;
	this->big_endian = 0;
	this->windows = 0;
	this->win32 = 0;
	this->win64 = 0;
	this->linux = 0;
	this->macos = 0;
	this->freebsd = 0;
	this->isUnix = 0;
	this->android = 0;
	this->cygwin = 0;
	this->mingw32 = 0;
	this->mingw64 = 0;
	this->emscripten = 0;
	this->x86_64 = 0;
	this->x86 = 0;
	this->i386 = 0;
	this->arm = 0;
	this->aarch64 = 0;
	this->powerpc = 0;
	this->powerpc64 = 0;
	this->riscv = 0;
	this->s390x = 0;
	this->wasm32 = 0;
	this->wasm64 = 0;
}