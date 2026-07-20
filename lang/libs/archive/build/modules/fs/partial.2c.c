
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/src/path.ch **/
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/src/utf.ch **/
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/src/types.ch **/
struct fs_fsFsError;
struct fs_fsMetadata;
struct fs_fsOpenOptions;
struct fs_fsFile;
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/src/main.ch **/
struct fs_UnitTy;
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/src/metadata.ch **/
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/src/directory.ch **/
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/src/error_helpers.ch **/
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/src/file_io.ch **/
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/posix/permission.ch **/
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/posix/directory.ch **/
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/posix/link.ch **/
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/posix/path.ch **/
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/posix/file_io.ch **/
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/posix/lock.ch **/
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/posix/metadata.ch **/
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/posix/disk.ch **/
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/posix/api.ch **/
struct statvfs;
struct fs_fsStatvfs;
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/src/path.ch **/
typedef const char* fs_fspath_ptr;
typedef char* fs_fsmut_path_ptr;
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/src/utf.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/src/types.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/src/main.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/src/metadata.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/src/directory.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/src/error_helpers.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/src/file_io.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/posix/permission.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/posix/directory.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/posix/link.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/posix/path.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/posix/file_io.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/posix/lock.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/posix/metadata.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/posix/disk.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/posix/api.ch **/
typedef unsigned long fs___fsblkcnt_t;
typedef unsigned long fs___fsfilcnt_t;
typedef long isize;
typedef isize fs_fsssize_t;
/** FwdDeclare:Generics fs **/
struct std_stdResult__cgs__31;
struct std_stdResult__cgs__32;
struct std_stdResult__cgs__33;
struct std_stdResult__cgs__34;
struct std_stdResult__cgs__35;
struct std_stdResult__cgs__36;
/** Declare:Generics fs **/
struct fs_fsFsError {
	int __chx__vt_621827;
	union {
		struct {
			int code;
			const char* message;
		} Io;
		struct {
		} NotFound;
		struct {
		} AlreadyExists;
		struct {
		} PermissionDenied;
		struct {
		} InvalidInput;
		struct {
		} NotADirectory;
		struct {
		} IsADirectory;
		struct {
		} WouldBlock;
		struct {
		} PathTooLong;
		struct {
		} Unsupported;
		struct {
			const char* msg;
		} Other;
	};
};
struct std_stdResult__cgs__31 {
	int __chx__vt_621827;
	union {
		struct {
			size_t value;
		} Ok;
		struct {
			struct fs_fsFsError error;
		} Err;
	};
};
struct fs_fsFile {
	struct {
		int fd;
	} _unix;
	_Bool valid;
};
struct std_stdResult__cgs__32 {
	int __chx__vt_621827;
	union {
		struct {
			struct fs_fsFile value;
		} Ok;
		struct {
			struct fs_fsFsError error;
		} Err;
	};
};
struct fs_fsMetadata {
	_Bool is_file;
	_Bool is_dir;
	_Bool is_symlink;
	size_t len;
	int64_t modified;
	int64_t accessed;
	int64_t created;
	uint32_t perms;
};
struct std_stdResult__cgs__33 {
	int __chx__vt_621827;
	union {
		struct {
			struct fs_fsMetadata value;
		} Ok;
		struct {
			struct fs_fsFsError error;
		} Err;
	};
};
struct fs_UnitTy {
};
struct std_stdResult__cgs__34 {
	int __chx__vt_621827;
	union {
		struct {
			struct fs_UnitTy value;
		} Ok;
		struct {
			struct fs_fsFsError error;
		} Err;
	};
};
struct std_stdResult__cgs__35 {
	int __chx__vt_621827;
	union {
		struct {
			_Bool value;
		} Ok;
		struct {
			struct fs_fsFsError error;
		} Err;
	};
};
struct std_stdResult__cgs__36 {
	int __chx__vt_621827;
	union {
		struct {
			struct std_stdvector__cgs__3 value;
		} Ok;
		struct {
			struct fs_fsFsError error;
		} Err;
	};
};
void std_stdResult__cgs__36delete(struct std_stdResult__cgs__36* self);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/src/path.ch **/
typedef const char* fs_fspath_ptr;
typedef char* fs_fsmut_path_ptr;
void fs_fsbasename(struct std_stdResult__cgs__31* __chx_struct_ret_param_xx, const char* path, char* out, size_t out_len);
void fs_fsdirname(struct std_stdResult__cgs__31* __chx_struct_ret_param_xx, const char* path, char* out, size_t out_len);
void fs_fsextension(struct std_stdResult__cgs__31* __chx_struct_ret_param_xx, const char* path, char* out, size_t out_len);
void fs_fsjoin_path(struct std_stdResult__cgs__31* __chx_struct_ret_param_xx, const char* a, const char* b, char* out, size_t out_len);
void fs_fsnormalize_path(struct std_stdResult__cgs__31* __chx_struct_ret_param_xx, const char* path_in, char* out_buf, size_t out_len);
static int fs_fsfind_last_pos_of_or(const char* str, size_t len, char value, char value2);
void fs_fsparent_path_view(struct std_stdstring* __chx_struct_ret_param_xx, struct std_stdstring_view* str);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/src/utf.ch **/
static void fs_fsutf8_to_utf16(struct std_stdResult__cgs__31* __chx_struct_ret_param_xx, const char* in_utf8, uint16_t* out_w, size_t out_w_len);
static void fs_fsutf16_to_utf8(struct std_stdResult__cgs__31* __chx_struct_ret_param_xx, const uint16_t* in_w, char* out, size_t out_len);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/src/types.ch **/
void fs_fsFsErrormessage(struct std_stdstring* __chx_struct_ret_param_xx, struct fs_fsFsError*const self);
struct fs_fsOpenOptions {
	_Bool read;
	_Bool write;
	_Bool append;
	_Bool create;
	_Bool create_new;
	_Bool truncate;
	_Bool binary;
};
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/src/main.ch **/
void fs_UnitTymake(struct fs_UnitTy* this);






/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/src/metadata.ch **/
_Bool fs_fsexists(const char* path);
void fs_fsis_file(struct std_stdResult__cgs__35* __chx_struct_ret_param_xx, const char* path);
void fs_fsis_dir(struct std_stdResult__cgs__35* __chx_struct_ret_param_xx, const char* path);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/src/directory.ch **/
void fs_fscopy_directory(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, const char* src, const char* dst, _Bool preserve_metadata);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/src/error_helpers.ch **/
static void fs_fsposix_errno_to_fs(struct fs_fsFsError* __chx_struct_ret_param_xx, int e);
static void fs_fswinerr_to_fs(struct fs_fsFsError* __chx_struct_ret_param_xx, int code);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/src/file_io.ch **/
static void fs_fsfile_read_exact(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, struct fs_fsFile* f, uint8_t* buf, size_t buf_len);
static void fs_fsfile_write_all(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, struct fs_fsFile* f, const uint8_t* buf, size_t buf_len);
static size_t fs_fsint_to_str(int v, char* out, size_t out_len);
void fs_fsatomic_write(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, const char* path, const uint8_t* data, size_t data_len);
void fs_fsread_entire_file(struct std_stdResult__cgs__36* __chx_struct_ret_param_xx, const char* path);
void fs_fsread_to_buffer(struct std_stdResult__cgs__31* __chx_struct_ret_param_xx, const char* path, uint8_t* buf, size_t buf_len);
void fs_fswrite_text_file(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, const char* path, const uint8_t* data, size_t data_len);
void fs_fsmove_path(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, const char* src, const char* dst);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/posix/permission.ch **/
void fs_fsset_permissions_native(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, fs_fspath_ptr path, uint32_t mode);
void fs_fsset_permissions(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, const char* path, uint32_t mode);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/posix/directory.ch **/
static int fs_posix_mkdir(const char* pathname, unsigned int mode);
void fs_fscreate_dir_native(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, fs_fspath_ptr path);
void fs_fscreate_dir_all(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, const char* path);
void fs_fsremove_dir_native(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, fs_fspath_ptr path);
static void fs_fsremove_dir_all_at(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, int dirfd);
void fs_fsremove_dir_all_recursive_native(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, fs_fspath_ptr path);
void fs_fstemp_dir(struct std_stdResult__cgs__31* __chx_struct_ret_param_xx, char* out, size_t out_len);
void fs_fsread_dir(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, const char* path, std_stdfunction* callback);
void fs_fscreate_dir(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, const char* path);
void fs_fsremove_dir(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, const char* path);
void fs_fsremove_dir_all_recursive(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, const char* path);
int fs_fsmkdir(const char* pathname);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/posix/link.ch **/
static void fs_fscreate_hard_link_native(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, fs_fspath_ptr existing, fs_fspath_ptr newpath);
static void fs_fscreate_symlink_native(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, fs_fspath_ptr target, fs_fspath_ptr linkpath, _Bool dir);
static void fs_fsread_link_native(struct std_stdResult__cgs__31* __chx_struct_ret_param_xx, fs_fspath_ptr path, char* out, size_t out_len);
static void fs_fsis_symlink(struct std_stdResult__cgs__35* __chx_struct_ret_param_xx, const char* path);
static void fs_fscreate_symlink(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, const char* target, const char* linkpath, _Bool dir);
static void fs_fsread_link(struct std_stdResult__cgs__31* __chx_struct_ret_param_xx, const char* path, char* out, size_t out_len);
static void fs_fscreate_hard_link(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, const char* existing, const char* newpath);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/posix/path.ch **/
static void fs_fscanonicalize(struct std_stdResult__cgs__31* __chx_struct_ret_param_xx, const char* path, char* out, size_t out_len);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/posix/file_io.ch **/
static void fs_fsfile_open_native(struct std_stdResult__cgs__32* __chx_struct_ret_param_xx, fs_fspath_ptr path, struct fs_fsOpenOptions* opts);
void fs_fsfile_close(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, struct fs_fsFile* f);
void fs_fsfile_read(struct std_stdResult__cgs__31* __chx_struct_ret_param_xx, struct fs_fsFile* f, uint8_t* buf, size_t buf_len);
void fs_fsfile_write(struct std_stdResult__cgs__31* __chx_struct_ret_param_xx, struct fs_fsFile* f, const uint8_t* buf, size_t buf_len);
void fs_fsfile_flush(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, struct fs_fsFile* f);
void fs_fsremove_file_native(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, fs_fspath_ptr path);
static void fs_fsset_times_native(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, fs_fspath_ptr path, int64_t atime, int64_t mtime);
static void fs_fscopy_file_native(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, fs_fspath_ptr src, fs_fspath_ptr dst);
static void fs_fscreate_temp_file_in_native(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, fs_fspath_ptr dir, fs_fspath_ptr prefix, fs_fsmut_path_ptr out_path, struct fs_fsFile* fh);
void fs_fsremove_file(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, const char* path);
void fs_fscopy_file(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, const char* src, const char* dst);
static void fs_fscreate_temp_file_in(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, const char* dir, const char* prefix, char* out_path, size_t out_len, struct fs_fsFile* fh);
void fs_fsfile_open(struct std_stdResult__cgs__32* __chx_struct_ret_param_xx, const char* path, struct fs_fsOpenOptions* opts);
void fs_fsset_times(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, const char* path, int64_t atime, int64_t mtime);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/posix/lock.ch **/
static void fs_fslock_file_shared(struct std_stdResult__cgs__32* __chx_struct_ret_param_xx, const char* path);
static void fs_fslock_file_exclusive(struct std_stdResult__cgs__32* __chx_struct_ret_param_xx, const char* path);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/posix/metadata.ch **/
void fs_fsmetadata(struct std_stdResult__cgs__33* __chx_struct_ret_param_xx, const char* path);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/posix/disk.ch **/
static void fs_fsdisk_space_native(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, fs_fspath_ptr path, uint64_t* total_out, uint64_t* free_out, uint64_t* avail_out);
static void fs_fsdisk_space(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, const char* path, uint64_t* total_out, uint64_t* free_out, uint64_t* avail_out);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/posix/api.ch **/
typedef unsigned long fs___fsblkcnt_t;
typedef unsigned long fs___fsfilcnt_t;
typedef unsigned long fs___fsblkcnt_t;
typedef unsigned long fs___fsfilcnt_t;
struct statvfs {
	unsigned long f_bsize;
	unsigned long f_frsize;
	fs___fsblkcnt_t f_blocks;
	fs___fsblkcnt_t f_bfree;
	fs___fsblkcnt_t f_bavail;
	fs___fsfilcnt_t f_files;
	fs___fsfilcnt_t f_ffree;
	fs___fsfilcnt_t f_favail;
	unsigned long f_fsid;
	unsigned long f_flag;
	unsigned long f_namemax;
	unsigned int f_type;
	int __f_spare[5];
};
extern const char* realpath(const char* path, char* resolved);
extern int chmod(const char* path, uint32_t mode);
static int fs_fsAT_FDCWD;
static int fs_fsAT_SYMLINK_NOFOLLOW;
extern int utimensat(int dirfd, const char* pathname, const struct timespec* times, int flags);
extern int fsync(int fd);
extern int link(const char* oldpath, const char* newpath);
extern int symlink(const char* target, const char* linkpath);
extern int mkstemp(char* template);
typedef uint64_t cstd_fsblkcnt_t;
typedef uint64_t cstd_fsfilcnt_t;
struct fs_fsStatvfs {
	unsigned long f_bsize;
	unsigned long f_frsize;
	cstd_fsblkcnt_t f_blocks;
	cstd_fsblkcnt_t f_bfree;
	cstd_fsblkcnt_t f_bavail;
	cstd_fsfilcnt_t f_files;
	cstd_fsfilcnt_t f_ffree;
	cstd_fsfilcnt_t f_favail;
	unsigned long f_fsid;
	unsigned long f_flag;
	unsigned long f_namemax;
};
extern int statvfs(const char* path, struct fs_fsStatvfs* out);
extern int flock(int fd, int operation);
static int fs_fsLOCK_SH;
static int fs_fsLOCK_EX;
static int fs_fsLOCK_NB;
static int fs_fsLOCK_UN;
typedef isize fs_fsssize_t;
extern fs_fsssize_t readlink(const char* path, char* buf, size_t bufsiz);
extern int unlink(const char* path);
extern int rmdir(const char* path);
extern int openat(int dirfd, const char* path, int flags, uint32_t mode);
extern int dup(int fd);
extern struct cstd_DIR* fdopendir(int fd);
extern int fstatat(int dirfd, const char* pathname, struct cstd_Stat* out, int flags);
extern int unlinkat(int dirfd, const char* pathname, int flags);
static int fs_fsO_EXCL;
static int fs_fsO_APPEND;
static int fs_fsO_DIRECTORY;
static int fs_fsAT_REMOVEDIR;
/** Implement:Generics fs **/
void std_stdResult__cgs__36delete(struct std_stdResult__cgs__36* self){
	__chx__dstctr_clnup_blk__:{
		switch(self->__chx__vt_621827) {
			case 0:
			std_stdvector__cgs__3delete(&self->Ok.value);
			break;
			case 1:
			break;
		}
	}
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/src/path.ch **/
void fs_fsbasename(struct std_stdResult__cgs__31* __chx_struct_ret_param_xx, const char* path, char* out, size_t out_len){
	size_t len = 0;
	while((path[len] != 0)) {
		len++;
	}
	if((len == 0)){
		if((out_len < 2)){
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 1, 
				.Err.error = (struct fs_fsFsError) { .__chx__vt_621827 = 8, 
				}
			};
			return;
		}
		out[0] = '.';
		out[1] = 0;
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 0, 
			.Ok.value = 1
		};
		return;
	}
	size_t i = len;
	while(((i > 0) && ((path[(i - 1)] == '/') || (path[(i - 1)] == '\\')))) {
		i--;
	}
	if((i == 0)){
		if((out_len < 2)){
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 1, 
				.Err.error = (struct fs_fsFsError) { .__chx__vt_621827 = 8, 
				}
			};
			return;
		}
		out[0] = '/';
		out[1] = 0;
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 0, 
			.Ok.value = 1
		};
		return;
	}
	size_t j = i;
	while((((j > 0) && (path[(j - 1)] != '/')) && (path[(j - 1)] != '\\'))) {
		j--;
	}
	unsigned long comp_len = (i - j);
	if(((comp_len + 1) > out_len)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 1, 
			.Err.error = (struct fs_fsFsError) { .__chx__vt_621827 = 8, 
			}
		};
		return;
	}
	size_t k = 0;
	while((k < comp_len)) {
		out[k] = path[(j + k)];
		k++;
	}
	out[k] = 0;
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 0, 
		.Ok.value = comp_len
	};
	return;
}
void fs_fsdirname(struct std_stdResult__cgs__31* __chx_struct_ret_param_xx, const char* path, char* out, size_t out_len){
	size_t len = 0;
	while((path[len] != 0)) {
		len++;
	}
	if((len == 0)){
		if((out_len < 2)){
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 1, 
				.Err.error = (struct fs_fsFsError) { .__chx__vt_621827 = 8, 
				}
			};
			return;
		}
		out[0] = '.';
		out[1] = 0;
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 0, 
			.Ok.value = 1
		};
		return;
	}
	size_t i = len;
	while(((i > 0) && ((path[(i - 1)] == '/') || (path[(i - 1)] == '\\')))) {
		i--;
	}
	if((i == 0)){
		if((out_len < 2)){
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 1, 
				.Err.error = (struct fs_fsFsError) { .__chx__vt_621827 = 8, 
				}
			};
			return;
		}
		out[0] = '/';
		out[1] = 0;
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 0, 
			.Ok.value = 1
		};
		return;
	}
	size_t j = i;
	while((((j > 0) && (path[(j - 1)] != '/')) && (path[(j - 1)] != '\\'))) {
		j--;
	}
	if((j == 0)){
		if((out_len < 2)){
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 1, 
				.Err.error = (struct fs_fsFsError) { .__chx__vt_621827 = 8, 
				}
			};
			return;
		}
		out[0] = '.';
		out[1] = 0;
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 0, 
			.Ok.value = 1
		};
		return;
	}
	size_t end = j;
	while(((end > 0) && ((path[(end - 1)] == '/') || (path[(end - 1)] == '\\')))) {
		end--;
	}
	if((end == 0)){
		if((out_len < 2)){
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 1, 
				.Err.error = (struct fs_fsFsError) { .__chx__vt_621827 = 8, 
				}
			};
			return;
		}
		out[0] = '/';
		out[1] = 0;
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 0, 
			.Ok.value = 1
		};
		return;
	}
	if(((end + 1) > out_len)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 1, 
			.Err.error = (struct fs_fsFsError) { .__chx__vt_621827 = 8, 
			}
		};
		return;
	}
	size_t k = 0;
	while((k < end)) {
		out[k] = path[k];
		k++;
	}
	out[k] = 0;
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 0, 
		.Ok.value = end
	};
	return;
}
void fs_fsextension(struct std_stdResult__cgs__31* __chx_struct_ret_param_xx, const char* path, char* out, size_t out_len){
	char base_buf[4096];
	struct std_stdResult__cgs__31 r = (*({ struct std_stdResult__cgs__31 __chx__lv__0; fs_fsbasename(&__chx__lv__0, path, &base_buf[0], ((size_t) 4096)); &__chx__lv__0; }));
	if((r.__chx__vt_621827 == 1)){
		struct std_stdResult__cgs__31* __chx__lv__1 = &r;
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 1, 
			.Err.error = __chx__lv__1->Err.error
		};
		return;
	}
	struct std_stdResult__cgs__31* __chx__lv__2 = &r;
	size_t i = __chx__lv__2->Ok.value;
	while(((i > 0) && (base_buf[(i - 1)] != '.'))) {
		i--;
	}
	if((i == 0)){
		if((out_len < 1)){
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 1, 
				.Err.error = (struct fs_fsFsError) { .__chx__vt_621827 = 8, 
				}
			};
			return;
		}
		out[0] = 0;
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 0, 
			.Ok.value = 0
		};
		return;
	}
	unsigned long ext_len = (__chx__lv__2->Ok.value - i);
	if(((ext_len + 1) > out_len)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 1, 
			.Err.error = (struct fs_fsFsError) { .__chx__vt_621827 = 8, 
			}
		};
		return;
	}
	size_t k = 0;
	while((k < ext_len)) {
		out[k] = base_buf[(i + k)];
		k++;
	}
	out[k] = 0;
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 0, 
		.Ok.value = ext_len
	};
	return;
}
void fs_fsjoin_path(struct std_stdResult__cgs__31* __chx_struct_ret_param_xx, const char* a, const char* b, char* out, size_t out_len){
	size_t a_len = 0;
	while((a[a_len] != 0)) {
		a_len++;
	}
	size_t b_len = 0;
	while((b[b_len] != 0)) {
		b_len++;
	}
	if((a_len == 0)){
		if(((b_len + 1) > out_len)){
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 1, 
				.Err.error = (struct fs_fsFsError) { .__chx__vt_621827 = 8, 
				}
			};
			return;
		}
		size_t i = 0;
		while((i <= b_len)) {
			out[i] = b[i];
			i++;
		}
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 0, 
			.Ok.value = b_len
		};
		return;
	}
	_Bool need_sep = 0;
	if(((a[(a_len - 1)] != '/') && (a[(a_len - 1)] != '\\'))){
		need_sep = 1;
	}
	unsigned long total = ((a_len + need_sep ? ({ 
	1; }) : ({ 
	0; })) + b_len);
	if(((total + 1) > out_len)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 1, 
			.Err.error = (struct fs_fsFsError) { .__chx__vt_621827 = 8, 
			}
		};
		return;
	}
	size_t pos = 0;
	size_t i = 0;
	while((i < a_len)) {
		out[pos++] = a[i++];
	}
	if(need_sep){
		out[pos++] = '/';
	}
	i = 0;
	while((i <= b_len)) {
		out[pos++] = b[i++];
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 0, 
		.Ok.value = total
	};
	return;
}
void fs_fsnormalize_path(struct std_stdResult__cgs__31* __chx_struct_ret_param_xx, const char* path_in, char* out_buf, size_t out_len){
	size_t len = 0;
	while((path_in[len] != 0)) {
		len++;
	}
	int MAX_COMPS = 512;
	size_t offs[512];
	size_t lens[512];
	size_t count = 0;
	size_t i = 0;
	while((i < len)) {
		while(((i < len) && ((path_in[i] == '/') || (path_in[i] == '\\')))) {
			i++;
		}
		if((i >= len)){
			break;
		}
		size_t start = i;
		while((((i < len) && (path_in[i] != '/')) && (path_in[i] != '\\'))) {
			i++;
		}
		unsigned long c_len = (i - start);
		if(((c_len == 1) && (path_in[start] == '.'))){
		}else if((((c_len == 2) && (path_in[start] == '.')) && (path_in[(start + 1)] == '.'))){
			if((count > 0)){
				count -= 1;
			} else {
				if((count >= MAX_COMPS)){
					*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 1, 
						.Err.error = (struct fs_fsFsError) { .__chx__vt_621827 = 8, 
						}
					};
					return;
				}
				offs[count] = start;
				lens[count] = c_len;
				count++;
			}
		} else {
			if((count >= MAX_COMPS)){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 1, 
					.Err.error = (struct fs_fsFsError) { .__chx__vt_621827 = 8, 
					}
				};
				return;
			}
			offs[count] = start;
			lens[count] = c_len;
			count++;
		}
	}
	if((count == 0)){
		if((out_len < 2)){
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 1, 
				.Err.error = (struct fs_fsFsError) { .__chx__vt_621827 = 8, 
				}
			};
			return;
		}
		out_buf[0] = '.';
		out_buf[1] = 0;
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 0, 
			.Ok.value = 1
		};
		return;
	}
	size_t pos = 0;
	size_t j = 0;
	while((j < count)) {
		if((j > 0)){
			if(((pos + 1) >= out_len)){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 1, 
					.Err.error = (struct fs_fsFsError) { .__chx__vt_621827 = 8, 
					}
				};
				return;
			}
			out_buf[pos++] = '/';
		}
		size_t off = offs[j];
		size_t l = lens[j];
		if(((pos + l) >= out_len)){
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 1, 
				.Err.error = (struct fs_fsFsError) { .__chx__vt_621827 = 8, 
				}
			};
			return;
		}
		size_t k = 0;
		while((k < l)) {
			out_buf[(pos + k)] = path_in[(off + k)];
			k++;
		}
		pos += l;
		j++;
	}
	if((pos >= out_len)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 1, 
			.Err.error = (struct fs_fsFsError) { .__chx__vt_621827 = 8, 
			}
		};
		return;
	}
	out_buf[pos] = 0;
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 0, 
		.Ok.value = pos
	};
	return;
}
static int fs_fsfind_last_pos_of_or(const char* str, size_t len, char value, char value2){
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
void fs_fsparent_path_view(struct std_stdstring* __chx_struct_ret_param_xx, struct std_stdstring_view* str){
	struct std_stdstring final = (*({ struct std_stdstring __chx__lv__3; std_stdstringempty_str(&__chx__lv__3); &__chx__lv__3; }));
	_Bool __chx__lv__4 = true;
	int pos;
	
	pos = fs_fsfind_last_pos_of_or(std_stdstring_viewdata(str), std_stdstring_viewsize(str), '/', '/');
	if((pos > 0)){
		std_stdstringappend_with_len(&final, std_stdstring_viewdata(str), ((size_t) (pos + 1)));
	}
	*__chx_struct_ret_param_xx = final;
	return;
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/src/utf.ch **/
static void fs_fsutf8_to_utf16(struct std_stdResult__cgs__31* __chx_struct_ret_param_xx, const char* in_utf8, uint16_t* out_w, size_t out_w_len){
	size_t i = 0;
	size_t wpos = 0;
	while((in_utf8[i] != 0)) {
		uint8_t c = ((uint8_t) in_utf8[i]);
		if((c < 128)){
			if(((wpos + 1) >= out_w_len)){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 1, 
					.Err.error = (struct fs_fsFsError) { .__chx__vt_621827 = 8, 
					}
				};
				return;
			}
			out_w[wpos++] = ((uint16_t) c);
			i += 1;
			continue;
		}
		if(((c & 224) == 192)){
			if((in_utf8[(i + 1)] == 0)){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 1, 
					.Err.error = (struct fs_fsFsError) { .__chx__vt_621827 = 4, 
					}
				};
				return;
			}
			uint8_t c2 = ((uint8_t) in_utf8[(i + 1)]);
			if(((c2 & 192) != 128)){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 1, 
					.Err.error = (struct fs_fsFsError) { .__chx__vt_621827 = 4, 
					}
				};
				return;
			}
			uint32_t code = ((((uint32_t) (c & 31)) << 6) | ((uint32_t) (c2 & 63)));
			if((code < 128)){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 1, 
					.Err.error = (struct fs_fsFsError) { .__chx__vt_621827 = 4, 
					}
				};
				return;
			}
			if(((wpos + 1) >= out_w_len)){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 1, 
					.Err.error = (struct fs_fsFsError) { .__chx__vt_621827 = 8, 
					}
				};
				return;
			}
			out_w[wpos++] = ((uint16_t) code);
			i += 2;
			continue;
		}
		if(((c & 240) == 224)){
			if(((in_utf8[(i + 1)] == 0) || (in_utf8[(i + 2)] == 0))){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 1, 
					.Err.error = (struct fs_fsFsError) { .__chx__vt_621827 = 4, 
					}
				};
				return;
			}
			uint8_t c2 = ((uint8_t) in_utf8[(i + 1)]);
			uint8_t c3 = ((uint8_t) in_utf8[(i + 2)]);
			if((((c2 & 192) != 128) || ((c3 & 192) != 128))){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 1, 
					.Err.error = (struct fs_fsFsError) { .__chx__vt_621827 = 4, 
					}
				};
				return;
			}
			uint32_t code = (((((uint32_t) (c & 15)) << 12) | (((uint32_t) (c2 & 63)) << 6)) | ((uint32_t) (c3 & 63)));
			if((code < 2048)){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 1, 
					.Err.error = (struct fs_fsFsError) { .__chx__vt_621827 = 4, 
					}
				};
				return;
			}
			if(((code >= 55296) && (code <= 57343))){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 1, 
					.Err.error = (struct fs_fsFsError) { .__chx__vt_621827 = 4, 
					}
				};
				return;
			}
			if(((wpos + 1) >= out_w_len)){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 1, 
					.Err.error = (struct fs_fsFsError) { .__chx__vt_621827 = 8, 
					}
				};
				return;
			}
			out_w[wpos++] = ((uint16_t) code);
			i += 3;
			continue;
		}
		if(((c & 248) == 240)){
			if((((in_utf8[(i + 1)] == 0) || (in_utf8[(i + 2)] == 0)) || (in_utf8[(i + 3)] == 0))){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 1, 
					.Err.error = (struct fs_fsFsError) { .__chx__vt_621827 = 4, 
					}
				};
				return;
			}
			uint8_t c2 = ((uint8_t) in_utf8[(i + 1)]);
			uint8_t c3 = ((uint8_t) in_utf8[(i + 2)]);
			uint8_t c4 = ((uint8_t) in_utf8[(i + 3)]);
			if(((((c2 & 192) != 128) || ((c3 & 192) != 128)) || ((c4 & 192) != 128))){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 1, 
					.Err.error = (struct fs_fsFsError) { .__chx__vt_621827 = 4, 
					}
				};
				return;
			}
			uint32_t code = ((((((uint32_t) (c & 7)) << 18) | (((uint32_t) (c2 & 63)) << 12)) | (((uint32_t) (c3 & 63)) << 6)) | ((uint32_t) (c4 & 63)));
			if(((code < 65536) || (code > 1114111))){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 1, 
					.Err.error = (struct fs_fsFsError) { .__chx__vt_621827 = 4, 
					}
				};
				return;
			}
			code -= 65536;
			uint16_t high = (55296 + ((uint16_t) (code >> 10)));
			uint16_t low = (56320 + ((uint16_t) (code & 1023)));
			if(((wpos + 2) >= out_w_len)){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 1, 
					.Err.error = (struct fs_fsFsError) { .__chx__vt_621827 = 8, 
					}
				};
				return;
			}
			out_w[wpos++] = high;
			out_w[wpos++] = low;
			i += 4;
			continue;
		}
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 1, 
			.Err.error = (struct fs_fsFsError) { .__chx__vt_621827 = 4, 
			}
		};
		return;
	}
	if((wpos >= out_w_len)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 1, 
			.Err.error = (struct fs_fsFsError) { .__chx__vt_621827 = 8, 
			}
		};
		return;
	}
	out_w[wpos] = 0;
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 0, 
		.Ok.value = wpos
	};
	return;
}
static void fs_fsutf16_to_utf8(struct std_stdResult__cgs__31* __chx_struct_ret_param_xx, const uint16_t* in_w, char* out, size_t out_len){
	size_t i = 0;
	size_t pos = 0;
	while(1) {
		uint16_t w = in_w[i];
		if((w == 0)){
			break;
		}
		if((w < 128)){
			if(((pos + 1) >= out_len)){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 1, 
					.Err.error = (struct fs_fsFsError) { .__chx__vt_621827 = 8, 
					}
				};
				return;
			}
			out[pos++] = ((char) w);
		}else if((w < 2048)){
			if(((pos + 2) >= out_len)){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 1, 
					.Err.error = (struct fs_fsFsError) { .__chx__vt_621827 = 8, 
					}
				};
				return;
			}
			out[pos++] = ((char) (192 | ((w >> 6) & 31)));
			out[pos++] = ((char) (128 | (w & 63)));
		}else if(((w >= 55296) && (w <= 56319))){
			uint16_t w2 = in_w[(i + 1)];
			uint32_t code = (65536 + ((((uint32_t) (w & 1023)) << 10) | ((uint32_t) (w2 & 1023))));
			if(((pos + 4) >= out_len)){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 1, 
					.Err.error = (struct fs_fsFsError) { .__chx__vt_621827 = 8, 
					}
				};
				return;
			}
			out[pos++] = ((char) (240 | ((code >> 18) & 7)));
			out[pos++] = ((char) (128 | ((code >> 12) & 63)));
			out[pos++] = ((char) (128 | ((code >> 6) & 63)));
			out[pos++] = ((char) (128 | (code & 63)));
			i += 1;
		} else {
			if(((pos + 3) >= out_len)){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 1, 
					.Err.error = (struct fs_fsFsError) { .__chx__vt_621827 = 8, 
					}
				};
				return;
			}
			out[pos++] = ((char) (224 | ((w >> 12) & 15)));
			out[pos++] = ((char) (128 | ((w >> 6) & 63)));
			out[pos++] = ((char) (128 | (w & 63)));
		}
		i++;
	}
	if((pos >= out_len)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 1, 
			.Err.error = (struct fs_fsFsError) { .__chx__vt_621827 = 8, 
			}
		};
		return;
	}
	out[pos] = 0;
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 0, 
		.Ok.value = pos
	};
	return;
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/src/types.ch **/
void fs_fsFsErrormessage(struct std_stdstring* __chx_struct_ret_param_xx, struct fs_fsFsError*const self){
	switch((self)->__chx__vt_621827) {
		case 0:{
			struct std_stdstring msg = (*({ struct std_stdstring __chx__lv__5; std_stdstringconstructor2(&__chx__lv__5, "io error with code ", 19, 0); &__chx__lv__5; }));
			_Bool __chx__lv__6 = true;
			std_stdstringappend_integer(&msg, (self)->Io.code);
			std_stdstringappend_view(&msg, &(*({ struct std_stdstring_view __chx__lv__7; std_stdstring_viewconstructor(&__chx__lv__7, " and message \'", 14); &__chx__lv__7; })));
			std_stdstringappend_char_ptr(&msg, (self)->Io.message);
			std_stdstringappend(&msg, '\'');
			*__chx_struct_ret_param_xx = msg;
			return;
			break;
		}
		case 1:{
			*__chx_struct_ret_param_xx = (*({ struct std_stdstring __chx__lv__8; std_stdstringconstructor2(&__chx__lv__8, "FsError.NotFound", 16, 0); &__chx__lv__8; }));
			return;
			break;
		}
		case 2:{
			*__chx_struct_ret_param_xx = (*({ struct std_stdstring __chx__lv__9; std_stdstringconstructor2(&__chx__lv__9, "FsError.AlreadyExists", 21, 0); &__chx__lv__9; }));
			return;
			break;
		}
		case 3:{
			*__chx_struct_ret_param_xx = (*({ struct std_stdstring __chx__lv__10; std_stdstringconstructor2(&__chx__lv__10, "FsError.PermissionDenied", 24, 0); &__chx__lv__10; }));
			return;
			break;
		}
		case 4:{
			*__chx_struct_ret_param_xx = (*({ struct std_stdstring __chx__lv__11; std_stdstringconstructor2(&__chx__lv__11, "FsError.InvalidInput", 20, 0); &__chx__lv__11; }));
			return;
			break;
		}
		case 5:{
			*__chx_struct_ret_param_xx = (*({ struct std_stdstring __chx__lv__12; std_stdstringconstructor2(&__chx__lv__12, "FsError.NotADirectory", 21, 0); &__chx__lv__12; }));
			return;
			break;
		}
		case 6:{
			*__chx_struct_ret_param_xx = (*({ struct std_stdstring __chx__lv__13; std_stdstringconstructor2(&__chx__lv__13, "FsError.IsADirectory", 20, 0); &__chx__lv__13; }));
			return;
			break;
		}
		case 7:{
			*__chx_struct_ret_param_xx = (*({ struct std_stdstring __chx__lv__14; std_stdstringconstructor2(&__chx__lv__14, "FsError.WouldBlock", 18, 0); &__chx__lv__14; }));
			return;
			break;
		}
		case 8:{
			*__chx_struct_ret_param_xx = (*({ struct std_stdstring __chx__lv__15; std_stdstringconstructor2(&__chx__lv__15, "FsError.PathTooLong", 19, 0); &__chx__lv__15; }));
			return;
			break;
		}
		case 9:{
			*__chx_struct_ret_param_xx = (*({ struct std_stdstring __chx__lv__16; std_stdstringconstructor2(&__chx__lv__16, "FsError.Unsupported", 19, 0); &__chx__lv__16; }));
			return;
			break;
		}
		case 10:{
			*__chx_struct_ret_param_xx = (*({ struct std_stdstring __chx__lv__17; std_stdstringmake_no_len(&__chx__lv__17, (self)->Other.msg); &__chx__lv__17; }));
			return;
			break;
		}
	}
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/src/main.ch **/
void fs_UnitTymake(struct fs_UnitTy* this){
}






/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/src/metadata.ch **/
_Bool fs_fsexists(const char* path){
	struct std_stdResult__cgs__33 r = (*({ struct std_stdResult__cgs__33 __chx__lv__18; fs_fsmetadata(&__chx__lv__18, path); &__chx__lv__18; }));
	return !(r.__chx__vt_621827 == 1);
}
void fs_fsis_file(struct std_stdResult__cgs__35* __chx_struct_ret_param_xx, const char* path){
	struct std_stdResult__cgs__33 r = (*({ struct std_stdResult__cgs__33 __chx__lv__19; fs_fsmetadata(&__chx__lv__19, path); &__chx__lv__19; }));
	if((r.__chx__vt_621827 == 1)){
		struct std_stdResult__cgs__33* __chx__lv__20 = &r;
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__35) { .__chx__vt_621827 = 1, 
			.Err.error = __chx__lv__20->Err.error
		};
		return;
	}
	struct std_stdResult__cgs__33* __chx__lv__21 = &r;
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__35) { .__chx__vt_621827 = 0, 
		.Ok.value = __chx__lv__21->Ok.value.is_file
	};
	return;
}
void fs_fsis_dir(struct std_stdResult__cgs__35* __chx_struct_ret_param_xx, const char* path){
	struct std_stdResult__cgs__33 r = (*({ struct std_stdResult__cgs__33 __chx__lv__22; fs_fsmetadata(&__chx__lv__22, path); &__chx__lv__22; }));
	if((r.__chx__vt_621827 == 1)){
		struct std_stdResult__cgs__33* __chx__lv__23 = &r;
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__35) { .__chx__vt_621827 = 1, 
			.Err.error = __chx__lv__23->Err.error
		};
		return;
	}
	struct std_stdResult__cgs__33* __chx__lv__24 = &r;
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__35) { .__chx__vt_621827 = 0, 
		.Ok.value = __chx__lv__24->Ok.value.is_dir
	};
	return;
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/src/directory.ch **/
struct __chemda_356_1_cap {
	const char* src;
	const char* dst;
	_Bool preserve_metadata;
};
static inline void __chemda_356_1_cap_destr(struct __chemda_356_1_cap* self) {}
static _Bool __chemda_356_1(void* this, const char* name, size_t name_len, _Bool is_dir){
	if(((name_len == 1) && (name[0] == '.'))){
		return 1;
	}
	if((((name_len == 2) && (name[0] == '.')) && (name[1] == '.'))){
		return 1;
	}
	char srcchild[4096];
	char dstchild[4096];
	size_t p = 0;
	while((((struct __chemda_356_1_cap*) this)->src[p] != 0)) {
		srcchild[p] = ((struct __chemda_356_1_cap*) this)->src[p];
		p++;
	}
	if(((p > 0) && (srcchild[(p - 1)] != '/'))){
		srcchild[p++] = '/';
	}
	size_t q = 0;
	while((q <= name_len)) {
		srcchild[(p + q)] = name[q];
		q++;
	}
	size_t r = 0;
	while((((struct __chemda_356_1_cap*) this)->dst[r] != 0)) {
		dstchild[r] = ((struct __chemda_356_1_cap*) this)->dst[r];
		r++;
	}
	if(((r > 0) && (dstchild[(r - 1)] != '/'))){
		dstchild[r++] = '/';
	}
	q = 0;
	while((q <= name_len)) {
		dstchild[(r + q)] = name[q];
		q++;
	}
	if(is_dir){
		struct std_stdResult__cgs__34 c = (*({ struct std_stdResult__cgs__34 __chx__lv__31; fs_fscopy_directory(&__chx__lv__31, &srcchild[0], &dstchild[0], ((struct __chemda_356_1_cap*) this)->preserve_metadata); &__chx__lv__31; }));
		if((c.__chx__vt_621827 == 1)){
			struct std_stdResult__cgs__34* __chx__lv__32 = &c;
			return 0;
		}
	} else {
		struct std_stdResult__cgs__34 c = (*({ struct std_stdResult__cgs__34 __chx__lv__33; fs_fscopy_file(&__chx__lv__33, &srcchild[0], &dstchild[0]); &__chx__lv__33; }));
		if((c.__chx__vt_621827 == 1)){
			struct std_stdResult__cgs__34* __chx__lv__34 = &c;
			return 0;
		}
		if(((struct __chemda_356_1_cap*) this)->preserve_metadata){
			struct std_stdResult__cgs__33 meta = (*({ struct std_stdResult__cgs__33 __chx__lv__35; fs_fsmetadata(&__chx__lv__35, &srcchild[0]); &__chx__lv__35; }));
			if((meta.__chx__vt_621827 == 1)){
				struct std_stdResult__cgs__33* __chx__lv__36 = &meta;
				return 0;
			}
			struct std_stdResult__cgs__33* __chx__lv__37 = &meta;
			struct std_stdResult__cgs__34 setp = (*({ struct std_stdResult__cgs__34 __chx__lv__38; fs_fsset_permissions(&__chx__lv__38, &dstchild[0], __chx__lv__37->Ok.value.perms); &__chx__lv__38; }));
			if((setp.__chx__vt_621827 == 1)){
				struct std_stdResult__cgs__34* __chx__lv__39 = &setp;
			}
			struct std_stdResult__cgs__34 stimes = (*({ struct std_stdResult__cgs__34 __chx__lv__40; fs_fsset_times(&__chx__lv__40, &dstchild[0], __chx__lv__37->Ok.value.accessed, __chx__lv__37->Ok.value.modified); &__chx__lv__40; }));
			if((stimes.__chx__vt_621827 == 1)){
				struct std_stdResult__cgs__34* __chx__lv__41 = &stimes;
			}
		}
	}
	return 1;
}
void fs_fscopy_directory(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, const char* src, const char* dst, _Bool preserve_metadata){
	struct std_stdResult__cgs__33 st_res = (*({ struct std_stdResult__cgs__33 __chx__lv__25; fs_fsmetadata(&__chx__lv__25, src); &__chx__lv__25; }));
	if((st_res.__chx__vt_621827 == 1)){
		struct std_stdResult__cgs__33* __chx__lv__26 = &st_res;
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
			.Err.error = __chx__lv__26->Err.error
		};
		return;
	}
	struct std_stdResult__cgs__33* __chx__lv__27 = &st_res;
	if(!__chx__lv__27->Ok.value.is_dir){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
			.Err.error = (struct fs_fsFsError) { .__chx__vt_621827 = 5, 
			}
		};
		return;
	}
	struct std_stdResult__cgs__34 cd = (*({ struct std_stdResult__cgs__34 __chx__lv__28; fs_fscreate_dir_all(&__chx__lv__28, dst); &__chx__lv__28; }));
	if((cd.__chx__vt_621827 == 1)){
		struct std_stdResult__cgs__34* __chx__lv__29 = &cd;
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
			.Err.error = __chx__lv__29->Err.error
		};
		return;
	}
	struct std_stdResult__cgs__34 res = (*({ struct std_stdResult__cgs__34 __chx__lv__30; fs_fsread_dir(&__chx__lv__30, src, &*({    __chemical_fat_pointer__* __chemda_356_1_pair = (&(__chemical_fat_pointer__){__chemda_356_1,(&(struct __chemda_356_1_cap){src,dst,preserve_metadata})}); &(*({ struct std_stddefault_function_instance __chx__lv__42; std_stddefault_function_instancemake2(&__chx__lv__42, __chemda_356_1_pair->first, __chemda_356_1_pair->second, ((std_stddestructor_type) __chemda_356_1_cap_destr), sizeof(struct __chemda_356_1_cap), _Alignof(struct __chemda_356_1_cap)); &__chx__lv__42; })); })); &__chx__lv__30; }));
	if((res.__chx__vt_621827 == 1)){
		struct std_stdResult__cgs__34* __chx__lv__43 = &res;
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
			.Err.error = __chx__lv__43->Err.error
		};
		return;
	}
	struct std_stdResult__cgs__34* __chx__lv__44 = &res;
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 0, 
		.Ok.value = (struct fs_UnitTy){ 
		}
	};
	return;
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/src/error_helpers.ch **/
static void fs_fsposix_errno_to_fs(struct fs_fsFsError* __chx_struct_ret_param_xx, int e){
	if((e == 2)){
		*__chx_struct_ret_param_xx = (struct fs_fsFsError) { .__chx__vt_621827 = 1, 
		};
		return;
	}
	if((e == 13)){
		*__chx_struct_ret_param_xx = (struct fs_fsFsError) { .__chx__vt_621827 = 3, 
		};
		return;
	}
	if((e == 17)){
		*__chx_struct_ret_param_xx = (struct fs_fsFsError) { .__chx__vt_621827 = 2, 
		};
		return;
	}
	if((e == 21)){
		*__chx_struct_ret_param_xx = (struct fs_fsFsError) { .__chx__vt_621827 = 6, 
		};
		return;
	}
	if((e == 20)){
		*__chx_struct_ret_param_xx = (struct fs_fsFsError) { .__chx__vt_621827 = 5, 
		};
		return;
	}
	if((e == 11)){
		*__chx_struct_ret_param_xx = (struct fs_fsFsError) { .__chx__vt_621827 = 7, 
		};
		return;
	}
	*__chx_struct_ret_param_xx = (struct fs_fsFsError) { .__chx__vt_621827 = 0, 
		.Io.code = e, 
		.Io.message = "posix error\0"
	};
	return;
}
static void fs_fswinerr_to_fs(struct fs_fsFsError* __chx_struct_ret_param_xx, int code){
	if((code == 2)){
		*__chx_struct_ret_param_xx = (struct fs_fsFsError) { .__chx__vt_621827 = 1, 
		};
		return;
	}
	if((code == 5)){
		*__chx_struct_ret_param_xx = (struct fs_fsFsError) { .__chx__vt_621827 = 3, 
		};
		return;
	}
	if((code == 80)){
		*__chx_struct_ret_param_xx = (struct fs_fsFsError) { .__chx__vt_621827 = 2, 
		};
		return;
	}
	*__chx_struct_ret_param_xx = (struct fs_fsFsError) { .__chx__vt_621827 = 0, 
		.Io.code = code, 
		.Io.message = "win32 error\0"
	};
	return;
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/src/file_io.ch **/
static void fs_fsfile_read_exact(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, struct fs_fsFile* f, uint8_t* buf, size_t buf_len){
	size_t pos = 0;
	while((pos < buf_len)) {
		struct std_stdResult__cgs__31 r = (*({ struct std_stdResult__cgs__31 __chx__lv__45; fs_fsfile_read(&__chx__lv__45, f, (buf + pos), (buf_len - pos)); &__chx__lv__45; }));
		if((r.__chx__vt_621827 == 1)){
			struct std_stdResult__cgs__31* __chx__lv__46 = &r;
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
				.Err.error = __chx__lv__46->Err.error
			};
			return;
		}
		struct std_stdResult__cgs__31* __chx__lv__47 = &r;
		if((__chx__lv__47->Ok.value == 0)){
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
				.Err.error = (struct fs_fsFsError) { .__chx__vt_621827 = 0, 
					.Io.code = 0, 
					.Io.message = "unexpected EOF\0"
				}
			};
			return;
		}
		pos += __chx__lv__47->Ok.value;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 0, 
		.Ok.value = (struct fs_UnitTy){ 
		}
	};
	return;
}
static void fs_fsfile_write_all(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, struct fs_fsFile* f, const uint8_t* buf, size_t buf_len){
	size_t pos = 0;
	while((pos < buf_len)) {
		struct std_stdResult__cgs__31 r = (*({ struct std_stdResult__cgs__31 __chx__lv__48; fs_fsfile_write(&__chx__lv__48, f, (buf + pos), (buf_len - pos)); &__chx__lv__48; }));
		if((r.__chx__vt_621827 == 1)){
			struct std_stdResult__cgs__31* __chx__lv__49 = &r;
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
				.Err.error = __chx__lv__49->Err.error
			};
			return;
		}
		struct std_stdResult__cgs__31* __chx__lv__50 = &r;
		if((__chx__lv__50->Ok.value == 0)){
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
				.Err.error = (struct fs_fsFsError) { .__chx__vt_621827 = 0, 
					.Io.code = 0, 
					.Io.message = "write returned 0\0"
				}
			};
			return;
		}
		pos += __chx__lv__50->Ok.value;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 0, 
		.Ok.value = (struct fs_UnitTy){ 
		}
	};
	return;
}
static size_t fs_fsint_to_str(int v, char* out, size_t out_len){
	if((out_len == 0)){
		return 0;
	}
	char tmp[32];
	size_t i = 0;
	int val = v;
	if((val == 0)){
		out[0] = '0';
		out[1] = 0;
		return 1;
	}
	_Bool neg = 0;
	if((val < 0)){
		neg = 1;
		val = -val;
	}
	while((val > 0)) {
		uint8_t d = ((uint8_t) (val % 10));
		tmp[i++] = ((char) (((uint8_t) '0') + d));
		val = (val / 10);
	}
	size_t pos = 0;
	if(neg){
		if(((pos + 1) >= out_len)){
			return 0;
		}
		out[pos++] = '-';
	}
	while((i > 0)) {
		i -= 1;
		if(((pos + 1) >= out_len)){
			return 0;
		}
		out[pos++] = tmp[i];
	}
	if((pos >= out_len)){
		return 0;
	}
	out[pos] = 0;
	return pos;
}
void fs_fsatomic_write(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, const char* path, const uint8_t* data, size_t data_len){
	char dir_buf[4096];
	struct std_stdResult__cgs__31 r = (*({ struct std_stdResult__cgs__31 __chx__lv__51; fs_fsdirname(&__chx__lv__51, path, &dir_buf[0], ((size_t) 4096)); &__chx__lv__51; }));
	if((r.__chx__vt_621827 == 1)){
		struct std_stdResult__cgs__31* __chx__lv__52 = &r;
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
			.Err.error = __chx__lv__52->Err.error
		};
		return;
	}
	struct std_stdResult__cgs__31* __chx__lv__53 = &r;
	char tmpbuf[4096];
	char tmpname[64];
	size_t p = 0;
	while((p < __chx__lv__53->Ok.value)) {
		tmpbuf[p] = dir_buf[p];
		p++;
	}
	if(((p > 0) && (tmpbuf[(p - 1)] != '/'))){
		tmpbuf[p++] = '/';
	}
	const char* pref = ".tmpfs.";
	size_t pi = 0;
	while((pref[pi] != 0)) {
		tmpbuf[(p + pi)] = pref[pi];
		pi++;
	}
	p += pi;
	int PID = 12345;
	size_t tlen = fs_fsint_to_str(PID, &tmpname[0], 64);
	if((tlen == 0)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
			.Err.error = (struct fs_fsFsError) { .__chx__vt_621827 = 10, 
				.Other.msg = "pid conversion failed\0"
			}
		};
		return;
	}
	size_t q = 0;
	while((q <= tlen)) {
		tmpbuf[(p + q)] = tmpname[q];
		q++;
	}
	tmpbuf[(p + tlen)] = 0;
	struct std_stdResult__cgs__34 wr = (*({ struct std_stdResult__cgs__34 __chx__lv__54; fs_fswrite_text_file(&__chx__lv__54, &tmpbuf[0], data, data_len); &__chx__lv__54; }));
	if((wr.__chx__vt_621827 == 1)){
		struct std_stdResult__cgs__34* __chx__lv__55 = &wr;
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
			.Err.error = __chx__lv__55->Err.error
		};
		return;
	}
	struct std_stdResult__cgs__34 rnm = (*({ struct std_stdResult__cgs__34 __chx__lv__56; fs_fsmove_path(&__chx__lv__56, &tmpbuf[0], path); &__chx__lv__56; }));
	if((rnm.__chx__vt_621827 == 1)){
		struct std_stdResult__cgs__34* __chx__lv__57 = &rnm;
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
			.Err.error = __chx__lv__57->Err.error
		};
		return;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 0, 
		.Ok.value = (struct fs_UnitTy){ 
		}
	};
	return;
}
void fs_fsread_entire_file(struct std_stdResult__cgs__36* __chx_struct_ret_param_xx, const char* path){
	struct fs_fsOpenOptions opts;
	opts.read = 1;
	opts.write = 0;
	opts.append = 0;
	opts.create = 0;
	opts.create_new = 0;
	opts.truncate = 0;
	opts.binary = 1;
	struct std_stdResult__cgs__32 fo = (*({ struct std_stdResult__cgs__32 __chx__lv__58; fs_fsfile_open(&__chx__lv__58, path, ({ struct fs_fsOpenOptions __chx__lv__59 = opts; &__chx__lv__59; })); &__chx__lv__58; }));
	if((fo.__chx__vt_621827 == 1)){
		struct std_stdResult__cgs__32* __chx__lv__60 = &fo;
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__36) { .__chx__vt_621827 = 1, 
			.Err.error = __chx__lv__60->Err.error
		};
		return;
	}
	struct std_stdResult__cgs__32* __chx__lv__61 = &fo;
	struct std_stdvector__cgs__3 vec = (*({ struct std_stdvector__cgs__3 __chx__lv__62; std_stdvector__cgs__3make(&__chx__lv__62); &__chx__lv__62; }));
	_Bool __chx__lv__63 = true;
	size_t cap = 1048;
	std_stdvector__cgs__3reserve(&vec, cap);
	size_t pos = 0;
	while(1) {
		if((pos >= cap)){
			unsigned long newcap = (cap * 2);
			if((newcap <= cap)){
				(*({ struct std_stdResult__cgs__34 __chx__lv__64; fs_fsfile_close(&__chx__lv__64, &__chx__lv__61->Ok.value); &__chx__lv__64; }));
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__36) { .__chx__vt_621827 = 1, 
					.Err.error = (struct fs_fsFsError) { .__chx__vt_621827 = 10, 
						.Other.msg = "file too large to read"
					}
				};
				if(__chx__lv__63) {
					std_stdvector__cgs__3delete(&vec);
				}
				return;
			}
			cap = newcap;
			std_stdvector__cgs__3reserve(&vec, cap);
		}
		const uint8_t* ptr = (std_stdvector__cgs__3data(&vec) + ((isize) pos));
		unsigned long want = (cap - pos);
		struct std_stdResult__cgs__31 r = (*({ struct std_stdResult__cgs__31 __chx__lv__65; fs_fsfile_read(&__chx__lv__65, &__chx__lv__61->Ok.value, ((uint8_t*) ptr), want); &__chx__lv__65; }));
		if((r.__chx__vt_621827 == 1)){
			struct std_stdResult__cgs__31* __chx__lv__66 = &r;
			(*({ struct std_stdResult__cgs__34 __chx__lv__67; fs_fsfile_close(&__chx__lv__67, &__chx__lv__61->Ok.value); &__chx__lv__67; }));
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__36) { .__chx__vt_621827 = 1, 
				.Err.error = __chx__lv__66->Err.error
			};
			if(__chx__lv__63) {
				std_stdvector__cgs__3delete(&vec);
			}
			return;
		}
		struct std_stdResult__cgs__31* __chx__lv__68 = &r;
		if((__chx__lv__68->Ok.value == 0)){
			break;
		}
		pos += __chx__lv__68->Ok.value;
	}
	std_stdvector__cgs__3set_len(&vec, pos);
	(*({ struct std_stdResult__cgs__34 __chx__lv__69; fs_fsfile_close(&__chx__lv__69, &__chx__lv__61->Ok.value); &__chx__lv__69; }));
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__36) { .__chx__vt_621827 = 0, 
		.Ok.value = ({ __chx__lv__63 = false; vec; })
	};
	if(__chx__lv__63) {
		std_stdvector__cgs__3delete(&vec);
	}
	return;
}
void fs_fsread_to_buffer(struct std_stdResult__cgs__31* __chx_struct_ret_param_xx, const char* path, uint8_t* buf, size_t buf_len){
	struct fs_fsOpenOptions opts;
	opts.read = 1;
	opts.write = 0;
	opts.append = 0;
	opts.create = 0;
	opts.create_new = 0;
	opts.truncate = 0;
	opts.binary = 1;
	struct std_stdResult__cgs__32 fo = (*({ struct std_stdResult__cgs__32 __chx__lv__70; fs_fsfile_open(&__chx__lv__70, path, ({ struct fs_fsOpenOptions __chx__lv__71 = opts; &__chx__lv__71; })); &__chx__lv__70; }));
	if((fo.__chx__vt_621827 == 1)){
		struct std_stdResult__cgs__32* __chx__lv__72 = &fo;
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 1, 
			.Err.error = __chx__lv__72->Err.error
		};
		return;
	}
	struct std_stdResult__cgs__32* __chx__lv__73 = &fo;
	size_t pos = 0;
	while(1) {
		struct std_stdResult__cgs__31 r = (*({ struct std_stdResult__cgs__31 __chx__lv__74; fs_fsfile_read(&__chx__lv__74, &__chx__lv__73->Ok.value, (buf + pos), (buf_len - pos)); &__chx__lv__74; }));
		if((r.__chx__vt_621827 == 1)){
			struct std_stdResult__cgs__31* __chx__lv__75 = &r;
			(*({ struct std_stdResult__cgs__34 __chx__lv__76; fs_fsfile_close(&__chx__lv__76, &__chx__lv__73->Ok.value); &__chx__lv__76; }));
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 1, 
				.Err.error = __chx__lv__75->Err.error
			};
			return;
		}
		struct std_stdResult__cgs__31* __chx__lv__77 = &r;
		if((__chx__lv__77->Ok.value == 0)){
			break;
		}
		pos += __chx__lv__77->Ok.value;
		if((pos >= buf_len)){
			(*({ struct std_stdResult__cgs__34 __chx__lv__78; fs_fsfile_close(&__chx__lv__78, &__chx__lv__73->Ok.value); &__chx__lv__78; }));
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 1, 
				.Err.error = (struct fs_fsFsError) { .__chx__vt_621827 = 8, 
				}
			};
			return;
		}
	}
	(*({ struct std_stdResult__cgs__34 __chx__lv__79; fs_fsfile_close(&__chx__lv__79, &__chx__lv__73->Ok.value); &__chx__lv__79; }));
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 0, 
		.Ok.value = pos
	};
	return;
}
void fs_fswrite_text_file(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, const char* path, const uint8_t* data, size_t data_len){
	struct fs_fsOpenOptions opts;
	opts.read = 0;
	opts.write = 1;
	opts.append = 0;
	opts.create = 1;
	opts.create_new = 0;
	opts.truncate = 1;
	opts.binary = 1;
	struct std_stdResult__cgs__32 fo = (*({ struct std_stdResult__cgs__32 __chx__lv__80; fs_fsfile_open(&__chx__lv__80, path, ({ struct fs_fsOpenOptions __chx__lv__81 = opts; &__chx__lv__81; })); &__chx__lv__80; }));
	if((fo.__chx__vt_621827 == 1)){
		struct std_stdResult__cgs__32* __chx__lv__82 = &fo;
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
			.Err.error = __chx__lv__82->Err.error
		};
		return;
	}
	struct std_stdResult__cgs__32* __chx__lv__83 = &fo;
	struct std_stdResult__cgs__34 r = (*({ struct std_stdResult__cgs__34 __chx__lv__84; fs_fsfile_write_all(&__chx__lv__84, &__chx__lv__83->Ok.value, data, data_len); &__chx__lv__84; }));
	if((r.__chx__vt_621827 == 1)){
		struct std_stdResult__cgs__34* __chx__lv__85 = &r;
		(*({ struct std_stdResult__cgs__34 __chx__lv__86; fs_fsfile_close(&__chx__lv__86, &__chx__lv__83->Ok.value); &__chx__lv__86; }));
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
			.Err.error = __chx__lv__85->Err.error
		};
		return;
	}
	(*({ struct std_stdResult__cgs__34 __chx__lv__87; fs_fsfile_close(&__chx__lv__87, &__chx__lv__83->Ok.value); &__chx__lv__87; }));
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 0, 
		.Ok.value = (struct fs_UnitTy){ 
		}
	};
	return;
}
void fs_fsmove_path(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, const char* src, const char* dst){
	int r = rename(src, dst);
	if((r == 0)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 0, 
			.Ok.value = (struct fs_UnitTy){ 
			}
		};
		return;
	}
	struct std_stdResult__cgs__33 src_meta = (*({ struct std_stdResult__cgs__33 __chx__lv__88; fs_fsmetadata(&__chx__lv__88, src); &__chx__lv__88; }));
	if((src_meta.__chx__vt_621827 == 1)){
		struct std_stdResult__cgs__33* __chx__lv__89 = &src_meta;
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
			.Err.error = __chx__lv__89->Err.error
		};
		return;
	}
	struct std_stdResult__cgs__33* __chx__lv__90 = &src_meta;
	if(__chx__lv__90->Ok.value.is_dir){
		struct std_stdResult__cgs__34 c = (*({ struct std_stdResult__cgs__34 __chx__lv__91; fs_fscopy_directory(&__chx__lv__91, src, dst, 1); &__chx__lv__91; }));
		if((c.__chx__vt_621827 == 1)){
			struct std_stdResult__cgs__34* __chx__lv__92 = &c;
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
				.Err.error = __chx__lv__92->Err.error
			};
			return;
		}
		struct std_stdResult__cgs__34 rem = (*({ struct std_stdResult__cgs__34 __chx__lv__93; fs_fsremove_dir_all_recursive(&__chx__lv__93, src); &__chx__lv__93; }));
		if((rem.__chx__vt_621827 == 1)){
			struct std_stdResult__cgs__34* __chx__lv__94 = &rem;
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
				.Err.error = __chx__lv__94->Err.error
			};
			return;
		}
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 0, 
			.Ok.value = (struct fs_UnitTy){ 
			}
		};
		return;
	} else {
		struct std_stdResult__cgs__34 c = (*({ struct std_stdResult__cgs__34 __chx__lv__95; fs_fscopy_file(&__chx__lv__95, src, dst); &__chx__lv__95; }));
		if((c.__chx__vt_621827 == 1)){
			struct std_stdResult__cgs__34* __chx__lv__96 = &c;
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
				.Err.error = __chx__lv__96->Err.error
			};
			return;
		}
		struct std_stdResult__cgs__34 rem = (*({ struct std_stdResult__cgs__34 __chx__lv__97; fs_fsremove_file(&__chx__lv__97, src); &__chx__lv__97; }));
		if((rem.__chx__vt_621827 == 1)){
			struct std_stdResult__cgs__34* __chx__lv__98 = &rem;
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
				.Err.error = __chx__lv__98->Err.error
			};
			return;
		}
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 0, 
			.Ok.value = (struct fs_UnitTy){ 
			}
		};
		return;
	}
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/posix/permission.ch **/
void fs_fsset_permissions_native(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, fs_fspath_ptr path, uint32_t mode){
	int r = chmod(path, mode);
	if((r != 0)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
			.Err.error = (*({ struct fs_fsFsError __chx__lv__99; fs_fsposix_errno_to_fs(&__chx__lv__99, cstd_get_errno()); &__chx__lv__99; }))
		};
		return;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 0, 
		.Ok.value = (struct fs_UnitTy){ 
		}
	};
	return;
}
void fs_fsset_permissions(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, const char* path, uint32_t mode){
	*__chx_struct_ret_param_xx = (*({ struct std_stdResult__cgs__34 __chx__lv__100; fs_fsset_permissions_native(&__chx__lv__100, path, mode); &__chx__lv__100; }));
	return;
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/posix/directory.ch **/
static int fs_posix_mkdir(const char* pathname, unsigned int mode){
	return mkdir(pathname, mode);
}
void fs_fscreate_dir_native(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, fs_fspath_ptr path){
	int r = fs_posix_mkdir(path, 511);
	if((r != 0)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
			.Err.error = (*({ struct fs_fsFsError __chx__lv__101; fs_fsposix_errno_to_fs(&__chx__lv__101, cstd_get_errno()); &__chx__lv__101; }))
		};
		return;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 0, 
		.Ok.value = (struct fs_UnitTy){ 
		}
	};
	return;
}
void fs_fscreate_dir_all(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, const char* path){
	char buf[4096];
	struct std_stdResult__cgs__31 r = (*({ struct std_stdResult__cgs__31 __chx__lv__102; fs_fsnormalize_path(&__chx__lv__102, path, &buf[0], ((size_t) 4096)); &__chx__lv__102; }));
	if((r.__chx__vt_621827 == 1)){
		struct std_stdResult__cgs__31* __chx__lv__103 = &r;
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
			.Err.error = __chx__lv__103->Err.error
		};
		return;
	}
	struct std_stdResult__cgs__31* __chx__lv__104 = &r;
	size_t i = 1;
	if((buf[0] != '/')){
		i = 0;
	}
	while((i <= __chx__lv__104->Ok.value)) {
		if(((i == __chx__lv__104->Ok.value) || (buf[i] == '/'))){
			char prefix[4096];
			size_t k = 0;
			while((k < i)) {
				prefix[k] = buf[k];
				k++;
			}
			prefix[k] = 0;
			if((k == 0)){
				i++;
				continue;
			}
			struct std_stdResult__cgs__33 stat_res = (*({ struct std_stdResult__cgs__33 __chx__lv__105; fs_fsmetadata(&__chx__lv__105, &prefix[0]); &__chx__lv__105; }));
			if((stat_res.__chx__vt_621827 == 1)){
				struct std_stdResult__cgs__33* __chx__lv__106 = &stat_res;
				if((__chx__lv__106->Err.error.__chx__vt_621827 == 1)){
					struct std_stdResult__cgs__34 c = (*({ struct std_stdResult__cgs__34 __chx__lv__107; fs_fscreate_dir(&__chx__lv__107, &prefix[0]); &__chx__lv__107; }));
					if((c.__chx__vt_621827 == 1)){
						struct std_stdResult__cgs__34* __chx__lv__108 = &c;
						*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
							.Err.error = __chx__lv__108->Err.error
						};
						return;
					}
				} else {
					if((__chx__lv__106->Err.error.__chx__vt_621827 == 2)){
					} else {
						*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
							.Err.error = __chx__lv__106->Err.error
						};
						return;
					}
				}
			}
		}
		i++;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 0, 
		.Ok.value = (struct fs_UnitTy){ 
		}
	};
	return;
}
void fs_fsremove_dir_native(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, fs_fspath_ptr path){
	int r = rmdir(path);
	if((r != 0)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
			.Err.error = (*({ struct fs_fsFsError __chx__lv__109; fs_fsposix_errno_to_fs(&__chx__lv__109, cstd_get_errno()); &__chx__lv__109; }))
		};
		return;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 0, 
		.Ok.value = (struct fs_UnitTy){ 
		}
	};
	return;
}
static void fs_fsremove_dir_all_at(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, int dirfd){
	int ddup = dup(dirfd);
	if((ddup < 0)){
		int e = cstd_get_errno();
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
			.Err.error = (*({ struct fs_fsFsError __chx__lv__110; fs_fsposix_errno_to_fs(&__chx__lv__110, e); &__chx__lv__110; }))
		};
		return;
	}
	struct cstd_DIR* dir = fdopendir(ddup);
	if((dir == NULL)){
		int e = cstd_get_errno();
		close(ddup);
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
			.Err.error = (*({ struct fs_fsFsError __chx__lv__111; fs_fsposix_errno_to_fs(&__chx__lv__111, e); &__chx__lv__111; }))
		};
		return;
	}
	while(1){
		cstd_set_errno(0);
		struct cstd_dirent* ent = readdir(dir);
		if((ent == NULL)){
			if((cstd_get_errno() != 0)){
				int re = cstd_get_errno();
				closedir(dir);
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
					.Err.error = (*({ struct fs_fsFsError __chx__lv__112; fs_fsposix_errno_to_fs(&__chx__lv__112, re); &__chx__lv__112; }))
				};
				return;
			}
			break;
		}
		if(((ent->d_name[0] == '.') && (ent->d_name[1] == 0))){
			continue;
		}
		if((((ent->d_name[0] == '.') && (ent->d_name[1] == '.')) && (ent->d_name[2] == 0))){
			continue;
		}
		struct cstd_Stat st;
		cstd_set_errno(0);
		if((fstatat(dirfd, &ent->d_name[0], &st, fs_fsAT_SYMLINK_NOFOLLOW) != 0)){
			int e = cstd_get_errno();
			if((e == 2)){
				continue;
			} else {
				closedir(dir);
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
					.Err.error = (*({ struct fs_fsFsError __chx__lv__113; fs_fsposix_errno_to_fs(&__chx__lv__113, e); &__chx__lv__113; }))
				};
				return;
			}
		}
		if(((st.st_mode & 61440) == 16384)){
			cstd_set_errno(0);
			int childfd = openat(dirfd, &ent->d_name[0], (0 | fs_fsO_DIRECTORY), 0);
			if((childfd < 0)){
				int e2 = cstd_get_errno();
				if((e2 == 2)){
					continue;
				} else {
					closedir(dir);
					*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
						.Err.error = (*({ struct fs_fsFsError __chx__lv__114; fs_fsposix_errno_to_fs(&__chx__lv__114, e2); &__chx__lv__114; }))
					};
					return;
				}
			}
			struct std_stdResult__cgs__34 rem = (*({ struct std_stdResult__cgs__34 __chx__lv__115; fs_fsremove_dir_all_at(&__chx__lv__115, childfd); &__chx__lv__115; }));
			close(childfd);
			if((rem.__chx__vt_621827 == 1)){
				struct std_stdResult__cgs__34* __chx__lv__116 = &rem;
				closedir(dir);
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
					.Err.error = __chx__lv__116->Err.error
				};
				return;
			}
			cstd_set_errno(0);
			if((unlinkat(dirfd, &ent->d_name[0], fs_fsAT_REMOVEDIR) != 0)){
				int e3 = cstd_get_errno();
				if((e3 == 2)){
					continue;
				} else {
					closedir(dir);
					*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
						.Err.error = (*({ struct fs_fsFsError __chx__lv__117; fs_fsposix_errno_to_fs(&__chx__lv__117, e3); &__chx__lv__117; }))
					};
					return;
				}
			} else {
			}
		} else {
			cstd_set_errno(0);
			if((unlinkat(dirfd, &ent->d_name[0], 0) != 0)){
				int e4 = cstd_get_errno();
				if((e4 == 2)){
					continue;
				} else {
					closedir(dir);
					*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
						.Err.error = (*({ struct fs_fsFsError __chx__lv__118; fs_fsposix_errno_to_fs(&__chx__lv__118, e4); &__chx__lv__118; }))
					};
					return;
				}
			} else {
			}
		}
	}
	closedir(dir);
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 0, 
		.Ok.value = (struct fs_UnitTy){ 
		}
	};
	return;
}
void fs_fsremove_dir_all_recursive_native(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, fs_fspath_ptr path){
	int dirfd = open(path, (0 | fs_fsO_DIRECTORY), 0);
	if((dirfd < 0)){
		int e = cstd_get_errno();
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
			.Err.error = (*({ struct fs_fsFsError __chx__lv__119; fs_fsposix_errno_to_fs(&__chx__lv__119, e); &__chx__lv__119; }))
		};
		return;
	}
	struct std_stdResult__cgs__34 rem = (*({ struct std_stdResult__cgs__34 __chx__lv__120; fs_fsremove_dir_all_at(&__chx__lv__120, dirfd); &__chx__lv__120; }));
	int c = close(dirfd);
	if((rem.__chx__vt_621827 == 1)){
		struct std_stdResult__cgs__34* __chx__lv__121 = &rem;
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
			.Err.error = __chx__lv__121->Err.error
		};
		return;
	}
	cstd_set_errno(0);
	if((rmdir(path) != 0)){
		int er = cstd_get_errno();
		if((er == 2)){
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 0, 
				.Ok.value = (struct fs_UnitTy){ 
				}
			};
			return;
		}
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
			.Err.error = (*({ struct fs_fsFsError __chx__lv__122; fs_fsposix_errno_to_fs(&__chx__lv__122, er); &__chx__lv__122; }))
		};
		return;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 0, 
		.Ok.value = (struct fs_UnitTy){ 
		}
	};
	return;
}
void fs_fstemp_dir(struct std_stdResult__cgs__31* __chx_struct_ret_param_xx, char* out, size_t out_len){
	const char* tmp = "/tmp\0";
	size_t i = 0;
	while((tmp[i] != 0)) {
		out[i] = tmp[i];
		i++;
	}
	out[i] = 0;
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 0, 
		.Ok.value = i
	};
	return;
}
void fs_fsread_dir(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, const char* path, std_stdfunction* callback){
	_Bool __chx__lv__123 = true;
	struct cstd_DIR* d = opendir(path);
	if((d == NULL)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
			.Err.error = (*({ struct fs_fsFsError __chx__lv__124; fs_fsposix_errno_to_fs(&__chx__lv__124, cstd_get_errno()); &__chx__lv__124; }))
		};
		if(__chx__lv__123) {
			std_stddefault_function_instancedelete(callback);
		}
		return;
	}
	while(1) {
		struct cstd_dirent* ent = readdir(d);
		if((ent == NULL)){
			break;
		}
		const char* name_ptr = &ent->d_name[0];
		size_t nl = 0;
		while((name_ptr[nl] != 0)) {
			nl++;
		}
		_Bool isdir = 0;
		char child[4096];
		size_t p = 0;
		while((path[p] != 0)) {
			child[p] = path[p];
			p++;
		}
		if(((p > 0) && (child[(p - 1)] != '/'))){
			child[p++] = '/';
		}
		size_t q = 0;
		while((q <= nl)) {
			child[(p + q)] = name_ptr[q];
			q++;
		}
		struct cstd_Stat st;
		int r = lstat(&child[0], &st);
		if((r == 0)){
			isdir = ((st.st_mode & 61440) == 16384);
		}
		_Bool cont = ({ struct std_stddefault_function_instance* __chx__lv__125 = callback; ((_Bool(*)(void*, const char* name, size_t name_len, _Bool is_dir))std_stddefault_function_instanceget_fn_ptr(__chx__lv__125))(std_stddefault_function_instanceget_data_ptr(__chx__lv__125), name_ptr, nl, isdir); });
		if(!cont){
			break;
		}
	}
	closedir(d);
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 0, 
		.Ok.value = (struct fs_UnitTy){ 
		}
	};
	if(__chx__lv__123) {
		std_stddefault_function_instancedelete(callback);
	}
	return;
}
void fs_fscreate_dir(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, const char* path){
	*__chx_struct_ret_param_xx = (*({ struct std_stdResult__cgs__34 __chx__lv__126; fs_fscreate_dir_native(&__chx__lv__126, path); &__chx__lv__126; }));
	return;
}
void fs_fsremove_dir(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, const char* path){
	*__chx_struct_ret_param_xx = (*({ struct std_stdResult__cgs__34 __chx__lv__127; fs_fsremove_dir_native(&__chx__lv__127, path); &__chx__lv__127; }));
	return;
}
void fs_fsremove_dir_all_recursive(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, const char* path){
	*__chx_struct_ret_param_xx = (*({ struct std_stdResult__cgs__34 __chx__lv__128; fs_fsremove_dir_all_recursive_native(&__chx__lv__128, path); &__chx__lv__128; }));
	return;
}
int fs_fsmkdir(const char* pathname){
	return fs_posix_mkdir(pathname, ((unsigned int) 448));
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/posix/link.ch **/
static void fs_fscreate_hard_link_native(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, fs_fspath_ptr existing, fs_fspath_ptr newpath){
	int r = link(existing, newpath);
	if((r != 0)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
			.Err.error = (*({ struct fs_fsFsError __chx__lv__129; fs_fsposix_errno_to_fs(&__chx__lv__129, cstd_get_errno()); &__chx__lv__129; }))
		};
		return;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 0, 
		.Ok.value = (struct fs_UnitTy){ 
		}
	};
	return;
}
static void fs_fscreate_symlink_native(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, fs_fspath_ptr target, fs_fspath_ptr linkpath, _Bool dir){
	int r = symlink(target, linkpath);
	if((r != 0)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
			.Err.error = (*({ struct fs_fsFsError __chx__lv__130; fs_fsposix_errno_to_fs(&__chx__lv__130, cstd_get_errno()); &__chx__lv__130; }))
		};
		return;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 0, 
		.Ok.value = (struct fs_UnitTy){ 
		}
	};
	return;
}
static void fs_fsread_link_native(struct std_stdResult__cgs__31* __chx_struct_ret_param_xx, fs_fspath_ptr path, char* out, size_t out_len){
	fs_fsssize_t r = readlink(path, out, out_len);
	if((r < 0)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 1, 
			.Err.error = (*({ struct fs_fsFsError __chx__lv__131; fs_fsposix_errno_to_fs(&__chx__lv__131, cstd_get_errno()); &__chx__lv__131; }))
		};
		return;
	}
	if((((size_t) r) < out_len)){
		out[r] = 0;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 0, 
		.Ok.value = ((size_t) r)
	};
	return;
}
static void fs_fsis_symlink(struct std_stdResult__cgs__35* __chx_struct_ret_param_xx, const char* path){
	struct cstd_Stat st;
	int r = lstat(path, &st);
	if((r != 0)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__35) { .__chx__vt_621827 = 1, 
			.Err.error = (*({ struct fs_fsFsError __chx__lv__132; fs_fsposix_errno_to_fs(&__chx__lv__132, cstd_get_errno()); &__chx__lv__132; }))
		};
		return;
	}
	_Bool islnk = ((st.st_mode & 61440) == 40960);
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__35) { .__chx__vt_621827 = 0, 
		.Ok.value = islnk
	};
	return;
}
static void fs_fscreate_symlink(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, const char* target, const char* linkpath, _Bool dir){
	*__chx_struct_ret_param_xx = (*({ struct std_stdResult__cgs__34 __chx__lv__133; fs_fscreate_symlink_native(&__chx__lv__133, target, linkpath, dir); &__chx__lv__133; }));
	return;
}
static void fs_fsread_link(struct std_stdResult__cgs__31* __chx_struct_ret_param_xx, const char* path, char* out, size_t out_len){
	*__chx_struct_ret_param_xx = (*({ struct std_stdResult__cgs__31 __chx__lv__134; fs_fsread_link_native(&__chx__lv__134, path, out, out_len); &__chx__lv__134; }));
	return;
}
static void fs_fscreate_hard_link(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, const char* existing, const char* newpath){
	*__chx_struct_ret_param_xx = (*({ struct std_stdResult__cgs__34 __chx__lv__135; fs_fscreate_hard_link_native(&__chx__lv__135, existing, newpath); &__chx__lv__135; }));
	return;
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/posix/path.ch **/
static void fs_fscanonicalize(struct std_stdResult__cgs__31* __chx_struct_ret_param_xx, const char* path, char* out, size_t out_len){
	const char* resptr = realpath(path, out);
	if((resptr == 0)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 1, 
			.Err.error = (struct fs_fsFsError) { .__chx__vt_621827 = 0, 
				.Io.code = -1, 
				.Io.message = "realpath failed\0"
			}
		};
		return;
	}
	size_t plen = 0;
	while((out[plen] != 0)) {
		plen++;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 0, 
		.Ok.value = plen
	};
	return;
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/posix/file_io.ch **/
static void fs_fsfile_open_native(struct std_stdResult__cgs__32* __chx_struct_ret_param_xx, fs_fspath_ptr path, struct fs_fsOpenOptions* opts){
	int flags = 0;
	if((opts->read && !opts->write)){
		flags = 0;
	}else if((opts->read && opts->write)){
		flags = 2;
	}else if((!opts->read && opts->write)){
		flags = 1;
	}
	if(opts->create){
		flags = (flags | 64);
	}
	if(opts->truncate){
		flags = (flags | 512);
	}
	if(opts->create_new){
		flags = (flags | fs_fsO_EXCL);
	}
	if(opts->append){
		flags = (flags | fs_fsO_APPEND);
	}
	int fd = open(path, flags, 438);
	if((fd < 0)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__32) { .__chx__vt_621827 = 1, 
			.Err.error = (*({ struct fs_fsFsError __chx__lv__136; fs_fsposix_errno_to_fs(&__chx__lv__136, cstd_get_errno()); &__chx__lv__136; }))
		};
		return;
	}
	struct fs_fsFile f;
	f._unix.fd = fd;
	f.valid = 1;
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__32) { .__chx__vt_621827 = 0, 
		.Ok.value = f
	};
	return;
}
void fs_fsfile_close(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, struct fs_fsFile* f){
	if(!f->valid){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 0, 
			.Ok.value = (struct fs_UnitTy){ 
			}
		};
		return;
	}
	int r = close(f->_unix.fd);
	if((r != 0)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
			.Err.error = (*({ struct fs_fsFsError __chx__lv__137; fs_fsposix_errno_to_fs(&__chx__lv__137, cstd_get_errno()); &__chx__lv__137; }))
		};
		return;
	}
	f->valid = 0;
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 0, 
		.Ok.value = (struct fs_UnitTy){ 
		}
	};
	return;
}
void fs_fsfile_read(struct std_stdResult__cgs__31* __chx_struct_ret_param_xx, struct fs_fsFile* f, uint8_t* buf, size_t buf_len){
	cstd_ssize_t n = read(f->_unix.fd, ((void*) buf), buf_len);
	if((n < 0)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 1, 
			.Err.error = (*({ struct fs_fsFsError __chx__lv__138; fs_fsposix_errno_to_fs(&__chx__lv__138, cstd_get_errno()); &__chx__lv__138; }))
		};
		return;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 0, 
		.Ok.value = ((size_t) n)
	};
	return;
}
void fs_fsfile_write(struct std_stdResult__cgs__31* __chx_struct_ret_param_xx, struct fs_fsFile* f, const uint8_t* buf, size_t buf_len){
	cstd_ssize_t n = write(f->_unix.fd, ((const void*) buf), buf_len);
	if((n < 0)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 1, 
			.Err.error = (*({ struct fs_fsFsError __chx__lv__139; fs_fsposix_errno_to_fs(&__chx__lv__139, cstd_get_errno()); &__chx__lv__139; }))
		};
		return;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__31) { .__chx__vt_621827 = 0, 
		.Ok.value = ((size_t) n)
	};
	return;
}
void fs_fsfile_flush(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, struct fs_fsFile* f){
	int r = fsync(f->_unix.fd);
	if((r != 0)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
			.Err.error = (*({ struct fs_fsFsError __chx__lv__140; fs_fsposix_errno_to_fs(&__chx__lv__140, cstd_get_errno()); &__chx__lv__140; }))
		};
		return;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 0, 
		.Ok.value = (struct fs_UnitTy){ 
		}
	};
	return;
}
void fs_fsremove_file_native(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, fs_fspath_ptr path){
	if((unlink(path) != 0)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
			.Err.error = (*({ struct fs_fsFsError __chx__lv__141; fs_fsposix_errno_to_fs(&__chx__lv__141, cstd_get_errno()); &__chx__lv__141; }))
		};
		return;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 0, 
		.Ok.value = (struct fs_UnitTy){ 
		}
	};
	return;
}
static void fs_fsset_times_native(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, fs_fspath_ptr path, int64_t atime, int64_t mtime){
	struct timespec times[2];
	times[0].tv_sec = atime;
	times[0].tv_nsec = 0;
	times[1].tv_sec = mtime;
	times[1].tv_nsec = 0;
	int r = utimensat(0, path, &times[0], 0);
	if((r != 0)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
			.Err.error = (*({ struct fs_fsFsError __chx__lv__142; fs_fsposix_errno_to_fs(&__chx__lv__142, cstd_get_errno()); &__chx__lv__142; }))
		};
		return;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 0, 
		.Ok.value = (struct fs_UnitTy){ 
		}
	};
	return;
}
static void fs_fscopy_file_native(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, fs_fspath_ptr src, fs_fspath_ptr dst){
	struct fs_fsOpenOptions srcopts;
	srcopts.read = 1;
	srcopts.write = 0;
	srcopts.append = 0;
	srcopts.create = 0;
	srcopts.create_new = 0;
	srcopts.truncate = 0;
	srcopts.binary = 1;
	struct std_stdResult__cgs__32 sres = (*({ struct std_stdResult__cgs__32 __chx__lv__143; fs_fsfile_open(&__chx__lv__143, src, ({ struct fs_fsOpenOptions __chx__lv__144 = srcopts; &__chx__lv__144; })); &__chx__lv__143; }));
	if((sres.__chx__vt_621827 == 1)){
		struct std_stdResult__cgs__32* __chx__lv__145 = &sres;
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
			.Err.error = __chx__lv__145->Err.error
		};
		return;
	}
	struct std_stdResult__cgs__32* __chx__lv__146 = &sres;
	struct fs_fsOpenOptions dstopts;
	dstopts.read = 0;
	dstopts.write = 1;
	dstopts.append = 0;
	dstopts.create = 1;
	dstopts.create_new = 0;
	dstopts.truncate = 1;
	dstopts.binary = 1;
	struct std_stdResult__cgs__32 dres = (*({ struct std_stdResult__cgs__32 __chx__lv__147; fs_fsfile_open(&__chx__lv__147, dst, ({ struct fs_fsOpenOptions __chx__lv__148 = dstopts; &__chx__lv__148; })); &__chx__lv__147; }));
	if((dres.__chx__vt_621827 == 1)){
		(*({ struct std_stdResult__cgs__34 __chx__lv__149; fs_fsfile_close(&__chx__lv__149, &__chx__lv__146->Ok.value); &__chx__lv__149; }));
		struct std_stdResult__cgs__32* __chx__lv__150 = &dres;
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
			.Err.error = __chx__lv__150->Err.error
		};
		return;
	}
	struct std_stdResult__cgs__32* __chx__lv__151 = &dres;
	uint8_t buf[65536];
	while(1) {
		struct std_stdResult__cgs__31 r = (*({ struct std_stdResult__cgs__31 __chx__lv__152; fs_fsfile_read(&__chx__lv__152, &__chx__lv__146->Ok.value, &buf[0], sizeof(buf)); &__chx__lv__152; }));
		if((r.__chx__vt_621827 == 1)){
			(*({ struct std_stdResult__cgs__34 __chx__lv__153; fs_fsfile_close(&__chx__lv__153, &__chx__lv__146->Ok.value); &__chx__lv__153; }));
			(*({ struct std_stdResult__cgs__34 __chx__lv__154; fs_fsfile_close(&__chx__lv__154, &__chx__lv__151->Ok.value); &__chx__lv__154; }));
			struct std_stdResult__cgs__31* __chx__lv__155 = &r;
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
				.Err.error = __chx__lv__155->Err.error
			};
			return;
		}
		struct std_stdResult__cgs__31* __chx__lv__156 = &r;
		if((__chx__lv__156->Ok.value == 0)){
			break;
		}
		struct std_stdResult__cgs__34 w = (*({ struct std_stdResult__cgs__34 __chx__lv__157; fs_fsfile_write_all(&__chx__lv__157, &__chx__lv__151->Ok.value, &buf[0], __chx__lv__156->Ok.value); &__chx__lv__157; }));
		if((w.__chx__vt_621827 == 1)){
			(*({ struct std_stdResult__cgs__34 __chx__lv__158; fs_fsfile_close(&__chx__lv__158, &__chx__lv__146->Ok.value); &__chx__lv__158; }));
			(*({ struct std_stdResult__cgs__34 __chx__lv__159; fs_fsfile_close(&__chx__lv__159, &__chx__lv__151->Ok.value); &__chx__lv__159; }));
			struct std_stdResult__cgs__34* __chx__lv__160 = &w;
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
				.Err.error = __chx__lv__160->Err.error
			};
			return;
		}
	}
	(*({ struct std_stdResult__cgs__34 __chx__lv__161; fs_fsfile_close(&__chx__lv__161, &__chx__lv__146->Ok.value); &__chx__lv__161; }));
	(*({ struct std_stdResult__cgs__34 __chx__lv__162; fs_fsfile_close(&__chx__lv__162, &__chx__lv__151->Ok.value); &__chx__lv__162; }));
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 0, 
		.Ok.value = (struct fs_UnitTy){ 
		}
	};
	return;
}
static void fs_fscreate_temp_file_in_native(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, fs_fspath_ptr dir, fs_fspath_ptr prefix, fs_fsmut_path_ptr out_path, struct fs_fsFile* fh){
	char tmpl[4096];
	size_t p = 0;
	while((dir[p] != 0)) {
		tmpl[p] = dir[p];
		p++;
	}
	if(((p > 0) && (tmpl[(p - 1)] != '/'))){
		tmpl[p++] = '/';
	}
	size_t q = 0;
	while((prefix[q] != 0)) {
		tmpl[(p + q)] = prefix[q];
		q++;
	}
	p += q;
	const char* sfx = "XXXXXX\0";
	size_t r = 0;
	while((sfx[r] != 0)) {
		tmpl[(p + r)] = sfx[r];
		r++;
	}
	tmpl[(p + r)] = 0;
	int fd = mkstemp(&tmpl[0]);
	if((fd < 0)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
			.Err.error = (*({ struct fs_fsFsError __chx__lv__163; fs_fsposix_errno_to_fs(&__chx__lv__163, cstd_get_errno()); &__chx__lv__163; }))
		};
		return;
	}
	size_t i = 0;
	while((tmpl[i] != 0)) {
		out_path[i] = tmpl[i];
		i++;
	}
	out_path[i] = 0;
	fh->_unix.fd = fd;
	fh->valid = 1;
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 0, 
		.Ok.value = (struct fs_UnitTy){ 
		}
	};
	return;
}
void fs_fsremove_file(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, const char* path){
	*__chx_struct_ret_param_xx = (*({ struct std_stdResult__cgs__34 __chx__lv__164; fs_fsremove_file_native(&__chx__lv__164, path); &__chx__lv__164; }));
	return;
}
void fs_fscopy_file(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, const char* src, const char* dst){
	*__chx_struct_ret_param_xx = (*({ struct std_stdResult__cgs__34 __chx__lv__165; fs_fscopy_file_native(&__chx__lv__165, src, dst); &__chx__lv__165; }));
	return;
}
static void fs_fscreate_temp_file_in(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, const char* dir, const char* prefix, char* out_path, size_t out_len, struct fs_fsFile* fh){
	*__chx_struct_ret_param_xx = (*({ struct std_stdResult__cgs__34 __chx__lv__166; fs_fscreate_temp_file_in_native(&__chx__lv__166, dir, prefix, out_path, fh); &__chx__lv__166; }));
	return;
}
void fs_fsfile_open(struct std_stdResult__cgs__32* __chx_struct_ret_param_xx, const char* path, struct fs_fsOpenOptions* opts){
	*__chx_struct_ret_param_xx = (*({ struct std_stdResult__cgs__32 __chx__lv__167; fs_fsfile_open_native(&__chx__lv__167, path, ({ struct fs_fsOpenOptions __chx__lv__168 = *opts; &__chx__lv__168; })); &__chx__lv__167; }));
	return;
}
void fs_fsset_times(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, const char* path, int64_t atime, int64_t mtime){
	*__chx_struct_ret_param_xx = (*({ struct std_stdResult__cgs__34 __chx__lv__169; fs_fsset_times_native(&__chx__lv__169, path, atime, mtime); &__chx__lv__169; }));
	return;
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/posix/lock.ch **/
static void fs_fslock_file_shared(struct std_stdResult__cgs__32* __chx_struct_ret_param_xx, const char* path){
	struct fs_fsOpenOptions opts;
	opts.read = 1;
	opts.write = 0;
	struct std_stdResult__cgs__32 fo = (*({ struct std_stdResult__cgs__32 __chx__lv__170; fs_fsfile_open(&__chx__lv__170, path, ({ struct fs_fsOpenOptions __chx__lv__171 = opts; &__chx__lv__171; })); &__chx__lv__170; }));
	if((fo.__chx__vt_621827 == 1)){
		struct std_stdResult__cgs__32* __chx__lv__172 = &fo;
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__32) { .__chx__vt_621827 = 1, 
			.Err.error = __chx__lv__172->Err.error
		};
		return;
	}
	struct std_stdResult__cgs__32* __chx__lv__173 = &fo;
	int r = flock(__chx__lv__173->Ok.value._unix.fd, fs_fsLOCK_SH);
	if((r != 0)){
		(*({ struct std_stdResult__cgs__34 __chx__lv__174; fs_fsfile_close(&__chx__lv__174, &__chx__lv__173->Ok.value); &__chx__lv__174; }));
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__32) { .__chx__vt_621827 = 1, 
			.Err.error = (*({ struct fs_fsFsError __chx__lv__175; fs_fsposix_errno_to_fs(&__chx__lv__175, cstd_get_errno()); &__chx__lv__175; }))
		};
		return;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__32) { .__chx__vt_621827 = 0, 
		.Ok.value = __chx__lv__173->Ok.value
	};
	return;
}
static void fs_fslock_file_exclusive(struct std_stdResult__cgs__32* __chx_struct_ret_param_xx, const char* path){
	struct fs_fsOpenOptions opts;
	opts.read = 1;
	opts.write = 1;
	struct std_stdResult__cgs__32 fo = (*({ struct std_stdResult__cgs__32 __chx__lv__176; fs_fsfile_open(&__chx__lv__176, path, ({ struct fs_fsOpenOptions __chx__lv__177 = opts; &__chx__lv__177; })); &__chx__lv__176; }));
	if((fo.__chx__vt_621827 == 1)){
		struct std_stdResult__cgs__32* __chx__lv__178 = &fo;
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__32) { .__chx__vt_621827 = 1, 
			.Err.error = __chx__lv__178->Err.error
		};
		return;
	}
	struct std_stdResult__cgs__32* __chx__lv__179 = &fo;
	int r = flock(__chx__lv__179->Ok.value._unix.fd, fs_fsLOCK_EX);
	if((r != 0)){
		(*({ struct std_stdResult__cgs__34 __chx__lv__180; fs_fsfile_close(&__chx__lv__180, &__chx__lv__179->Ok.value); &__chx__lv__180; }));
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__32) { .__chx__vt_621827 = 1, 
			.Err.error = (*({ struct fs_fsFsError __chx__lv__181; fs_fsposix_errno_to_fs(&__chx__lv__181, cstd_get_errno()); &__chx__lv__181; }))
		};
		return;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__32) { .__chx__vt_621827 = 0, 
		.Ok.value = __chx__lv__179->Ok.value
	};
	return;
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/posix/metadata.ch **/
void fs_fsmetadata(struct std_stdResult__cgs__33* __chx_struct_ret_param_xx, const char* path){
	struct cstd_Stat st;
	int r = lstat(path, &st);
	if((r != 0)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__33) { .__chx__vt_621827 = 1, 
			.Err.error = (*({ struct fs_fsFsError __chx__lv__182; fs_fsposix_errno_to_fs(&__chx__lv__182, cstd_get_errno()); &__chx__lv__182; }))
		};
		return;
	}
	struct fs_fsMetadata m;
	int mode = ((int) st.st_mode);
	m.is_dir = ((mode & 61440) == 16384);
	m.is_file = ((mode & 61440) == 32768);
	m.is_symlink = ((mode & 61440) == 40960);
	m.len = ((size_t) st.st_size);
	m.modified = ((int64_t) st.st_mtime.tv_sec);
	m.accessed = ((int64_t) st.st_atime.tv_sec);
	m.created = ((int64_t) st.st_ctime.tv_sec);
	m.perms = ((uint32_t) (st.st_mode & 511));
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__33) { .__chx__vt_621827 = 0, 
		.Ok.value = m
	};
	return;
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/posix/disk.ch **/
static void fs_fsdisk_space_native(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, fs_fspath_ptr path, uint64_t* total_out, uint64_t* free_out, uint64_t* avail_out){
	struct fs_fsStatvfs st;
	int r = statvfs(path, &st);
	if((r != 0)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 1, 
			.Err.error = (*({ struct fs_fsFsError __chx__lv__183; fs_fsposix_errno_to_fs(&__chx__lv__183, cstd_get_errno()); &__chx__lv__183; }))
		};
		return;
	}
	*total_out = (((uint64_t) st.f_blocks) * ((uint64_t) st.f_frsize));
	*free_out = (((uint64_t) st.f_bfree) * ((uint64_t) st.f_frsize));
	*avail_out = (((uint64_t) st.f_bavail) * ((uint64_t) st.f_frsize));
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__34) { .__chx__vt_621827 = 0, 
		.Ok.value = (struct fs_UnitTy){ 
		}
	};
	return;
}
static void fs_fsdisk_space(struct std_stdResult__cgs__34* __chx_struct_ret_param_xx, const char* path, uint64_t* total_out, uint64_t* free_out, uint64_t* avail_out){
	*__chx_struct_ret_param_xx = (*({ struct std_stdResult__cgs__34 __chx__lv__184; fs_fsdisk_space_native(&__chx__lv__184, path, total_out, free_out, avail_out); &__chx__lv__184; }));
	return;
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/fs/posix/api.ch **/
static int fs_fsAT_FDCWD = -100;
static int fs_fsAT_SYMLINK_NOFOLLOW = 256;
static int fs_fsLOCK_SH = 1;
static int fs_fsLOCK_EX = 2;
static int fs_fsLOCK_NB = 4;
static int fs_fsLOCK_UN = 8;
static int fs_fsO_EXCL = 128;
static int fs_fsO_APPEND = 1024;
static int fs_fsO_DIRECTORY = 131072;
static int fs_fsAT_REMOVEDIR = 512;