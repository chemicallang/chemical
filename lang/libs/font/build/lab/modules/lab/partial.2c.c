
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/lab/src/lab.ch **/
struct lab_ModuleChecksOptions;
struct lab_ModuleOptions;
struct lab_DependencySymbolInfo;
struct lab_ModuleDependency;
struct lab_ImportSymbol;
struct lab_ImportRepo;
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/lab/src/target_data.ch **/
struct lab_TargetData;
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/lab/src/AnnotationController.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/lab/src/lab.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/lab/src/target_data.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/lab/src/AnnotationController.ch **/
/** FwdDeclare:Generics lab **/
struct std_stdspan__cgs__1;
struct std_stdvector__cgs__4;
struct std_stdspan__cgs__2;
struct std_stdvector__cgs__5;
struct std_stdspan__cgs__3;
struct std_stdspan__cgs__4;
struct std_stdvector__cgs__6;
/** Declare:Generics lab **/
struct std_stdspan__cgs__1 {
	const struct lab_ImportSymbol* _data;
	size_t _size;
};;;
void std_stdspan__cgs__1constructor(struct std_stdspan__cgs__1* this, const struct lab_ImportSymbol* array_ptr, size_t array_size);
void std_stdspan__cgs__1empty_make(struct std_stdspan__cgs__1* this);
const struct lab_ImportSymbol* std_stdspan__cgs__1data(struct std_stdspan__cgs__1*const self);
const struct lab_ImportSymbol* std_stdspan__cgs__1get(struct std_stdspan__cgs__1*const self, size_t loc);
size_t std_stdspan__cgs__1size(struct std_stdspan__cgs__1*const self);
_Bool std_stdspan__cgs__1empty(struct std_stdspan__cgs__1*const self);
struct std_stdvector__cgs__4 {
	struct lab_ImportSymbol* data_ptr;
	size_t data_size;
	size_t data_cap;
};
void std_stdvector__cgs__4make(struct std_stdvector__cgs__4* this);
void std_stdvector__cgs__4make_with_capacity(struct std_stdvector__cgs__4* this, size_t init_cap);
void std_stdvector__cgs__4resize(struct std_stdvector__cgs__4* self, size_t new_size);
void std_stdvector__cgs__4reserve(struct std_stdvector__cgs__4* self, size_t new_cap);
void std_stdvector__cgs__4ensure_capacity_for_one_more(struct std_stdvector__cgs__4* self);
void std_stdvector__cgs__4push(struct std_stdvector__cgs__4* self, struct lab_ImportSymbol* value);
void std_stdvector__cgs__4push_back(struct std_stdvector__cgs__4* self, struct lab_ImportSymbol* value);
void std_stdvector__cgs__4get(struct lab_ImportSymbol* __chx_struct_ret_param_xx, struct std_stdvector__cgs__4*const self, size_t index);
struct lab_ImportSymbol* std_stdvector__cgs__4get_ptr(struct std_stdvector__cgs__4*const self, size_t index);
struct lab_ImportSymbol* std_stdvector__cgs__4get_ref(struct std_stdvector__cgs__4*const self, size_t index);
void std_stdvector__cgs__4set(struct std_stdvector__cgs__4* self, size_t index, struct lab_ImportSymbol* value);
size_t std_stdvector__cgs__4size(struct std_stdvector__cgs__4*const self);
size_t std_stdvector__cgs__4capacity(struct std_stdvector__cgs__4*const self);
const struct lab_ImportSymbol* std_stdvector__cgs__4data(struct std_stdvector__cgs__4*const self);
struct lab_ImportSymbol* std_stdvector__cgs__4last_ptr(struct std_stdvector__cgs__4*const self);
void std_stdvector__cgs__4remove(struct std_stdvector__cgs__4* self, size_t index);
void std_stdvector__cgs__4erase(struct std_stdvector__cgs__4* self, size_t index);
void std_stdvector__cgs__4remove_last(struct std_stdvector__cgs__4* self);
void std_stdvector__cgs__4pop_back(struct std_stdvector__cgs__4* self);
void std_stdvector__cgs__4take_last(struct lab_ImportSymbol* __chx_struct_ret_param_xx, struct std_stdvector__cgs__4* self);
_Bool std_stdvector__cgs__4empty(struct std_stdvector__cgs__4*const self);
void std_stdvector__cgs__4clear(struct std_stdvector__cgs__4* self);
void std_stdvector__cgs__4set_len(struct std_stdvector__cgs__4* self, size_t new_size);
void std_stdvector__cgs__4delete(struct std_stdvector__cgs__4* self);
typedef struct __chx_core_coreiterableLinear__cgs__4_vt_t {
	const struct lab_ImportSymbol*(*data)(void* self);uint64_t(*size)(void* self);
} __chx_core_coreiterableLinear__cgs__4_vt_t;
struct std_stdspan__cgs__2 {
	const struct lab_ModuleDependency* _data;
	size_t _size;
};;;
void std_stdspan__cgs__2constructor(struct std_stdspan__cgs__2* this, const struct lab_ModuleDependency* array_ptr, size_t array_size);
void std_stdspan__cgs__2empty_make(struct std_stdspan__cgs__2* this);
const struct lab_ModuleDependency* std_stdspan__cgs__2data(struct std_stdspan__cgs__2*const self);
const struct lab_ModuleDependency* std_stdspan__cgs__2get(struct std_stdspan__cgs__2*const self, size_t loc);
size_t std_stdspan__cgs__2size(struct std_stdspan__cgs__2*const self);
_Bool std_stdspan__cgs__2empty(struct std_stdspan__cgs__2*const self);
struct std_stdvector__cgs__5 {
	struct lab_ModuleDependency* data_ptr;
	size_t data_size;
	size_t data_cap;
};
void std_stdvector__cgs__5make(struct std_stdvector__cgs__5* this);
void std_stdvector__cgs__5make_with_capacity(struct std_stdvector__cgs__5* this, size_t init_cap);
void std_stdvector__cgs__5resize(struct std_stdvector__cgs__5* self, size_t new_size);
void std_stdvector__cgs__5reserve(struct std_stdvector__cgs__5* self, size_t new_cap);
void std_stdvector__cgs__5ensure_capacity_for_one_more(struct std_stdvector__cgs__5* self);
void std_stdvector__cgs__5push(struct std_stdvector__cgs__5* self, struct lab_ModuleDependency* value);
void std_stdvector__cgs__5push_back(struct std_stdvector__cgs__5* self, struct lab_ModuleDependency* value);
void std_stdvector__cgs__5get(struct lab_ModuleDependency* __chx_struct_ret_param_xx, struct std_stdvector__cgs__5*const self, size_t index);
struct lab_ModuleDependency* std_stdvector__cgs__5get_ptr(struct std_stdvector__cgs__5*const self, size_t index);
struct lab_ModuleDependency* std_stdvector__cgs__5get_ref(struct std_stdvector__cgs__5*const self, size_t index);
void std_stdvector__cgs__5set(struct std_stdvector__cgs__5* self, size_t index, struct lab_ModuleDependency* value);
size_t std_stdvector__cgs__5size(struct std_stdvector__cgs__5*const self);
size_t std_stdvector__cgs__5capacity(struct std_stdvector__cgs__5*const self);
const struct lab_ModuleDependency* std_stdvector__cgs__5data(struct std_stdvector__cgs__5*const self);
struct lab_ModuleDependency* std_stdvector__cgs__5last_ptr(struct std_stdvector__cgs__5*const self);
void std_stdvector__cgs__5remove(struct std_stdvector__cgs__5* self, size_t index);
void std_stdvector__cgs__5erase(struct std_stdvector__cgs__5* self, size_t index);
void std_stdvector__cgs__5remove_last(struct std_stdvector__cgs__5* self);
void std_stdvector__cgs__5pop_back(struct std_stdvector__cgs__5* self);
void std_stdvector__cgs__5take_last(struct lab_ModuleDependency* __chx_struct_ret_param_xx, struct std_stdvector__cgs__5* self);
_Bool std_stdvector__cgs__5empty(struct std_stdvector__cgs__5*const self);
void std_stdvector__cgs__5clear(struct std_stdvector__cgs__5* self);
void std_stdvector__cgs__5set_len(struct std_stdvector__cgs__5* self, size_t new_size);
void std_stdvector__cgs__5delete(struct std_stdvector__cgs__5* self);
typedef struct __chx_core_coreiterableLinear__cgs__5_vt_t {
	const struct lab_ModuleDependency*(*data)(void* self);uint64_t(*size)(void* self);
} __chx_core_coreiterableLinear__cgs__5_vt_t;
struct std_stdspan__cgs__3 {
	const struct std_stdstring_view* _data;
	size_t _size;
};;;
void std_stdspan__cgs__3constructor(struct std_stdspan__cgs__3* this, const struct std_stdstring_view* array_ptr, size_t array_size);
void std_stdspan__cgs__3empty_make(struct std_stdspan__cgs__3* this);
const struct std_stdstring_view* std_stdspan__cgs__3data(struct std_stdspan__cgs__3*const self);
const struct std_stdstring_view* std_stdspan__cgs__3get(struct std_stdspan__cgs__3*const self, size_t loc);
size_t std_stdspan__cgs__3size(struct std_stdspan__cgs__3*const self);
_Bool std_stdspan__cgs__3empty(struct std_stdspan__cgs__3*const self);
struct std_stdspan__cgs__4 {
	const void*** _data;
	size_t _size;
};;;
void std_stdspan__cgs__4constructor(struct std_stdspan__cgs__4* this, const void*** array_ptr, size_t array_size);
void std_stdspan__cgs__4empty_make(struct std_stdspan__cgs__4* this);
const void*** std_stdspan__cgs__4data(struct std_stdspan__cgs__4*const self);
const void*** std_stdspan__cgs__4get(struct std_stdspan__cgs__4*const self, size_t loc);
size_t std_stdspan__cgs__4size(struct std_stdspan__cgs__4*const self);
_Bool std_stdspan__cgs__4empty(struct std_stdspan__cgs__4*const self);
struct std_stdvector__cgs__6 {
	void*** data_ptr;
	size_t data_size;
	size_t data_cap;
};
void std_stdvector__cgs__6make(struct std_stdvector__cgs__6* this);
void std_stdvector__cgs__6make_with_capacity(struct std_stdvector__cgs__6* this, size_t init_cap);
void std_stdvector__cgs__6resize(struct std_stdvector__cgs__6* self, size_t new_size);
void std_stdvector__cgs__6reserve(struct std_stdvector__cgs__6* self, size_t new_cap);
void std_stdvector__cgs__6ensure_capacity_for_one_more(struct std_stdvector__cgs__6* self);
void std_stdvector__cgs__6push(struct std_stdvector__cgs__6* self, void** value);
void std_stdvector__cgs__6push_back(struct std_stdvector__cgs__6* self, void** value);
void** std_stdvector__cgs__6get(struct std_stdvector__cgs__6*const self, size_t index);
void*** std_stdvector__cgs__6get_ptr(struct std_stdvector__cgs__6*const self, size_t index);
void*** std_stdvector__cgs__6get_ref(struct std_stdvector__cgs__6*const self, size_t index);
void std_stdvector__cgs__6set(struct std_stdvector__cgs__6* self, size_t index, void** value);
size_t std_stdvector__cgs__6size(struct std_stdvector__cgs__6*const self);
size_t std_stdvector__cgs__6capacity(struct std_stdvector__cgs__6*const self);
const void*** std_stdvector__cgs__6data(struct std_stdvector__cgs__6*const self);
void*** std_stdvector__cgs__6last_ptr(struct std_stdvector__cgs__6*const self);
void std_stdvector__cgs__6remove(struct std_stdvector__cgs__6* self, size_t index);
void std_stdvector__cgs__6erase(struct std_stdvector__cgs__6* self, size_t index);
void std_stdvector__cgs__6remove_last(struct std_stdvector__cgs__6* self);
void std_stdvector__cgs__6pop_back(struct std_stdvector__cgs__6* self);
void** std_stdvector__cgs__6take_last(struct std_stdvector__cgs__6* self);
_Bool std_stdvector__cgs__6empty(struct std_stdvector__cgs__6*const self);
void std_stdvector__cgs__6clear(struct std_stdvector__cgs__6* self);
void std_stdvector__cgs__6set_len(struct std_stdvector__cgs__6* self, size_t new_size);
void std_stdvector__cgs__6delete(struct std_stdvector__cgs__6* self);
typedef struct __chx_core_coreiterableLinear__cgs__6_vt_t {
	const void***(*data)(void* self);uint64_t(*size)(void* self);
} __chx_core_coreiterableLinear__cgs__6_vt_t;
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/lab/src/lab.ch **/
struct lab_ModuleChecksOptions {
	_Bool bounds;
	_Bool overflow;
	_Bool null;
};
void lab_ModuleChecksOptionsmake(struct lab_ModuleChecksOptions* this);
struct lab_ModuleOptions {
	_Bool safety;
	struct lab_ModuleChecksOptions checks;
	struct std_stdstring_view safe_mode;
	struct std_stdstring_view stack_protector;
	int optimization_level;
};
void lab_ModuleOptionsmake(struct lab_ModuleOptions* this);
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
struct lab_ModuleOptions* lab_ModulegetOptions(void**const self);
int lab_LabJobgetType(void**const self);
void lab_LabJobgetName(struct std_stdstring_view* __chx_struct_ret_param_xx, void**const self);
void lab_LabJobgetAbsPath(struct std_stdstring_view* __chx_struct_ret_param_xx, void**const self);
void lab_LabJobgetBuildDir(struct std_stdstring_view* __chx_struct_ret_param_xx, void**const self);
int lab_LabJobgetStatus(void**const self);
void lab_LabJobgetTargetTriple(struct std_stdstring_view* __chx_struct_ret_param_xx, void**const self);
int lab_LabJobgetMode(void**const self);
struct lab_TargetData*const lab_LabJobgetTarget(void**const self);
void lab_LabJobsetAbsPath(void**const self, struct std_stdstring_view* path);
struct lab_DependencySymbolInfo {
	struct std_stdspan__cgs__1 symbols;
	struct std_stdstring_view alias;
	uint64_t location;
};
struct lab_ModuleDependency {
	void** module;
	struct lab_DependencySymbolInfo* info;
};
void** lab_BuildContextgetAnnotationController(void**const self);
void** lab_BuildContextnew_package(void**const self, int type, int package_kind, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name, struct std_stdspan__cgs__2* dependencies);
void lab_BuildContextset_module_symbol_info(void**const self, void** module, unsigned int index, struct lab_DependencySymbolInfo*const info);
void** lab_BuildContextget_cached(void**const self, const void** job, struct std_stdstring_view*const path);
void lab_BuildContextset_cached(void**const self, const void** job, struct std_stdstring_view*const path, void** module);
void lab_BuildContextadd_path(void**const self, void** module, struct std_stdstring_view*const path);
void lab_BuildContextadd_dependency(void**const self, void** job, void** module, struct lab_DependencySymbolInfo* info);
void lab_BuildContextadd_module(void**const self, void** job, void** module);
void lab_BuildContextput_job_before(void**const self, void** newJob, void** existingJob);
void lab_BuildContextlink_system_lib(void**const self, void** job, struct std_stdstring_view*const name, void** module);
void lab_BuildContextship_file(void**const self, void** job, struct std_stdstring_view*const path);
void lab_BuildContextadd_lib_search_path(void**const self, void** job, struct std_stdstring_view*const path, void** module);
_Bool lab_BuildContextadd_compiler_interface(void**const self, void** module, struct std_stdstring_view*const interface);
_Bool lab_BuildContextresolve_condition(void**const self, const void** job, struct std_stdstring_view*const condition);
void lab_BuildContextinclude_header(void**const self, void** module, struct std_stdstring_view*const header);
void** lab_BuildContexttranslate_to_chemical(void**const self, void** module, struct std_stdstring_view*const output_path);
void** lab_BuildContexttranslate_to_c(void**const self, struct std_stdstring_view*const name, struct std_stdstring_view*const output_path);
void** lab_BuildContextbuild_exe(void**const self, struct std_stdstring_view*const name);
void** lab_BuildContextrun_jit_exe(void**const self, struct std_stdstring_view*const name);
void** lab_BuildContextbuild_dynamic_lib(void**const self, struct std_stdstring_view*const name);
void** lab_BuildContextbuild_interpretation(void**const self, struct std_stdstring_view*const name);
void** lab_BuildContextbuild_cbi(void**const self, struct std_stdstring_view*const name);
void lab_BuildContextset_environment_testing(void**const self, void** job, _Bool value);
_Bool lab_BuildContextcontains_cbi(void**const self, struct std_stdstring_view*const key);
void lab_BuildContextset_contains_cbi(void**const self, struct std_stdstring_view*const key);
_Bool lab_BuildContextindex_cbi_fn(void**const self, void** job, struct std_stdstring_view*const key, struct std_stdstring_view*const fn_name, int fn_type);
void lab_BuildContextadd_object(void**const self, const void** job, struct std_stdstring_view*const path);
void lab_BuildContextbuild_path(struct std_stdstring_view* __chx_struct_ret_param_xx, void**const self);
_Bool lab_BuildContexthas_arg(void**const self, struct std_stdstring_view*const name);
void lab_BuildContextget_arg(struct std_stdstring_view* __chx_struct_ret_param_xx, void**const self, struct std_stdstring_view*const name);
void lab_BuildContextremove_arg(void**const self, struct std_stdstring_view*const name);
_Bool lab_BuildContextdefine(void**const self, const void** job, struct std_stdstring_view*const name);
_Bool lab_BuildContextundefine(void**const self, const void** job, struct std_stdstring_view*const name);
int lab_BuildContextinvoke_dlltool(void**const self, struct std_stdspan__cgs__3* string_arr);
int lab_BuildContextinvoke_ranlib(void**const self, struct std_stdspan__cgs__3* string_arr);
int lab_BuildContextinvoke_lib(void**const self, struct std_stdspan__cgs__3* string_arr);
int lab_BuildContextinvoke_ar(void**const self, struct std_stdspan__cgs__3* string_arr);
void lab_BuildContextset_conflict_resolution_strategy(void**const self, void** job, int strategy);
_Bool lab_BuildContextfetch_job_dependency(void**const self, void** job, struct lab_ImportRepo*const dep, int strategy);
_Bool lab_BuildContextfetch_mod_dependency(void**const self, void** job, void** mod, struct lab_ImportRepo*const dep, int strategy);
void lab_BuildContextadd_include_dir(void**const self, void** module, struct std_stdstring_view*const path);
void** lab_BuildContextnew_module(void**const ctx, int type, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name, struct std_stdspan__cgs__2* dependencies);
void** lab_BuildContextnew_app_module(void**const ctx, int type, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name, struct std_stdspan__cgs__2* dependencies);
void** lab_BuildContextdirectory_module(void**const ctx, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name, struct std_stdspan__cgs__2* dependencies);
static void lab_make_deps(struct std_stdvector__cgs__5* vec, struct std_stdspan__cgs__4* dependencies);
void** lab_BuildContextnew_module_with_deps(void**const ctx, int type, int package_kind, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name, struct std_stdspan__cgs__4* dependencies);
void** lab_BuildContextc_file_module(void**const ctx, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name, struct std_stdstring_view*const path, struct std_stdspan__cgs__4* dependencies);
void** lab_BuildContextcpp_file_module(void**const ctx, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name, struct std_stdstring_view*const path, struct std_stdspan__cgs__4* dependencies);
void** lab_BuildContextobject_module(void**const ctx, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name, struct std_stdstring_view*const path);
void** lab_BuildContextempty_module(void**const ctx, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name);
void** lab_BuildContextchemical_dir_module(void**const ctx, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name, struct std_stdstring_view*const path, struct std_stdspan__cgs__4* dependencies);
void** lab_BuildContextdirectory_app_module(void**const ctx, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name, struct std_stdstring_view*const path, struct std_stdspan__cgs__4* dependencies);
struct lab_ImportSymbol {
	struct std_stdspan__cgs__3 parts;
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
	_Bool orphan_branch;
	struct std_stdstring_view commit;
	struct std_stdstring_view alias;
	struct std_stdspan__cgs__1 symbols;
	uint64_t location;
};
void lab_BuildContextadd_compiler_interfaces(void**const ctx, void** mod, struct std_stdspan__cgs__3* interfaces);
void** lab_BuildContextcreate_module(void**const ctx, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name, struct std_stdstring_view*const dir_path, struct std_stdspan__cgs__4* dependencies, struct std_stdspan__cgs__3* interfaces);
void** lab_BuildContextdefault_get(void**const ctx, _Bool* buildFlag, void*** cached, void**(*build)(const void** ctx));
void** lab_BuildContextfile_module(void**const ctx, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name, struct std_stdstring_view*const path, struct std_stdspan__cgs__4* dependencies);
void** lab_BuildContexttranslate_file_to_chemical(void**const ctx, struct std_stdstring_view*const c_path, struct std_stdstring_view*const output_path);
void lab_BuildContextinclude_headers(void**const ctx, void** module, struct std_stdspan__cgs__3* headers);
void lab_BuildContextindex_def_cbi_fn(void**const ctx, void** job, struct std_stdstring_view*const name, int type);
void lab_BuildContextbuild_job_dir_path(struct std_stdstring* __chx_struct_ret_param_xx, void**const ctx, struct std_stdstring_view*const job_name);
void lab_BuildContextjob_dir_path(struct std_stdstring* __chx_struct_ret_param_xx, void**const ctx, void** job);
void lab_BuildContextbuild_mod_file_path(struct std_stdstring* __chx_struct_ret_param_xx, void**const ctx, struct std_stdstring_view*const job_name, struct std_stdstring_view*const mod_scope, struct std_stdstring_view*const mod_name, struct std_stdstring_view*const file);
void lab_BuildContextbuild_llvm_ir_path(struct std_stdstring* __chx_struct_ret_param_xx, void**const ctx, struct std_stdstring_view*const job_name, struct std_stdstring_view*const mod_scope, struct std_stdstring_view*const mod_name);
void lab_BuildContextbuild_asm_path(struct std_stdstring* __chx_struct_ret_param_xx, void**const ctx, struct std_stdstring_view*const job_name, struct std_stdstring_view*const mod_scope, struct std_stdstring_view*const mod_name);
void lab_BuildContextbuild_bitcode_path(struct std_stdstring* __chx_struct_ret_param_xx, void**const ctx, struct std_stdstring_view*const job_name, struct std_stdstring_view*const mod_scope, struct std_stdstring_view*const mod_name);
void lab_BuildContextllvm_ir_path(struct std_stdstring* __chx_struct_ret_param_xx, void**const ctx, void** job, void** mod);
void lab_BuildContextasm_path(struct std_stdstring* __chx_struct_ret_param_xx, void**const ctx, void** job, void** mod);
void lab_BuildContextbitcode_path(struct std_stdstring* __chx_struct_ret_param_xx, void**const ctx, void** job, void** mod);
static int lab_labfind_last_pos_of_or(const char* str, size_t len, char value, char value2);
static void lab_labparent_path(struct std_stdstring* __chx_struct_ret_param_xx, struct std_stdstring_view* str);
void lab_labcurr_dir_of(struct std_stdstring* __chx_struct_ret_param_xx, const char* path, size_t len);
void lab_labview_of(struct std_stdstring_view* __chx_struct_ret_param_xx, const char* path, size_t len);
void lab_labappended_str(struct std_stdstring* __chx_struct_ret_param_xx, struct std_stdstring* str, const char* path);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/lab/src/target_data.ch **/
struct lab_TargetData {
	_Bool c;
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
	_Bool musl;
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
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/lab/src/AnnotationController.ch **/
void lab_AnnotationControllercreateSingleMarkerAnnotation(void**const self, struct std_stdstring_view*const name, int policy);
void lab_AnnotationControllercreateMarkerAnnotation(void**const self, struct std_stdstring_view*const name);
void lab_AnnotationControllercreateCollectorAnnotation(void**const self, struct std_stdstring_view*const name, unsigned int expected_usage);
void lab_AnnotationControllercreateMarkerAndCollectorAnnotation(void**const self, struct std_stdstring_view*const name, unsigned int expected_usage);
/** Implement:Generics lab **/
void std_stdspan__cgs__1constructor(struct std_stdspan__cgs__1* this, const struct lab_ImportSymbol* array_ptr, size_t array_size){
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
const struct lab_ImportSymbol* std_stdspan__cgs__1data(struct std_stdspan__cgs__1*const self){
	return self->_data;
}
const struct lab_ImportSymbol* std_stdspan__cgs__1get(struct std_stdspan__cgs__1*const self, size_t loc){
	return (self->_data + loc);
}
size_t std_stdspan__cgs__1size(struct std_stdspan__cgs__1*const self){
	return self->_size;
}
_Bool std_stdspan__cgs__1empty(struct std_stdspan__cgs__1*const self){
	return (self->_size == 0);
}
const struct lab_ImportSymbol* core_coreiterableLinear__cgs__4_span__cgs__1_data(struct std_stdspan__cgs__1*const self){
	return self->_data;
}
size_t core_coreiterableLinear__cgs__4_span__cgs__1_size(struct std_stdspan__cgs__1*const self){
	return self->_size;
}
const __chx_core_coreiterableLinear__cgs__4_vt_t core_coreiterableLinear__cgs__4std_stdspan__cgs__1 = {
	(const struct lab_ImportSymbol*(*)(void* self)) core_coreiterableLinear__cgs__4_span__cgs__1_data,
	(uint64_t(*)(void* self)) core_coreiterableLinear__cgs__4_span__cgs__1_size,
};
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
		.data_ptr = ((struct lab_ImportSymbol*) malloc((sizeof(struct lab_ImportSymbol) * init_cap))), 
		.data_size = 0, 
		.data_cap = init_cap
	};
	return;
}
void std_stdvector__cgs__4resize(struct std_stdvector__cgs__4* self, size_t new_size){
	if((new_size == self->data_size)){
		return;
	}
	if((new_size < self->data_size)){
		unsigned long to_destruct = (self->data_size - new_size);
		struct lab_ImportSymbol* start = (self->data_ptr + new_size);
		
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
		std_stdvector__cgs__4reserve(self, new_cap);
	}
	size_t i = self->data_size;
	while((i < new_size)) {
		self->data_ptr[i] = ((struct lab_ImportSymbol){0});
		i = (i + 1);
	}
	self->data_size = new_size;
}
void std_stdvector__cgs__4reserve(struct std_stdvector__cgs__4* self, size_t new_cap){
	if((new_cap <= self->data_cap)){
		return;
	}
	struct lab_ImportSymbol* new_data = ((struct lab_ImportSymbol*) realloc(self->data_ptr, (sizeof(struct lab_ImportSymbol) * new_cap)));
	
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
void std_stdvector__cgs__4ensure_capacity_for_one_more(struct std_stdvector__cgs__4* self){
	if((self->data_cap == 0)){
		std_stdvector__cgs__4reserve(self, 2);
	} else {
		std_stdvector__cgs__4reserve(self, (self->data_cap * 2));
	}
}
void std_stdvector__cgs__4push(struct std_stdvector__cgs__4* self, struct lab_ImportSymbol* value){
	size_t s = self->data_size;
	if((s >= self->data_cap)){
		std_stdvector__cgs__4ensure_capacity_for_one_more(self);
	}
	memcpy(&self->data_ptr[s], value, sizeof(struct lab_ImportSymbol));
	0;
	self->data_size = (s + 1);
}
void std_stdvector__cgs__4push_back(struct std_stdvector__cgs__4* self, struct lab_ImportSymbol* value){
	std_stdvector__cgs__4push(self, *({  &value; }));
}
void std_stdvector__cgs__4get(struct lab_ImportSymbol* __chx_struct_ret_param_xx, struct std_stdvector__cgs__4*const self, size_t index){
	*__chx_struct_ret_param_xx = self->data_ptr[index];
	return;
}
struct lab_ImportSymbol* std_stdvector__cgs__4get_ptr(struct std_stdvector__cgs__4*const self, size_t index){
	return &self->data_ptr[index];
}
struct lab_ImportSymbol* std_stdvector__cgs__4get_ref(struct std_stdvector__cgs__4*const self, size_t index){
	return &self->data_ptr[index];
}
void std_stdvector__cgs__4set(struct std_stdvector__cgs__4* self, size_t index, struct lab_ImportSymbol* value){
	self->data_ptr[index] = ({  *value; });
}
size_t std_stdvector__cgs__4size(struct std_stdvector__cgs__4*const self){
	return self->data_size;
}
size_t std_stdvector__cgs__4capacity(struct std_stdvector__cgs__4*const self){
	return self->data_cap;
}
const struct lab_ImportSymbol* std_stdvector__cgs__4data(struct std_stdvector__cgs__4*const self){
	return self->data_ptr;
}
struct lab_ImportSymbol* std_stdvector__cgs__4last_ptr(struct std_stdvector__cgs__4*const self){
	return (self->data_ptr + (self->data_size - 1));
}
void std_stdvector__cgs__4remove(struct std_stdvector__cgs__4* self, size_t index){
	size_t s = self->data_size;
	unsigned long last = (s - 1);
	
	if((index == last)){
		self->data_size = last;
	} else {
		memmove(&self->data_ptr[index], &self->data_ptr[(index + 1)], (sizeof(struct lab_ImportSymbol) * (last - index)));
		self->data_size = last;
	}
}
void std_stdvector__cgs__4erase(struct std_stdvector__cgs__4* self, size_t index){
	std_stdvector__cgs__4remove(self, index);
}
void std_stdvector__cgs__4remove_last(struct std_stdvector__cgs__4* self){
	size_t s = self->data_size;
	unsigned long last = (s - 1);
	struct lab_ImportSymbol* ptr = std_stdvector__cgs__4get_ptr(self, last);
	
	self->data_size = last;
}
void std_stdvector__cgs__4pop_back(struct std_stdvector__cgs__4* self){
	std_stdvector__cgs__4remove_last(self);
}
void std_stdvector__cgs__4take_last(struct lab_ImportSymbol* __chx_struct_ret_param_xx, struct std_stdvector__cgs__4* self){
	unsigned long last = (self->data_size - 1);
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
void std_stdvector__cgs__4set_len(struct std_stdvector__cgs__4* self, size_t new_size){
	self->data_size = new_size;
}
void std_stdvector__cgs__4delete(struct std_stdvector__cgs__4* self){
	if((self->data_ptr != NULL)){
		
		free(self->data_ptr);
	}
	__chx__dstctr_clnup_blk__:{
	}
}
const struct lab_ImportSymbol* core_coreiterableLinear__cgs__4_vector__cgs__4_data(struct std_stdvector__cgs__4*const self){
	return self->data_ptr;
}
size_t core_coreiterableLinear__cgs__4_vector__cgs__4_size(struct std_stdvector__cgs__4*const self){
	return self->data_size;
}
const __chx_core_coreiterableLinear__cgs__4_vt_t core_coreiterableLinear__cgs__4std_stdvector__cgs__4 = {
	(const struct lab_ImportSymbol*(*)(void* self)) core_coreiterableLinear__cgs__4_vector__cgs__4_data,
	(uint64_t(*)(void* self)) core_coreiterableLinear__cgs__4_vector__cgs__4_size,
};
void std_stdspan__cgs__2constructor(struct std_stdspan__cgs__2* this, const struct lab_ModuleDependency* array_ptr, size_t array_size){
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
const struct lab_ModuleDependency* std_stdspan__cgs__2data(struct std_stdspan__cgs__2*const self){
	return self->_data;
}
const struct lab_ModuleDependency* std_stdspan__cgs__2get(struct std_stdspan__cgs__2*const self, size_t loc){
	return (self->_data + loc);
}
size_t std_stdspan__cgs__2size(struct std_stdspan__cgs__2*const self){
	return self->_size;
}
_Bool std_stdspan__cgs__2empty(struct std_stdspan__cgs__2*const self){
	return (self->_size == 0);
}
const struct lab_ModuleDependency* core_coreiterableLinear__cgs__5_span__cgs__2_data(struct std_stdspan__cgs__2*const self){
	return self->_data;
}
size_t core_coreiterableLinear__cgs__5_span__cgs__2_size(struct std_stdspan__cgs__2*const self){
	return self->_size;
}
const __chx_core_coreiterableLinear__cgs__5_vt_t core_coreiterableLinear__cgs__5std_stdspan__cgs__2 = {
	(const struct lab_ModuleDependency*(*)(void* self)) core_coreiterableLinear__cgs__5_span__cgs__2_data,
	(uint64_t(*)(void* self)) core_coreiterableLinear__cgs__5_span__cgs__2_size,
};
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
		.data_ptr = ((struct lab_ModuleDependency*) malloc((sizeof(struct lab_ModuleDependency) * init_cap))), 
		.data_size = 0, 
		.data_cap = init_cap
	};
	return;
}
void std_stdvector__cgs__5resize(struct std_stdvector__cgs__5* self, size_t new_size){
	if((new_size == self->data_size)){
		return;
	}
	if((new_size < self->data_size)){
		unsigned long to_destruct = (self->data_size - new_size);
		struct lab_ModuleDependency* start = (self->data_ptr + new_size);
		
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
		std_stdvector__cgs__5reserve(self, new_cap);
	}
	size_t i = self->data_size;
	while((i < new_size)) {
		self->data_ptr[i] = ((struct lab_ModuleDependency){0});
		i = (i + 1);
	}
	self->data_size = new_size;
}
void std_stdvector__cgs__5reserve(struct std_stdvector__cgs__5* self, size_t new_cap){
	if((new_cap <= self->data_cap)){
		return;
	}
	struct lab_ModuleDependency* new_data = ((struct lab_ModuleDependency*) realloc(self->data_ptr, (sizeof(struct lab_ModuleDependency) * new_cap)));
	
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
void std_stdvector__cgs__5ensure_capacity_for_one_more(struct std_stdvector__cgs__5* self){
	if((self->data_cap == 0)){
		std_stdvector__cgs__5reserve(self, 2);
	} else {
		std_stdvector__cgs__5reserve(self, (self->data_cap * 2));
	}
}
void std_stdvector__cgs__5push(struct std_stdvector__cgs__5* self, struct lab_ModuleDependency* value){
	size_t s = self->data_size;
	if((s >= self->data_cap)){
		std_stdvector__cgs__5ensure_capacity_for_one_more(self);
	}
	memcpy(&self->data_ptr[s], value, sizeof(struct lab_ModuleDependency));
	0;
	self->data_size = (s + 1);
}
void std_stdvector__cgs__5push_back(struct std_stdvector__cgs__5* self, struct lab_ModuleDependency* value){
	std_stdvector__cgs__5push(self, *({  &value; }));
}
void std_stdvector__cgs__5get(struct lab_ModuleDependency* __chx_struct_ret_param_xx, struct std_stdvector__cgs__5*const self, size_t index){
	*__chx_struct_ret_param_xx = self->data_ptr[index];
	return;
}
struct lab_ModuleDependency* std_stdvector__cgs__5get_ptr(struct std_stdvector__cgs__5*const self, size_t index){
	return &self->data_ptr[index];
}
struct lab_ModuleDependency* std_stdvector__cgs__5get_ref(struct std_stdvector__cgs__5*const self, size_t index){
	return &self->data_ptr[index];
}
void std_stdvector__cgs__5set(struct std_stdvector__cgs__5* self, size_t index, struct lab_ModuleDependency* value){
	self->data_ptr[index] = ({  *value; });
}
size_t std_stdvector__cgs__5size(struct std_stdvector__cgs__5*const self){
	return self->data_size;
}
size_t std_stdvector__cgs__5capacity(struct std_stdvector__cgs__5*const self){
	return self->data_cap;
}
const struct lab_ModuleDependency* std_stdvector__cgs__5data(struct std_stdvector__cgs__5*const self){
	return self->data_ptr;
}
struct lab_ModuleDependency* std_stdvector__cgs__5last_ptr(struct std_stdvector__cgs__5*const self){
	return (self->data_ptr + (self->data_size - 1));
}
void std_stdvector__cgs__5remove(struct std_stdvector__cgs__5* self, size_t index){
	size_t s = self->data_size;
	unsigned long last = (s - 1);
	
	if((index == last)){
		self->data_size = last;
	} else {
		memmove(&self->data_ptr[index], &self->data_ptr[(index + 1)], (sizeof(struct lab_ModuleDependency) * (last - index)));
		self->data_size = last;
	}
}
void std_stdvector__cgs__5erase(struct std_stdvector__cgs__5* self, size_t index){
	std_stdvector__cgs__5remove(self, index);
}
void std_stdvector__cgs__5remove_last(struct std_stdvector__cgs__5* self){
	size_t s = self->data_size;
	unsigned long last = (s - 1);
	struct lab_ModuleDependency* ptr = std_stdvector__cgs__5get_ptr(self, last);
	
	self->data_size = last;
}
void std_stdvector__cgs__5pop_back(struct std_stdvector__cgs__5* self){
	std_stdvector__cgs__5remove_last(self);
}
void std_stdvector__cgs__5take_last(struct lab_ModuleDependency* __chx_struct_ret_param_xx, struct std_stdvector__cgs__5* self){
	unsigned long last = (self->data_size - 1);
	self->data_size = last;
	
	*__chx_struct_ret_param_xx = *std_stdvector__cgs__5get_ptr(self, last);
	return;
}
_Bool std_stdvector__cgs__5empty(struct std_stdvector__cgs__5*const self){
	return (self->data_size == 0);
}
void std_stdvector__cgs__5clear(struct std_stdvector__cgs__5* self){
	
	self->data_size = 0;
}
void std_stdvector__cgs__5set_len(struct std_stdvector__cgs__5* self, size_t new_size){
	self->data_size = new_size;
}
void std_stdvector__cgs__5delete(struct std_stdvector__cgs__5* self){
	if((self->data_ptr != NULL)){
		
		free(self->data_ptr);
	}
	__chx__dstctr_clnup_blk__:{
	}
}
const struct lab_ModuleDependency* core_coreiterableLinear__cgs__5_vector__cgs__5_data(struct std_stdvector__cgs__5*const self){
	return self->data_ptr;
}
size_t core_coreiterableLinear__cgs__5_vector__cgs__5_size(struct std_stdvector__cgs__5*const self){
	return self->data_size;
}
const __chx_core_coreiterableLinear__cgs__5_vt_t core_coreiterableLinear__cgs__5std_stdvector__cgs__5 = {
	(const struct lab_ModuleDependency*(*)(void* self)) core_coreiterableLinear__cgs__5_vector__cgs__5_data,
	(uint64_t(*)(void* self)) core_coreiterableLinear__cgs__5_vector__cgs__5_size,
};
void std_stdspan__cgs__3constructor(struct std_stdspan__cgs__3* this, const struct std_stdstring_view* array_ptr, size_t array_size){
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
const struct std_stdstring_view* std_stdspan__cgs__3data(struct std_stdspan__cgs__3*const self){
	return self->_data;
}
const struct std_stdstring_view* std_stdspan__cgs__3get(struct std_stdspan__cgs__3*const self, size_t loc){
	return (self->_data + loc);
}
size_t std_stdspan__cgs__3size(struct std_stdspan__cgs__3*const self){
	return self->_size;
}
_Bool std_stdspan__cgs__3empty(struct std_stdspan__cgs__3*const self){
	return (self->_size == 0);
}
const struct std_stdstring_view* core_coreiterableLinear__cgs__1_span__cgs__3_data(struct std_stdspan__cgs__3*const self){
	return self->_data;
}
size_t core_coreiterableLinear__cgs__1_span__cgs__3_size(struct std_stdspan__cgs__3*const self){
	return self->_size;
}
const __chx_core_coreiterableLinear__cgs__1_vt_t core_coreiterableLinear__cgs__1std_stdspan__cgs__3 = {
	(const struct std_stdstring_view*(*)(void* self)) core_coreiterableLinear__cgs__1_span__cgs__3_data,
	(uint64_t(*)(void* self)) core_coreiterableLinear__cgs__1_span__cgs__3_size,
};
void std_stdspan__cgs__4constructor(struct std_stdspan__cgs__4* this, const void*** array_ptr, size_t array_size){
	*this = (struct std_stdspan__cgs__4){ 
		._data = array_ptr, 
		._size = array_size
	};
	return;
}
void std_stdspan__cgs__4empty_make(struct std_stdspan__cgs__4* this){
	*this = (struct std_stdspan__cgs__4){ 
		._data = NULL, 
		._size = 0
	};
	return;
}
const void*** std_stdspan__cgs__4data(struct std_stdspan__cgs__4*const self){
	return self->_data;
}
const void*** std_stdspan__cgs__4get(struct std_stdspan__cgs__4*const self, size_t loc){
	return (self->_data + loc);
}
size_t std_stdspan__cgs__4size(struct std_stdspan__cgs__4*const self){
	return self->_size;
}
_Bool std_stdspan__cgs__4empty(struct std_stdspan__cgs__4*const self){
	return (self->_size == 0);
}
const void*** core_coreiterableLinear__cgs__6_span__cgs__4_data(struct std_stdspan__cgs__4*const self){
	return self->_data;
}
size_t core_coreiterableLinear__cgs__6_span__cgs__4_size(struct std_stdspan__cgs__4*const self){
	return self->_size;
}
const __chx_core_coreiterableLinear__cgs__6_vt_t core_coreiterableLinear__cgs__6std_stdspan__cgs__4 = {
	(const void***(*)(void* self)) core_coreiterableLinear__cgs__6_span__cgs__4_data,
	(uint64_t(*)(void* self)) core_coreiterableLinear__cgs__6_span__cgs__4_size,
};
void std_stdvector__cgs__6make(struct std_stdvector__cgs__6* this){
	*this = (struct std_stdvector__cgs__6){ 
		.data_ptr = NULL, 
		.data_size = 0, 
		.data_cap = 0
	};
	return;
}
void std_stdvector__cgs__6make_with_capacity(struct std_stdvector__cgs__6* this, size_t init_cap){
	*this = (struct std_stdvector__cgs__6){ 
		.data_ptr = ((void***) malloc((sizeof(void**) * init_cap))), 
		.data_size = 0, 
		.data_cap = init_cap
	};
	return;
}
void std_stdvector__cgs__6resize(struct std_stdvector__cgs__6* self, size_t new_size){
	if((new_size == self->data_size)){
		return;
	}
	if((new_size < self->data_size)){
		unsigned long to_destruct = (self->data_size - new_size);
		void*** start = (self->data_ptr + new_size);
		
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
		std_stdvector__cgs__6reserve(self, new_cap);
	}
	size_t i = self->data_size;
	while((i < new_size)) {
		self->data_ptr[i] = ((void**){0});
		i = (i + 1);
	}
	self->data_size = new_size;
}
void std_stdvector__cgs__6reserve(struct std_stdvector__cgs__6* self, size_t new_cap){
	if((new_cap <= self->data_cap)){
		return;
	}
	void*** new_data = ((void***) realloc(self->data_ptr, (sizeof(void**) * new_cap)));
	
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
void std_stdvector__cgs__6ensure_capacity_for_one_more(struct std_stdvector__cgs__6* self){
	if((self->data_cap == 0)){
		std_stdvector__cgs__6reserve(self, 2);
	} else {
		std_stdvector__cgs__6reserve(self, (self->data_cap * 2));
	}
}
void std_stdvector__cgs__6push(struct std_stdvector__cgs__6* self, void** value){
	size_t s = self->data_size;
	if((s >= self->data_cap)){
		std_stdvector__cgs__6ensure_capacity_for_one_more(self);
	}
	memcpy(&self->data_ptr[s], &value, sizeof(void**));
	0;
	self->data_size = (s + 1);
}
void std_stdvector__cgs__6push_back(struct std_stdvector__cgs__6* self, void** value){
	std_stdvector__cgs__6push(self, value);
}
void** std_stdvector__cgs__6get(struct std_stdvector__cgs__6*const self, size_t index){
	return self->data_ptr[index];
}
void*** std_stdvector__cgs__6get_ptr(struct std_stdvector__cgs__6*const self, size_t index){
	return &self->data_ptr[index];
}
void*** std_stdvector__cgs__6get_ref(struct std_stdvector__cgs__6*const self, size_t index){
	return &self->data_ptr[index];
}
void std_stdvector__cgs__6set(struct std_stdvector__cgs__6* self, size_t index, void** value){
	self->data_ptr[index] = ({  value; });
}
size_t std_stdvector__cgs__6size(struct std_stdvector__cgs__6*const self){
	return self->data_size;
}
size_t std_stdvector__cgs__6capacity(struct std_stdvector__cgs__6*const self){
	return self->data_cap;
}
const void*** std_stdvector__cgs__6data(struct std_stdvector__cgs__6*const self){
	return self->data_ptr;
}
void*** std_stdvector__cgs__6last_ptr(struct std_stdvector__cgs__6*const self){
	return (self->data_ptr + (self->data_size - 1));
}
void std_stdvector__cgs__6remove(struct std_stdvector__cgs__6* self, size_t index){
	size_t s = self->data_size;
	unsigned long last = (s - 1);
	
	if((index == last)){
		self->data_size = last;
	} else {
		memmove(&self->data_ptr[index], &self->data_ptr[(index + 1)], (sizeof(void**) * (last - index)));
		self->data_size = last;
	}
}
void std_stdvector__cgs__6erase(struct std_stdvector__cgs__6* self, size_t index){
	std_stdvector__cgs__6remove(self, index);
}
void std_stdvector__cgs__6remove_last(struct std_stdvector__cgs__6* self){
	size_t s = self->data_size;
	unsigned long last = (s - 1);
	void*** ptr = std_stdvector__cgs__6get_ptr(self, last);
	
	self->data_size = last;
}
void std_stdvector__cgs__6pop_back(struct std_stdvector__cgs__6* self){
	std_stdvector__cgs__6remove_last(self);
}
void** std_stdvector__cgs__6take_last(struct std_stdvector__cgs__6* self){
	unsigned long last = (self->data_size - 1);
	self->data_size = last;
	
	return *std_stdvector__cgs__6get_ptr(self, last);
}
_Bool std_stdvector__cgs__6empty(struct std_stdvector__cgs__6*const self){
	return (self->data_size == 0);
}
void std_stdvector__cgs__6clear(struct std_stdvector__cgs__6* self){
	
	self->data_size = 0;
}
void std_stdvector__cgs__6set_len(struct std_stdvector__cgs__6* self, size_t new_size){
	self->data_size = new_size;
}
void std_stdvector__cgs__6delete(struct std_stdvector__cgs__6* self){
	if((self->data_ptr != NULL)){
		
		free(self->data_ptr);
	}
	__chx__dstctr_clnup_blk__:{
	}
}
const void*** core_coreiterableLinear__cgs__6_vector__cgs__6_data(struct std_stdvector__cgs__6*const self){
	return self->data_ptr;
}
size_t core_coreiterableLinear__cgs__6_vector__cgs__6_size(struct std_stdvector__cgs__6*const self){
	return self->data_size;
}
const __chx_core_coreiterableLinear__cgs__6_vt_t core_coreiterableLinear__cgs__6std_stdvector__cgs__6 = {
	(const void***(*)(void* self)) core_coreiterableLinear__cgs__6_vector__cgs__6_data,
	(uint64_t(*)(void* self)) core_coreiterableLinear__cgs__6_vector__cgs__6_size,
};
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/lab/src/lab.ch **/
void lab_ModuleChecksOptionsmake(struct lab_ModuleChecksOptions* this){
	this->bounds = 0;
	this->overflow = 0;
	this->null = 0;
}
void lab_ModuleOptionsmake(struct lab_ModuleOptions* this){
	this->safety = 1;
	lab_ModuleChecksOptionsmake(&this->checks);
	std_stdstring_viewempty_make(&this->safe_mode);
	std_stdstring_viewempty_make(&this->stack_protector);
	this->optimization_level = 0;
}
void** lab_BuildContextnew_module(void**const ctx, int type, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name, struct std_stdspan__cgs__2* dependencies){
	return lab_BuildContextnew_package(ctx, type, 0, scope_name, name, ({ struct std_stdspan__cgs__2 __chx__lv__0 = *dependencies; &__chx__lv__0; }));
}
void** lab_BuildContextnew_app_module(void**const ctx, int type, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name, struct std_stdspan__cgs__2* dependencies){
	return lab_BuildContextnew_package(ctx, type, 1, scope_name, name, ({ struct std_stdspan__cgs__2 __chx__lv__1 = *dependencies; &__chx__lv__1; }));
}
void** lab_BuildContextdirectory_module(void**const ctx, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name, struct std_stdspan__cgs__2* dependencies){
	return lab_BuildContextnew_package(ctx, 4, 0, scope_name, name, ({ struct std_stdspan__cgs__2 __chx__lv__2 = *dependencies; &__chx__lv__2; }));
}
static void lab_make_deps(struct std_stdvector__cgs__5* vec, struct std_stdspan__cgs__4* dependencies){
	if(std_stdspan__cgs__4empty(dependencies)){
		return;
	}
	std_stdvector__cgs__5reserve(vec, std_stdspan__cgs__4size(dependencies));
	const void*** start = std_stdspan__cgs__4data(dependencies);
	const void*** end = (start + std_stdspan__cgs__4size(dependencies));
	while((start != end)) {
		std_stdvector__cgs__5push(vec, &(struct lab_ModuleDependency){ 
			.module = ((void**) *start), 
			.info = NULL
		});
		start++;
	}
}
void** lab_BuildContextnew_module_with_deps(void**const ctx, int type, int package_kind, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name, struct std_stdspan__cgs__4* dependencies){
	struct std_stdvector__cgs__5 vec = (*({ struct std_stdvector__cgs__5 __chx__lv__3; std_stdvector__cgs__5make(&__chx__lv__3); &__chx__lv__3; }));
	_Bool __chx__lv__4 = true;
	lab_make_deps(&vec, ({ struct std_stdspan__cgs__4 __chx__lv__5 = *dependencies; &__chx__lv__5; }));
	const void** __chx__lv__6 = lab_BuildContextnew_package(ctx, type, package_kind, scope_name, name, &(*({ struct std_stdspan__cgs__2 __chx__lv__7; std_stdspan__cgs__2constructor(&__chx__lv__7, std_stdvector__cgs__5data(&vec), std_stdvector__cgs__5size(&vec)); &__chx__lv__7; })));
	if(__chx__lv__4) {
		std_stdvector__cgs__5delete(&vec);
	}
	return __chx__lv__6;
}
void** lab_BuildContextc_file_module(void**const ctx, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name, struct std_stdstring_view*const path, struct std_stdspan__cgs__4* dependencies){
	struct std_stdvector__cgs__5 vec = (*({ struct std_stdvector__cgs__5 __chx__lv__8; std_stdvector__cgs__5make(&__chx__lv__8); &__chx__lv__8; }));
	_Bool __chx__lv__9 = true;
	lab_make_deps(&vec, ({ struct std_stdspan__cgs__4 __chx__lv__10 = *dependencies; &__chx__lv__10; }));
	void** mod = lab_BuildContextnew_package(ctx, 1, 0, scope_name, name, &(*({ struct std_stdspan__cgs__2 __chx__lv__11; std_stdspan__cgs__2constructor(&__chx__lv__11, std_stdvector__cgs__5data(&vec), std_stdvector__cgs__5size(&vec)); &__chx__lv__11; })));
	lab_BuildContextadd_path(ctx, mod, path);
	const void** __chx__lv__12 = mod;
	if(__chx__lv__9) {
		std_stdvector__cgs__5delete(&vec);
	}
	return __chx__lv__12;
}
void** lab_BuildContextcpp_file_module(void**const ctx, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name, struct std_stdstring_view*const path, struct std_stdspan__cgs__4* dependencies){
	struct std_stdvector__cgs__5 vec = (*({ struct std_stdvector__cgs__5 __chx__lv__13; std_stdvector__cgs__5make(&__chx__lv__13); &__chx__lv__13; }));
	_Bool __chx__lv__14 = true;
	lab_make_deps(&vec, ({ struct std_stdspan__cgs__4 __chx__lv__15 = *dependencies; &__chx__lv__15; }));
	void** mod = lab_BuildContextnew_package(ctx, 2, 0, scope_name, name, &(*({ struct std_stdspan__cgs__2 __chx__lv__16; std_stdspan__cgs__2constructor(&__chx__lv__16, std_stdvector__cgs__5data(&vec), std_stdvector__cgs__5size(&vec)); &__chx__lv__16; })));
	lab_BuildContextadd_path(ctx, mod, path);
	const void** __chx__lv__17 = mod;
	if(__chx__lv__14) {
		std_stdvector__cgs__5delete(&vec);
	}
	return __chx__lv__17;
}
void** lab_BuildContextobject_module(void**const ctx, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name, struct std_stdstring_view*const path){
	void** mod = lab_BuildContextnew_package(ctx, 3, 0, scope_name, name, &(*({ struct std_stdspan__cgs__2 __chx__lv__18; std_stdspan__cgs__2empty_make(&__chx__lv__18); &__chx__lv__18; })));
	lab_BuildContextadd_path(ctx, mod, path);
	return mod;
}
void** lab_BuildContextempty_module(void**const ctx, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name){
	return lab_BuildContextnew_module_with_deps(ctx, 4, 0, scope_name, name, &(*({ struct std_stdspan__cgs__4 __chx__lv__19; std_stdspan__cgs__4empty_make(&__chx__lv__19); &__chx__lv__19; })));
}
void** lab_BuildContextchemical_dir_module(void**const ctx, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name, struct std_stdstring_view*const path, struct std_stdspan__cgs__4* dependencies){
	void** mod = lab_BuildContextnew_module_with_deps(ctx, 4, 0, scope_name, name, ({ struct std_stdspan__cgs__4 __chx__lv__20 = *dependencies; &__chx__lv__20; }));
	lab_BuildContextadd_path(ctx, mod, path);
	return mod;
}
void** lab_BuildContextdirectory_app_module(void**const ctx, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name, struct std_stdstring_view*const path, struct std_stdspan__cgs__4* dependencies){
	void** mod = lab_BuildContextnew_module_with_deps(ctx, 4, 1, scope_name, name, ({ struct std_stdspan__cgs__4 __chx__lv__21 = *dependencies; &__chx__lv__21; }));
	lab_BuildContextadd_path(ctx, mod, path);
	return mod;
}
void lab_ImportSymbolmake(struct lab_ImportSymbol* this){
	std_stdspan__cgs__3empty_make(&this->parts);
	std_stdstring_viewempty_make(&this->alias);
}
void lab_BuildContextadd_compiler_interfaces(void**const ctx, void** mod, struct std_stdspan__cgs__3* interfaces){
	if(!std_stdspan__cgs__3empty(interfaces)){
		unsigned int i = 0;
		size_t s = std_stdspan__cgs__3size(interfaces);
		while((i < s)) {
			const struct std_stdstring_view* ci = std_stdspan__cgs__3get(interfaces, i);
			lab_BuildContextadd_compiler_interface(ctx, mod, &*ci);
			i++;
		}
	}
}
void** lab_BuildContextcreate_module(void**const ctx, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name, struct std_stdstring_view*const dir_path, struct std_stdspan__cgs__4* dependencies, struct std_stdspan__cgs__3* interfaces){
	void** mod = lab_BuildContextchemical_dir_module(ctx, scope_name, name, dir_path, ({ struct std_stdspan__cgs__4 __chx__lv__22 = *dependencies; &__chx__lv__22; }));
	lab_BuildContextadd_compiler_interfaces(ctx, mod, ({ struct std_stdspan__cgs__3 __chx__lv__23 = *interfaces; &__chx__lv__23; }));
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
void** lab_BuildContextfile_module(void**const ctx, struct std_stdstring_view*const scope_name, struct std_stdstring_view*const name, struct std_stdstring_view*const path, struct std_stdspan__cgs__4* dependencies){
	if(std_stdstring_viewends_with(path, &(*({ struct std_stdstring_view __chx__lv__24; std_stdstring_viewconstructor(&__chx__lv__24, ".c", 2); &__chx__lv__24; })))){
		void** mod = lab_BuildContextnew_module_with_deps(ctx, 1, 0, scope_name, name, ({ struct std_stdspan__cgs__4 __chx__lv__25 = *dependencies; &__chx__lv__25; }));
		lab_BuildContextadd_path(ctx, mod, path);
		return mod;
	}else if(std_stdstring_viewends_with(path, &(*({ struct std_stdstring_view __chx__lv__26; std_stdstring_viewconstructor(&__chx__lv__26, ".cpp", 4); &__chx__lv__26; })))){
		void** mod = lab_BuildContextnew_module_with_deps(ctx, 2, 0, scope_name, name, ({ struct std_stdspan__cgs__4 __chx__lv__27 = *dependencies; &__chx__lv__27; }));
		lab_BuildContextadd_path(ctx, mod, path);
		return mod;
	}else if(std_stdstring_viewends_with(path, &(*({ struct std_stdstring_view __chx__lv__28; std_stdstring_viewconstructor(&__chx__lv__28, ".o", 2); &__chx__lv__28; })))){
		void** mod = lab_BuildContextnew_module_with_deps(ctx, 3, 0, scope_name, name, ({ struct std_stdspan__cgs__4 __chx__lv__29 = *dependencies; &__chx__lv__29; }));
		lab_BuildContextadd_path(ctx, mod, path);
		return mod;
	} else {
		void** mod = lab_BuildContextnew_module_with_deps(ctx, 4, 0, scope_name, name, ({ struct std_stdspan__cgs__4 __chx__lv__30 = *dependencies; &__chx__lv__30; }));
		lab_BuildContextadd_path(ctx, mod, path);
		return mod;
	}
}
void** lab_BuildContexttranslate_file_to_chemical(void**const ctx, struct std_stdstring_view*const c_path, struct std_stdstring_view*const output_path){
	void** deps[] = {};
	void** mod = lab_BuildContextfile_module(ctx, &(*({ struct std_stdstring_view __chx__lv__31; std_stdstring_viewconstructor(&__chx__lv__31, "", 0); &__chx__lv__31; })), &(*({ struct std_stdstring_view __chx__lv__32; std_stdstring_viewconstructor(&__chx__lv__32, "CFile", 5); &__chx__lv__32; })), c_path, &(*({ struct std_stdspan__cgs__4 __chx__lv__33; std_stdspan__cgs__4constructor(&__chx__lv__33, (void**[]){}, 0); &__chx__lv__33; })));
	return lab_BuildContexttranslate_to_chemical(ctx, mod, output_path);
}
void lab_BuildContextinclude_headers(void**const ctx, void** module, struct std_stdspan__cgs__3* headers){
	int i = 0;
	size_t total = std_stdspan__cgs__3size(headers);
	while((i < total)) {
		const struct std_stdstring_view* ele = std_stdspan__cgs__3get(headers, ((size_t) i));
		lab_BuildContextinclude_header(ctx, module, &*ele);
		i++;
	}
}
void lab_BuildContextindex_def_cbi_fn(void**const ctx, void** job, struct std_stdstring_view*const name, int type){
	lab_BuildContextindex_cbi_fn(ctx, job, &(*({ struct std_stdstring_view __chx__lv__34; lab_LabJobgetName(&__chx__lv__34, job); &__chx__lv__34; })), name, type);
}
void lab_BuildContextbuild_job_dir_path(struct std_stdstring* __chx_struct_ret_param_xx, void**const ctx, struct std_stdstring_view*const job_name){
	struct std_stdstring_view view = (*({ struct std_stdstring_view __chx__lv__35; lab_BuildContextbuild_path(&__chx__lv__35, ctx); &__chx__lv__35; }));
	struct std_stdstring new_path = (*({ struct std_stdstring __chx__lv__36; std_stdstringview_make(&__chx__lv__36, &view); &__chx__lv__36; }));
	_Bool __chx__lv__37 = true;
	std_stdstringappend(&new_path, '/');
	std_stdstringappend_view(&new_path, job_name);
	std_stdstringappend_view(&new_path, &(*({ struct std_stdstring_view __chx__lv__38; std_stdstring_viewconstructor(&__chx__lv__38, ".dir", 4); &__chx__lv__38; })));
	std_stdstringappend(&new_path, '/');
	*__chx_struct_ret_param_xx = new_path;
	return;
}
void lab_BuildContextjob_dir_path(struct std_stdstring* __chx_struct_ret_param_xx, void**const ctx, void** job){
	*__chx_struct_ret_param_xx = (*({ struct std_stdstring __chx__lv__39; lab_BuildContextbuild_job_dir_path(&__chx__lv__39, ctx, &(*({ struct std_stdstring_view __chx__lv__40; lab_LabJobgetName(&__chx__lv__40, job); &__chx__lv__40; }))); &__chx__lv__39; }));
	return;
}
void lab_BuildContextbuild_mod_file_path(struct std_stdstring* __chx_struct_ret_param_xx, void**const ctx, struct std_stdstring_view*const job_name, struct std_stdstring_view*const mod_scope, struct std_stdstring_view*const mod_name, struct std_stdstring_view*const file){
	struct std_stdstring str = (*({ struct std_stdstring __chx__lv__41; lab_BuildContextbuild_job_dir_path(&__chx__lv__41, ctx, job_name); &__chx__lv__41; }));
	_Bool __chx__lv__42 = true;
	std_stdstringappend_view(&str, &(*({ struct std_stdstring_view __chx__lv__43; std_stdstring_viewconstructor(&__chx__lv__43, "modules/", 8); &__chx__lv__43; })));
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
	*__chx_struct_ret_param_xx = (*({ struct std_stdstring __chx__lv__44; lab_BuildContextbuild_mod_file_path(&__chx__lv__44, ctx, job_name, mod_scope, mod_name, &(*({ struct std_stdstring_view __chx__lv__45; std_stdstring_viewconstructor(&__chx__lv__45, "llvm_ir.ll", 10); &__chx__lv__45; }))); &__chx__lv__44; }));
	return;
}
void lab_BuildContextbuild_asm_path(struct std_stdstring* __chx_struct_ret_param_xx, void**const ctx, struct std_stdstring_view*const job_name, struct std_stdstring_view*const mod_scope, struct std_stdstring_view*const mod_name){
	*__chx_struct_ret_param_xx = (*({ struct std_stdstring __chx__lv__46; lab_BuildContextbuild_mod_file_path(&__chx__lv__46, ctx, job_name, mod_scope, mod_name, &(*({ struct std_stdstring_view __chx__lv__47; std_stdstring_viewconstructor(&__chx__lv__47, "asm.s", 5); &__chx__lv__47; }))); &__chx__lv__46; }));
	return;
}
void lab_BuildContextbuild_bitcode_path(struct std_stdstring* __chx_struct_ret_param_xx, void**const ctx, struct std_stdstring_view*const job_name, struct std_stdstring_view*const mod_scope, struct std_stdstring_view*const mod_name){
	*__chx_struct_ret_param_xx = (*({ struct std_stdstring __chx__lv__48; lab_BuildContextbuild_mod_file_path(&__chx__lv__48, ctx, job_name, mod_scope, mod_name, &(*({ struct std_stdstring_view __chx__lv__49; std_stdstring_viewconstructor(&__chx__lv__49, "bitcode.bc", 10); &__chx__lv__49; }))); &__chx__lv__48; }));
	return;
}
void lab_BuildContextllvm_ir_path(struct std_stdstring* __chx_struct_ret_param_xx, void**const ctx, void** job, void** mod){
	*__chx_struct_ret_param_xx = (*({ struct std_stdstring __chx__lv__50; lab_BuildContextbuild_llvm_ir_path(&__chx__lv__50, ctx, &(*({ struct std_stdstring_view __chx__lv__51; lab_LabJobgetName(&__chx__lv__51, job); &__chx__lv__51; })), &(*({ struct std_stdstring_view __chx__lv__52; lab_ModulegetScopeName(&__chx__lv__52, mod); &__chx__lv__52; })), &(*({ struct std_stdstring_view __chx__lv__53; lab_ModulegetName(&__chx__lv__53, mod); &__chx__lv__53; }))); &__chx__lv__50; }));
	return;
}
void lab_BuildContextasm_path(struct std_stdstring* __chx_struct_ret_param_xx, void**const ctx, void** job, void** mod){
	*__chx_struct_ret_param_xx = (*({ struct std_stdstring __chx__lv__54; lab_BuildContextbuild_asm_path(&__chx__lv__54, ctx, &(*({ struct std_stdstring_view __chx__lv__55; lab_LabJobgetName(&__chx__lv__55, job); &__chx__lv__55; })), &(*({ struct std_stdstring_view __chx__lv__56; lab_ModulegetScopeName(&__chx__lv__56, mod); &__chx__lv__56; })), &(*({ struct std_stdstring_view __chx__lv__57; lab_ModulegetName(&__chx__lv__57, mod); &__chx__lv__57; }))); &__chx__lv__54; }));
	return;
}
void lab_BuildContextbitcode_path(struct std_stdstring* __chx_struct_ret_param_xx, void**const ctx, void** job, void** mod){
	*__chx_struct_ret_param_xx = (*({ struct std_stdstring __chx__lv__58; lab_BuildContextbuild_bitcode_path(&__chx__lv__58, ctx, &(*({ struct std_stdstring_view __chx__lv__59; lab_LabJobgetName(&__chx__lv__59, job); &__chx__lv__59; })), &(*({ struct std_stdstring_view __chx__lv__60; lab_ModulegetScopeName(&__chx__lv__60, mod); &__chx__lv__60; })), &(*({ struct std_stdstring_view __chx__lv__61; lab_ModulegetName(&__chx__lv__61, mod); &__chx__lv__61; }))); &__chx__lv__58; }));
	return;
}
static int lab_labfind_last_pos_of_or(const char* str, size_t len, char value, char value2){
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
static void lab_labparent_path(struct std_stdstring* __chx_struct_ret_param_xx, struct std_stdstring_view* str){
	struct std_stdstring final = (*({ struct std_stdstring __chx__lv__62; std_stdstringempty_str(&__chx__lv__62); &__chx__lv__62; }));
	_Bool __chx__lv__63 = true;
	int pos;
	
	pos = lab_labfind_last_pos_of_or(std_stdstring_viewdata(str), std_stdstring_viewsize(str), '/', '/');
	if((pos > 0)){
		std_stdstringappend_with_len(&final, std_stdstring_viewdata(str), ((size_t) (pos + 1)));
	}
	*__chx_struct_ret_param_xx = final;
	return;
}
void lab_labcurr_dir_of(struct std_stdstring* __chx_struct_ret_param_xx, const char* path, size_t len){
	*__chx_struct_ret_param_xx = (*({ struct std_stdstring __chx__lv__64; lab_labparent_path(&__chx__lv__64, &(*({ struct std_stdstring_view __chx__lv__65; std_stdstring_viewconstructor(&__chx__lv__65, path, len); &__chx__lv__65; }))); &__chx__lv__64; }));
	return;
}
void lab_labview_of(struct std_stdstring_view* __chx_struct_ret_param_xx, const char* path, size_t len){
	*__chx_struct_ret_param_xx = (*({ struct std_stdstring_view __chx__lv__66; std_stdstring_viewconstructor(&__chx__lv__66, path, len); &__chx__lv__66; }));
	return;
}
void lab_labappended_str(struct std_stdstring* __chx_struct_ret_param_xx, struct std_stdstring* str, const char* path){
	_Bool __chx__lv__67 = true;
	std_stdstringappend_char_ptr(str, path);
	*__chx_struct_ret_param_xx = *str;
	return;
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/lab/src/target_data.ch **/
void lab_TargetDatamake(struct lab_TargetData* this){
	this->c = 0;
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
	this->musl = 0;
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
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/lab/src/AnnotationController.ch **/