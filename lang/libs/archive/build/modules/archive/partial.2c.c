
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/archive/src/crc32.ch **/
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/archive/src/zip_write.ch **/
struct archive_archiveZipWriter;
struct archive_archiveZipWriteEntry;
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/archive/src/types.ch **/
struct archive_archiveArchiveError;
struct archive_archiveArchiveEntry;
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/archive/src/endian.ch **/
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/archive/src/tar_read.ch **/
struct archive_archiveTarArchive;
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/archive/src/zip_read.ch **/
struct archive_archiveZipArchive;
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/archive/src/main.ch **/
struct archive_archiveArchive;
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/archive/src/deflate.ch **/
struct archive_archiveBitReader;
struct archive_archiveHuffEntry;
struct archive_archiveHuffTable;
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/archive/src/crc32.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/archive/src/zip_write.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/archive/src/types.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/archive/src/endian.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/archive/src/tar_read.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/archive/src/zip_read.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/archive/src/main.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/archive/src/deflate.ch **/
/** FwdDeclare:Generics archive **/
struct std_stdvector__cgs__4;
struct std_stdvector__cgs__5;
struct std_stdResult__cgs__37;
struct std_stdvector__cgs__6;
struct std_stdResult__cgs__38;
struct std_stdResult__cgs__39;
struct std_stdResult__cgs__40;
/** Declare:Generics archive **/
struct std_stdvector__cgs__4 {
	struct archive_archiveZipWriteEntry* data_ptr;
	size_t data_size;
	size_t data_cap;
};
void std_stdvector__cgs__4make(struct std_stdvector__cgs__4* this);
void std_stdvector__cgs__4make_with_capacity(struct std_stdvector__cgs__4* this, size_t init_cap);
void std_stdvector__cgs__4resize(struct std_stdvector__cgs__4* self, size_t new_size);
void std_stdvector__cgs__4reserve(struct std_stdvector__cgs__4* self, size_t new_cap);
void std_stdvector__cgs__4ensure_capacity_for_one_more(struct std_stdvector__cgs__4* self);
void std_stdvector__cgs__4push(struct std_stdvector__cgs__4* self, struct archive_archiveZipWriteEntry* value);
void std_stdvector__cgs__4push_back(struct std_stdvector__cgs__4* self, struct archive_archiveZipWriteEntry* value);
void std_stdvector__cgs__4get(struct archive_archiveZipWriteEntry* __chx_struct_ret_param_xx, struct std_stdvector__cgs__4*const self, size_t index);
struct archive_archiveZipWriteEntry* std_stdvector__cgs__4get_ptr(struct std_stdvector__cgs__4*const self, size_t index);
struct archive_archiveZipWriteEntry* std_stdvector__cgs__4get_ref(struct std_stdvector__cgs__4*const self, size_t index);
void std_stdvector__cgs__4set(struct std_stdvector__cgs__4* self, size_t index, struct archive_archiveZipWriteEntry* value);
size_t std_stdvector__cgs__4size(struct std_stdvector__cgs__4*const self);
size_t std_stdvector__cgs__4capacity(struct std_stdvector__cgs__4*const self);
const struct archive_archiveZipWriteEntry* std_stdvector__cgs__4data(struct std_stdvector__cgs__4*const self);
struct archive_archiveZipWriteEntry* std_stdvector__cgs__4last_ptr(struct std_stdvector__cgs__4*const self);
void std_stdvector__cgs__4remove(struct std_stdvector__cgs__4* self, size_t index);
void std_stdvector__cgs__4erase(struct std_stdvector__cgs__4* self, size_t index);
void std_stdvector__cgs__4remove_last(struct std_stdvector__cgs__4* self);
void std_stdvector__cgs__4pop_back(struct std_stdvector__cgs__4* self);
void std_stdvector__cgs__4take_last(struct archive_archiveZipWriteEntry* __chx_struct_ret_param_xx, struct std_stdvector__cgs__4* self);
_Bool std_stdvector__cgs__4empty(struct std_stdvector__cgs__4*const self);
void std_stdvector__cgs__4clear(struct std_stdvector__cgs__4* self);
void std_stdvector__cgs__4set_len(struct std_stdvector__cgs__4* self, size_t new_size);
void std_stdvector__cgs__4delete(struct std_stdvector__cgs__4* self);
struct std_stdvector__cgs__5 {
	struct archive_archiveArchiveEntry* data_ptr;
	size_t data_size;
	size_t data_cap;
};
void std_stdvector__cgs__5make(struct std_stdvector__cgs__5* this);
void std_stdvector__cgs__5make_with_capacity(struct std_stdvector__cgs__5* this, size_t init_cap);
void std_stdvector__cgs__5resize(struct std_stdvector__cgs__5* self, size_t new_size);
void std_stdvector__cgs__5reserve(struct std_stdvector__cgs__5* self, size_t new_cap);
void std_stdvector__cgs__5ensure_capacity_for_one_more(struct std_stdvector__cgs__5* self);
void std_stdvector__cgs__5push(struct std_stdvector__cgs__5* self, struct archive_archiveArchiveEntry* value);
void std_stdvector__cgs__5push_back(struct std_stdvector__cgs__5* self, struct archive_archiveArchiveEntry* value);
void std_stdvector__cgs__5get(struct archive_archiveArchiveEntry* __chx_struct_ret_param_xx, struct std_stdvector__cgs__5*const self, size_t index);
struct archive_archiveArchiveEntry* std_stdvector__cgs__5get_ptr(struct std_stdvector__cgs__5*const self, size_t index);
struct archive_archiveArchiveEntry* std_stdvector__cgs__5get_ref(struct std_stdvector__cgs__5*const self, size_t index);
void std_stdvector__cgs__5set(struct std_stdvector__cgs__5* self, size_t index, struct archive_archiveArchiveEntry* value);
size_t std_stdvector__cgs__5size(struct std_stdvector__cgs__5*const self);
size_t std_stdvector__cgs__5capacity(struct std_stdvector__cgs__5*const self);
const struct archive_archiveArchiveEntry* std_stdvector__cgs__5data(struct std_stdvector__cgs__5*const self);
struct archive_archiveArchiveEntry* std_stdvector__cgs__5last_ptr(struct std_stdvector__cgs__5*const self);
void std_stdvector__cgs__5remove(struct std_stdvector__cgs__5* self, size_t index);
void std_stdvector__cgs__5erase(struct std_stdvector__cgs__5* self, size_t index);
void std_stdvector__cgs__5remove_last(struct std_stdvector__cgs__5* self);
void std_stdvector__cgs__5pop_back(struct std_stdvector__cgs__5* self);
void std_stdvector__cgs__5take_last(struct archive_archiveArchiveEntry* __chx_struct_ret_param_xx, struct std_stdvector__cgs__5* self);
_Bool std_stdvector__cgs__5empty(struct std_stdvector__cgs__5*const self);
void std_stdvector__cgs__5clear(struct std_stdvector__cgs__5* self);
void std_stdvector__cgs__5set_len(struct std_stdvector__cgs__5* self, size_t new_size);
void std_stdvector__cgs__5delete(struct std_stdvector__cgs__5* self);
struct archive_archiveZipArchive {
	struct std_stdvector__cgs__3 data;
	struct std_stdvector__cgs__5 entries;
	_Bool data_loaded;
};
struct archive_archiveTarArchive {
	struct std_stdvector__cgs__3 data;
	struct std_stdvector__cgs__5 entries;
	_Bool data_loaded;
};
struct archive_archiveArchive {
	struct archive_archiveZipArchive zip_data;
	struct archive_archiveTarArchive tar_data;
	int archive_type;
	_Bool loaded;
};
struct archive_archiveArchiveError {
	int __chx__vt_621827;
	union {
		struct {
		} FileNotFound;
		struct {
			struct std_stdstring msg;
		} InvalidFormat;
		struct {
			int method;
		} UnsupportedCompression;
		struct {
			struct std_stdstring msg;
		} DecompressionFailed;
		struct {
		} CrcMismatch;
		struct {
			struct std_stdstring msg;
		} IoError;
		struct {
		} BufferTooSmall;
		struct {
			struct std_stdstring name;
		} FileAlreadyExists;
	};
};
struct std_stdResult__cgs__37 {
	int __chx__vt_621827;
	union {
		struct {
			struct archive_archiveArchive value;
		} Ok;
		struct {
			struct archive_archiveArchiveError error;
		} Err;
	};
};
void std_stdResult__cgs__37delete(struct std_stdResult__cgs__37* self);
struct std_stdvector__cgs__6 {
	struct archive_archiveHuffEntry* data_ptr;
	size_t data_size;
	size_t data_cap;
};
void std_stdvector__cgs__6make(struct std_stdvector__cgs__6* this);
void std_stdvector__cgs__6make_with_capacity(struct std_stdvector__cgs__6* this, size_t init_cap);
void std_stdvector__cgs__6resize(struct std_stdvector__cgs__6* self, size_t new_size);
void std_stdvector__cgs__6reserve(struct std_stdvector__cgs__6* self, size_t new_cap);
void std_stdvector__cgs__6ensure_capacity_for_one_more(struct std_stdvector__cgs__6* self);
void std_stdvector__cgs__6push(struct std_stdvector__cgs__6* self, struct archive_archiveHuffEntry* value);
void std_stdvector__cgs__6push_back(struct std_stdvector__cgs__6* self, struct archive_archiveHuffEntry* value);
void std_stdvector__cgs__6get(struct archive_archiveHuffEntry* __chx_struct_ret_param_xx, struct std_stdvector__cgs__6*const self, size_t index);
struct archive_archiveHuffEntry* std_stdvector__cgs__6get_ptr(struct std_stdvector__cgs__6*const self, size_t index);
struct archive_archiveHuffEntry* std_stdvector__cgs__6get_ref(struct std_stdvector__cgs__6*const self, size_t index);
void std_stdvector__cgs__6set(struct std_stdvector__cgs__6* self, size_t index, struct archive_archiveHuffEntry* value);
size_t std_stdvector__cgs__6size(struct std_stdvector__cgs__6*const self);
size_t std_stdvector__cgs__6capacity(struct std_stdvector__cgs__6*const self);
const struct archive_archiveHuffEntry* std_stdvector__cgs__6data(struct std_stdvector__cgs__6*const self);
struct archive_archiveHuffEntry* std_stdvector__cgs__6last_ptr(struct std_stdvector__cgs__6*const self);
void std_stdvector__cgs__6remove(struct std_stdvector__cgs__6* self, size_t index);
void std_stdvector__cgs__6erase(struct std_stdvector__cgs__6* self, size_t index);
void std_stdvector__cgs__6remove_last(struct std_stdvector__cgs__6* self);
void std_stdvector__cgs__6pop_back(struct std_stdvector__cgs__6* self);
void std_stdvector__cgs__6take_last(struct archive_archiveHuffEntry* __chx_struct_ret_param_xx, struct std_stdvector__cgs__6* self);
_Bool std_stdvector__cgs__6empty(struct std_stdvector__cgs__6*const self);
void std_stdvector__cgs__6clear(struct std_stdvector__cgs__6* self);
void std_stdvector__cgs__6set_len(struct std_stdvector__cgs__6* self, size_t new_size);
void std_stdvector__cgs__6delete(struct std_stdvector__cgs__6* self);
struct std_stdResult__cgs__38 {
	int __chx__vt_621827;
	union {
		struct {
			struct std_stdUnit value;
		} Ok;
		struct {
			struct archive_archiveArchiveError error;
		} Err;
	};
};
void std_stdResult__cgs__38delete(struct std_stdResult__cgs__38* self);
struct std_stdResult__cgs__39 {
	int __chx__vt_621827;
	union {
		struct {
			struct std_stdvector__cgs__3 value;
		} Ok;
		struct {
			struct archive_archiveArchiveError error;
		} Err;
	};
};
void std_stdResult__cgs__39delete(struct std_stdResult__cgs__39* self);
typedef struct __chx_core_coreiterableLinear__cgs__4_vt_t {
	const struct archive_archiveArchiveEntry*(*data)(void* self);uint64_t(*size)(void* self);
} __chx_core_coreiterableLinear__cgs__4_vt_t;
typedef struct __chx_core_coreiterableLinear__cgs__5_vt_t {
	const struct archive_archiveHuffEntry*(*data)(void* self);uint64_t(*size)(void* self);
} __chx_core_coreiterableLinear__cgs__5_vt_t;
typedef struct __chx_core_coreiterableLinear__cgs__6_vt_t {
	const struct archive_archiveZipWriteEntry*(*data)(void* self);uint64_t(*size)(void* self);
} __chx_core_coreiterableLinear__cgs__6_vt_t;
struct std_stdResult__cgs__40 {
	int __chx__vt_621827;
	union {
		struct {
			size_t value;
		} Ok;
		struct {
			struct archive_archiveArchiveError error;
		} Err;
	};
};
void std_stdResult__cgs__40delete(struct std_stdResult__cgs__40* self);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/archive/src/crc32.ch **/
static uint32_t archive_archivecrc32_table_entry(uint32_t i);
uint32_t archive_archivecrc32_compute(const uint8_t* data, size_t data_len);
uint32_t archive_archivecrc32_update(uint32_t crc, const uint8_t* data, size_t data_len);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/archive/src/zip_write.ch **/
struct archive_archiveZipWriter {
	struct std_stdvector__cgs__4 entries;
	struct std_stdvector__cgs__3 data;
};
void archive_archiveZipWritermake(struct archive_archiveZipWriter* this);
void archive_archiveZipWriterdelete(struct archive_archiveZipWriter* self);
struct archive_archiveZipWriteEntry {
	struct std_stdstring name;
	uint32_t uncompressed_size;
	uint32_t compressed_size;
	uint32_t crc;
	uint16_t method;
	uint32_t local_header_offset;
};
void archive_archiveZipWriteEntrydelete(struct archive_archiveZipWriteEntry* self);
void archive_archivezip_writer_add_file(struct archive_archiveZipWriter* writer, const char* name, const uint8_t* file_data, size_t file_len);
void archive_archivezip_writer_add_bytes(struct archive_archiveZipWriter* writer, const char* name, const uint8_t* data, size_t data_len);
void archive_archivezip_writer_add_store(struct archive_archiveZipWriter* writer, const char* name, const uint8_t* data, size_t data_len);
static void archive_archivewrite_local_header(struct archive_archiveZipWriter* writer, const char* name, uint32_t size, uint32_t crc);
void archive_archivezip_writer_close(struct archive_archiveZipWriter* writer);
struct std_stdvector__cgs__3* archive_archivezip_writer_to_bytes(struct archive_archiveZipWriter* writer);
void archive_archivezip_writer_save(struct std_stdResult__cgs__38* __chx_struct_ret_param_xx, struct archive_archiveZipWriter* writer, const char* path);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/archive/src/types.ch **/
void archive_archiveArchiveErrormessage(struct std_stdstring* __chx_struct_ret_param_xx, struct archive_archiveArchiveError*const self);
void archive_archiveArchiveErrordelete(struct archive_archiveArchiveError* self);
struct archive_archiveArchiveEntry {
	struct std_stdstring name;
	uint64_t size;
	uint64_t compressed_size;
	uint16_t compression_method;
	uint32_t crc32;
	_Bool is_directory;
	uint64_t offset;
};
void archive_archiveArchiveEntrydelete(struct archive_archiveArchiveEntry* self);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/archive/src/endian.ch **/
static uint16_t archive_archiveread_u16_le(const uint8_t* data, size_t offset);
static uint32_t archive_archiveread_u32_le(const uint8_t* data, size_t offset);
static uint64_t archive_archiveread_u64_le(const uint8_t* data, size_t offset);
static void archive_archivewrite_u16_le(uint8_t* out, size_t offset, uint16_t val);
static void archive_archivewrite_u32_le(uint8_t* out, size_t offset, uint32_t val);
static void archive_archivewrite_u64_le(uint8_t* out, size_t offset, uint64_t val);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/archive/src/tar_read.ch **/
void archive_archiveTarArchivedelete(struct archive_archiveTarArchive* self);
void archive_archiveopen_tar(struct std_stdResult__cgs__38* __chx_struct_ret_param_xx, const char* path, struct archive_archiveTarArchive* output);
static void archive_archiveparse_tar_entries(struct std_stdResult__cgs__38* __chx_struct_ret_param_xx, struct archive_archiveTarArchive* archive);
static uint64_t archive_archivetar_parse_octal(struct std_stdstring* str);
struct std_stdvector__cgs__5* archive_archivetar_entries(struct archive_archiveTarArchive* archive);
size_t archive_archivetar_entry_count(struct archive_archiveTarArchive* archive);
void archive_archivetar_find_entry(struct std_stdResult__cgs__38* __chx_struct_ret_param_xx, struct archive_archiveTarArchive* archive, const char* name, struct archive_archiveArchiveEntry* output);
void archive_archivetar_read_entry(struct std_stdResult__cgs__38* __chx_struct_ret_param_xx, struct archive_archiveTarArchive* archive, struct archive_archiveArchiveEntry* entry, struct std_stdvector__cgs__3* output);
void archive_archivetar_read_file(struct std_stdResult__cgs__39* __chx_struct_ret_param_xx, struct archive_archiveTarArchive* archive, const char* name);
_Bool archive_archivetar_contains(struct archive_archiveTarArchive* archive, const char* name);
void archive_archivetar_extract_entry(struct std_stdResult__cgs__38* __chx_struct_ret_param_xx, struct archive_archiveTarArchive* archive, struct archive_archiveArchiveEntry* entry, const char* dest_dir);
void archive_archivetar_extract_all(struct std_stdResult__cgs__38* __chx_struct_ret_param_xx, struct archive_archiveTarArchive* archive, const char* dest_dir);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/archive/src/zip_read.ch **/
static uint32_t archive_archiveZIP_LOCAL_FILE_HEADER;
static uint32_t archive_archiveZIP_CENTRAL_DIR_HEADER;
static uint32_t archive_archiveZIP_END_OF_CENTRAL_DIR;
void archive_archiveZipArchivedelete(struct archive_archiveZipArchive* self);
void archive_archiveopen_zip(struct std_stdResult__cgs__38* __chx_struct_ret_param_xx, const char* path, struct archive_archiveZipArchive* output);
void archive_archiveopen_zip_bytes(struct std_stdResult__cgs__38* __chx_struct_ret_param_xx, const uint8_t* data, size_t data_len, struct archive_archiveZipArchive* output);
static void archive_archiveparse_zip_central_dir(struct std_stdResult__cgs__38* __chx_struct_ret_param_xx, struct archive_archiveZipArchive* archive);
struct std_stdvector__cgs__5* archive_archivezip_entries(struct archive_archiveZipArchive* archive);
size_t archive_archivezip_entry_count(struct archive_archiveZipArchive* archive);
static _Bool archive_archivestr_equals_cstr(struct std_stdstring* s, const char* cstr);
void archive_archivezip_find_entry(struct std_stdResult__cgs__38* __chx_struct_ret_param_xx, struct archive_archiveZipArchive* archive, const char* name, struct archive_archiveArchiveEntry* output);
void archive_archivezip_read_entry(struct std_stdResult__cgs__38* __chx_struct_ret_param_xx, struct archive_archiveZipArchive* archive, struct archive_archiveArchiveEntry* entry, struct std_stdvector__cgs__3* output);
void archive_archivezip_read_file(struct std_stdResult__cgs__39* __chx_struct_ret_param_xx, struct archive_archiveZipArchive* archive, const char* name);
_Bool archive_archivezip_contains(struct archive_archiveZipArchive* archive, const char* name);
void archive_archivezip_extract_entry(struct std_stdResult__cgs__38* __chx_struct_ret_param_xx, struct archive_archiveZipArchive* archive, struct archive_archiveArchiveEntry* entry, const char* dest_dir);
void archive_archivezip_extract_all(struct std_stdResult__cgs__38* __chx_struct_ret_param_xx, struct archive_archiveZipArchive* archive, const char* dest_dir);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/archive/src/main.ch **/
void archive_archiveopen(struct std_stdResult__cgs__37* __chx_struct_ret_param_xx, const char* path);
struct std_stdvector__cgs__5* archive_archiveentries(struct archive_archiveArchive* archive);
size_t archive_archiveentry_count(struct archive_archiveArchive* archive);
_Bool archive_archivecontains(struct archive_archiveArchive* archive, const char* name);
void archive_archiveread(struct std_stdResult__cgs__39* __chx_struct_ret_param_xx, struct archive_archiveArchive* archive, const char* name);
void archive_archiveextract(struct std_stdResult__cgs__38* __chx_struct_ret_param_xx, struct archive_archiveArchive* archive, const char* dest);
void archive_archiveextract_entry(struct std_stdResult__cgs__38* __chx_struct_ret_param_xx, struct archive_archiveArchive* archive, const char* name, const char* dest_dir);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/archive/src/deflate.ch **/
struct archive_archiveBitReader {
	const uint8_t* data;
	size_t data_len;
	size_t byte_pos;
	uint32_t bit_buf;
	int bits_in_buf;
};
static void archive_archiveBitReaderinit(struct archive_archiveBitReader* __chx_struct_ret_param_xx, const uint8_t* d, size_t d_len);
static uint32_t archive_archiveBitReaderread_bits(struct archive_archiveBitReader* self, int n);
static uint32_t archive_archiveBitReaderread_bits_reversed(struct archive_archiveBitReader* self, int n);
static void archive_archiveBitReaderalign_to_byte(struct archive_archiveBitReader* self);
static int archive_archiveMAX_HUFFMAN_CODES;
static int archive_archiveMAX_HUFFMAN_BITS;
struct archive_archiveHuffEntry {
	uint16_t code;
	uint8_t bits;
	uint16_t value;
};
struct archive_archiveHuffTable {
	struct std_stdvector__cgs__6 entries;
	int min_bits;
	int max_bits;
};
static void archive_archiveHuffTabledelete(struct archive_archiveHuffTable* self);
static void archive_archivebuild_huffman_table(struct std_stdResult__cgs__38* __chx_struct_ret_param_xx, const uint8_t* lengths, int num_codes, struct archive_archiveHuffTable* output);
static uint16_t archive_archivedecode_huffman(struct archive_archiveBitReader* reader, struct archive_archiveHuffTable* table);
void archive_archivedeflate_decompress(struct std_stdResult__cgs__40* __chx_struct_ret_param_xx, const uint8_t* input, size_t input_len, uint8_t* output, size_t output_capacity);
/** Implement:Generics archive **/
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
		.data_ptr = ((struct archive_archiveZipWriteEntry*) malloc((sizeof(struct archive_archiveZipWriteEntry) * init_cap))), 
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
		struct archive_archiveZipWriteEntry* start = (self->data_ptr + new_size);
		struct archive_archiveZipWriteEntry*__chx__lv__0 = start;
		
		for(int __chx__lv__1 = to_destruct - 1; __chx__lv__1 >= 0;__chx__lv__1--){
			archive_archiveZipWriteEntrydelete(&__chx__lv__0[__chx__lv__1]);
		}
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
		{
			struct archive_archiveZipWriteEntry __chx__lv__2 = ((struct archive_archiveZipWriteEntry){0});
			archive_archiveZipWriteEntrydelete(&self->data_ptr[i]);
			self->data_ptr[i] = __chx__lv__2;
		};
		i = (i + 1);
	}
	self->data_size = new_size;
}
void std_stdvector__cgs__4reserve(struct std_stdvector__cgs__4* self, size_t new_cap){
	if((new_cap <= self->data_cap)){
		return;
	}
	struct archive_archiveZipWriteEntry* new_data = ((struct archive_archiveZipWriteEntry*) realloc(self->data_ptr, (sizeof(struct archive_archiveZipWriteEntry) * new_cap)));
	
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
void std_stdvector__cgs__4push(struct std_stdvector__cgs__4* self, struct archive_archiveZipWriteEntry* value){
	_Bool __chx__lv__3 = true;
	size_t s = self->data_size;
	if((s >= self->data_cap)){
		std_stdvector__cgs__4ensure_capacity_for_one_more(self);
	}
	memcpy(&self->data_ptr[s], value, sizeof(struct archive_archiveZipWriteEntry));
	1;
	self->data_size = (s + 1);
}
void std_stdvector__cgs__4push_back(struct std_stdvector__cgs__4* self, struct archive_archiveZipWriteEntry* value){
	_Bool __chx__lv__4 = true;
	std_stdvector__cgs__4push(self, *({ __chx__lv__4 = false; &value; }));
	if(__chx__lv__4) {
		archive_archiveZipWriteEntrydelete(value);
	}
}
void std_stdvector__cgs__4get(struct archive_archiveZipWriteEntry* __chx_struct_ret_param_xx, struct std_stdvector__cgs__4*const self, size_t index){
	*__chx_struct_ret_param_xx = self->data_ptr[index];
	return;
}
struct archive_archiveZipWriteEntry* std_stdvector__cgs__4get_ptr(struct std_stdvector__cgs__4*const self, size_t index){
	return &self->data_ptr[index];
}
struct archive_archiveZipWriteEntry* std_stdvector__cgs__4get_ref(struct std_stdvector__cgs__4*const self, size_t index){
	return &self->data_ptr[index];
}
void std_stdvector__cgs__4set(struct std_stdvector__cgs__4* self, size_t index, struct archive_archiveZipWriteEntry* value){
	_Bool __chx__lv__5 = true;
	{
		struct archive_archiveZipWriteEntry __chx__lv__6 = ({ __chx__lv__5 = false; *value; });
		archive_archiveZipWriteEntrydelete(&self->data_ptr[index]);
		self->data_ptr[index] = __chx__lv__6;
	};
	if(__chx__lv__5) {
		archive_archiveZipWriteEntrydelete(value);
	}
}
size_t std_stdvector__cgs__4size(struct std_stdvector__cgs__4*const self){
	return self->data_size;
}
size_t std_stdvector__cgs__4capacity(struct std_stdvector__cgs__4*const self){
	return self->data_cap;
}
const struct archive_archiveZipWriteEntry* std_stdvector__cgs__4data(struct std_stdvector__cgs__4*const self){
	return self->data_ptr;
}
struct archive_archiveZipWriteEntry* std_stdvector__cgs__4last_ptr(struct std_stdvector__cgs__4*const self){
	return (self->data_ptr + (self->data_size - 1));
}
void std_stdvector__cgs__4remove(struct std_stdvector__cgs__4* self, size_t index){
	size_t s = self->data_size;
	unsigned long last = (s - 1);
	struct archive_archiveZipWriteEntry*__chx__lv__7 = std_stdvector__cgs__4get_ptr(self, index);
	
	archive_archiveZipWriteEntrydelete(__chx__lv__7);
	if((index == last)){
		self->data_size = last;
	} else {
		memmove(&self->data_ptr[index], &self->data_ptr[(index + 1)], (sizeof(struct archive_archiveZipWriteEntry) * (last - index)));
		self->data_size = last;
	}
}
void std_stdvector__cgs__4erase(struct std_stdvector__cgs__4* self, size_t index){
	std_stdvector__cgs__4remove(self, index);
}
void std_stdvector__cgs__4remove_last(struct std_stdvector__cgs__4* self){
	size_t s = self->data_size;
	unsigned long last = (s - 1);
	struct archive_archiveZipWriteEntry* ptr = std_stdvector__cgs__4get_ptr(self, last);
	struct archive_archiveZipWriteEntry*__chx__lv__8 = ptr;
	
	archive_archiveZipWriteEntrydelete(__chx__lv__8);
	self->data_size = last;
}
void std_stdvector__cgs__4pop_back(struct std_stdvector__cgs__4* self){
	std_stdvector__cgs__4remove_last(self);
}
void std_stdvector__cgs__4take_last(struct archive_archiveZipWriteEntry* __chx_struct_ret_param_xx, struct std_stdvector__cgs__4* self){
	unsigned long last = (self->data_size - 1);
	self->data_size = last;
	
	*__chx_struct_ret_param_xx = *std_stdvector__cgs__4get_ptr(self, last);
	return;
}
_Bool std_stdvector__cgs__4empty(struct std_stdvector__cgs__4*const self){
	return (self->data_size == 0);
}
void std_stdvector__cgs__4clear(struct std_stdvector__cgs__4* self){
	struct archive_archiveZipWriteEntry*__chx__lv__9 = self->data_ptr;
	
	for(int __chx__lv__10 = self->data_size - 1; __chx__lv__10 >= 0;__chx__lv__10--){
		archive_archiveZipWriteEntrydelete(&__chx__lv__9[__chx__lv__10]);
	}
	self->data_size = 0;
}
void std_stdvector__cgs__4set_len(struct std_stdvector__cgs__4* self, size_t new_size){
	self->data_size = new_size;
}
void std_stdvector__cgs__4delete(struct std_stdvector__cgs__4* self){
	if((self->data_ptr != NULL)){
		struct archive_archiveZipWriteEntry*__chx__lv__11 = self->data_ptr;
		
		for(int __chx__lv__12 = self->data_size - 1; __chx__lv__12 >= 0;__chx__lv__12--){
			archive_archiveZipWriteEntrydelete(&__chx__lv__11[__chx__lv__12]);
		}
		free(self->data_ptr);
	}
	__chx__dstctr_clnup_blk__:{
	}
}
const struct archive_archiveZipWriteEntry* core_coreiterableLinear__cgs__6_vector__cgs__4_data(struct std_stdvector__cgs__4*const self){
	return self->data_ptr;
}
size_t core_coreiterableLinear__cgs__6_vector__cgs__4_size(struct std_stdvector__cgs__4*const self){
	return self->data_size;
}
const __chx_core_coreiterableLinear__cgs__6_vt_t core_coreiterableLinear__cgs__6std_stdvector__cgs__4 = {
	(const struct archive_archiveZipWriteEntry*(*)(void* self)) core_coreiterableLinear__cgs__6_vector__cgs__4_data,
	(uint64_t(*)(void* self)) core_coreiterableLinear__cgs__6_vector__cgs__4_size,
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
		.data_ptr = ((struct archive_archiveArchiveEntry*) malloc((sizeof(struct archive_archiveArchiveEntry) * init_cap))), 
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
		struct archive_archiveArchiveEntry* start = (self->data_ptr + new_size);
		struct archive_archiveArchiveEntry*__chx__lv__13 = start;
		
		for(int __chx__lv__14 = to_destruct - 1; __chx__lv__14 >= 0;__chx__lv__14--){
			archive_archiveArchiveEntrydelete(&__chx__lv__13[__chx__lv__14]);
		}
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
		{
			struct archive_archiveArchiveEntry __chx__lv__15 = ((struct archive_archiveArchiveEntry){0});
			archive_archiveArchiveEntrydelete(&self->data_ptr[i]);
			self->data_ptr[i] = __chx__lv__15;
		};
		i = (i + 1);
	}
	self->data_size = new_size;
}
void std_stdvector__cgs__5reserve(struct std_stdvector__cgs__5* self, size_t new_cap){
	if((new_cap <= self->data_cap)){
		return;
	}
	struct archive_archiveArchiveEntry* new_data = ((struct archive_archiveArchiveEntry*) realloc(self->data_ptr, (sizeof(struct archive_archiveArchiveEntry) * new_cap)));
	
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
void std_stdvector__cgs__5push(struct std_stdvector__cgs__5* self, struct archive_archiveArchiveEntry* value){
	_Bool __chx__lv__16 = true;
	size_t s = self->data_size;
	if((s >= self->data_cap)){
		std_stdvector__cgs__5ensure_capacity_for_one_more(self);
	}
	memcpy(&self->data_ptr[s], value, sizeof(struct archive_archiveArchiveEntry));
	1;
	self->data_size = (s + 1);
}
void std_stdvector__cgs__5push_back(struct std_stdvector__cgs__5* self, struct archive_archiveArchiveEntry* value){
	_Bool __chx__lv__17 = true;
	std_stdvector__cgs__5push(self, *({ __chx__lv__17 = false; &value; }));
	if(__chx__lv__17) {
		archive_archiveArchiveEntrydelete(value);
	}
}
void std_stdvector__cgs__5get(struct archive_archiveArchiveEntry* __chx_struct_ret_param_xx, struct std_stdvector__cgs__5*const self, size_t index){
	*__chx_struct_ret_param_xx = self->data_ptr[index];
	return;
}
struct archive_archiveArchiveEntry* std_stdvector__cgs__5get_ptr(struct std_stdvector__cgs__5*const self, size_t index){
	return &self->data_ptr[index];
}
struct archive_archiveArchiveEntry* std_stdvector__cgs__5get_ref(struct std_stdvector__cgs__5*const self, size_t index){
	return &self->data_ptr[index];
}
void std_stdvector__cgs__5set(struct std_stdvector__cgs__5* self, size_t index, struct archive_archiveArchiveEntry* value){
	_Bool __chx__lv__18 = true;
	{
		struct archive_archiveArchiveEntry __chx__lv__19 = ({ __chx__lv__18 = false; *value; });
		archive_archiveArchiveEntrydelete(&self->data_ptr[index]);
		self->data_ptr[index] = __chx__lv__19;
	};
	if(__chx__lv__18) {
		archive_archiveArchiveEntrydelete(value);
	}
}
size_t std_stdvector__cgs__5size(struct std_stdvector__cgs__5*const self){
	return self->data_size;
}
size_t std_stdvector__cgs__5capacity(struct std_stdvector__cgs__5*const self){
	return self->data_cap;
}
const struct archive_archiveArchiveEntry* std_stdvector__cgs__5data(struct std_stdvector__cgs__5*const self){
	return self->data_ptr;
}
struct archive_archiveArchiveEntry* std_stdvector__cgs__5last_ptr(struct std_stdvector__cgs__5*const self){
	return (self->data_ptr + (self->data_size - 1));
}
void std_stdvector__cgs__5remove(struct std_stdvector__cgs__5* self, size_t index){
	size_t s = self->data_size;
	unsigned long last = (s - 1);
	struct archive_archiveArchiveEntry*__chx__lv__20 = std_stdvector__cgs__5get_ptr(self, index);
	
	archive_archiveArchiveEntrydelete(__chx__lv__20);
	if((index == last)){
		self->data_size = last;
	} else {
		memmove(&self->data_ptr[index], &self->data_ptr[(index + 1)], (sizeof(struct archive_archiveArchiveEntry) * (last - index)));
		self->data_size = last;
	}
}
void std_stdvector__cgs__5erase(struct std_stdvector__cgs__5* self, size_t index){
	std_stdvector__cgs__5remove(self, index);
}
void std_stdvector__cgs__5remove_last(struct std_stdvector__cgs__5* self){
	size_t s = self->data_size;
	unsigned long last = (s - 1);
	struct archive_archiveArchiveEntry* ptr = std_stdvector__cgs__5get_ptr(self, last);
	struct archive_archiveArchiveEntry*__chx__lv__21 = ptr;
	
	archive_archiveArchiveEntrydelete(__chx__lv__21);
	self->data_size = last;
}
void std_stdvector__cgs__5pop_back(struct std_stdvector__cgs__5* self){
	std_stdvector__cgs__5remove_last(self);
}
void std_stdvector__cgs__5take_last(struct archive_archiveArchiveEntry* __chx_struct_ret_param_xx, struct std_stdvector__cgs__5* self){
	unsigned long last = (self->data_size - 1);
	self->data_size = last;
	
	*__chx_struct_ret_param_xx = *std_stdvector__cgs__5get_ptr(self, last);
	return;
}
_Bool std_stdvector__cgs__5empty(struct std_stdvector__cgs__5*const self){
	return (self->data_size == 0);
}
void std_stdvector__cgs__5clear(struct std_stdvector__cgs__5* self){
	struct archive_archiveArchiveEntry*__chx__lv__22 = self->data_ptr;
	
	for(int __chx__lv__23 = self->data_size - 1; __chx__lv__23 >= 0;__chx__lv__23--){
		archive_archiveArchiveEntrydelete(&__chx__lv__22[__chx__lv__23]);
	}
	self->data_size = 0;
}
void std_stdvector__cgs__5set_len(struct std_stdvector__cgs__5* self, size_t new_size){
	self->data_size = new_size;
}
void std_stdvector__cgs__5delete(struct std_stdvector__cgs__5* self){
	if((self->data_ptr != NULL)){
		struct archive_archiveArchiveEntry*__chx__lv__24 = self->data_ptr;
		
		for(int __chx__lv__25 = self->data_size - 1; __chx__lv__25 >= 0;__chx__lv__25--){
			archive_archiveArchiveEntrydelete(&__chx__lv__24[__chx__lv__25]);
		}
		free(self->data_ptr);
	}
	__chx__dstctr_clnup_blk__:{
	}
}
const struct archive_archiveArchiveEntry* core_coreiterableLinear__cgs__4_vector__cgs__5_data(struct std_stdvector__cgs__5*const self){
	return self->data_ptr;
}
size_t core_coreiterableLinear__cgs__4_vector__cgs__5_size(struct std_stdvector__cgs__5*const self){
	return self->data_size;
}
const __chx_core_coreiterableLinear__cgs__4_vt_t core_coreiterableLinear__cgs__4std_stdvector__cgs__5 = {
	(const struct archive_archiveArchiveEntry*(*)(void* self)) core_coreiterableLinear__cgs__4_vector__cgs__5_data,
	(uint64_t(*)(void* self)) core_coreiterableLinear__cgs__4_vector__cgs__5_size,
};
void std_stdResult__cgs__37delete(struct std_stdResult__cgs__37* self){
	__chx__dstctr_clnup_blk__:{
		switch(self->__chx__vt_621827) {
			case 0:
			break;
			case 1:
			archive_archiveArchiveErrordelete(&self->Err.error);
			break;
		}
	}
}
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
		.data_ptr = ((struct archive_archiveHuffEntry*) malloc((sizeof(struct archive_archiveHuffEntry) * init_cap))), 
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
		struct archive_archiveHuffEntry* start = (self->data_ptr + new_size);
		
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
		self->data_ptr[i] = ((struct archive_archiveHuffEntry){0});
		i = (i + 1);
	}
	self->data_size = new_size;
}
void std_stdvector__cgs__6reserve(struct std_stdvector__cgs__6* self, size_t new_cap){
	if((new_cap <= self->data_cap)){
		return;
	}
	struct archive_archiveHuffEntry* new_data = ((struct archive_archiveHuffEntry*) realloc(self->data_ptr, (sizeof(struct archive_archiveHuffEntry) * new_cap)));
	
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
void std_stdvector__cgs__6push(struct std_stdvector__cgs__6* self, struct archive_archiveHuffEntry* value){
	size_t s = self->data_size;
	if((s >= self->data_cap)){
		std_stdvector__cgs__6ensure_capacity_for_one_more(self);
	}
	memcpy(&self->data_ptr[s], value, sizeof(struct archive_archiveHuffEntry));
	0;
	self->data_size = (s + 1);
}
void std_stdvector__cgs__6push_back(struct std_stdvector__cgs__6* self, struct archive_archiveHuffEntry* value){
	std_stdvector__cgs__6push(self, *({  &value; }));
}
void std_stdvector__cgs__6get(struct archive_archiveHuffEntry* __chx_struct_ret_param_xx, struct std_stdvector__cgs__6*const self, size_t index){
	*__chx_struct_ret_param_xx = self->data_ptr[index];
	return;
}
struct archive_archiveHuffEntry* std_stdvector__cgs__6get_ptr(struct std_stdvector__cgs__6*const self, size_t index){
	return &self->data_ptr[index];
}
struct archive_archiveHuffEntry* std_stdvector__cgs__6get_ref(struct std_stdvector__cgs__6*const self, size_t index){
	return &self->data_ptr[index];
}
void std_stdvector__cgs__6set(struct std_stdvector__cgs__6* self, size_t index, struct archive_archiveHuffEntry* value){
	self->data_ptr[index] = ({  *value; });
}
size_t std_stdvector__cgs__6size(struct std_stdvector__cgs__6*const self){
	return self->data_size;
}
size_t std_stdvector__cgs__6capacity(struct std_stdvector__cgs__6*const self){
	return self->data_cap;
}
const struct archive_archiveHuffEntry* std_stdvector__cgs__6data(struct std_stdvector__cgs__6*const self){
	return self->data_ptr;
}
struct archive_archiveHuffEntry* std_stdvector__cgs__6last_ptr(struct std_stdvector__cgs__6*const self){
	return (self->data_ptr + (self->data_size - 1));
}
void std_stdvector__cgs__6remove(struct std_stdvector__cgs__6* self, size_t index){
	size_t s = self->data_size;
	unsigned long last = (s - 1);
	
	if((index == last)){
		self->data_size = last;
	} else {
		memmove(&self->data_ptr[index], &self->data_ptr[(index + 1)], (sizeof(struct archive_archiveHuffEntry) * (last - index)));
		self->data_size = last;
	}
}
void std_stdvector__cgs__6erase(struct std_stdvector__cgs__6* self, size_t index){
	std_stdvector__cgs__6remove(self, index);
}
void std_stdvector__cgs__6remove_last(struct std_stdvector__cgs__6* self){
	size_t s = self->data_size;
	unsigned long last = (s - 1);
	struct archive_archiveHuffEntry* ptr = std_stdvector__cgs__6get_ptr(self, last);
	
	self->data_size = last;
}
void std_stdvector__cgs__6pop_back(struct std_stdvector__cgs__6* self){
	std_stdvector__cgs__6remove_last(self);
}
void std_stdvector__cgs__6take_last(struct archive_archiveHuffEntry* __chx_struct_ret_param_xx, struct std_stdvector__cgs__6* self){
	unsigned long last = (self->data_size - 1);
	self->data_size = last;
	
	*__chx_struct_ret_param_xx = *std_stdvector__cgs__6get_ptr(self, last);
	return;
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
const struct archive_archiveHuffEntry* core_coreiterableLinear__cgs__5_vector__cgs__6_data(struct std_stdvector__cgs__6*const self){
	return self->data_ptr;
}
size_t core_coreiterableLinear__cgs__5_vector__cgs__6_size(struct std_stdvector__cgs__6*const self){
	return self->data_size;
}
const __chx_core_coreiterableLinear__cgs__5_vt_t core_coreiterableLinear__cgs__5std_stdvector__cgs__6 = {
	(const struct archive_archiveHuffEntry*(*)(void* self)) core_coreiterableLinear__cgs__5_vector__cgs__6_data,
	(uint64_t(*)(void* self)) core_coreiterableLinear__cgs__5_vector__cgs__6_size,
};
void std_stdResult__cgs__38delete(struct std_stdResult__cgs__38* self){
	__chx__dstctr_clnup_blk__:{
		switch(self->__chx__vt_621827) {
			case 0:
			break;
			case 1:
			archive_archiveArchiveErrordelete(&self->Err.error);
			break;
		}
	}
}
void std_stdResult__cgs__39delete(struct std_stdResult__cgs__39* self){
	__chx__dstctr_clnup_blk__:{
		switch(self->__chx__vt_621827) {
			case 0:
			std_stdvector__cgs__3delete(&self->Ok.value);
			break;
			case 1:
			archive_archiveArchiveErrordelete(&self->Err.error);
			break;
		}
	}
}
void std_stdResult__cgs__40delete(struct std_stdResult__cgs__40* self){
	__chx__dstctr_clnup_blk__:{
		switch(self->__chx__vt_621827) {
			case 0:
			break;
			case 1:
			archive_archiveArchiveErrordelete(&self->Err.error);
			break;
		}
	}
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/archive/src/crc32.ch **/
static uint32_t archive_archivecrc32_table_entry(uint32_t i){
	uint32_t crc = i;
	uint32_t j = 0;
	while((j < 8)) {
		if((crc & (1 != 0))){
			crc = ((crc >> 1) ^ 3988292384);
		} else {
			crc = (crc >> 1);
		}
		j += 1;
	}
	return crc;
}
uint32_t archive_archivecrc32_compute(const uint8_t* data, size_t data_len){
	uint32_t crc = 4294967295;
	size_t i = 0;
	while((i < data_len)) {
		uint32_t idx = ((crc ^ ((uint32_t) data[i])) & 255);
		uint32_t table_val = archive_archivecrc32_table_entry(idx);
		crc = (table_val ^ (crc >> 8));
		i += 1;
	}
	return (crc ^ 4294967295);
}
uint32_t archive_archivecrc32_update(uint32_t crc, const uint8_t* data, size_t data_len){
	uint32_t c = (crc ^ 4294967295);
	size_t i = 0;
	while((i < data_len)) {
		uint32_t idx = ((c ^ ((uint32_t) data[i])) & 255);
		uint32_t table_val = archive_archivecrc32_table_entry(idx);
		c = (table_val ^ (c >> 8));
		i += 1;
	}
	return (c ^ 4294967295);
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/archive/src/zip_write.ch **/
void archive_archiveZipWritermake(struct archive_archiveZipWriter* this){
	std_stdvector__cgs__4make(&this->entries);
	std_stdvector__cgs__3make(&this->data);
}
void archive_archiveZipWriterdelete(struct archive_archiveZipWriter* self){
	__chx__dstctr_clnup_blk__:{
		std_stdvector__cgs__4delete(&self->entries);
		std_stdvector__cgs__3delete(&self->data);
	}
}
void archive_archiveZipWriteEntrydelete(struct archive_archiveZipWriteEntry* self){
	__chx__dstctr_clnup_blk__:{
		std_stdstringdelete(&self->name);
	}
}
void archive_archivezip_writer_add_file(struct archive_archiveZipWriter* writer, const char* name, const uint8_t* file_data, size_t file_len){
	uint32_t crc = archive_archivecrc32_compute(file_data, file_len);
	struct archive_archiveZipWriteEntry entry;
	_Bool __chx__lv__26 = true;
	{
		struct std_stdstring __chx__lv__27 = (*({ struct std_stdstring __chx__lv__28; std_stdstringconstructor2(&__chx__lv__28, "", 0, 0); &__chx__lv__28; }));
		std_stdstringdelete(&entry.name);
		entry.name = __chx__lv__27;
	};
	std_stdstringappend_char_ptr(&entry.name, name);
	entry.uncompressed_size = ((uint32_t) file_len);
	entry.compressed_size = ((uint32_t) file_len);
	entry.crc = crc;
	entry.method = 0;
	entry.local_header_offset = ((uint32_t) std_stdvector__cgs__3size(&writer->data));
	archive_archivewrite_local_header(writer, name, ((uint32_t) file_len), crc);
	size_t i = 0;
	while((i < file_len)) {
		std_stdvector__cgs__3push(&writer->data, file_data[i]);
		i += 1;
	}
	std_stdvector__cgs__4push(&writer->entries, ({ __chx__lv__26 = false; &entry; }));
	if(__chx__lv__26) {
		archive_archiveZipWriteEntrydelete(&entry);
	}
}
void archive_archivezip_writer_add_bytes(struct archive_archiveZipWriter* writer, const char* name, const uint8_t* data, size_t data_len){
	archive_archivezip_writer_add_file(writer, name, data, data_len);
}
void archive_archivezip_writer_add_store(struct archive_archiveZipWriter* writer, const char* name, const uint8_t* data, size_t data_len){
	archive_archivezip_writer_add_file(writer, name, data, data_len);
}
static void archive_archivewrite_local_header(struct archive_archiveZipWriter* writer, const char* name, uint32_t size, uint32_t crc){
	size_t name_len = 0;
	const char* temp = name;
	while((temp[0] != ((char) '\0'))) {
		name_len += 1;
		temp = (temp + 1);
	}
	size_t header_size = (30 + name_len);
	struct std_stdvector__cgs__3 header = (*({ struct std_stdvector__cgs__3 __chx__lv__29; std_stdvector__cgs__3make(&__chx__lv__29); &__chx__lv__29; }));
	_Bool __chx__lv__30 = true;
	std_stdvector__cgs__3resize(&header, header_size);
	archive_archivewrite_u32_le(((uint8_t*) std_stdvector__cgs__3data(&header)), 0, archive_archiveZIP_LOCAL_FILE_HEADER);
	archive_archivewrite_u16_le(((uint8_t*) std_stdvector__cgs__3data(&header)), 4, 20);
	archive_archivewrite_u16_le(((uint8_t*) std_stdvector__cgs__3data(&header)), 6, 0);
	archive_archivewrite_u16_le(((uint8_t*) std_stdvector__cgs__3data(&header)), 8, 0);
	archive_archivewrite_u16_le(((uint8_t*) std_stdvector__cgs__3data(&header)), 10, 0);
	archive_archivewrite_u16_le(((uint8_t*) std_stdvector__cgs__3data(&header)), 12, 0);
	archive_archivewrite_u32_le(((uint8_t*) std_stdvector__cgs__3data(&header)), 14, crc);
	archive_archivewrite_u32_le(((uint8_t*) std_stdvector__cgs__3data(&header)), 18, size);
	archive_archivewrite_u32_le(((uint8_t*) std_stdvector__cgs__3data(&header)), 22, size);
	archive_archivewrite_u16_le(((uint8_t*) std_stdvector__cgs__3data(&header)), 26, ((uint16_t) name_len));
	archive_archivewrite_u16_le(((uint8_t*) std_stdvector__cgs__3data(&header)), 28, 0);
	size_t i = 0;
	uint8_t* hptr = ((uint8_t*) std_stdvector__cgs__3data(&header));
	while((i < name_len)) {
		hptr[(30 + i)] = ((uint8_t) name[i]);
		i += 1;
	}
	i = 0;
	while((i < header_size)) {
		std_stdvector__cgs__3push(&writer->data, std_stdvector__cgs__3data(&header)[i]);
		i += 1;
	}
	if(__chx__lv__30) {
		std_stdvector__cgs__3delete(&header);
	}
}
void archive_archivezip_writer_close(struct archive_archiveZipWriter* writer){
	uint32_t cd_offset = ((uint32_t) std_stdvector__cgs__3size(&writer->data));
	uint32_t cd_size = 0;
	size_t i = 0;
	while((i < std_stdvector__cgs__4size(&writer->entries))) {
		struct archive_archiveZipWriteEntry* entry = std_stdvector__cgs__4get_ptr(&writer->entries, i);
		uint16_t name_len = ((uint16_t) std_stdstringsize(&entry->name));
		struct std_stdvector__cgs__3 header = (*({ struct std_stdvector__cgs__3 __chx__lv__31; std_stdvector__cgs__3make(&__chx__lv__31); &__chx__lv__31; }));
		_Bool __chx__lv__32 = true;
		std_stdvector__cgs__3resize(&header, (46 + ((size_t) name_len)));
		archive_archivewrite_u32_le(((uint8_t*) std_stdvector__cgs__3data(&header)), 0, archive_archiveZIP_CENTRAL_DIR_HEADER);
		archive_archivewrite_u16_le(((uint8_t*) std_stdvector__cgs__3data(&header)), 4, 20);
		archive_archivewrite_u16_le(((uint8_t*) std_stdvector__cgs__3data(&header)), 6, 20);
		archive_archivewrite_u16_le(((uint8_t*) std_stdvector__cgs__3data(&header)), 8, 0);
		archive_archivewrite_u16_le(((uint8_t*) std_stdvector__cgs__3data(&header)), 10, entry->method);
		archive_archivewrite_u16_le(((uint8_t*) std_stdvector__cgs__3data(&header)), 12, 0);
		archive_archivewrite_u16_le(((uint8_t*) std_stdvector__cgs__3data(&header)), 14, 0);
		archive_archivewrite_u32_le(((uint8_t*) std_stdvector__cgs__3data(&header)), 16, entry->crc);
		archive_archivewrite_u32_le(((uint8_t*) std_stdvector__cgs__3data(&header)), 20, entry->compressed_size);
		archive_archivewrite_u32_le(((uint8_t*) std_stdvector__cgs__3data(&header)), 24, entry->uncompressed_size);
		archive_archivewrite_u16_le(((uint8_t*) std_stdvector__cgs__3data(&header)), 28, name_len);
		archive_archivewrite_u16_le(((uint8_t*) std_stdvector__cgs__3data(&header)), 30, 0);
		archive_archivewrite_u16_le(((uint8_t*) std_stdvector__cgs__3data(&header)), 32, 0);
		archive_archivewrite_u16_le(((uint8_t*) std_stdvector__cgs__3data(&header)), 34, 0);
		archive_archivewrite_u16_le(((uint8_t*) std_stdvector__cgs__3data(&header)), 36, 0);
		archive_archivewrite_u32_le(((uint8_t*) std_stdvector__cgs__3data(&header)), 38, 0);
		archive_archivewrite_u32_le(((uint8_t*) std_stdvector__cgs__3data(&header)), 42, entry->local_header_offset);
		uint16_t j = 0;
		uint8_t* hptr2 = ((uint8_t*) std_stdvector__cgs__3data(&header));
		while((j < name_len)) {
			hptr2[(46 + ((size_t) j))] = ((uint8_t) std_stdstringget(&entry->name, ((size_t) j)));
			j += 1;
		}
		size_t k = 0;
		while((k < (46 + ((size_t) name_len)))) {
			std_stdvector__cgs__3push(&writer->data, std_stdvector__cgs__3data(&header)[k]);
			k += 1;
		}
		cd_size += (46 + ((uint32_t) name_len));
		i += 1;
		if(__chx__lv__32) {
			std_stdvector__cgs__3delete(&header);
		}
	}
	struct std_stdvector__cgs__3 eocd = (*({ struct std_stdvector__cgs__3 __chx__lv__33; std_stdvector__cgs__3make(&__chx__lv__33); &__chx__lv__33; }));
	_Bool __chx__lv__34 = true;
	std_stdvector__cgs__3resize(&eocd, 22);
	archive_archivewrite_u32_le(((uint8_t*) std_stdvector__cgs__3data(&eocd)), 0, archive_archiveZIP_END_OF_CENTRAL_DIR);
	archive_archivewrite_u16_le(((uint8_t*) std_stdvector__cgs__3data(&eocd)), 4, 0);
	archive_archivewrite_u16_le(((uint8_t*) std_stdvector__cgs__3data(&eocd)), 6, 0);
	archive_archivewrite_u16_le(((uint8_t*) std_stdvector__cgs__3data(&eocd)), 8, ((uint16_t) std_stdvector__cgs__4size(&writer->entries)));
	archive_archivewrite_u16_le(((uint8_t*) std_stdvector__cgs__3data(&eocd)), 10, ((uint16_t) std_stdvector__cgs__4size(&writer->entries)));
	archive_archivewrite_u32_le(((uint8_t*) std_stdvector__cgs__3data(&eocd)), 12, cd_size);
	archive_archivewrite_u32_le(((uint8_t*) std_stdvector__cgs__3data(&eocd)), 16, cd_offset);
	archive_archivewrite_u16_le(((uint8_t*) std_stdvector__cgs__3data(&eocd)), 20, 0);
	size_t m = 0;
	while((m < 22)) {
		std_stdvector__cgs__3push(&writer->data, std_stdvector__cgs__3data(&eocd)[m]);
		m += 1;
	}
	if(__chx__lv__34) {
		std_stdvector__cgs__3delete(&eocd);
	}
}
struct std_stdvector__cgs__3* archive_archivezip_writer_to_bytes(struct archive_archiveZipWriter* writer){
	archive_archivezip_writer_close(writer);
	return &writer->data;
}
void archive_archivezip_writer_save(struct std_stdResult__cgs__38* __chx_struct_ret_param_xx, struct archive_archiveZipWriter* writer, const char* path){
	archive_archivezip_writer_close(writer);
	struct std_stdResult__cgs__34 write_result = (*({ struct std_stdResult__cgs__34 __chx__lv__35; fs_fswrite_text_file(&__chx__lv__35, path, std_stdvector__cgs__3data(&writer->data), std_stdvector__cgs__3size(&writer->data)); &__chx__lv__35; }));
	if((write_result.__chx__vt_621827 == 1)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 1, 
			.Err.error = (struct archive_archiveArchiveError) { .__chx__vt_621827 = 5, 
				.IoError.msg = (*({ struct std_stdstring __chx__lv__36; std_stdstringconstructor2(&__chx__lv__36, "failed to write ZIP data", 24, 0); &__chx__lv__36; }))
			}
		};
		return;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 0, 
		.Ok.value = (struct std_stdUnit){ 
		}
	};
	return;
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/archive/src/types.ch **/
void archive_archiveArchiveErrormessage(struct std_stdstring* __chx_struct_ret_param_xx, struct archive_archiveArchiveError*const self){
	switch((self)->__chx__vt_621827) {
		case 0:{
			*__chx_struct_ret_param_xx = (*({ struct std_stdstring __chx__lv__37; std_stdstringconstructor2(&__chx__lv__37, "ArchiveError: file not found", 28, 0); &__chx__lv__37; }));
			return;
			break;
		}
		case 1:{
			struct std_stdstring s = (*({ struct std_stdstring __chx__lv__38; std_stdstringconstructor2(&__chx__lv__38, "ArchiveError: ", 14, 0); &__chx__lv__38; }));
			_Bool __chx__lv__39 = true;
			std_stdstringappend_string(&s, &(self)->InvalidFormat.msg);
			*__chx_struct_ret_param_xx = s;
			return;
			break;
		}
		case 2:{
			struct std_stdstring s = (*({ struct std_stdstring __chx__lv__40; std_stdstringconstructor2(&__chx__lv__40, "ArchiveError: unsupported compression method ", 45, 0); &__chx__lv__40; }));
			_Bool __chx__lv__41 = true;
			std_stdstringappend_integer(&s, (self)->UnsupportedCompression.method);
			*__chx_struct_ret_param_xx = s;
			return;
			break;
		}
		case 3:{
			struct std_stdstring s = (*({ struct std_stdstring __chx__lv__42; std_stdstringconstructor2(&__chx__lv__42, "ArchiveError: decompression failed: ", 36, 0); &__chx__lv__42; }));
			_Bool __chx__lv__43 = true;
			std_stdstringappend_string(&s, &(self)->DecompressionFailed.msg);
			*__chx_struct_ret_param_xx = s;
			return;
			break;
		}
		case 4:{
			*__chx_struct_ret_param_xx = (*({ struct std_stdstring __chx__lv__44; std_stdstringconstructor2(&__chx__lv__44, "ArchiveError: CRC mismatch", 26, 0); &__chx__lv__44; }));
			return;
			break;
		}
		case 5:{
			struct std_stdstring s = (*({ struct std_stdstring __chx__lv__45; std_stdstringconstructor2(&__chx__lv__45, "ArchiveError: IO error: ", 24, 0); &__chx__lv__45; }));
			_Bool __chx__lv__46 = true;
			std_stdstringappend_string(&s, &(self)->IoError.msg);
			*__chx_struct_ret_param_xx = s;
			return;
			break;
		}
		case 6:{
			*__chx_struct_ret_param_xx = (*({ struct std_stdstring __chx__lv__47; std_stdstringconstructor2(&__chx__lv__47, "ArchiveError: buffer too small", 30, 0); &__chx__lv__47; }));
			return;
			break;
		}
		case 7:{
			struct std_stdstring s = (*({ struct std_stdstring __chx__lv__48; std_stdstringconstructor2(&__chx__lv__48, "ArchiveError: file already exists: ", 35, 0); &__chx__lv__48; }));
			_Bool __chx__lv__49 = true;
			std_stdstringappend_string(&s, &(self)->FileAlreadyExists.name);
			*__chx_struct_ret_param_xx = s;
			return;
			break;
		}
	}
}
void archive_archiveArchiveErrordelete(struct archive_archiveArchiveError* self){
	__chx__dstctr_clnup_blk__:{
		switch(self->__chx__vt_621827) {
			case 0:
			break;
			case 1:
			std_stdstringdelete(&self->InvalidFormat.msg);
			break;
			case 2:
			break;
			case 3:
			std_stdstringdelete(&self->DecompressionFailed.msg);
			break;
			case 4:
			break;
			case 5:
			std_stdstringdelete(&self->IoError.msg);
			break;
			case 6:
			break;
			case 7:
			std_stdstringdelete(&self->FileAlreadyExists.name);
			break;
		}
	}
}
void archive_archiveArchiveEntrydelete(struct archive_archiveArchiveEntry* self){
	__chx__dstctr_clnup_blk__:{
		std_stdstringdelete(&self->name);
	}
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/archive/src/endian.ch **/
static uint16_t archive_archiveread_u16_le(const uint8_t* data, size_t offset){
	return (((uint16_t) data[offset]) | (((uint16_t) data[(offset + 1)]) << 8));
}
static uint32_t archive_archiveread_u32_le(const uint8_t* data, size_t offset){
	return (((((uint32_t) data[offset]) | (((uint32_t) data[(offset + 1)]) << 8)) | (((uint32_t) data[(offset + 2)]) << 16)) | (((uint32_t) data[(offset + 3)]) << 24));
}
static uint64_t archive_archiveread_u64_le(const uint8_t* data, size_t offset){
	return (((((((((uint64_t) data[offset]) | (((uint64_t) data[(offset + 1)]) << 8)) | (((uint64_t) data[(offset + 2)]) << 16)) | (((uint64_t) data[(offset + 3)]) << 24)) | (((uint64_t) data[(offset + 4)]) << 32)) | (((uint64_t) data[(offset + 5)]) << 40)) | (((uint64_t) data[(offset + 6)]) << 48)) | (((uint64_t) data[(offset + 7)]) << 56));
}
static void archive_archivewrite_u16_le(uint8_t* out, size_t offset, uint16_t val){
	out[offset] = ((uint8_t) (val & 255));
	out[(offset + 1)] = ((uint8_t) ((val >> 8) & 255));
}
static void archive_archivewrite_u32_le(uint8_t* out, size_t offset, uint32_t val){
	out[offset] = ((uint8_t) (val & 255));
	out[(offset + 1)] = ((uint8_t) ((val >> 8) & 255));
	out[(offset + 2)] = ((uint8_t) ((val >> 16) & 255));
	out[(offset + 3)] = ((uint8_t) ((val >> 24) & 255));
}
static void archive_archivewrite_u64_le(uint8_t* out, size_t offset, uint64_t val){
	out[offset] = ((uint8_t) (val & 255));
	out[(offset + 1)] = ((uint8_t) ((val >> 8) & 255));
	out[(offset + 2)] = ((uint8_t) ((val >> 16) & 255));
	out[(offset + 3)] = ((uint8_t) ((val >> 24) & 255));
	out[(offset + 4)] = ((uint8_t) ((val >> 32) & 255));
	out[(offset + 5)] = ((uint8_t) ((val >> 40) & 255));
	out[(offset + 6)] = ((uint8_t) ((val >> 48) & 255));
	out[(offset + 7)] = ((uint8_t) ((val >> 56) & 255));
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/archive/src/tar_read.ch **/
void archive_archiveTarArchivedelete(struct archive_archiveTarArchive* self){
	__chx__dstctr_clnup_blk__:{
		std_stdvector__cgs__3delete(&self->data);
		std_stdvector__cgs__5delete(&self->entries);
	}
}
void archive_archiveopen_tar(struct std_stdResult__cgs__38* __chx_struct_ret_param_xx, const char* path, struct archive_archiveTarArchive* output){
	struct std_stdResult__cgs__36 read_result = (*({ struct std_stdResult__cgs__36 __chx__lv__50; fs_fsread_entire_file(&__chx__lv__50, path); &__chx__lv__50; }));
	_Bool __chx__lv__51 = true;
	if((read_result.__chx__vt_621827 == 1)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 1, 
			.Err.error = (struct archive_archiveArchiveError) { .__chx__vt_621827 = 0, 
			}
		};
		if(__chx__lv__51) {
			std_stdResult__cgs__36delete(&read_result);
		}
		return;
	}
	struct std_stdResult__cgs__36* __chx__lv__52 = &read_result;
	memcpy(&output->data, &__chx__lv__52->Ok.value, sizeof(struct std_stdvector__cgs__3));
	({ struct std_stdvector__cgs__3* __chx__lv__53 = &__chx__lv__52->Ok.value; *__chx__lv__53 = (*({ struct std_stdvector__cgs__3 __chx__lv__54; std_stdvector__cgs__3make(&__chx__lv__54); &__chx__lv__54; })); __chx__lv__53; });
	output->data_loaded = 1;
	struct std_stdResult__cgs__38 parse_result = (*({ struct std_stdResult__cgs__38 __chx__lv__55; archive_archiveparse_tar_entries(&__chx__lv__55, output); &__chx__lv__55; }));
	_Bool __chx__lv__56 = true;
	if((parse_result.__chx__vt_621827 == 1)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 1, 
			.Err.error = (struct archive_archiveArchiveError) { .__chx__vt_621827 = 1, 
				.InvalidFormat.msg = (*({ struct std_stdstring __chx__lv__57; std_stdstringconstructor2(&__chx__lv__57, "failed to parse TAR entries", 27, 0); &__chx__lv__57; }))
			}
		};
		if(__chx__lv__56) {
			std_stdResult__cgs__38delete(&parse_result);
		}
		if(__chx__lv__51) {
			std_stdResult__cgs__36delete(&read_result);
		}
		return;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 0, 
		.Ok.value = (struct std_stdUnit){ 
		}
	};
	if(__chx__lv__56) {
		std_stdResult__cgs__38delete(&parse_result);
	}
	if(__chx__lv__51) {
		std_stdResult__cgs__36delete(&read_result);
	}
	return;
}
static void archive_archiveparse_tar_entries(struct std_stdResult__cgs__38* __chx_struct_ret_param_xx, struct archive_archiveTarArchive* archive){
	size_t pos = 0;
	size_t data_size = std_stdvector__cgs__3size(&archive->data);
	while(((pos + 512) <= data_size)) {
		_Bool all_zero = 1;
		size_t i = 0;
		while((i < 512)) {
			if((std_stdvector__cgs__3data(&archive->data)[(pos + i)] != 0)){
				all_zero = 0;
				break;
			}
			i += 1;
		}
		if(all_zero){
			break;
		}
		struct std_stdstring name = (*({ struct std_stdstring __chx__lv__58; std_stdstringconstructor2(&__chx__lv__58, "", 0, 0); &__chx__lv__58; }));
		_Bool __chx__lv__59 = true;
		size_t j = 0;
		while(((j < 100) && (std_stdvector__cgs__3data(&archive->data)[(pos + j)] != 0))) {
			std_stdstringappend(&name, ((char) std_stdvector__cgs__3data(&archive->data)[(pos + j)]));
			j += 1;
		}
		struct std_stdstring size_str = (*({ struct std_stdstring __chx__lv__60; std_stdstringconstructor2(&__chx__lv__60, "", 0, 0); &__chx__lv__60; }));
		_Bool __chx__lv__61 = true;
		j = 124;
		while(((j < 136) && (std_stdvector__cgs__3data(&archive->data)[(pos + j)] != 0))) {
			std_stdstringappend(&size_str, ((char) std_stdvector__cgs__3data(&archive->data)[(pos + j)]));
			j += 1;
		}
		uint64_t size = archive_archivetar_parse_octal(&size_str);
		uint8_t typeflag = std_stdvector__cgs__3data(&archive->data)[(pos + 156)];
		struct std_stdstring prefix = (*({ struct std_stdstring __chx__lv__62; std_stdstringconstructor2(&__chx__lv__62, "", 0, 0); &__chx__lv__62; }));
		_Bool __chx__lv__63 = true;
		j = 345;
		while(((j < 500) && (std_stdvector__cgs__3data(&archive->data)[(pos + j)] != 0))) {
			std_stdstringappend(&prefix, ((char) std_stdvector__cgs__3data(&archive->data)[(pos + j)]));
			j += 1;
		}
		struct std_stdstring full_name = (*({ struct std_stdstring __chx__lv__64; std_stdstringconstructor2(&__chx__lv__64, "", 0, 0); &__chx__lv__64; }));
		_Bool __chx__lv__65 = true;
		if((std_stdstringsize(&prefix) > 0)){
			std_stdstringappend_string(&full_name, &prefix);
			std_stdstringappend(&full_name, '/');
		}
		std_stdstringappend_string(&full_name, &name);
		_Bool is_dir = ((typeflag == ((uint8_t) '5')) || (typeflag == ((uint8_t) '/')));
		if(((std_stdstringsize(&name) > 0) && (std_stdstringget(&name, (std_stdstringsize(&name) - 1)) == ((char) '/')))){
			is_dir = 1;
		}
		struct archive_archiveArchiveEntry entry;
		_Bool __chx__lv__66 = true;
		{
			struct std_stdstring __chx__lv__67 = ({ __chx__lv__65 = false; full_name; });
			std_stdstringdelete(&entry.name);
			entry.name = __chx__lv__67;
		};
		entry.size = size;
		entry.compressed_size = size;
		entry.compression_method = 0;
		entry.crc32 = 0;
		entry.is_directory = is_dir;
		entry.offset = ((uint64_t) (pos + 512));
		std_stdvector__cgs__5push(&archive->entries, ({ __chx__lv__66 = false; &entry; }));
		uint64_t data_blocks = ((size + 511) / 512);
		pos += (512 + (data_blocks * 512));
		if(__chx__lv__66) {
			archive_archiveArchiveEntrydelete(&entry);
		}
		if(__chx__lv__65) {
			std_stdstringdelete(&full_name);
		}
		if(__chx__lv__63) {
			std_stdstringdelete(&prefix);
		}
		if(__chx__lv__61) {
			std_stdstringdelete(&size_str);
		}
		if(__chx__lv__59) {
			std_stdstringdelete(&name);
		}
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 0, 
		.Ok.value = (struct std_stdUnit){ 
		}
	};
	return;
}
static uint64_t archive_archivetar_parse_octal(struct std_stdstring* str){
	uint64_t result = 0;
	size_t i = 0;
	while((i < std_stdstringsize(str))) {
		char c = std_stdstringget(str, i);
		if(((c >= ((char) '0')) && (c <= ((char) '7')))){
			result = ((result * 8) + (((uint64_t) c) - ((uint64_t) '0')));
		}
		i += 1;
	}
	return result;
}
struct std_stdvector__cgs__5* archive_archivetar_entries(struct archive_archiveTarArchive* archive){
	return &archive->entries;
}
size_t archive_archivetar_entry_count(struct archive_archiveTarArchive* archive){
	return std_stdvector__cgs__5size(&archive->entries);
}
void archive_archivetar_find_entry(struct std_stdResult__cgs__38* __chx_struct_ret_param_xx, struct archive_archiveTarArchive* archive, const char* name, struct archive_archiveArchiveEntry* output){
	size_t i = 0;
	while((i < std_stdvector__cgs__5size(&archive->entries))) {
		struct archive_archiveArchiveEntry* entry = std_stdvector__cgs__5get_ptr(&archive->entries, i);
		if(archive_archivestr_equals_cstr(&entry->name, name)){
			memcpy(&output, &entry, sizeof(struct archive_archiveArchiveEntry));
			({ struct archive_archiveArchiveEntry* __chx__lv__68 = entry; *__chx__lv__68 = (struct archive_archiveArchiveEntry){ 
				.name = (*({ struct std_stdstring __chx__lv__69; std_stdstringconstructor2(&__chx__lv__69, "", 0, 0); &__chx__lv__69; })), 
				.size = 0, 
				.compressed_size = 0, 
				.compression_method = 0, 
				.crc32 = 0, 
				.is_directory = 0, 
				.offset = 0
			}; __chx__lv__68; });
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 0, 
				.Ok.value = (struct std_stdUnit){ 
				}
			};
			return;
		}
		i += 1;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 1, 
		.Err.error = (struct archive_archiveArchiveError) { .__chx__vt_621827 = 0, 
		}
	};
	return;
}
void archive_archivetar_read_entry(struct std_stdResult__cgs__38* __chx_struct_ret_param_xx, struct archive_archiveTarArchive* archive, struct archive_archiveArchiveEntry* entry, struct std_stdvector__cgs__3* output){
	if(!archive->data_loaded){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 1, 
			.Err.error = (struct archive_archiveArchiveError) { .__chx__vt_621827 = 5, 
				.IoError.msg = (*({ struct std_stdstring __chx__lv__70; std_stdstringconstructor2(&__chx__lv__70, "archive data not loaded", 23, 0); &__chx__lv__70; }))
			}
		};
		return;
	}
	size_t offset = ((size_t) entry->offset);
	size_t size = ((size_t) entry->size);
	if(((offset + size) > std_stdvector__cgs__3size(&archive->data))){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 1, 
			.Err.error = (struct archive_archiveArchiveError) { .__chx__vt_621827 = 1, 
				.InvalidFormat.msg = (*({ struct std_stdstring __chx__lv__71; std_stdstringconstructor2(&__chx__lv__71, "entry data extends beyond file", 30, 0); &__chx__lv__71; }))
			}
		};
		return;
	}
	std_stdvector__cgs__3resize(output, size);
	uint8_t* ptr = ((uint8_t*) std_stdvector__cgs__3data(output));
	size_t i = 0;
	while((i < size)) {
		ptr[i] = std_stdvector__cgs__3data(&archive->data)[(offset + i)];
		i += 1;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 0, 
		.Ok.value = (struct std_stdUnit){ 
		}
	};
	return;
}
void archive_archivetar_read_file(struct std_stdResult__cgs__39* __chx_struct_ret_param_xx, struct archive_archiveTarArchive* archive, const char* name){
	struct archive_archiveArchiveEntry entry;
	_Bool __chx__lv__72 = true;
	struct std_stdResult__cgs__38 entry_result = (*({ struct std_stdResult__cgs__38 __chx__lv__73; archive_archivetar_find_entry(&__chx__lv__73, archive, name, &entry); &__chx__lv__73; }));
	_Bool __chx__lv__74 = true;
	if((entry_result.__chx__vt_621827 == 1)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__39) { .__chx__vt_621827 = 1, 
			.Err.error = (struct archive_archiveArchiveError) { .__chx__vt_621827 = 0, 
			}
		};
		if(__chx__lv__74) {
			std_stdResult__cgs__38delete(&entry_result);
		}
		if(__chx__lv__72) {
			archive_archiveArchiveEntrydelete(&entry);
		}
		return;
	}
	struct std_stdvector__cgs__3 content = (*({ struct std_stdvector__cgs__3 __chx__lv__75; std_stdvector__cgs__3make(&__chx__lv__75); &__chx__lv__75; }));
	_Bool __chx__lv__76 = true;
	struct std_stdResult__cgs__38 content_result = (*({ struct std_stdResult__cgs__38 __chx__lv__77; archive_archivetar_read_entry(&__chx__lv__77, archive, &entry, &content); &__chx__lv__77; }));
	_Bool __chx__lv__78 = true;
	if((content_result.__chx__vt_621827 == 1)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__39) { .__chx__vt_621827 = 1, 
			.Err.error = (struct archive_archiveArchiveError) { .__chx__vt_621827 = 5, 
				.IoError.msg = (*({ struct std_stdstring __chx__lv__79; std_stdstringconstructor2(&__chx__lv__79, "failed to read entry data", 25, 0); &__chx__lv__79; }))
			}
		};
		if(__chx__lv__78) {
			std_stdResult__cgs__38delete(&content_result);
		}
		if(__chx__lv__76) {
			std_stdvector__cgs__3delete(&content);
		}
		if(__chx__lv__74) {
			std_stdResult__cgs__38delete(&entry_result);
		}
		if(__chx__lv__72) {
			archive_archiveArchiveEntrydelete(&entry);
		}
		return;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__39) { .__chx__vt_621827 = 0, 
		.Ok.value = ({ __chx__lv__76 = false; content; })
	};
	if(__chx__lv__78) {
		std_stdResult__cgs__38delete(&content_result);
	}
	if(__chx__lv__76) {
		std_stdvector__cgs__3delete(&content);
	}
	if(__chx__lv__74) {
		std_stdResult__cgs__38delete(&entry_result);
	}
	if(__chx__lv__72) {
		archive_archiveArchiveEntrydelete(&entry);
	}
	return;
}
_Bool archive_archivetar_contains(struct archive_archiveTarArchive* archive, const char* name){
	struct archive_archiveArchiveEntry entry;
	_Bool __chx__lv__80 = true;
	struct std_stdResult__cgs__38 r = (*({ struct std_stdResult__cgs__38 __chx__lv__81; archive_archivetar_find_entry(&__chx__lv__81, archive, name, &entry); &__chx__lv__81; }));
	_Bool __chx__lv__82 = true;
	const _Bool __chx__lv__83 = (r.__chx__vt_621827 == 0);
	if(__chx__lv__82) {
		std_stdResult__cgs__38delete(&r);
	}
	if(__chx__lv__80) {
		archive_archiveArchiveEntrydelete(&entry);
	}
	return __chx__lv__83;
}
void archive_archivetar_extract_entry(struct std_stdResult__cgs__38* __chx_struct_ret_param_xx, struct archive_archiveTarArchive* archive, struct archive_archiveArchiveEntry* entry, const char* dest_dir){
	struct std_stdstring full_path = (*({ struct std_stdstring __chx__lv__84; std_stdstringmake_no_len(&__chx__lv__84, dest_dir); &__chx__lv__84; }));
	_Bool __chx__lv__85 = true;
	if(((std_stdstringsize(&full_path) > 0) && (std_stdstringget(&full_path, (std_stdstringsize(&full_path) - 1)) != ((char) '/')))){
		std_stdstringappend(&full_path, '/');
	}
	std_stdstringappend_string(&full_path, &entry->name);
	if(entry->is_directory){
		(*({ struct std_stdResult__cgs__34 __chx__lv__86; fs_fscreate_dir_all(&__chx__lv__86, std_stdstringc_str(&full_path)); &__chx__lv__86; }));
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 0, 
			.Ok.value = (struct std_stdUnit){ 
			}
		};
		if(__chx__lv__85) {
			std_stdstringdelete(&full_path);
		}
		return;
	}
	struct std_stdvector__cgs__3 content = (*({ struct std_stdvector__cgs__3 __chx__lv__87; std_stdvector__cgs__3make(&__chx__lv__87); &__chx__lv__87; }));
	_Bool __chx__lv__88 = true;
	struct std_stdResult__cgs__38 content_result = (*({ struct std_stdResult__cgs__38 __chx__lv__89; archive_archivetar_read_entry(&__chx__lv__89, archive, entry, &content); &__chx__lv__89; }));
	_Bool __chx__lv__90 = true;
	if((content_result.__chx__vt_621827 == 1)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 1, 
			.Err.error = (struct archive_archiveArchiveError) { .__chx__vt_621827 = 5, 
				.IoError.msg = (*({ struct std_stdstring __chx__lv__91; std_stdstringconstructor2(&__chx__lv__91, "failed to read entry for extraction", 35, 0); &__chx__lv__91; }))
			}
		};
		if(__chx__lv__90) {
			std_stdResult__cgs__38delete(&content_result);
		}
		if(__chx__lv__88) {
			std_stdvector__cgs__3delete(&content);
		}
		if(__chx__lv__85) {
			std_stdstringdelete(&full_path);
		}
		return;
	}
	(*({ struct std_stdResult__cgs__34 __chx__lv__92; fs_fscreate_dir_all(&__chx__lv__92, std_stdstringc_str(&full_path)); &__chx__lv__92; }));
	struct std_stdResult__cgs__34 write_result = (*({ struct std_stdResult__cgs__34 __chx__lv__93; fs_fswrite_text_file(&__chx__lv__93, std_stdstringc_str(&full_path), std_stdvector__cgs__3data(&content), std_stdvector__cgs__3size(&content)); &__chx__lv__93; }));
	if((write_result.__chx__vt_621827 == 1)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 1, 
			.Err.error = (struct archive_archiveArchiveError) { .__chx__vt_621827 = 5, 
				.IoError.msg = (*({ struct std_stdstring __chx__lv__94; std_stdstringconstructor2(&__chx__lv__94, "failed to write file", 20, 0); &__chx__lv__94; }))
			}
		};
		if(__chx__lv__90) {
			std_stdResult__cgs__38delete(&content_result);
		}
		if(__chx__lv__88) {
			std_stdvector__cgs__3delete(&content);
		}
		if(__chx__lv__85) {
			std_stdstringdelete(&full_path);
		}
		return;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 0, 
		.Ok.value = (struct std_stdUnit){ 
		}
	};
	if(__chx__lv__90) {
		std_stdResult__cgs__38delete(&content_result);
	}
	if(__chx__lv__88) {
		std_stdvector__cgs__3delete(&content);
	}
	if(__chx__lv__85) {
		std_stdstringdelete(&full_path);
	}
	return;
}
void archive_archivetar_extract_all(struct std_stdResult__cgs__38* __chx_struct_ret_param_xx, struct archive_archiveTarArchive* archive, const char* dest_dir){
	(*({ struct std_stdResult__cgs__34 __chx__lv__95; fs_fscreate_dir_all(&__chx__lv__95, dest_dir); &__chx__lv__95; }));
	size_t i = 0;
	while((i < std_stdvector__cgs__5size(&archive->entries))) {
		struct archive_archiveArchiveEntry* entry = std_stdvector__cgs__5get_ptr(&archive->entries, i);
		struct std_stdResult__cgs__38 result = (*({ struct std_stdResult__cgs__38 __chx__lv__96; archive_archivetar_extract_entry(&__chx__lv__96, archive, entry, dest_dir); &__chx__lv__96; }));
		_Bool __chx__lv__97 = true;
		if((result.__chx__vt_621827 == 1)){
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 1, 
				.Err.error = (struct archive_archiveArchiveError) { .__chx__vt_621827 = 5, 
					.IoError.msg = (*({ struct std_stdstring __chx__lv__98; std_stdstringconstructor2(&__chx__lv__98, "failed to extract entry", 23, 0); &__chx__lv__98; }))
				}
			};
			if(__chx__lv__97) {
				std_stdResult__cgs__38delete(&result);
			}
			return;
		}
		i += 1;
		if(__chx__lv__97) {
			std_stdResult__cgs__38delete(&result);
		}
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 0, 
		.Ok.value = (struct std_stdUnit){ 
		}
	};
	return;
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/archive/src/zip_read.ch **/
static uint32_t archive_archiveZIP_LOCAL_FILE_HEADER = 67324752;
static uint32_t archive_archiveZIP_CENTRAL_DIR_HEADER = 33639248;
static uint32_t archive_archiveZIP_END_OF_CENTRAL_DIR = 101010256;
void archive_archiveZipArchivedelete(struct archive_archiveZipArchive* self){
	__chx__dstctr_clnup_blk__:{
		std_stdvector__cgs__3delete(&self->data);
		std_stdvector__cgs__5delete(&self->entries);
	}
}
void archive_archiveopen_zip(struct std_stdResult__cgs__38* __chx_struct_ret_param_xx, const char* path, struct archive_archiveZipArchive* output){
	struct std_stdResult__cgs__36 read_result = (*({ struct std_stdResult__cgs__36 __chx__lv__99; fs_fsread_entire_file(&__chx__lv__99, path); &__chx__lv__99; }));
	_Bool __chx__lv__100 = true;
	if((read_result.__chx__vt_621827 == 1)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 1, 
			.Err.error = (struct archive_archiveArchiveError) { .__chx__vt_621827 = 0, 
			}
		};
		if(__chx__lv__100) {
			std_stdResult__cgs__36delete(&read_result);
		}
		return;
	}
	struct std_stdResult__cgs__36* __chx__lv__101 = &read_result;
	memcpy(&output->data, &__chx__lv__101->Ok.value, sizeof(struct std_stdvector__cgs__3));
	({ struct std_stdvector__cgs__3* __chx__lv__102 = &__chx__lv__101->Ok.value; *__chx__lv__102 = (*({ struct std_stdvector__cgs__3 __chx__lv__103; std_stdvector__cgs__3make(&__chx__lv__103); &__chx__lv__103; })); __chx__lv__102; });
	output->data_loaded = 1;
	struct std_stdResult__cgs__38 parse_result = (*({ struct std_stdResult__cgs__38 __chx__lv__104; archive_archiveparse_zip_central_dir(&__chx__lv__104, output); &__chx__lv__104; }));
	_Bool __chx__lv__105 = true;
	if((parse_result.__chx__vt_621827 == 1)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 1, 
			.Err.error = (struct archive_archiveArchiveError) { .__chx__vt_621827 = 1, 
				.InvalidFormat.msg = (*({ struct std_stdstring __chx__lv__106; std_stdstringconstructor2(&__chx__lv__106, "failed to parse ZIP central directory", 37, 0); &__chx__lv__106; }))
			}
		};
		if(__chx__lv__105) {
			std_stdResult__cgs__38delete(&parse_result);
		}
		if(__chx__lv__100) {
			std_stdResult__cgs__36delete(&read_result);
		}
		return;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 0, 
		.Ok.value = (struct std_stdUnit){ 
		}
	};
	if(__chx__lv__105) {
		std_stdResult__cgs__38delete(&parse_result);
	}
	if(__chx__lv__100) {
		std_stdResult__cgs__36delete(&read_result);
	}
	return;
}
void archive_archiveopen_zip_bytes(struct std_stdResult__cgs__38* __chx_struct_ret_param_xx, const uint8_t* data, size_t data_len, struct archive_archiveZipArchive* output){
	struct std_stdvector__cgs__3 vec = (*({ struct std_stdvector__cgs__3 __chx__lv__107; std_stdvector__cgs__3make(&__chx__lv__107); &__chx__lv__107; }));
	_Bool __chx__lv__108 = true;
	std_stdvector__cgs__3resize(&vec, data_len);
	size_t i = 0;
	uint8_t* vptr = ((uint8_t*) std_stdvector__cgs__3data(&vec));
	while((i < data_len)) {
		vptr[i] = data[i];
		i += 1;
	}
	{
		struct std_stdvector__cgs__3 __chx__lv__109 = ({ __chx__lv__108 = false; vec; });
		std_stdvector__cgs__3delete(&output->data);
		output->data = __chx__lv__109;
	};
	output->data_loaded = 1;
	struct std_stdResult__cgs__38 parse_result = (*({ struct std_stdResult__cgs__38 __chx__lv__110; archive_archiveparse_zip_central_dir(&__chx__lv__110, output); &__chx__lv__110; }));
	_Bool __chx__lv__111 = true;
	if((parse_result.__chx__vt_621827 == 1)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 1, 
			.Err.error = (struct archive_archiveArchiveError) { .__chx__vt_621827 = 1, 
				.InvalidFormat.msg = (*({ struct std_stdstring __chx__lv__112; std_stdstringconstructor2(&__chx__lv__112, "failed to parse ZIP central directory", 37, 0); &__chx__lv__112; }))
			}
		};
		if(__chx__lv__111) {
			std_stdResult__cgs__38delete(&parse_result);
		}
		if(__chx__lv__108) {
			std_stdvector__cgs__3delete(&vec);
		}
		return;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 0, 
		.Ok.value = (struct std_stdUnit){ 
		}
	};
	if(__chx__lv__111) {
		std_stdResult__cgs__38delete(&parse_result);
	}
	if(__chx__lv__108) {
		std_stdvector__cgs__3delete(&vec);
	}
	return;
}
static void archive_archiveparse_zip_central_dir(struct std_stdResult__cgs__38* __chx_struct_ret_param_xx, struct archive_archiveZipArchive* archive){
	if((std_stdvector__cgs__3size(&archive->data) < 22)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 1, 
			.Err.error = (struct archive_archiveArchiveError) { .__chx__vt_621827 = 1, 
				.InvalidFormat.msg = (*({ struct std_stdstring __chx__lv__113; std_stdstringconstructor2(&__chx__lv__113, "file too small for ZIP", 22, 0); &__chx__lv__113; }))
			}
		};
		return;
	}
	size_t eocd_pos = 0;
	_Bool found = 0;
	unsigned long search_pos = (std_stdvector__cgs__3size(&archive->data) - 22);
	while((search_pos > 0)) {
		uint32_t sig = archive_archiveread_u32_le(std_stdvector__cgs__3data(&archive->data), search_pos);
		if((sig == archive_archiveZIP_END_OF_CENTRAL_DIR)){
			eocd_pos = search_pos;
			found = 1;
			break;
		}
		search_pos -= 1;
	}
	if(!found){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 1, 
			.Err.error = (struct archive_archiveArchiveError) { .__chx__vt_621827 = 1, 
				.InvalidFormat.msg = (*({ struct std_stdstring __chx__lv__114; std_stdstringconstructor2(&__chx__lv__114, "no End of Central Directory found", 33, 0); &__chx__lv__114; }))
			}
		};
		return;
	}
	size_t num_entries = ((size_t) archive_archiveread_u16_le(std_stdvector__cgs__3data(&archive->data), (eocd_pos + 10)));
	size_t cd_offset = ((size_t) archive_archiveread_u32_le(std_stdvector__cgs__3data(&archive->data), (eocd_pos + 16)));
	size_t pos = cd_offset;
	size_t i = 0;
	while((i < num_entries)) {
		if(((pos + 46) > std_stdvector__cgs__3size(&archive->data))){
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 1, 
				.Err.error = (struct archive_archiveArchiveError) { .__chx__vt_621827 = 1, 
					.InvalidFormat.msg = (*({ struct std_stdstring __chx__lv__115; std_stdstringconstructor2(&__chx__lv__115, "truncated central directory entry", 33, 0); &__chx__lv__115; }))
				}
			};
			return;
		}
		uint32_t sig = archive_archiveread_u32_le(std_stdvector__cgs__3data(&archive->data), pos);
		if((sig != archive_archiveZIP_CENTRAL_DIR_HEADER)){
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 1, 
				.Err.error = (struct archive_archiveArchiveError) { .__chx__vt_621827 = 1, 
					.InvalidFormat.msg = (*({ struct std_stdstring __chx__lv__116; std_stdstringconstructor2(&__chx__lv__116, "invalid central directory entry signature", 41, 0); &__chx__lv__116; }))
				}
			};
			return;
		}
		uint16_t method = archive_archiveread_u16_le(std_stdvector__cgs__3data(&archive->data), (pos + 10));
		uint32_t crc = archive_archiveread_u32_le(std_stdvector__cgs__3data(&archive->data), (pos + 16));
		uint64_t comp_size = ((uint64_t) archive_archiveread_u32_le(std_stdvector__cgs__3data(&archive->data), (pos + 20)));
		uint64_t uncomp_size = ((uint64_t) archive_archiveread_u32_le(std_stdvector__cgs__3data(&archive->data), (pos + 24)));
		size_t name_len = ((size_t) archive_archiveread_u16_le(std_stdvector__cgs__3data(&archive->data), (pos + 28)));
		size_t extra_len = ((size_t) archive_archiveread_u16_le(std_stdvector__cgs__3data(&archive->data), (pos + 30)));
		size_t comment_len = ((size_t) archive_archiveread_u16_le(std_stdvector__cgs__3data(&archive->data), (pos + 32)));
		uint64_t local_offset = ((uint64_t) archive_archiveread_u32_le(std_stdvector__cgs__3data(&archive->data), (pos + 42)));
		unsigned long name_start = (pos + 46);
		struct std_stdstring name = (*({ struct std_stdstring __chx__lv__117; std_stdstringconstructor2(&__chx__lv__117, "", 0, 0); &__chx__lv__117; }));
		_Bool __chx__lv__118 = true;
		size_t j = 0;
		while(((j < name_len) && ((name_start + j) < std_stdvector__cgs__3size(&archive->data)))) {
			std_stdstringappend(&name, ((char) std_stdvector__cgs__3data(&archive->data)[(name_start + j)]));
			j += 1;
		}
		_Bool is_dir = 0;
		if((name_len > 0)){
			uint8_t last_char = std_stdvector__cgs__3data(&archive->data)[((name_start + name_len) - 1)];
			if(((last_char == ((uint8_t) '/')) || (last_char == ((uint8_t) '\\')))){
				is_dir = 1;
			}
		}
		struct archive_archiveArchiveEntry entry;
		_Bool __chx__lv__119 = true;
		{
			struct std_stdstring __chx__lv__120 = ({ __chx__lv__118 = false; name; });
			std_stdstringdelete(&entry.name);
			entry.name = __chx__lv__120;
		};
		entry.size = uncomp_size;
		entry.compressed_size = comp_size;
		entry.compression_method = method;
		entry.crc32 = crc;
		entry.is_directory = is_dir;
		entry.offset = local_offset;
		std_stdvector__cgs__5push(&archive->entries, ({ __chx__lv__119 = false; &entry; }));
		pos += (((46 + name_len) + extra_len) + comment_len);
		i += 1;
		if(__chx__lv__119) {
			archive_archiveArchiveEntrydelete(&entry);
		}
		if(__chx__lv__118) {
			std_stdstringdelete(&name);
		}
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 0, 
		.Ok.value = (struct std_stdUnit){ 
		}
	};
	return;
}
struct std_stdvector__cgs__5* archive_archivezip_entries(struct archive_archiveZipArchive* archive){
	return &archive->entries;
}
size_t archive_archivezip_entry_count(struct archive_archiveZipArchive* archive){
	return std_stdvector__cgs__5size(&archive->entries);
}
static _Bool archive_archivestr_equals_cstr(struct std_stdstring* s, const char* cstr){
	size_t i = 0;
	size_t s_len = std_stdstringsize(s);
	while((i < s_len)) {
		if((cstr[i] == ((char) '\0'))){
			return 0;
		}
		if((std_stdstringget(s, i) != cstr[i])){
			return 0;
		}
		i += 1;
	}
	return (cstr[i] == ((char) '\0'));
}
void archive_archivezip_find_entry(struct std_stdResult__cgs__38* __chx_struct_ret_param_xx, struct archive_archiveZipArchive* archive, const char* name, struct archive_archiveArchiveEntry* output){
	size_t i = 0;
	while((i < std_stdvector__cgs__5size(&archive->entries))) {
		struct archive_archiveArchiveEntry* entry = std_stdvector__cgs__5get_ptr(&archive->entries, i);
		if(archive_archivestr_equals_cstr(&entry->name, name)){
			memcpy(&output, &entry, sizeof(struct archive_archiveArchiveEntry));
			({ struct archive_archiveArchiveEntry* __chx__lv__121 = entry; *__chx__lv__121 = (struct archive_archiveArchiveEntry){ 
				.name = (*({ struct std_stdstring __chx__lv__122; std_stdstringconstructor2(&__chx__lv__122, "", 0, 0); &__chx__lv__122; })), 
				.size = 0, 
				.compressed_size = 0, 
				.compression_method = 0, 
				.crc32 = 0, 
				.is_directory = 0, 
				.offset = 0
			}; __chx__lv__121; });
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 0, 
				.Ok.value = (struct std_stdUnit){ 
				}
			};
			return;
		}
		i += 1;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 1, 
		.Err.error = (struct archive_archiveArchiveError) { .__chx__vt_621827 = 0, 
		}
	};
	return;
}
void archive_archivezip_read_entry(struct std_stdResult__cgs__38* __chx_struct_ret_param_xx, struct archive_archiveZipArchive* archive, struct archive_archiveArchiveEntry* entry, struct std_stdvector__cgs__3* output){
	if(!archive->data_loaded){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 1, 
			.Err.error = (struct archive_archiveArchiveError) { .__chx__vt_621827 = 5, 
				.IoError.msg = (*({ struct std_stdstring __chx__lv__123; std_stdstringconstructor2(&__chx__lv__123, "archive data not loaded", 23, 0); &__chx__lv__123; }))
			}
		};
		return;
	}
	size_t pos = ((size_t) entry->offset);
	if(((pos + 30) > std_stdvector__cgs__3size(&archive->data))){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 1, 
			.Err.error = (struct archive_archiveArchiveError) { .__chx__vt_621827 = 1, 
				.InvalidFormat.msg = (*({ struct std_stdstring __chx__lv__124; std_stdstringconstructor2(&__chx__lv__124, "truncated local file header", 27, 0); &__chx__lv__124; }))
			}
		};
		return;
	}
	uint32_t sig = archive_archiveread_u32_le(std_stdvector__cgs__3data(&archive->data), pos);
	if((sig != archive_archiveZIP_LOCAL_FILE_HEADER)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 1, 
			.Err.error = (struct archive_archiveArchiveError) { .__chx__vt_621827 = 1, 
				.InvalidFormat.msg = (*({ struct std_stdstring __chx__lv__125; std_stdstringconstructor2(&__chx__lv__125, "invalid local file header signature", 35, 0); &__chx__lv__125; }))
			}
		};
		return;
	}
	size_t local_name_len = ((size_t) archive_archiveread_u16_le(std_stdvector__cgs__3data(&archive->data), (pos + 26)));
	size_t local_extra_len = ((size_t) archive_archiveread_u16_le(std_stdvector__cgs__3data(&archive->data), (pos + 28)));
	unsigned long data_offset = (((pos + 30) + local_name_len) + local_extra_len);
	if((entry->compression_method == 0)){
		size_t size = ((size_t) entry->compressed_size);
		std_stdvector__cgs__3resize(output, size);
		uint8_t* ptr = ((uint8_t*) std_stdvector__cgs__3data(output));
		size_t i = 0;
		while(((i < size) && ((data_offset + i) < std_stdvector__cgs__3size(&archive->data)))) {
			ptr[i] = std_stdvector__cgs__3data(&archive->data)[(data_offset + i)];
			i += 1;
		}
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 0, 
			.Ok.value = (struct std_stdUnit){ 
			}
		};
		return;
	}else if((entry->compression_method == 8)){
		if(((data_offset + ((size_t) entry->compressed_size)) > std_stdvector__cgs__3size(&archive->data))){
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 1, 
				.Err.error = (struct archive_archiveArchiveError) { .__chx__vt_621827 = 1, 
					.InvalidFormat.msg = (*({ struct std_stdstring __chx__lv__126; std_stdstringconstructor2(&__chx__lv__126, "compressed data extends beyond file", 35, 0); &__chx__lv__126; }))
				}
			};
			return;
		}
		const uint8_t* comp_data = (std_stdvector__cgs__3data(&archive->data) + data_offset);
		size_t comp_len = ((size_t) entry->compressed_size);
		size_t uncomp_len = ((size_t) entry->size);
		std_stdvector__cgs__3resize(output, uncomp_len);
		struct std_stdResult__cgs__40 def_result = (*({ struct std_stdResult__cgs__40 __chx__lv__127; archive_archivedeflate_decompress(&__chx__lv__127, comp_data, comp_len, ((uint8_t*) std_stdvector__cgs__3data(output)), uncomp_len); &__chx__lv__127; }));
		_Bool __chx__lv__128 = true;
		if((def_result.__chx__vt_621827 == 1)){
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 1, 
				.Err.error = (struct archive_archiveArchiveError) { .__chx__vt_621827 = 3, 
					.DecompressionFailed.msg = (*({ struct std_stdstring __chx__lv__129; std_stdstringconstructor2(&__chx__lv__129, "deflate decompression failed", 28, 0); &__chx__lv__129; }))
				}
			};
			if(__chx__lv__128) {
				std_stdResult__cgs__40delete(&def_result);
			}
			return;
		}
		struct std_stdResult__cgs__40* __chx__lv__130 = &def_result;
		uint32_t computed_crc = archive_archivecrc32_compute(std_stdvector__cgs__3data(output), __chx__lv__130->Ok.value);
		if((computed_crc != entry->crc32)){
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 1, 
				.Err.error = (struct archive_archiveArchiveError) { .__chx__vt_621827 = 4, 
				}
			};
			if(__chx__lv__128) {
				std_stdResult__cgs__40delete(&def_result);
			}
			return;
		}
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 0, 
			.Ok.value = (struct std_stdUnit){ 
			}
		};
		if(__chx__lv__128) {
			std_stdResult__cgs__40delete(&def_result);
		}
		return;
	} else {
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 1, 
			.Err.error = (struct archive_archiveArchiveError) { .__chx__vt_621827 = 2, 
				.UnsupportedCompression.method = ((int) entry->compression_method)
			}
		};
		return;
	}
}
void archive_archivezip_read_file(struct std_stdResult__cgs__39* __chx_struct_ret_param_xx, struct archive_archiveZipArchive* archive, const char* name){
	struct archive_archiveArchiveEntry entry;
	_Bool __chx__lv__131 = true;
	struct std_stdResult__cgs__38 entry_result = (*({ struct std_stdResult__cgs__38 __chx__lv__132; archive_archivezip_find_entry(&__chx__lv__132, archive, name, &entry); &__chx__lv__132; }));
	_Bool __chx__lv__133 = true;
	if((entry_result.__chx__vt_621827 == 1)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__39) { .__chx__vt_621827 = 1, 
			.Err.error = (struct archive_archiveArchiveError) { .__chx__vt_621827 = 0, 
			}
		};
		if(__chx__lv__133) {
			std_stdResult__cgs__38delete(&entry_result);
		}
		if(__chx__lv__131) {
			archive_archiveArchiveEntrydelete(&entry);
		}
		return;
	}
	struct std_stdvector__cgs__3 content = (*({ struct std_stdvector__cgs__3 __chx__lv__134; std_stdvector__cgs__3make(&__chx__lv__134); &__chx__lv__134; }));
	_Bool __chx__lv__135 = true;
	struct std_stdResult__cgs__38 content_result = (*({ struct std_stdResult__cgs__38 __chx__lv__136; archive_archivezip_read_entry(&__chx__lv__136, archive, &entry, &content); &__chx__lv__136; }));
	_Bool __chx__lv__137 = true;
	if((content_result.__chx__vt_621827 == 1)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__39) { .__chx__vt_621827 = 1, 
			.Err.error = (struct archive_archiveArchiveError) { .__chx__vt_621827 = 3, 
				.DecompressionFailed.msg = (*({ struct std_stdstring __chx__lv__138; std_stdstringconstructor2(&__chx__lv__138, "failed to read entry", 20, 0); &__chx__lv__138; }))
			}
		};
		if(__chx__lv__137) {
			std_stdResult__cgs__38delete(&content_result);
		}
		if(__chx__lv__135) {
			std_stdvector__cgs__3delete(&content);
		}
		if(__chx__lv__133) {
			std_stdResult__cgs__38delete(&entry_result);
		}
		if(__chx__lv__131) {
			archive_archiveArchiveEntrydelete(&entry);
		}
		return;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__39) { .__chx__vt_621827 = 0, 
		.Ok.value = ({ __chx__lv__135 = false; content; })
	};
	if(__chx__lv__137) {
		std_stdResult__cgs__38delete(&content_result);
	}
	if(__chx__lv__135) {
		std_stdvector__cgs__3delete(&content);
	}
	if(__chx__lv__133) {
		std_stdResult__cgs__38delete(&entry_result);
	}
	if(__chx__lv__131) {
		archive_archiveArchiveEntrydelete(&entry);
	}
	return;
}
_Bool archive_archivezip_contains(struct archive_archiveZipArchive* archive, const char* name){
	struct archive_archiveArchiveEntry entry;
	_Bool __chx__lv__139 = true;
	struct std_stdResult__cgs__38 r = (*({ struct std_stdResult__cgs__38 __chx__lv__140; archive_archivezip_find_entry(&__chx__lv__140, archive, name, &entry); &__chx__lv__140; }));
	_Bool __chx__lv__141 = true;
	const _Bool __chx__lv__142 = (r.__chx__vt_621827 == 0);
	if(__chx__lv__141) {
		std_stdResult__cgs__38delete(&r);
	}
	if(__chx__lv__139) {
		archive_archiveArchiveEntrydelete(&entry);
	}
	return __chx__lv__142;
}
void archive_archivezip_extract_entry(struct std_stdResult__cgs__38* __chx_struct_ret_param_xx, struct archive_archiveZipArchive* archive, struct archive_archiveArchiveEntry* entry, const char* dest_dir){
	struct std_stdvector__cgs__3 content = (*({ struct std_stdvector__cgs__3 __chx__lv__143; std_stdvector__cgs__3make(&__chx__lv__143); &__chx__lv__143; }));
	_Bool __chx__lv__144 = true;
	struct std_stdResult__cgs__38 content_result = (*({ struct std_stdResult__cgs__38 __chx__lv__145; archive_archivezip_read_entry(&__chx__lv__145, archive, entry, &content); &__chx__lv__145; }));
	_Bool __chx__lv__146 = true;
	if((content_result.__chx__vt_621827 == 1)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 1, 
			.Err.error = (struct archive_archiveArchiveError) { .__chx__vt_621827 = 3, 
				.DecompressionFailed.msg = (*({ struct std_stdstring __chx__lv__147; std_stdstringconstructor2(&__chx__lv__147, "failed to read entry for extraction", 35, 0); &__chx__lv__147; }))
			}
		};
		if(__chx__lv__146) {
			std_stdResult__cgs__38delete(&content_result);
		}
		if(__chx__lv__144) {
			std_stdvector__cgs__3delete(&content);
		}
		return;
	}
	struct std_stdstring full_path = (*({ struct std_stdstring __chx__lv__148; std_stdstringmake_no_len(&__chx__lv__148, dest_dir); &__chx__lv__148; }));
	_Bool __chx__lv__149 = true;
	if((std_stdstringsize(&full_path) > 0)){
		char last = std_stdstringget(&full_path, (std_stdstringsize(&full_path) - 1));
		if((last != ((char) '/'))){
			std_stdstringappend(&full_path, '/');
		}
	}
	std_stdstringappend_string(&full_path, &entry->name);
	if(!entry->is_directory){
		struct std_stdstring parent = (*({ struct std_stdstring __chx__lv__150; fs_fsparent_path_view(&__chx__lv__150, &(*({ struct std_stdstring_view __chx__lv__151; std_stdstring_viewconstructor(&__chx__lv__151, std_stdstringdata(&full_path), std_stdstringsize(&full_path)); &__chx__lv__151; }))); &__chx__lv__150; }));
		_Bool __chx__lv__152 = true;
		(*({ struct std_stdResult__cgs__34 __chx__lv__153; fs_fscreate_dir_all(&__chx__lv__153, std_stdstringc_str(&parent)); &__chx__lv__153; }));
		if(__chx__lv__152) {
			std_stdstringdelete(&parent);
		}
	}
	if(entry->is_directory){
		(*({ struct std_stdResult__cgs__34 __chx__lv__154; fs_fscreate_dir_all(&__chx__lv__154, std_stdstringc_str(&full_path)); &__chx__lv__154; }));
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 0, 
			.Ok.value = (struct std_stdUnit){ 
			}
		};
		if(__chx__lv__149) {
			std_stdstringdelete(&full_path);
		}
		if(__chx__lv__146) {
			std_stdResult__cgs__38delete(&content_result);
		}
		if(__chx__lv__144) {
			std_stdvector__cgs__3delete(&content);
		}
		return;
	}
	struct std_stdResult__cgs__34 write_result = (*({ struct std_stdResult__cgs__34 __chx__lv__155; fs_fswrite_text_file(&__chx__lv__155, std_stdstringc_str(&full_path), std_stdvector__cgs__3data(&content), std_stdvector__cgs__3size(&content)); &__chx__lv__155; }));
	if((write_result.__chx__vt_621827 == 1)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 1, 
			.Err.error = (struct archive_archiveArchiveError) { .__chx__vt_621827 = 5, 
				.IoError.msg = (*({ struct std_stdstring __chx__lv__156; std_stdstringconstructor2(&__chx__lv__156, "failed to write file", 20, 0); &__chx__lv__156; }))
			}
		};
		if(__chx__lv__149) {
			std_stdstringdelete(&full_path);
		}
		if(__chx__lv__146) {
			std_stdResult__cgs__38delete(&content_result);
		}
		if(__chx__lv__144) {
			std_stdvector__cgs__3delete(&content);
		}
		return;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 0, 
		.Ok.value = (struct std_stdUnit){ 
		}
	};
	if(__chx__lv__149) {
		std_stdstringdelete(&full_path);
	}
	if(__chx__lv__146) {
		std_stdResult__cgs__38delete(&content_result);
	}
	if(__chx__lv__144) {
		std_stdvector__cgs__3delete(&content);
	}
	return;
}
void archive_archivezip_extract_all(struct std_stdResult__cgs__38* __chx_struct_ret_param_xx, struct archive_archiveZipArchive* archive, const char* dest_dir){
	(*({ struct std_stdResult__cgs__34 __chx__lv__157; fs_fscreate_dir_all(&__chx__lv__157, dest_dir); &__chx__lv__157; }));
	size_t i = 0;
	while((i < std_stdvector__cgs__5size(&archive->entries))) {
		struct archive_archiveArchiveEntry* entry = std_stdvector__cgs__5get_ptr(&archive->entries, i);
		struct std_stdResult__cgs__38 result = (*({ struct std_stdResult__cgs__38 __chx__lv__158; archive_archivezip_extract_entry(&__chx__lv__158, archive, entry, dest_dir); &__chx__lv__158; }));
		_Bool __chx__lv__159 = true;
		if((result.__chx__vt_621827 == 1)){
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 1, 
				.Err.error = (struct archive_archiveArchiveError) { .__chx__vt_621827 = 5, 
					.IoError.msg = (*({ struct std_stdstring __chx__lv__160; std_stdstringconstructor2(&__chx__lv__160, "failed to extract entry", 23, 0); &__chx__lv__160; }))
				}
			};
			if(__chx__lv__159) {
				std_stdResult__cgs__38delete(&result);
			}
			return;
		}
		i += 1;
		if(__chx__lv__159) {
			std_stdResult__cgs__38delete(&result);
		}
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 0, 
		.Ok.value = (struct std_stdUnit){ 
		}
	};
	return;
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/archive/src/main.ch **/
void archive_archiveopen(struct std_stdResult__cgs__37* __chx_struct_ret_param_xx, const char* path){
	struct archive_archiveArchive archive;
	struct std_stdResult__cgs__38 zip_result = (*({ struct std_stdResult__cgs__38 __chx__lv__161; archive_archiveopen_zip(&__chx__lv__161, path, &archive.zip_data); &__chx__lv__161; }));
	_Bool __chx__lv__162 = true;
	if((zip_result.__chx__vt_621827 == 0)){
		archive.archive_type = 1;
		archive.loaded = 1;
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__37) { .__chx__vt_621827 = 0, 
			.Ok.value = archive
		};
		if(__chx__lv__162) {
			std_stdResult__cgs__38delete(&zip_result);
		}
		return;
	}
	struct std_stdResult__cgs__38 tar_result = (*({ struct std_stdResult__cgs__38 __chx__lv__163; archive_archiveopen_tar(&__chx__lv__163, path, &archive.tar_data); &__chx__lv__163; }));
	_Bool __chx__lv__164 = true;
	if((tar_result.__chx__vt_621827 == 0)){
		archive.archive_type = 2;
		archive.loaded = 1;
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__37) { .__chx__vt_621827 = 0, 
			.Ok.value = archive
		};
		if(__chx__lv__164) {
			std_stdResult__cgs__38delete(&tar_result);
		}
		if(__chx__lv__162) {
			std_stdResult__cgs__38delete(&zip_result);
		}
		return;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__37) { .__chx__vt_621827 = 1, 
		.Err.error = (struct archive_archiveArchiveError) { .__chx__vt_621827 = 1, 
			.InvalidFormat.msg = (*({ struct std_stdstring __chx__lv__165; std_stdstringconstructor2(&__chx__lv__165, "not a recognized archive format", 31, 0); &__chx__lv__165; }))
		}
	};
	if(__chx__lv__164) {
		std_stdResult__cgs__38delete(&tar_result);
	}
	if(__chx__lv__162) {
		std_stdResult__cgs__38delete(&zip_result);
	}
	return;
}
struct std_stdvector__cgs__5* archive_archiveentries(struct archive_archiveArchive* archive){
	if((archive->archive_type == 1)){
		return archive_archivezip_entries(&archive->zip_data);
	} else {
		return archive_archivetar_entries(&archive->tar_data);
	}
}
size_t archive_archiveentry_count(struct archive_archiveArchive* archive){
	if((archive->archive_type == 1)){
		return archive_archivezip_entry_count(&archive->zip_data);
	} else {
		return archive_archivetar_entry_count(&archive->tar_data);
	}
}
_Bool archive_archivecontains(struct archive_archiveArchive* archive, const char* name){
	if((archive->archive_type == 1)){
		return archive_archivezip_contains(&archive->zip_data, name);
	} else {
		return archive_archivetar_contains(&archive->tar_data, name);
	}
}
void archive_archiveread(struct std_stdResult__cgs__39* __chx_struct_ret_param_xx, struct archive_archiveArchive* archive, const char* name){
	if((archive->archive_type == 1)){
		*__chx_struct_ret_param_xx = (*({ struct std_stdResult__cgs__39 __chx__lv__166; archive_archivezip_read_file(&__chx__lv__166, &archive->zip_data, name); &__chx__lv__166; }));
		return;
	} else {
		*__chx_struct_ret_param_xx = (*({ struct std_stdResult__cgs__39 __chx__lv__167; archive_archivetar_read_file(&__chx__lv__167, &archive->tar_data, name); &__chx__lv__167; }));
		return;
	}
}
void archive_archiveextract(struct std_stdResult__cgs__38* __chx_struct_ret_param_xx, struct archive_archiveArchive* archive, const char* dest){
	if((archive->archive_type == 1)){
		*__chx_struct_ret_param_xx = (*({ struct std_stdResult__cgs__38 __chx__lv__168; archive_archivezip_extract_all(&__chx__lv__168, &archive->zip_data, dest); &__chx__lv__168; }));
		return;
	} else {
		*__chx_struct_ret_param_xx = (*({ struct std_stdResult__cgs__38 __chx__lv__169; archive_archivetar_extract_all(&__chx__lv__169, &archive->tar_data, dest); &__chx__lv__169; }));
		return;
	}
}
void archive_archiveextract_entry(struct std_stdResult__cgs__38* __chx_struct_ret_param_xx, struct archive_archiveArchive* archive, const char* name, const char* dest_dir){
	struct archive_archiveArchiveEntry entry;
	_Bool __chx__lv__170 = true;
	if((archive->archive_type == 1)){
		struct std_stdResult__cgs__38 entry_result = (*({ struct std_stdResult__cgs__38 __chx__lv__171; archive_archivezip_find_entry(&__chx__lv__171, &archive->zip_data, name, &entry); &__chx__lv__171; }));
		_Bool __chx__lv__172 = true;
		if((entry_result.__chx__vt_621827 == 1)){
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 1, 
				.Err.error = (struct archive_archiveArchiveError) { .__chx__vt_621827 = 1, 
					.InvalidFormat.msg = (*({ struct std_stdstring __chx__lv__173; std_stdstringconstructor2(&__chx__lv__173, "entry not found", 15, 0); &__chx__lv__173; }))
				}
			};
			if(__chx__lv__172) {
				std_stdResult__cgs__38delete(&entry_result);
			}
			if(__chx__lv__170) {
				archive_archiveArchiveEntrydelete(&entry);
			}
			return;
		}
		*__chx_struct_ret_param_xx = (*({ struct std_stdResult__cgs__38 __chx__lv__174; archive_archivezip_extract_entry(&__chx__lv__174, &archive->zip_data, &entry, dest_dir); &__chx__lv__174; }));
		if(__chx__lv__172) {
			std_stdResult__cgs__38delete(&entry_result);
		}
		if(__chx__lv__170) {
			archive_archiveArchiveEntrydelete(&entry);
		}
		return;
	} else {
		struct std_stdResult__cgs__38 entry_result = (*({ struct std_stdResult__cgs__38 __chx__lv__175; archive_archivetar_find_entry(&__chx__lv__175, &archive->tar_data, name, &entry); &__chx__lv__175; }));
		_Bool __chx__lv__176 = true;
		if((entry_result.__chx__vt_621827 == 1)){
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 1, 
				.Err.error = (struct archive_archiveArchiveError) { .__chx__vt_621827 = 1, 
					.InvalidFormat.msg = (*({ struct std_stdstring __chx__lv__177; std_stdstringconstructor2(&__chx__lv__177, "entry not found", 15, 0); &__chx__lv__177; }))
				}
			};
			if(__chx__lv__176) {
				std_stdResult__cgs__38delete(&entry_result);
			}
			if(__chx__lv__170) {
				archive_archiveArchiveEntrydelete(&entry);
			}
			return;
		}
		*__chx_struct_ret_param_xx = (*({ struct std_stdResult__cgs__38 __chx__lv__178; archive_archivetar_extract_entry(&__chx__lv__178, &archive->tar_data, &entry, dest_dir); &__chx__lv__178; }));
		if(__chx__lv__176) {
			std_stdResult__cgs__38delete(&entry_result);
		}
		if(__chx__lv__170) {
			archive_archiveArchiveEntrydelete(&entry);
		}
		return;
	}
	if(__chx__lv__170) {
		archive_archiveArchiveEntrydelete(&entry);
	}
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/archive/src/deflate.ch **/
static void archive_archiveBitReaderinit(struct archive_archiveBitReader* __chx_struct_ret_param_xx, const uint8_t* d, size_t d_len){
	*__chx_struct_ret_param_xx = (struct archive_archiveBitReader){ 
		.data = d, 
		.data_len = d_len, 
		.byte_pos = 0, 
		.bit_buf = 0, 
		.bits_in_buf = 0
	};
	return;
}
static uint32_t archive_archiveBitReaderread_bits(struct archive_archiveBitReader* self, int n){
	while((self->bits_in_buf < n)) {
		if((self->byte_pos < self->data_len)){
			self->bit_buf = (self->bit_buf | (((uint32_t) self->data[self->byte_pos]) << ((uint32_t) self->bits_in_buf)));
			self->byte_pos += 1;
			self->bits_in_buf += 8;
		} else {
			break;
		}
	}
	uint32_t val = (self->bit_buf & ((1 << ((uint32_t) n)) - 1));
	self->bit_buf = (self->bit_buf >> ((uint32_t) n));
	self->bits_in_buf -= n;
	return val;
}
static uint32_t archive_archiveBitReaderread_bits_reversed(struct archive_archiveBitReader* self, int n){
	uint32_t val = archive_archiveBitReaderread_bits(self, n);
	uint32_t result = 0;
	int i = 0;
	while((i < n)) {
		result = ((result << 1) | (val & 1));
		val = (val >> 1);
		i += 1;
	}
	return result;
}
static void archive_archiveBitReaderalign_to_byte(struct archive_archiveBitReader* self){
	int extra = (self->bits_in_buf % 8);
	if((extra > 0)){
		self->bit_buf = (self->bit_buf >> ((uint32_t) extra));
		self->bits_in_buf -= extra;
	}
}
static int archive_archiveMAX_HUFFMAN_CODES = 288;
static int archive_archiveMAX_HUFFMAN_BITS = 15;
static void archive_archiveHuffTabledelete(struct archive_archiveHuffTable* self){
	__chx__dstctr_clnup_blk__:{
		std_stdvector__cgs__6delete(&self->entries);
	}
}
static void archive_archivebuild_huffman_table(struct std_stdResult__cgs__38* __chx_struct_ret_param_xx, const uint8_t* lengths, int num_codes, struct archive_archiveHuffTable* output){
	uint32_t bl_count[16];
	int i = 0;
	while((i <= archive_archiveMAX_HUFFMAN_BITS)) {
		bl_count[i] = 0;
		i += 1;
	}
	i = 0;
	while((i < num_codes)) {
		int len = ((int) lengths[i]);
		if(((len > 0) && (len <= archive_archiveMAX_HUFFMAN_BITS))){
			bl_count[len] += 1;
		}
		i += 1;
	}
	int max_bits = 0;
	i = 1;
	while((i <= archive_archiveMAX_HUFFMAN_BITS)) {
		if((bl_count[i] > 0)){
			max_bits = i;
		}
		i += 1;
	}
	uint32_t next_code[16];
	uint32_t code = 0;
	i = 1;
	while((i <= archive_archiveMAX_HUFFMAN_BITS)) {
		code = ((code + bl_count[(i - 1)]) << 1);
		next_code[i] = code;
		i += 1;
	}
	struct std_stdvector__cgs__6 entries = (*({ struct std_stdvector__cgs__6 __chx__lv__179; std_stdvector__cgs__6make(&__chx__lv__179); &__chx__lv__179; }));
	_Bool __chx__lv__180 = true;
	int min_bits = (archive_archiveMAX_HUFFMAN_BITS + 1);
	if((max_bits > 0)){
		min_bits = max_bits;
	}
	i = 0;
	while((i < num_codes)) {
		int len = ((int) lengths[i]);
		if((len > 0)){
			struct archive_archiveHuffEntry entry;
			entry.code = ((uint16_t) next_code[len]);
			entry.bits = ((uint8_t) len);
			entry.value = ((uint16_t) i);
			std_stdvector__cgs__6push(&entries, ({ struct archive_archiveHuffEntry __chx__lv__181 = entry; &__chx__lv__181; }));
			if((len < min_bits)){
				min_bits = len;
			}
		}
		i += 1;
	}
	{
		struct std_stdvector__cgs__6 __chx__lv__182 = ({ __chx__lv__180 = false; entries; });
		std_stdvector__cgs__6delete(&output->entries);
		output->entries = __chx__lv__182;
	};
	output->min_bits = min_bits;
	output->max_bits = max_bits;
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 0, 
		.Ok.value = (struct std_stdUnit){ 
		}
	};
	if(__chx__lv__180) {
		std_stdvector__cgs__6delete(&entries);
	}
	return;
}
static uint16_t archive_archivedecode_huffman(struct archive_archiveBitReader* reader, struct archive_archiveHuffTable* table){
	uint32_t code = 0;
	int found_bits = 0;
	int i = 0;
	while((i < table->max_bits)) {
		code = ((code << 1) | archive_archiveBitReaderread_bits(reader, 1));
		found_bits += 1;
		size_t j = 0;
		while((j < std_stdvector__cgs__6size(&table->entries))) {
			struct archive_archiveHuffEntry* e = std_stdvector__cgs__6get_ptr(&table->entries, j);
			if(((((int) e->bits) == found_bits) && (((uint32_t) e->code) == code))){
				return e->value;
			}
			j += 1;
		}
	}
	return 0;
}
void archive_archivedeflate_decompress(struct std_stdResult__cgs__40* __chx_struct_ret_param_xx, const uint8_t* input, size_t input_len, uint8_t* output, size_t output_capacity){
	struct archive_archiveBitReader reader = (*({ struct archive_archiveBitReader __chx__lv__183; archive_archiveBitReaderinit(&__chx__lv__183, input, input_len); &__chx__lv__183; }));
	size_t out_pos = 0;
	_Bool last_block = 0;
	while(!last_block) {
		uint32_t bfinal = archive_archiveBitReaderread_bits(&reader, 1);
		uint32_t btype = archive_archiveBitReaderread_bits(&reader, 2);
		if((bfinal == 1)){
			last_block = 1;
		}
		if((btype == 0)){
			archive_archiveBitReaderalign_to_byte(&reader);
			if(((reader.byte_pos + 4) > input_len)){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__40) { .__chx__vt_621827 = 1, 
					.Err.error = (struct archive_archiveArchiveError) { .__chx__vt_621827 = 3, 
						.DecompressionFailed.msg = (*({ struct std_stdstring __chx__lv__184; std_stdstringconstructor2(&__chx__lv__184, "truncated stored block header", 29, 0); &__chx__lv__184; }))
					}
				};
				return;
			}
			uint16_t len = (((uint16_t) reader.data[reader.byte_pos]) | (((uint16_t) reader.data[(reader.byte_pos + 1)]) << 8));
			reader.byte_pos += 4;
			uint16_t i = 0;
			while((i < len)) {
				if(((out_pos >= output_capacity) || (reader.byte_pos >= input_len))){
					*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__40) { .__chx__vt_621827 = 1, 
						.Err.error = (struct archive_archiveArchiveError) { .__chx__vt_621827 = 3, 
							.DecompressionFailed.msg = (*({ struct std_stdstring __chx__lv__185; std_stdstringconstructor2(&__chx__lv__185, "truncated stored block data", 27, 0); &__chx__lv__185; }))
						}
					};
					return;
				}
				output[out_pos] = reader.data[reader.byte_pos];
				out_pos += 1;
				reader.byte_pos += 1;
				i += 1;
			}
		}else if(((btype == 1) || (btype == 2))){
			uint8_t lit_lens[288];
			uint8_t dist_lens[32];
			int num_lit_codes = 288;
			int num_dist_codes = 0;
			if((btype == 1)){
				int i = 0;
				while((i < 144)) {
					lit_lens[i] = 8;
					i += 1;
				}
				while((i < 256)) {
					lit_lens[i] = 9;
					i += 1;
				}
				while((i < 280)) {
					lit_lens[i] = 7;
					i += 1;
				}
				while((i < 288)) {
					lit_lens[i] = 8;
					i += 1;
				}
				i = 0;
				while((i < 32)) {
					dist_lens[i] = 5;
					i += 1;
				}
				num_dist_codes = 32;
			} else {
				int hlit = (((int) archive_archiveBitReaderread_bits(&reader, 5)) + 257);
				int hdist = (((int) archive_archiveBitReaderread_bits(&reader, 5)) + 1);
				int hclen = (((int) archive_archiveBitReaderread_bits(&reader, 4)) + 4);
				num_lit_codes = hlit;
				num_dist_codes = hdist;
				int cl_order[19] = {16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15};
				uint8_t cl_lens[19];
				int k = 0;
				while((k < 19)) {
					cl_lens[k] = 0;
					k += 1;
				}
				k = 0;
				while((k < hclen)) {
					cl_lens[cl_order[k]] = ((uint8_t) archive_archiveBitReaderread_bits(&reader, 3));
					k += 1;
				}
				struct archive_archiveHuffTable cl_table;
				_Bool __chx__lv__186 = true;
				struct std_stdResult__cgs__38 cl_result = (*({ struct std_stdResult__cgs__38 __chx__lv__187; archive_archivebuild_huffman_table(&__chx__lv__187, &cl_lens[0], 19, &cl_table); &__chx__lv__187; }));
				_Bool __chx__lv__188 = true;
				if((cl_result.__chx__vt_621827 == 1)){
					*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__40) { .__chx__vt_621827 = 1, 
						.Err.error = (struct archive_archiveArchiveError) { .__chx__vt_621827 = 3, 
							.DecompressionFailed.msg = (*({ struct std_stdstring __chx__lv__189; std_stdstringconstructor2(&__chx__lv__189, "invalid code length Huffman table", 33, 0); &__chx__lv__189; }))
						}
					};
					if(__chx__lv__188) {
						std_stdResult__cgs__38delete(&cl_result);
					}
					if(__chx__lv__186) {
						archive_archiveHuffTabledelete(&cl_table);
					}
					return;
				}
				int total = (hlit + hdist);
				int idx = 0;
				while((idx < total)) {
					int sym = ((int) archive_archivedecode_huffman(&reader, &cl_table));
					if((sym <= 15)){
						if((idx < hlit)){
							lit_lens[idx] = ((uint8_t) sym);
						} else {
							dist_lens[(idx - hlit)] = ((uint8_t) sym);
						}
						idx += 1;
					}else if((sym == 16)){
						int repeat = (((int) archive_archiveBitReaderread_bits(&reader, 2)) + 3);
						uint8_t prev_len = 0;
						if((idx > 0)){
							if(((idx - 1) < hlit)){
								prev_len = lit_lens[(idx - 1)];
							} else {
								prev_len = dist_lens[((idx - 1) - hlit)];
							}
						}
						int r = 0;
						while(((r < repeat) && (idx < total))) {
							if((idx < hlit)){
								lit_lens[idx] = prev_len;
							} else {
								dist_lens[(idx - hlit)] = prev_len;
							}
							idx += 1;
							r += 1;
						}
					}else if((sym == 17)){
						int repeat = (((int) archive_archiveBitReaderread_bits(&reader, 3)) + 3);
						int r = 0;
						while(((r < repeat) && (idx < total))) {
							if((idx < hlit)){
								lit_lens[idx] = 0;
							} else {
								dist_lens[(idx - hlit)] = 0;
							}
							idx += 1;
							r += 1;
						}
					}else if((sym == 18)){
						int repeat = (((int) archive_archiveBitReaderread_bits(&reader, 7)) + 11);
						int r = 0;
						while(((r < repeat) && (idx < total))) {
							if((idx < hlit)){
								lit_lens[idx] = 0;
							} else {
								dist_lens[(idx - hlit)] = 0;
							}
							idx += 1;
							r += 1;
						}
					}
				}
				if(__chx__lv__188) {
					std_stdResult__cgs__38delete(&cl_result);
				}
				if(__chx__lv__186) {
					archive_archiveHuffTabledelete(&cl_table);
				}
			}
			struct archive_archiveHuffTable lit_table;
			_Bool __chx__lv__190 = true;
			struct std_stdResult__cgs__38 lit_result = (*({ struct std_stdResult__cgs__38 __chx__lv__191; archive_archivebuild_huffman_table(&__chx__lv__191, &lit_lens[0], num_lit_codes, &lit_table); &__chx__lv__191; }));
			_Bool __chx__lv__192 = true;
			if((lit_result.__chx__vt_621827 == 1)){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__40) { .__chx__vt_621827 = 1, 
					.Err.error = (struct archive_archiveArchiveError) { .__chx__vt_621827 = 3, 
						.DecompressionFailed.msg = (*({ struct std_stdstring __chx__lv__193; std_stdstringconstructor2(&__chx__lv__193, "invalid literal/length Huffman table", 36, 0); &__chx__lv__193; }))
					}
				};
				if(__chx__lv__192) {
					std_stdResult__cgs__38delete(&lit_result);
				}
				if(__chx__lv__190) {
					archive_archiveHuffTabledelete(&lit_table);
				}
				return;
			}
			struct archive_archiveHuffTable dist_table;
			_Bool __chx__lv__194 = true;
			struct std_stdResult__cgs__38 dist_result = (*({ struct std_stdResult__cgs__38 __chx__lv__195; archive_archivebuild_huffman_table(&__chx__lv__195, &dist_lens[0], num_dist_codes, &dist_table); &__chx__lv__195; }));
			_Bool __chx__lv__196 = true;
			if((dist_result.__chx__vt_621827 == 1)){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__40) { .__chx__vt_621827 = 1, 
					.Err.error = (struct archive_archiveArchiveError) { .__chx__vt_621827 = 3, 
						.DecompressionFailed.msg = (*({ struct std_stdstring __chx__lv__197; std_stdstringconstructor2(&__chx__lv__197, "invalid distance Huffman table", 30, 0); &__chx__lv__197; }))
					}
				};
				if(__chx__lv__196) {
					std_stdResult__cgs__38delete(&dist_result);
				}
				if(__chx__lv__194) {
					archive_archiveHuffTabledelete(&dist_table);
				}
				if(__chx__lv__192) {
					std_stdResult__cgs__38delete(&lit_result);
				}
				if(__chx__lv__190) {
					archive_archiveHuffTabledelete(&lit_table);
				}
				return;
			}
			while(1){
				uint16_t sym = archive_archivedecode_huffman(&reader, &lit_table);
				if((sym < 256)){
					if((out_pos >= output_capacity)){
						*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__40) { .__chx__vt_621827 = 1, 
							.Err.error = (struct archive_archiveArchiveError) { .__chx__vt_621827 = 3, 
								.DecompressionFailed.msg = (*({ struct std_stdstring __chx__lv__198; std_stdstringconstructor2(&__chx__lv__198, "output buffer full", 18, 0); &__chx__lv__198; }))
							}
						};
						if(__chx__lv__196) {
							std_stdResult__cgs__38delete(&dist_result);
						}
						if(__chx__lv__194) {
							archive_archiveHuffTabledelete(&dist_table);
						}
						if(__chx__lv__192) {
							std_stdResult__cgs__38delete(&lit_result);
						}
						if(__chx__lv__190) {
							archive_archiveHuffTabledelete(&lit_table);
						}
						return;
					}
					output[out_pos] = ((uint8_t) sym);
					out_pos += 1;
				}else if((sym == 256)){
					break;
				} else {
					int len_extra_bits[29] = {0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0};
					int len_base[29] = {3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258};
					int len_idx = (((int) sym) - 257);
					if(((len_idx < 0) || (len_idx >= 29))){
						*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__40) { .__chx__vt_621827 = 1, 
							.Err.error = (struct archive_archiveArchiveError) { .__chx__vt_621827 = 3, 
								.DecompressionFailed.msg = (*({ struct std_stdstring __chx__lv__199; std_stdstringconstructor2(&__chx__lv__199, "invalid length code", 19, 0); &__chx__lv__199; }))
							}
						};
						if(__chx__lv__196) {
							std_stdResult__cgs__38delete(&dist_result);
						}
						if(__chx__lv__194) {
							archive_archiveHuffTabledelete(&dist_table);
						}
						if(__chx__lv__192) {
							std_stdResult__cgs__38delete(&lit_result);
						}
						if(__chx__lv__190) {
							archive_archiveHuffTabledelete(&lit_table);
						}
						return;
					}
					int length = (len_base[len_idx] + ((int) archive_archiveBitReaderread_bits(&reader, len_extra_bits[len_idx])));
					uint16_t dist_sym = archive_archivedecode_huffman(&reader, &dist_table);
					int dist_extra_bits[30] = {0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13};
					int dist_base[30] = {1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577};
					int dist_idx = ((int) dist_sym);
					if(((dist_idx < 0) || (dist_idx >= 30))){
						*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__40) { .__chx__vt_621827 = 1, 
							.Err.error = (struct archive_archiveArchiveError) { .__chx__vt_621827 = 3, 
								.DecompressionFailed.msg = (*({ struct std_stdstring __chx__lv__200; std_stdstringconstructor2(&__chx__lv__200, "invalid distance code", 21, 0); &__chx__lv__200; }))
							}
						};
						if(__chx__lv__196) {
							std_stdResult__cgs__38delete(&dist_result);
						}
						if(__chx__lv__194) {
							archive_archiveHuffTabledelete(&dist_table);
						}
						if(__chx__lv__192) {
							std_stdResult__cgs__38delete(&lit_result);
						}
						if(__chx__lv__190) {
							archive_archiveHuffTabledelete(&lit_table);
						}
						return;
					}
					int distance = (dist_base[dist_idx] + ((int) archive_archiveBitReaderread_bits(&reader, dist_extra_bits[dist_idx])));
					if(((distance <= 0) || (((size_t) distance) > out_pos))){
						*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__40) { .__chx__vt_621827 = 1, 
							.Err.error = (struct archive_archiveArchiveError) { .__chx__vt_621827 = 3, 
								.DecompressionFailed.msg = (*({ struct std_stdstring __chx__lv__201; std_stdstringconstructor2(&__chx__lv__201, "invalid distance", 16, 0); &__chx__lv__201; }))
							}
						};
						if(__chx__lv__196) {
							std_stdResult__cgs__38delete(&dist_result);
						}
						if(__chx__lv__194) {
							archive_archiveHuffTabledelete(&dist_table);
						}
						if(__chx__lv__192) {
							std_stdResult__cgs__38delete(&lit_result);
						}
						if(__chx__lv__190) {
							archive_archiveHuffTabledelete(&lit_table);
						}
						return;
					}
					unsigned long src_pos = (out_pos - ((size_t) distance));
					int i = 0;
					while((i < length)) {
						if((out_pos >= output_capacity)){
							*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__40) { .__chx__vt_621827 = 1, 
								.Err.error = (struct archive_archiveArchiveError) { .__chx__vt_621827 = 3, 
									.DecompressionFailed.msg = (*({ struct std_stdstring __chx__lv__202; std_stdstringconstructor2(&__chx__lv__202, "output buffer full during backreference", 39, 0); &__chx__lv__202; }))
								}
							};
							if(__chx__lv__196) {
								std_stdResult__cgs__38delete(&dist_result);
							}
							if(__chx__lv__194) {
								archive_archiveHuffTabledelete(&dist_table);
							}
							if(__chx__lv__192) {
								std_stdResult__cgs__38delete(&lit_result);
							}
							if(__chx__lv__190) {
								archive_archiveHuffTabledelete(&lit_table);
							}
							return;
						}
						output[out_pos] = output[src_pos];
						out_pos += 1;
						src_pos += 1;
						i += 1;
					}
				}
			}
			if(__chx__lv__196) {
				std_stdResult__cgs__38delete(&dist_result);
			}
			if(__chx__lv__194) {
				archive_archiveHuffTabledelete(&dist_table);
			}
			if(__chx__lv__192) {
				std_stdResult__cgs__38delete(&lit_result);
			}
			if(__chx__lv__190) {
				archive_archiveHuffTabledelete(&lit_table);
			}
		} else {
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__40) { .__chx__vt_621827 = 1, 
				.Err.error = (struct archive_archiveArchiveError) { .__chx__vt_621827 = 3, 
					.DecompressionFailed.msg = (*({ struct std_stdstring __chx__lv__203; std_stdstringconstructor2(&__chx__lv__203, "reserved block type", 19, 0); &__chx__lv__203; }))
				}
			};
			return;
		}
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__40) { .__chx__vt_621827 = 0, 
		.Ok.value = out_pos
	};
	return;
}