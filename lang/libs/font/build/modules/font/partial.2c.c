
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/font/src/ttf.ch **/
struct font_fontFont;
struct font_fontFontTable;
struct font_fontTextMetrics;
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/font/src/types.ch **/
struct font_fontFontError;
struct font_fontGlyphMetrics;
struct font_fontGlyphOutline;
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/font/src/ttf.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/font/src/types.ch **/
/** FwdDeclare:Generics font **/
struct std_stdvector__cgs__4;
struct std_stdvector__cgs__5;
struct std_stdResult__cgs__37;
struct std_stdResult__cgs__38;
struct std_stdResult__cgs__39;
struct std_stdResult__cgs__40;
struct std_stdResult__cgs__41;
struct std_stdResult__cgs__42;
/** Declare:Generics font **/
struct std_stdvector__cgs__4 {
	struct font_fontFontTable* data_ptr;
	size_t data_size;
	size_t data_cap;
};
void std_stdvector__cgs__4make(struct std_stdvector__cgs__4* this);
void std_stdvector__cgs__4make_with_capacity(struct std_stdvector__cgs__4* this, size_t init_cap);
void std_stdvector__cgs__4resize(struct std_stdvector__cgs__4* self, size_t new_size);
void std_stdvector__cgs__4reserve(struct std_stdvector__cgs__4* self, size_t new_cap);
void std_stdvector__cgs__4ensure_capacity_for_one_more(struct std_stdvector__cgs__4* self);
void std_stdvector__cgs__4push(struct std_stdvector__cgs__4* self, struct font_fontFontTable* value);
void std_stdvector__cgs__4push_back(struct std_stdvector__cgs__4* self, struct font_fontFontTable* value);
void std_stdvector__cgs__4get(struct font_fontFontTable* __chx_struct_ret_param_xx, struct std_stdvector__cgs__4*const self, size_t index);
struct font_fontFontTable* std_stdvector__cgs__4get_ptr(struct std_stdvector__cgs__4*const self, size_t index);
struct font_fontFontTable* std_stdvector__cgs__4get_ref(struct std_stdvector__cgs__4*const self, size_t index);
void std_stdvector__cgs__4set(struct std_stdvector__cgs__4* self, size_t index, struct font_fontFontTable* value);
size_t std_stdvector__cgs__4size(struct std_stdvector__cgs__4*const self);
size_t std_stdvector__cgs__4capacity(struct std_stdvector__cgs__4*const self);
const struct font_fontFontTable* std_stdvector__cgs__4data(struct std_stdvector__cgs__4*const self);
struct font_fontFontTable* std_stdvector__cgs__4last_ptr(struct std_stdvector__cgs__4*const self);
void std_stdvector__cgs__4remove(struct std_stdvector__cgs__4* self, size_t index);
void std_stdvector__cgs__4erase(struct std_stdvector__cgs__4* self, size_t index);
void std_stdvector__cgs__4remove_last(struct std_stdvector__cgs__4* self);
void std_stdvector__cgs__4pop_back(struct std_stdvector__cgs__4* self);
void std_stdvector__cgs__4take_last(struct font_fontFontTable* __chx_struct_ret_param_xx, struct std_stdvector__cgs__4* self);
_Bool std_stdvector__cgs__4empty(struct std_stdvector__cgs__4*const self);
void std_stdvector__cgs__4clear(struct std_stdvector__cgs__4* self);
void std_stdvector__cgs__4set_len(struct std_stdvector__cgs__4* self, size_t new_size);
void std_stdvector__cgs__4delete(struct std_stdvector__cgs__4* self);
struct std_stdvector__cgs__5 {
	int32_t* data_ptr;
	size_t data_size;
	size_t data_cap;
};
void std_stdvector__cgs__5make(struct std_stdvector__cgs__5* this);
void std_stdvector__cgs__5make_with_capacity(struct std_stdvector__cgs__5* this, size_t init_cap);
void std_stdvector__cgs__5resize(struct std_stdvector__cgs__5* self, size_t new_size);
void std_stdvector__cgs__5reserve(struct std_stdvector__cgs__5* self, size_t new_cap);
void std_stdvector__cgs__5ensure_capacity_for_one_more(struct std_stdvector__cgs__5* self);
void std_stdvector__cgs__5push(struct std_stdvector__cgs__5* self, int32_t value);
void std_stdvector__cgs__5push_back(struct std_stdvector__cgs__5* self, int32_t value);
int32_t std_stdvector__cgs__5get(struct std_stdvector__cgs__5*const self, size_t index);
int32_t* std_stdvector__cgs__5get_ptr(struct std_stdvector__cgs__5*const self, size_t index);
int32_t* std_stdvector__cgs__5get_ref(struct std_stdvector__cgs__5*const self, size_t index);
void std_stdvector__cgs__5set(struct std_stdvector__cgs__5* self, size_t index, int32_t value);
size_t std_stdvector__cgs__5size(struct std_stdvector__cgs__5*const self);
size_t std_stdvector__cgs__5capacity(struct std_stdvector__cgs__5*const self);
const int32_t* std_stdvector__cgs__5data(struct std_stdvector__cgs__5*const self);
int32_t* std_stdvector__cgs__5last_ptr(struct std_stdvector__cgs__5*const self);
void std_stdvector__cgs__5remove(struct std_stdvector__cgs__5* self, size_t index);
void std_stdvector__cgs__5erase(struct std_stdvector__cgs__5* self, size_t index);
void std_stdvector__cgs__5remove_last(struct std_stdvector__cgs__5* self);
void std_stdvector__cgs__5pop_back(struct std_stdvector__cgs__5* self);
int32_t std_stdvector__cgs__5take_last(struct std_stdvector__cgs__5* self);
_Bool std_stdvector__cgs__5empty(struct std_stdvector__cgs__5*const self);
void std_stdvector__cgs__5clear(struct std_stdvector__cgs__5* self);
void std_stdvector__cgs__5set_len(struct std_stdvector__cgs__5* self, size_t new_size);
void std_stdvector__cgs__5delete(struct std_stdvector__cgs__5* self);
typedef struct __chx_core_coreiterableLinear__cgs__4_vt_t {
	const struct font_fontFontTable*(*data)(void* self);uint64_t(*size)(void* self);
} __chx_core_coreiterableLinear__cgs__4_vt_t;
typedef struct __chx_core_coreiterableLinear__cgs__5_vt_t {
	const int32_t*(*data)(void* self);uint64_t(*size)(void* self);
} __chx_core_coreiterableLinear__cgs__5_vt_t;
struct font_fontFont {
	struct std_stdvector__cgs__3 data;
	struct std_stdvector__cgs__4 tables;
	uint16_t units_per_em;
	int16_t ascent;
	int16_t descent;
	uint16_t num_glyphs;
	_Bool loaded;
};
struct font_fontFontError {
	int __chx__vt_621827;
	union {
		struct {
		} FileNotFound;
		struct {
			struct std_stdstring msg;
		} InvalidFormat;
		struct {
			struct std_stdstring msg;
		} UnsupportedFeature;
		struct {
			struct std_stdstring msg;
		} IoError;
		struct {
			uint32_t codepoint;
		} GlyphNotFound;
	};
};
struct std_stdResult__cgs__37 {
	int __chx__vt_621827;
	union {
		struct {
			struct font_fontFont value;
		} Ok;
		struct {
			struct font_fontFontError error;
		} Err;
	};
};
void std_stdResult__cgs__37delete(struct std_stdResult__cgs__37* self);
struct std_stdResult__cgs__38 {
	int __chx__vt_621827;
	union {
		struct {
			struct std_stdUnit value;
		} Ok;
		struct {
			struct font_fontFontError error;
		} Err;
	};
};
void std_stdResult__cgs__38delete(struct std_stdResult__cgs__38* self);
struct font_fontFontTable {
	uint32_t tag;
	uint32_t offset;
	uint32_t length;
};
struct std_stdResult__cgs__39 {
	int __chx__vt_621827;
	union {
		struct {
			struct font_fontFontTable value;
		} Ok;
		struct {
			struct font_fontFontError error;
		} Err;
	};
};
void std_stdResult__cgs__39delete(struct std_stdResult__cgs__39* self);
struct font_fontGlyphMetrics {
	int32_t advance_width;
	int32_t left_side_bearing;
	int32_t xmin;
	int32_t ymin;
	int32_t xmax;
	int32_t ymax;
	int32_t width;
	int32_t height;
};
struct std_stdResult__cgs__40 {
	int __chx__vt_621827;
	union {
		struct {
			struct font_fontGlyphMetrics value;
		} Ok;
		struct {
			struct font_fontFontError error;
		} Err;
	};
};
void std_stdResult__cgs__40delete(struct std_stdResult__cgs__40* self);
struct font_fontGlyphOutline {
	struct std_stdvector__cgs__5 points;
	struct std_stdvector__cgs__3 flags;
	struct std_stdvector__cgs__3 instructions;
};
struct std_stdResult__cgs__41 {
	int __chx__vt_621827;
	union {
		struct {
			struct font_fontGlyphOutline value;
		} Ok;
		struct {
			struct font_fontFontError error;
		} Err;
	};
};
void std_stdResult__cgs__41delete(struct std_stdResult__cgs__41* self);
struct std_stdResult__cgs__42 {
	int __chx__vt_621827;
	union {
		struct {
			uint16_t value;
		} Ok;
		struct {
			struct font_fontFontError error;
		} Err;
	};
};
void std_stdResult__cgs__42delete(struct std_stdResult__cgs__42* self);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/font/src/ttf.ch **/
static uint16_t font_fontread_u16_be(const uint8_t* data, size_t offset);
static uint32_t font_fontread_u32_be(const uint8_t* data, size_t offset);
static int16_t font_fontread_i16_be(const uint8_t* data, size_t offset);
static int32_t font_fontread_i32_be(const uint8_t* data, size_t offset);
void font_fontFontmake(struct font_fontFont* this);
void font_fontFontdelete(struct font_fontFont* self);
void font_fontfont_load(struct std_stdResult__cgs__37* __chx_struct_ret_param_xx, const char* path);
static void font_fontparse_font_header(struct std_stdResult__cgs__38* __chx_struct_ret_param_xx, struct font_fontFont* font);
static void font_fontfind_table(struct std_stdResult__cgs__39* __chx_struct_ret_param_xx, struct font_fontFont* font, uint32_t tag);
uint16_t font_fontfont_units_per_em(struct font_fontFont* font);
int16_t font_fontfont_ascent(struct font_fontFont* font);
int16_t font_fontfont_descent(struct font_fontFont* font);
uint16_t font_fontfont_num_glyphs(struct font_fontFont* font);
int32_t font_fontfont_line_height(struct font_fontFont* font);
static uint16_t font_fontcmap_lookup(struct font_fontFont* font, uint32_t codepoint);
static uint16_t font_fontcmap_format4_lookup(struct font_fontFont* font, size_t base, uint32_t codepoint);
void font_fontfont_get_glyph_metrics(struct std_stdResult__cgs__40* __chx_struct_ret_param_xx, struct font_fontFont* font, uint16_t glyph_id);
void font_fontfont_get_glyph_outline(struct std_stdResult__cgs__41* __chx_struct_ret_param_xx, struct font_fontFont* font, uint16_t glyph_id);
void font_fontfont_get_glyph(struct std_stdResult__cgs__42* __chx_struct_ret_param_xx, struct font_fontFont* font, uint32_t codepoint);
struct font_fontTextMetrics {
	int32_t width;
	int32_t height;
	int32_t ascent;
	int32_t descent;
};
void font_fontfont_measure_text(struct font_fontTextMetrics* __chx_struct_ret_param_xx, struct font_fontFont* font, const char* text, size_t text_len, float font_size);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/font/src/types.ch **/
void font_fontFontErrormessage(struct std_stdstring* __chx_struct_ret_param_xx, struct font_fontFontError*const self);
void font_fontFontErrordelete(struct font_fontFontError* self);
void font_fontGlyphOutlinemake(struct font_fontGlyphOutline* this);
void font_fontGlyphOutlinedelete(struct font_fontGlyphOutline* self);
/** Implement:Generics font **/
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
		.data_ptr = ((struct font_fontFontTable*) malloc((sizeof(struct font_fontFontTable) * init_cap))), 
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
		struct font_fontFontTable* start = (self->data_ptr + new_size);
		
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
		self->data_ptr[i] = ((struct font_fontFontTable){0});
		i = (i + 1);
	}
	self->data_size = new_size;
}
void std_stdvector__cgs__4reserve(struct std_stdvector__cgs__4* self, size_t new_cap){
	if((new_cap <= self->data_cap)){
		return;
	}
	struct font_fontFontTable* new_data = ((struct font_fontFontTable*) realloc(self->data_ptr, (sizeof(struct font_fontFontTable) * new_cap)));
	
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
void std_stdvector__cgs__4push(struct std_stdvector__cgs__4* self, struct font_fontFontTable* value){
	size_t s = self->data_size;
	if((s >= self->data_cap)){
		std_stdvector__cgs__4ensure_capacity_for_one_more(self);
	}
	memcpy(&self->data_ptr[s], value, sizeof(struct font_fontFontTable));
	0;
	self->data_size = (s + 1);
}
void std_stdvector__cgs__4push_back(struct std_stdvector__cgs__4* self, struct font_fontFontTable* value){
	std_stdvector__cgs__4push(self, *({  &value; }));
}
void std_stdvector__cgs__4get(struct font_fontFontTable* __chx_struct_ret_param_xx, struct std_stdvector__cgs__4*const self, size_t index){
	*__chx_struct_ret_param_xx = self->data_ptr[index];
	return;
}
struct font_fontFontTable* std_stdvector__cgs__4get_ptr(struct std_stdvector__cgs__4*const self, size_t index){
	return &self->data_ptr[index];
}
struct font_fontFontTable* std_stdvector__cgs__4get_ref(struct std_stdvector__cgs__4*const self, size_t index){
	return &self->data_ptr[index];
}
void std_stdvector__cgs__4set(struct std_stdvector__cgs__4* self, size_t index, struct font_fontFontTable* value){
	self->data_ptr[index] = ({  *value; });
}
size_t std_stdvector__cgs__4size(struct std_stdvector__cgs__4*const self){
	return self->data_size;
}
size_t std_stdvector__cgs__4capacity(struct std_stdvector__cgs__4*const self){
	return self->data_cap;
}
const struct font_fontFontTable* std_stdvector__cgs__4data(struct std_stdvector__cgs__4*const self){
	return self->data_ptr;
}
struct font_fontFontTable* std_stdvector__cgs__4last_ptr(struct std_stdvector__cgs__4*const self){
	return (self->data_ptr + (self->data_size - 1));
}
void std_stdvector__cgs__4remove(struct std_stdvector__cgs__4* self, size_t index){
	size_t s = self->data_size;
	unsigned long last = (s - 1);
	
	if((index == last)){
		self->data_size = last;
	} else {
		memmove(&self->data_ptr[index], &self->data_ptr[(index + 1)], (sizeof(struct font_fontFontTable) * (last - index)));
		self->data_size = last;
	}
}
void std_stdvector__cgs__4erase(struct std_stdvector__cgs__4* self, size_t index){
	std_stdvector__cgs__4remove(self, index);
}
void std_stdvector__cgs__4remove_last(struct std_stdvector__cgs__4* self){
	size_t s = self->data_size;
	unsigned long last = (s - 1);
	struct font_fontFontTable* ptr = std_stdvector__cgs__4get_ptr(self, last);
	
	self->data_size = last;
}
void std_stdvector__cgs__4pop_back(struct std_stdvector__cgs__4* self){
	std_stdvector__cgs__4remove_last(self);
}
void std_stdvector__cgs__4take_last(struct font_fontFontTable* __chx_struct_ret_param_xx, struct std_stdvector__cgs__4* self){
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
const struct font_fontFontTable* core_coreiterableLinear__cgs__4_vector__cgs__4_data(struct std_stdvector__cgs__4*const self){
	return self->data_ptr;
}
size_t core_coreiterableLinear__cgs__4_vector__cgs__4_size(struct std_stdvector__cgs__4*const self){
	return self->data_size;
}
const __chx_core_coreiterableLinear__cgs__4_vt_t core_coreiterableLinear__cgs__4std_stdvector__cgs__4 = {
	(const struct font_fontFontTable*(*)(void* self)) core_coreiterableLinear__cgs__4_vector__cgs__4_data,
	(uint64_t(*)(void* self)) core_coreiterableLinear__cgs__4_vector__cgs__4_size,
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
		.data_ptr = ((int32_t*) malloc((sizeof(int32_t) * init_cap))), 
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
		int32_t* start = (self->data_ptr + new_size);
		
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
		self->data_ptr[i] = ((int32_t){0});
		i = (i + 1);
	}
	self->data_size = new_size;
}
void std_stdvector__cgs__5reserve(struct std_stdvector__cgs__5* self, size_t new_cap){
	if((new_cap <= self->data_cap)){
		return;
	}
	int32_t* new_data = ((int32_t*) realloc(self->data_ptr, (sizeof(int32_t) * new_cap)));
	
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
void std_stdvector__cgs__5push(struct std_stdvector__cgs__5* self, int32_t value){
	size_t s = self->data_size;
	if((s >= self->data_cap)){
		std_stdvector__cgs__5ensure_capacity_for_one_more(self);
	}
	memcpy(&self->data_ptr[s], &value, sizeof(int32_t));
	0;
	self->data_size = (s + 1);
}
void std_stdvector__cgs__5push_back(struct std_stdvector__cgs__5* self, int32_t value){
	std_stdvector__cgs__5push(self, value);
}
int32_t std_stdvector__cgs__5get(struct std_stdvector__cgs__5*const self, size_t index){
	return self->data_ptr[index];
}
int32_t* std_stdvector__cgs__5get_ptr(struct std_stdvector__cgs__5*const self, size_t index){
	return &self->data_ptr[index];
}
int32_t* std_stdvector__cgs__5get_ref(struct std_stdvector__cgs__5*const self, size_t index){
	return &self->data_ptr[index];
}
void std_stdvector__cgs__5set(struct std_stdvector__cgs__5* self, size_t index, int32_t value){
	self->data_ptr[index] = ({  value; });
}
size_t std_stdvector__cgs__5size(struct std_stdvector__cgs__5*const self){
	return self->data_size;
}
size_t std_stdvector__cgs__5capacity(struct std_stdvector__cgs__5*const self){
	return self->data_cap;
}
const int32_t* std_stdvector__cgs__5data(struct std_stdvector__cgs__5*const self){
	return self->data_ptr;
}
int32_t* std_stdvector__cgs__5last_ptr(struct std_stdvector__cgs__5*const self){
	return (self->data_ptr + (self->data_size - 1));
}
void std_stdvector__cgs__5remove(struct std_stdvector__cgs__5* self, size_t index){
	size_t s = self->data_size;
	unsigned long last = (s - 1);
	
	if((index == last)){
		self->data_size = last;
	} else {
		memmove(&self->data_ptr[index], &self->data_ptr[(index + 1)], (sizeof(int32_t) * (last - index)));
		self->data_size = last;
	}
}
void std_stdvector__cgs__5erase(struct std_stdvector__cgs__5* self, size_t index){
	std_stdvector__cgs__5remove(self, index);
}
void std_stdvector__cgs__5remove_last(struct std_stdvector__cgs__5* self){
	size_t s = self->data_size;
	unsigned long last = (s - 1);
	int32_t* ptr = std_stdvector__cgs__5get_ptr(self, last);
	
	self->data_size = last;
}
void std_stdvector__cgs__5pop_back(struct std_stdvector__cgs__5* self){
	std_stdvector__cgs__5remove_last(self);
}
int32_t std_stdvector__cgs__5take_last(struct std_stdvector__cgs__5* self){
	unsigned long last = (self->data_size - 1);
	self->data_size = last;
	
	return *std_stdvector__cgs__5get_ptr(self, last);
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
const int32_t* core_coreiterableLinear__cgs__5_vector__cgs__5_data(struct std_stdvector__cgs__5*const self){
	return self->data_ptr;
}
size_t core_coreiterableLinear__cgs__5_vector__cgs__5_size(struct std_stdvector__cgs__5*const self){
	return self->data_size;
}
const __chx_core_coreiterableLinear__cgs__5_vt_t core_coreiterableLinear__cgs__5std_stdvector__cgs__5 = {
	(const int32_t*(*)(void* self)) core_coreiterableLinear__cgs__5_vector__cgs__5_data,
	(uint64_t(*)(void* self)) core_coreiterableLinear__cgs__5_vector__cgs__5_size,
};
void std_stdResult__cgs__37delete(struct std_stdResult__cgs__37* self){
	__chx__dstctr_clnup_blk__:{
		switch(self->__chx__vt_621827) {
			case 0:
			font_fontFontdelete(&self->Ok.value);
			break;
			case 1:
			font_fontFontErrordelete(&self->Err.error);
			break;
		}
	}
}
void std_stdResult__cgs__38delete(struct std_stdResult__cgs__38* self){
	__chx__dstctr_clnup_blk__:{
		switch(self->__chx__vt_621827) {
			case 0:
			break;
			case 1:
			font_fontFontErrordelete(&self->Err.error);
			break;
		}
	}
}
void std_stdResult__cgs__39delete(struct std_stdResult__cgs__39* self){
	__chx__dstctr_clnup_blk__:{
		switch(self->__chx__vt_621827) {
			case 0:
			break;
			case 1:
			font_fontFontErrordelete(&self->Err.error);
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
			font_fontFontErrordelete(&self->Err.error);
			break;
		}
	}
}
void std_stdResult__cgs__41delete(struct std_stdResult__cgs__41* self){
	__chx__dstctr_clnup_blk__:{
		switch(self->__chx__vt_621827) {
			case 0:
			font_fontGlyphOutlinedelete(&self->Ok.value);
			break;
			case 1:
			font_fontFontErrordelete(&self->Err.error);
			break;
		}
	}
}
void std_stdResult__cgs__42delete(struct std_stdResult__cgs__42* self){
	__chx__dstctr_clnup_blk__:{
		switch(self->__chx__vt_621827) {
			case 0:
			break;
			case 1:
			font_fontFontErrordelete(&self->Err.error);
			break;
		}
	}
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/font/src/ttf.ch **/
static uint16_t font_fontread_u16_be(const uint8_t* data, size_t offset){
	return ((((uint16_t) data[offset]) << 8) | ((uint16_t) data[(offset + 1)]));
}
static uint32_t font_fontread_u32_be(const uint8_t* data, size_t offset){
	return ((((((uint32_t) data[offset]) << 24) | (((uint32_t) data[(offset + 1)]) << 16)) | (((uint32_t) data[(offset + 2)]) << 8)) | ((uint32_t) data[(offset + 3)]));
}
static int16_t font_fontread_i16_be(const uint8_t* data, size_t offset){
	uint16_t val = font_fontread_u16_be(data, offset);
	return ((int16_t) val);
}
static int32_t font_fontread_i32_be(const uint8_t* data, size_t offset){
	uint32_t val = font_fontread_u32_be(data, offset);
	return ((int32_t) val);
}
void font_fontFontmake(struct font_fontFont* this){
	*this = (struct font_fontFont){ 
		.data = (*({ struct std_stdvector__cgs__3 __chx__lv__0; std_stdvector__cgs__3make(&__chx__lv__0); &__chx__lv__0; })), 
		.tables = (*({ struct std_stdvector__cgs__4 __chx__lv__1; std_stdvector__cgs__4make(&__chx__lv__1); &__chx__lv__1; })), 
		.units_per_em = 0, 
		.ascent = 0, 
		.descent = 0, 
		.num_glyphs = 0, 
		.loaded = 0
	};
	return;
}
void font_fontFontdelete(struct font_fontFont* self){
	__chx__dstctr_clnup_blk__:{
		std_stdvector__cgs__3delete(&self->data);
		std_stdvector__cgs__4delete(&self->tables);
	}
}
void font_fontfont_load(struct std_stdResult__cgs__37* __chx_struct_ret_param_xx, const char* path){
	struct std_stdResult__cgs__36 read_result = (*({ struct std_stdResult__cgs__36 __chx__lv__2; fs_fsread_entire_file(&__chx__lv__2, path); &__chx__lv__2; }));
	_Bool __chx__lv__3 = true;
	if((read_result.__chx__vt_621827 == 1)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__37) { .__chx__vt_621827 = 1, 
			.Err.error = (struct font_fontFontError) { .__chx__vt_621827 = 0, 
			}
		};
		if(__chx__lv__3) {
			std_stdResult__cgs__36delete(&read_result);
		}
		return;
	}
	struct font_fontFont font = (*({ struct font_fontFont __chx__lv__4; font_fontFontmake(&__chx__lv__4); &__chx__lv__4; }));
	_Bool __chx__lv__5 = true;
	struct std_stdResult__cgs__36* __chx__lv__6 = &read_result;
	memcpy(&font.data, &__chx__lv__6->Ok.value, sizeof(struct std_stdvector__cgs__3));
	({ struct std_stdvector__cgs__3* __chx__lv__7 = &__chx__lv__6->Ok.value; *__chx__lv__7 = (*({ struct std_stdvector__cgs__3 __chx__lv__8; std_stdvector__cgs__3make(&__chx__lv__8); &__chx__lv__8; })); __chx__lv__7; });
	struct std_stdResult__cgs__38 parse_result = (*({ struct std_stdResult__cgs__38 __chx__lv__9; font_fontparse_font_header(&__chx__lv__9, &font); &__chx__lv__9; }));
	_Bool __chx__lv__10 = true;
	if((parse_result.__chx__vt_621827 == 1)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__37) { .__chx__vt_621827 = 1, 
			.Err.error = (struct font_fontFontError) { .__chx__vt_621827 = 1, 
				.InvalidFormat.msg = (*({ struct std_stdstring __chx__lv__11; std_stdstringconstructor2(&__chx__lv__11, "failed to parse font header", 27, 0); &__chx__lv__11; }))
			}
		};
		if(__chx__lv__10) {
			std_stdResult__cgs__38delete(&parse_result);
		}
		if(__chx__lv__5) {
			font_fontFontdelete(&font);
		}
		if(__chx__lv__3) {
			std_stdResult__cgs__36delete(&read_result);
		}
		return;
	}
	font.loaded = 1;
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__37) { .__chx__vt_621827 = 0, 
		.Ok.value = ({ __chx__lv__5 = false; font; })
	};
	if(__chx__lv__10) {
		std_stdResult__cgs__38delete(&parse_result);
	}
	if(__chx__lv__5) {
		font_fontFontdelete(&font);
	}
	if(__chx__lv__3) {
		std_stdResult__cgs__36delete(&read_result);
	}
	return;
}
static void font_fontparse_font_header(struct std_stdResult__cgs__38* __chx_struct_ret_param_xx, struct font_fontFont* font){
	if((std_stdvector__cgs__3size(&font->data) < 12)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 1, 
			.Err.error = (struct font_fontFontError) { .__chx__vt_621827 = 1, 
				.InvalidFormat.msg = (*({ struct std_stdstring __chx__lv__12; std_stdstringconstructor2(&__chx__lv__12, "font file too small", 19, 0); &__chx__lv__12; }))
			}
		};
		return;
	}
	uint32_t sfnt_version = font_fontread_u32_be(std_stdvector__cgs__3data(&font->data), 0);
	if(((sfnt_version != 65536) && (sfnt_version != 1330926671))){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 1, 
			.Err.error = (struct font_fontFontError) { .__chx__vt_621827 = 1, 
				.InvalidFormat.msg = (*({ struct std_stdstring __chx__lv__13; std_stdstringconstructor2(&__chx__lv__13, "not a TrueType or OpenType font", 31, 0); &__chx__lv__13; }))
			}
		};
		return;
	}
	uint16_t num_tables = font_fontread_u16_be(std_stdvector__cgs__3data(&font->data), 4);
	uint16_t i = 0;
	while((i < num_tables)) {
		unsigned long entry_offset = (12 + (((size_t) i) * 16));
		if(((entry_offset + 16) > std_stdvector__cgs__3size(&font->data))){
			break;
		}
		uint32_t tag = font_fontread_u32_be(std_stdvector__cgs__3data(&font->data), entry_offset);
		uint32_t checksum = font_fontread_u32_be(std_stdvector__cgs__3data(&font->data), (entry_offset + 4));
		uint32_t table_offset = font_fontread_u32_be(std_stdvector__cgs__3data(&font->data), (entry_offset + 8));
		uint32_t table_length = font_fontread_u32_be(std_stdvector__cgs__3data(&font->data), (entry_offset + 12));
		struct font_fontFontTable table;
		table.tag = tag;
		table.offset = table_offset;
		table.length = table_length;
		std_stdvector__cgs__4push(&font->tables, ({ struct font_fontFontTable __chx__lv__14 = table; &__chx__lv__14; }));
		i += 1;
	}
	struct std_stdResult__cgs__39 head_result = (*({ struct std_stdResult__cgs__39 __chx__lv__15; font_fontfind_table(&__chx__lv__15, font, 1751474532); &__chx__lv__15; }));
	_Bool __chx__lv__16 = true;
	if((head_result.__chx__vt_621827 == 0)){
		struct std_stdResult__cgs__39* __chx__lv__17 = &head_result;
		if(((((size_t) __chx__lv__17->Ok.value.offset) + 54) <= std_stdvector__cgs__3size(&font->data))){
			font->units_per_em = font_fontread_u16_be(std_stdvector__cgs__3data(&font->data), (((size_t) __chx__lv__17->Ok.value.offset) + 18));
		}
	}
	struct std_stdResult__cgs__39 hhea_result = (*({ struct std_stdResult__cgs__39 __chx__lv__18; font_fontfind_table(&__chx__lv__18, font, 1751672161); &__chx__lv__18; }));
	_Bool __chx__lv__19 = true;
	if((hhea_result.__chx__vt_621827 == 0)){
		struct std_stdResult__cgs__39* __chx__lv__20 = &hhea_result;
		if(((((size_t) __chx__lv__20->Ok.value.offset) + 36) <= std_stdvector__cgs__3size(&font->data))){
			font->ascent = font_fontread_i16_be(std_stdvector__cgs__3data(&font->data), (((size_t) __chx__lv__20->Ok.value.offset) + 4));
			font->descent = font_fontread_i16_be(std_stdvector__cgs__3data(&font->data), (((size_t) __chx__lv__20->Ok.value.offset) + 6));
		}
	}
	struct std_stdResult__cgs__39 maxp_result = (*({ struct std_stdResult__cgs__39 __chx__lv__21; font_fontfind_table(&__chx__lv__21, font, 1835104368); &__chx__lv__21; }));
	_Bool __chx__lv__22 = true;
	if((maxp_result.__chx__vt_621827 == 0)){
		struct std_stdResult__cgs__39* __chx__lv__23 = &maxp_result;
		if(((((size_t) __chx__lv__23->Ok.value.offset) + 32) <= std_stdvector__cgs__3size(&font->data))){
			font->num_glyphs = font_fontread_u16_be(std_stdvector__cgs__3data(&font->data), (((size_t) __chx__lv__23->Ok.value.offset) + 4));
		}
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 0, 
		.Ok.value = (struct std_stdUnit){ 
		}
	};
	if(__chx__lv__22) {
		std_stdResult__cgs__39delete(&maxp_result);
	}
	if(__chx__lv__19) {
		std_stdResult__cgs__39delete(&hhea_result);
	}
	if(__chx__lv__16) {
		std_stdResult__cgs__39delete(&head_result);
	}
	return;
}
static void font_fontfind_table(struct std_stdResult__cgs__39* __chx_struct_ret_param_xx, struct font_fontFont* font, uint32_t tag){
	size_t i = 0;
	while((i < std_stdvector__cgs__4size(&font->tables))) {
		struct font_fontFontTable* t = std_stdvector__cgs__4get_ptr(&font->tables, i);
		if((t->tag == tag)){
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__39) { .__chx__vt_621827 = 0, 
				.Ok.value = (*({ struct font_fontFontTable __chx__lv__24; std_stdvector__cgs__4get(&__chx__lv__24, &font->tables, i); &__chx__lv__24; }))
			};
			return;
		}
		i += 1;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__39) { .__chx__vt_621827 = 1, 
		.Err.error = (struct font_fontFontError) { .__chx__vt_621827 = 1, 
			.InvalidFormat.msg = (*({ struct std_stdstring __chx__lv__25; std_stdstringconstructor2(&__chx__lv__25, "required table not found", 24, 0); &__chx__lv__25; }))
		}
	};
	return;
}
uint16_t font_fontfont_units_per_em(struct font_fontFont* font){
	return font->units_per_em;
}
int16_t font_fontfont_ascent(struct font_fontFont* font){
	return font->ascent;
}
int16_t font_fontfont_descent(struct font_fontFont* font){
	return font->descent;
}
uint16_t font_fontfont_num_glyphs(struct font_fontFont* font){
	return font->num_glyphs;
}
int32_t font_fontfont_line_height(struct font_fontFont* font){
	return (((int32_t) font->ascent) - ((int32_t) font->descent));
}
static uint16_t font_fontcmap_lookup(struct font_fontFont* font, uint32_t codepoint){
	struct std_stdResult__cgs__39 cmap_result = (*({ struct std_stdResult__cgs__39 __chx__lv__26; font_fontfind_table(&__chx__lv__26, font, 1668112752); &__chx__lv__26; }));
	_Bool __chx__lv__27 = true;
	if((cmap_result.__chx__vt_621827 == 1)){
		const int __chx__lv__28 = 0;
		if(__chx__lv__27) {
			std_stdResult__cgs__39delete(&cmap_result);
		}
		return __chx__lv__28;
	}
	struct std_stdResult__cgs__39* __chx__lv__29 = &cmap_result;
	size_t base = ((size_t) __chx__lv__29->Ok.value.offset);
	if(((base + 4) > std_stdvector__cgs__3size(&font->data))){
		const int __chx__lv__30 = 0;
		if(__chx__lv__27) {
			std_stdResult__cgs__39delete(&cmap_result);
		}
		return __chx__lv__30;
	}
	uint16_t version = font_fontread_u16_be(std_stdvector__cgs__3data(&font->data), base);
	uint16_t num_subtables = font_fontread_u16_be(std_stdvector__cgs__3data(&font->data), (base + 2));
	uint16_t best_platform = 0;
	uint16_t best_encoding = 0;
	uint32_t best_offset = 0;
	uint16_t i = 0;
	while((i < num_subtables)) {
		unsigned long sub_offset = ((base + 4) + (((size_t) i) * 8));
		if(((sub_offset + 8) > std_stdvector__cgs__3size(&font->data))){
			break;
		}
		uint16_t platform = font_fontread_u16_be(std_stdvector__cgs__3data(&font->data), sub_offset);
		uint16_t encoding = font_fontread_u16_be(std_stdvector__cgs__3data(&font->data), (sub_offset + 2));
		uint32_t subtable_offset = font_fontread_u32_be(std_stdvector__cgs__3data(&font->data), (sub_offset + 4));
		if(((platform == 3) && (encoding == 1))){
			best_platform = platform;
			best_encoding = encoding;
			best_offset = subtable_offset;
			break;
		}
		if(((platform == 0) && (best_platform == 0))){
			best_platform = platform;
			best_encoding = encoding;
			best_offset = subtable_offset;
		}
		i += 1;
	}
	if((best_offset == 0)){
		const int __chx__lv__31 = 0;
		if(__chx__lv__27) {
			std_stdResult__cgs__39delete(&cmap_result);
		}
		return __chx__lv__31;
	}
	unsigned long st_base = (base + ((size_t) best_offset));
	if(((st_base + 2) > std_stdvector__cgs__3size(&font->data))){
		const int __chx__lv__32 = 0;
		if(__chx__lv__27) {
			std_stdResult__cgs__39delete(&cmap_result);
		}
		return __chx__lv__32;
	}
	uint16_t format = font_fontread_u16_be(std_stdvector__cgs__3data(&font->data), st_base);
	if((format == 0)){
		if((((st_base + 256) + 2) > std_stdvector__cgs__3size(&font->data))){
			const int __chx__lv__33 = 0;
			if(__chx__lv__27) {
				std_stdResult__cgs__39delete(&cmap_result);
			}
			return __chx__lv__33;
		}
		if((codepoint < 256)){
			const uint16_t __chx__lv__34 = font_fontread_u16_be(std_stdvector__cgs__3data(&font->data), ((st_base + 6) + ((size_t) codepoint)));
			if(__chx__lv__27) {
				std_stdResult__cgs__39delete(&cmap_result);
			}
			return __chx__lv__34;
		}
		const int __chx__lv__35 = 0;
		if(__chx__lv__27) {
			std_stdResult__cgs__39delete(&cmap_result);
		}
		return __chx__lv__35;
	}else if((format == 4)){
		const uint16_t __chx__lv__36 = font_fontcmap_format4_lookup(font, st_base, codepoint);
		if(__chx__lv__27) {
			std_stdResult__cgs__39delete(&cmap_result);
		}
		return __chx__lv__36;
	}else if((format == 6)){
		if(((st_base + 6) > std_stdvector__cgs__3size(&font->data))){
			const int __chx__lv__37 = 0;
			if(__chx__lv__27) {
				std_stdResult__cgs__39delete(&cmap_result);
			}
			return __chx__lv__37;
		}
		uint16_t length = font_fontread_u16_be(std_stdvector__cgs__3data(&font->data), (st_base + 2));
		uint16_t first_code = font_fontread_u16_be(std_stdvector__cgs__3data(&font->data), (st_base + 4));
		uint16_t entry_count = font_fontread_u16_be(std_stdvector__cgs__3data(&font->data), (st_base + 6));
		if(((codepoint < first_code) || (codepoint >= (first_code + entry_count)))){
			const int __chx__lv__38 = 0;
			if(__chx__lv__27) {
				std_stdResult__cgs__39delete(&cmap_result);
			}
			return __chx__lv__38;
		}
		const uint16_t __chx__lv__39 = font_fontread_u16_be(std_stdvector__cgs__3data(&font->data), ((st_base + 8) + (((size_t) (codepoint - ((uint32_t) first_code))) * 2)));
		if(__chx__lv__27) {
			std_stdResult__cgs__39delete(&cmap_result);
		}
		return __chx__lv__39;
	}
	const int __chx__lv__40 = 0;
	if(__chx__lv__27) {
		std_stdResult__cgs__39delete(&cmap_result);
	}
	return __chx__lv__40;
}
static uint16_t font_fontcmap_format4_lookup(struct font_fontFont* font, size_t base, uint32_t codepoint){
	if(((base + 8) > std_stdvector__cgs__3size(&font->data))){
		return 0;
	}
	uint16_t length = font_fontread_u16_be(std_stdvector__cgs__3data(&font->data), (base + 2));
	uint16_t seg_count = (font_fontread_u16_be(std_stdvector__cgs__3data(&font->data), (base + 6)) / 2);
	unsigned long search_offset = (base + 14);
	uint16_t i = 0;
	while((i < seg_count)) {
		unsigned long end_offset = (search_offset + (((size_t) i) * 4));
		unsigned long start_offset = ((search_offset + 2) + (((size_t) i) * 4));
		unsigned long id_delta_offset = ((search_offset + (((size_t) seg_count) * 4)) + (((size_t) i) * 2));
		unsigned long id_range_offset = ((search_offset + (((size_t) seg_count) * 6)) + (((size_t) i) * 2));
		if(((id_range_offset + 2) > std_stdvector__cgs__3size(&font->data))){
			break;
		}
		uint16_t end_code = font_fontread_u16_be(std_stdvector__cgs__3data(&font->data), end_offset);
		uint16_t start_code = font_fontread_u16_be(std_stdvector__cgs__3data(&font->data), start_offset);
		int16_t id_delta = font_fontread_i16_be(std_stdvector__cgs__3data(&font->data), id_delta_offset);
		uint16_t id_range_off = font_fontread_u16_be(std_stdvector__cgs__3data(&font->data), id_range_offset);
		if(((codepoint >= ((uint32_t) start_code)) && (codepoint <= ((uint32_t) end_code)))){
			if((id_range_off == 0)){
				return ((uint16_t) (((int32_t) codepoint) + ((int32_t) id_delta)));
			} else {
				unsigned long glyph_offset = ((((size_t) id_range_offset) + ((size_t) id_range_off)) + (((size_t) (codepoint - ((uint32_t) start_code))) * 2));
				if(((glyph_offset + 2) > std_stdvector__cgs__3size(&font->data))){
					return 0;
				}
				uint16_t glyph_id = font_fontread_u16_be(std_stdvector__cgs__3data(&font->data), glyph_offset);
				if((glyph_id != 0)){
					return ((uint16_t) ((((int32_t) glyph_id) + id_delta) & 65535));
				}
				return 0;
			}
		}
		i += 1;
	}
	return 0;
}
void font_fontfont_get_glyph_metrics(struct std_stdResult__cgs__40* __chx_struct_ret_param_xx, struct font_fontFont* font, uint16_t glyph_id){
	struct std_stdResult__cgs__39 hmtx_result = (*({ struct std_stdResult__cgs__39 __chx__lv__41; font_fontfind_table(&__chx__lv__41, font, 1752003704); &__chx__lv__41; }));
	_Bool __chx__lv__42 = true;
	if((hmtx_result.__chx__vt_621827 == 1)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__40) { .__chx__vt_621827 = 1, 
			.Err.error = (struct font_fontFontError) { .__chx__vt_621827 = 1, 
				.InvalidFormat.msg = (*({ struct std_stdstring __chx__lv__43; std_stdstringconstructor2(&__chx__lv__43, "hmtx table not found", 20, 0); &__chx__lv__43; }))
			}
		};
		if(__chx__lv__42) {
			std_stdResult__cgs__39delete(&hmtx_result);
		}
		return;
	}
	struct std_stdResult__cgs__39* __chx__lv__44 = &hmtx_result;
	size_t base = ((size_t) __chx__lv__44->Ok.value.offset);
	uint16_t num_lsb = 0;
	struct std_stdResult__cgs__39 hhea_result = (*({ struct std_stdResult__cgs__39 __chx__lv__45; font_fontfind_table(&__chx__lv__45, font, 1751672161); &__chx__lv__45; }));
	_Bool __chx__lv__46 = true;
	if((hhea_result.__chx__vt_621827 == 0)){
		struct std_stdResult__cgs__39* __chx__lv__47 = &hhea_result;
		if(((((size_t) __chx__lv__47->Ok.value.offset) + 34) <= std_stdvector__cgs__3size(&font->data))){
			num_lsb = font_fontread_u16_be(std_stdvector__cgs__3data(&font->data), (((size_t) __chx__lv__47->Ok.value.offset) + 34));
		}
	}
	struct font_fontGlyphMetrics metrics = (struct font_fontGlyphMetrics){ 
		.advance_width = 0, 
		.left_side_bearing = 0, 
		.xmin = 0, 
		.ymin = 0, 
		.xmax = 0, 
		.ymax = 0, 
		.width = 0, 
		.height = 0
	};
	if((((uint32_t) glyph_id) < ((uint32_t) num_lsb))){
		unsigned long entry_offset = (base + (((size_t) glyph_id) * 4));
		if(((entry_offset + 4) <= std_stdvector__cgs__3size(&font->data))){
			metrics.advance_width = ((int32_t) font_fontread_u16_be(std_stdvector__cgs__3data(&font->data), entry_offset));
			metrics.left_side_bearing = font_fontread_i16_be(std_stdvector__cgs__3data(&font->data), (entry_offset + 2));
		}
	} else {
		unsigned long last_offset = (base + ((((size_t) num_lsb) - 1) * 4));
		if(((last_offset + 4) <= std_stdvector__cgs__3size(&font->data))){
			metrics.advance_width = ((int32_t) font_fontread_u16_be(std_stdvector__cgs__3data(&font->data), last_offset));
		}
		unsigned long short_offset = ((base + (((size_t) num_lsb) * 4)) + ((((size_t) glyph_id) - ((size_t) num_lsb)) * 2));
		if(((short_offset + 2) <= std_stdvector__cgs__3size(&font->data))){
			metrics.left_side_bearing = font_fontread_i16_be(std_stdvector__cgs__3data(&font->data), short_offset);
		}
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__40) { .__chx__vt_621827 = 0, 
		.Ok.value = metrics
	};
	if(__chx__lv__46) {
		std_stdResult__cgs__39delete(&hhea_result);
	}
	if(__chx__lv__42) {
		std_stdResult__cgs__39delete(&hmtx_result);
	}
	return;
}
void font_fontfont_get_glyph_outline(struct std_stdResult__cgs__41* __chx_struct_ret_param_xx, struct font_fontFont* font, uint16_t glyph_id){
	struct std_stdResult__cgs__39 glyf_result = (*({ struct std_stdResult__cgs__39 __chx__lv__48; font_fontfind_table(&__chx__lv__48, font, 1735162214); &__chx__lv__48; }));
	_Bool __chx__lv__49 = true;
	if((glyf_result.__chx__vt_621827 == 1)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__41) { .__chx__vt_621827 = 1, 
			.Err.error = (struct font_fontFontError) { .__chx__vt_621827 = 2, 
				.UnsupportedFeature.msg = (*({ struct std_stdstring __chx__lv__50; std_stdstringconstructor2(&__chx__lv__50, "glyf table not found", 20, 0); &__chx__lv__50; }))
			}
		};
		if(__chx__lv__49) {
			std_stdResult__cgs__39delete(&glyf_result);
		}
		return;
	}
	struct std_stdResult__cgs__39* __chx__lv__51 = &glyf_result;
	size_t base = ((size_t) __chx__lv__51->Ok.value.offset);
	struct std_stdResult__cgs__39 loca_result = (*({ struct std_stdResult__cgs__39 __chx__lv__52; font_fontfind_table(&__chx__lv__52, font, 1819239265); &__chx__lv__52; }));
	_Bool __chx__lv__53 = true;
	if((loca_result.__chx__vt_621827 == 1)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__41) { .__chx__vt_621827 = 1, 
			.Err.error = (struct font_fontFontError) { .__chx__vt_621827 = 1, 
				.InvalidFormat.msg = (*({ struct std_stdstring __chx__lv__54; std_stdstringconstructor2(&__chx__lv__54, "loca table not found", 20, 0); &__chx__lv__54; }))
			}
		};
		if(__chx__lv__53) {
			std_stdResult__cgs__39delete(&loca_result);
		}
		if(__chx__lv__49) {
			std_stdResult__cgs__39delete(&glyf_result);
		}
		return;
	}
	struct std_stdResult__cgs__39* __chx__lv__55 = &loca_result;
	size_t loca_base = ((size_t) __chx__lv__55->Ok.value.offset);
	_Bool is_short = 1;
	struct std_stdResult__cgs__39 head_result = (*({ struct std_stdResult__cgs__39 __chx__lv__56; font_fontfind_table(&__chx__lv__56, font, 1751474532); &__chx__lv__56; }));
	_Bool __chx__lv__57 = true;
	if((head_result.__chx__vt_621827 == 0)){
		struct std_stdResult__cgs__39* __chx__lv__58 = &head_result;
		if(((((size_t) __chx__lv__58->Ok.value.offset) + 50) <= std_stdvector__cgs__3size(&font->data))){
			int32_t index_to_loc_format = font_fontread_i32_be(std_stdvector__cgs__3data(&font->data), (((size_t) __chx__lv__58->Ok.value.offset) + 50));
			is_short = (index_to_loc_format == 0);
		}
	}
	uint32_t glyph_offset = 0;
	if(is_short){
		unsigned long loc = (loca_base + (((size_t) glyph_id) * 2));
		if(((loc + 4) <= std_stdvector__cgs__3size(&font->data))){
			glyph_offset = (((uint32_t) font_fontread_u16_be(std_stdvector__cgs__3data(&font->data), loc)) * 2);
		}
	} else {
		unsigned long loc = (loca_base + (((size_t) glyph_id) * 4));
		if(((loc + 4) <= std_stdvector__cgs__3size(&font->data))){
			glyph_offset = font_fontread_u32_be(std_stdvector__cgs__3data(&font->data), loc);
		}
	}
	unsigned long glyf_offset = (base + ((size_t) glyph_offset));
	if(((glyf_offset + 10) > std_stdvector__cgs__3size(&font->data))){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__41) { .__chx__vt_621827 = 1, 
			.Err.error = (struct font_fontFontError) { .__chx__vt_621827 = 4, 
				.GlyphNotFound.codepoint = ((uint32_t) glyph_id)
			}
		};
		if(__chx__lv__57) {
			std_stdResult__cgs__39delete(&head_result);
		}
		if(__chx__lv__53) {
			std_stdResult__cgs__39delete(&loca_result);
		}
		if(__chx__lv__49) {
			std_stdResult__cgs__39delete(&glyf_result);
		}
		return;
	}
	int16_t num_contours = font_fontread_i16_be(std_stdvector__cgs__3data(&font->data), glyf_offset);
	int16_t xmin = font_fontread_i16_be(std_stdvector__cgs__3data(&font->data), (glyf_offset + 2));
	int16_t ymin = font_fontread_i16_be(std_stdvector__cgs__3data(&font->data), (glyf_offset + 4));
	int16_t xmax = font_fontread_i16_be(std_stdvector__cgs__3data(&font->data), (glyf_offset + 6));
	int16_t ymax = font_fontread_i16_be(std_stdvector__cgs__3data(&font->data), (glyf_offset + 8));
	struct font_fontGlyphOutline outline = (struct font_fontGlyphOutline){ 
		.points = ({ struct std_stdvector__cgs__5 __chx__lv__59; std_stdvector__cgs__5make(&__chx__lv__59); __chx__lv__59; }), 
		.flags = ({ struct std_stdvector__cgs__3 __chx__lv__60; std_stdvector__cgs__3make(&__chx__lv__60); __chx__lv__60; }), 
		.instructions = ({ struct std_stdvector__cgs__3 __chx__lv__61; std_stdvector__cgs__3make(&__chx__lv__61); __chx__lv__61; })
	};
	_Bool __chx__lv__62 = true;
	if((num_contours == 0)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__41) { .__chx__vt_621827 = 0, 
			.Ok.value = ({ __chx__lv__62 = false; outline; })
		};
		if(__chx__lv__62) {
			font_fontGlyphOutlinedelete(&outline);
		}
		if(__chx__lv__57) {
			std_stdResult__cgs__39delete(&head_result);
		}
		if(__chx__lv__53) {
			std_stdResult__cgs__39delete(&loca_result);
		}
		if(__chx__lv__49) {
			std_stdResult__cgs__39delete(&glyf_result);
		}
		return;
	}else if((num_contours > 0)){
		unsigned long contour_end_offset = ((glyf_offset + 10) + (((size_t) num_contours) * 2));
		unsigned long instructions_offset = (contour_end_offset + 2);
		uint16_t num_instructions = font_fontread_u16_be(std_stdvector__cgs__3data(&font->data), contour_end_offset);
		unsigned long points_offset = (instructions_offset + ((size_t) num_instructions));
		uint16_t num_points = 0;
		int16_t i = 0;
		while((i < num_contours)) {
			uint16_t end = font_fontread_u16_be(std_stdvector__cgs__3data(&font->data), ((glyf_offset + 10) + (((size_t) i) * 2)));
			if((end >= num_points)){
				num_points = (end + 1);
			}
			i += 1;
		}
		unsigned long pos = points_offset;
		uint16_t j = 0;
		while(((j < num_points) && (pos < std_stdvector__cgs__3size(&font->data)))) {
			uint8_t flags = std_stdvector__cgs__3data(&font->data)[pos];
			pos += 1;
			std_stdvector__cgs__3push(&outline.flags, flags);
			if((flags & (2 != 0))){
				if((pos < std_stdvector__cgs__3size(&font->data))){
					int32_t dx = ((int32_t) std_stdvector__cgs__3data(&font->data)[pos]);
					if((flags & (16 == 0))){
						dx = -dx;
					}
					int32_t prev_x = 0;
					if((std_stdvector__cgs__5size(&outline.points) > 0)){
						prev_x = std_stdvector__cgs__5get(&outline.points, (std_stdvector__cgs__5size(&outline.points) - 2));
					}
					std_stdvector__cgs__5push(&outline.points, (prev_x + dx));
					pos += 1;
				}
			} else {
				if((flags & (16 != 0))){
					int32_t prev_x = 0;
					if((std_stdvector__cgs__5size(&outline.points) > 0)){
						prev_x = std_stdvector__cgs__5get(&outline.points, (std_stdvector__cgs__5size(&outline.points) - 2));
					}
					std_stdvector__cgs__5push(&outline.points, prev_x);
				} else {
					if(((pos + 2) <= std_stdvector__cgs__3size(&font->data))){
						int32_t dx = ((int32_t) font_fontread_i16_be(std_stdvector__cgs__3data(&font->data), pos));
						int32_t prev_x = 0;
						if((std_stdvector__cgs__5size(&outline.points) > 0)){
							prev_x = std_stdvector__cgs__5get(&outline.points, (std_stdvector__cgs__5size(&outline.points) - 2));
						}
						std_stdvector__cgs__5push(&outline.points, (prev_x + dx));
						pos += 2;
					}
				}
			}
			if((flags & (4 != 0))){
				if((pos < std_stdvector__cgs__3size(&font->data))){
					int32_t dy = ((int32_t) std_stdvector__cgs__3data(&font->data)[pos]);
					if((flags & (32 == 0))){
						dy = -dy;
					}
					int32_t prev_y = 0;
					if((std_stdvector__cgs__5size(&outline.points) > 0)){
						prev_y = std_stdvector__cgs__5get(&outline.points, (std_stdvector__cgs__5size(&outline.points) - 1));
					}
					std_stdvector__cgs__5push(&outline.points, (prev_y + dy));
					pos += 1;
				}
			} else {
				if((flags & (32 != 0))){
					int32_t prev_y = 0;
					if((std_stdvector__cgs__5size(&outline.points) > 0)){
						prev_y = std_stdvector__cgs__5get(&outline.points, (std_stdvector__cgs__5size(&outline.points) - 1));
					}
					std_stdvector__cgs__5push(&outline.points, prev_y);
				} else {
					if(((pos + 2) <= std_stdvector__cgs__3size(&font->data))){
						int32_t dy = ((int32_t) font_fontread_i16_be(std_stdvector__cgs__3data(&font->data), pos));
						int32_t prev_y = 0;
						if((std_stdvector__cgs__5size(&outline.points) > 0)){
							prev_y = std_stdvector__cgs__5get(&outline.points, (std_stdvector__cgs__5size(&outline.points) - 1));
						}
						std_stdvector__cgs__5push(&outline.points, (prev_y + dy));
						pos += 2;
					}
				}
			}
			j += 1;
		}
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__41) { .__chx__vt_621827 = 0, 
			.Ok.value = ({ __chx__lv__62 = false; outline; })
		};
		if(__chx__lv__62) {
			font_fontGlyphOutlinedelete(&outline);
		}
		if(__chx__lv__57) {
			std_stdResult__cgs__39delete(&head_result);
		}
		if(__chx__lv__53) {
			std_stdResult__cgs__39delete(&loca_result);
		}
		if(__chx__lv__49) {
			std_stdResult__cgs__39delete(&glyf_result);
		}
		return;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__41) { .__chx__vt_621827 = 1, 
		.Err.error = (struct font_fontFontError) { .__chx__vt_621827 = 2, 
			.UnsupportedFeature.msg = (*({ struct std_stdstring __chx__lv__63; std_stdstringconstructor2(&__chx__lv__63, "composite glyphs not yet supported", 34, 0); &__chx__lv__63; }))
		}
	};
	if(__chx__lv__62) {
		font_fontGlyphOutlinedelete(&outline);
	}
	if(__chx__lv__57) {
		std_stdResult__cgs__39delete(&head_result);
	}
	if(__chx__lv__53) {
		std_stdResult__cgs__39delete(&loca_result);
	}
	if(__chx__lv__49) {
		std_stdResult__cgs__39delete(&glyf_result);
	}
	return;
}
void font_fontfont_get_glyph(struct std_stdResult__cgs__42* __chx_struct_ret_param_xx, struct font_fontFont* font, uint32_t codepoint){
	uint16_t glyph_id = font_fontcmap_lookup(font, codepoint);
	if(((glyph_id == 0) && (codepoint != 0))){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__42) { .__chx__vt_621827 = 1, 
			.Err.error = (struct font_fontFontError) { .__chx__vt_621827 = 4, 
				.GlyphNotFound.codepoint = codepoint
			}
		};
		return;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__42) { .__chx__vt_621827 = 0, 
		.Ok.value = glyph_id
	};
	return;
}
void font_fontfont_measure_text(struct font_fontTextMetrics* __chx_struct_ret_param_xx, struct font_fontFont* font, const char* text, size_t text_len, float font_size){
	float scale = (font_size / ((float) font->units_per_em));
	int32_t total_width = 0;
	int32_t max_height = 0;
	size_t i = 0;
	while((i < text_len)) {
		uint32_t cp = ((uint32_t) text[i]);
		struct std_stdResult__cgs__42 glyph_result = (*({ struct std_stdResult__cgs__42 __chx__lv__64; font_fontfont_get_glyph(&__chx__lv__64, font, cp); &__chx__lv__64; }));
		_Bool __chx__lv__65 = true;
		if((glyph_result.__chx__vt_621827 == 0)){
			struct std_stdResult__cgs__42* __chx__lv__66 = &glyph_result;
			struct std_stdResult__cgs__40 metrics_result = (*({ struct std_stdResult__cgs__40 __chx__lv__67; font_fontfont_get_glyph_metrics(&__chx__lv__67, font, __chx__lv__66->Ok.value); &__chx__lv__67; }));
			_Bool __chx__lv__68 = true;
			if((metrics_result.__chx__vt_621827 == 0)){
				struct std_stdResult__cgs__40* __chx__lv__69 = &metrics_result;
				total_width += ((int32_t) (((float) __chx__lv__69->Ok.value.advance_width) * scale));
				int32_t glyph_height = (__chx__lv__69->Ok.value.ymax - __chx__lv__69->Ok.value.ymin);
				if((glyph_height > max_height)){
					max_height = glyph_height;
				}
			}
			if(__chx__lv__68) {
				std_stdResult__cgs__40delete(&metrics_result);
			}
		}
		i += 1;
		if(__chx__lv__65) {
			std_stdResult__cgs__42delete(&glyph_result);
		}
	}
	*__chx_struct_ret_param_xx = (struct font_fontTextMetrics){ 
		.width = total_width, 
		.height = ((int32_t) (((float) max_height) * scale)), 
		.ascent = ((int32_t) (((float) font->ascent) * scale)), 
		.descent = ((int32_t) (((float) -font->descent) * scale))
	};
	return;
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/font/src/types.ch **/
void font_fontFontErrormessage(struct std_stdstring* __chx_struct_ret_param_xx, struct font_fontFontError*const self){
	switch((self)->__chx__vt_621827) {
		case 0:{
			*__chx_struct_ret_param_xx = (*({ struct std_stdstring __chx__lv__70; std_stdstringconstructor2(&__chx__lv__70, "FontError: file not found", 25, 0); &__chx__lv__70; }));
			return;
			break;
		}
		case 1:{
			struct std_stdstring s = (*({ struct std_stdstring __chx__lv__71; std_stdstringconstructor2(&__chx__lv__71, "FontError: ", 11, 0); &__chx__lv__71; }));
			_Bool __chx__lv__72 = true;
			std_stdstringappend_string(&s, &(self)->InvalidFormat.msg);
			*__chx_struct_ret_param_xx = s;
			return;
			break;
		}
		case 2:{
			struct std_stdstring s = (*({ struct std_stdstring __chx__lv__73; std_stdstringconstructor2(&__chx__lv__73, "FontError: unsupported: ", 24, 0); &__chx__lv__73; }));
			_Bool __chx__lv__74 = true;
			std_stdstringappend_string(&s, &(self)->UnsupportedFeature.msg);
			*__chx_struct_ret_param_xx = s;
			return;
			break;
		}
		case 3:{
			struct std_stdstring s = (*({ struct std_stdstring __chx__lv__75; std_stdstringconstructor2(&__chx__lv__75, "FontError: IO error: ", 21, 0); &__chx__lv__75; }));
			_Bool __chx__lv__76 = true;
			std_stdstringappend_string(&s, &(self)->IoError.msg);
			*__chx_struct_ret_param_xx = s;
			return;
			break;
		}
		case 4:{
			*__chx_struct_ret_param_xx = (*({ struct std_stdstring __chx__lv__77; std_stdstringconstructor2(&__chx__lv__77, "FontError: glyph not found", 26, 0); &__chx__lv__77; }));
			return;
			break;
		}
	}
}
void font_fontFontErrordelete(struct font_fontFontError* self){
	__chx__dstctr_clnup_blk__:{
		switch(self->__chx__vt_621827) {
			case 0:
			break;
			case 1:
			std_stdstringdelete(&self->InvalidFormat.msg);
			break;
			case 2:
			std_stdstringdelete(&self->UnsupportedFeature.msg);
			break;
			case 3:
			std_stdstringdelete(&self->IoError.msg);
			break;
			case 4:
			break;
		}
	}
}
void font_fontGlyphOutlinemake(struct font_fontGlyphOutline* this){
	*this = (struct font_fontGlyphOutline){ 
		.points = (*({ struct std_stdvector__cgs__5 __chx__lv__78; std_stdvector__cgs__5make(&__chx__lv__78); &__chx__lv__78; })), 
		.flags = (*({ struct std_stdvector__cgs__3 __chx__lv__79; std_stdvector__cgs__3make(&__chx__lv__79); &__chx__lv__79; })), 
		.instructions = (*({ struct std_stdvector__cgs__3 __chx__lv__80; std_stdvector__cgs__3make(&__chx__lv__80); &__chx__lv__80; }))
	};
	return;
}
void font_fontGlyphOutlinedelete(struct font_fontGlyphOutline* self){
	__chx__dstctr_clnup_blk__:{
		std_stdvector__cgs__5delete(&self->points);
		std_stdvector__cgs__3delete(&self->flags);
		std_stdvector__cgs__3delete(&self->instructions);
	}
}