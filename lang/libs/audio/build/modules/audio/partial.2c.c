
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/audio/src/wav.ch **/
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/audio/src/types.ch **/
struct audio_audioAudioError;
struct audio_audioAudio;
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/audio/src/wav.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/audio/src/types.ch **/
/** FwdDeclare:Generics audio **/
struct std_stdResult__cgs__37;
struct std_stdvector__cgs__4;
struct std_stdResult__cgs__38;
/** Declare:Generics audio **/
struct std_stdvector__cgs__4 {
	int16_t* data_ptr;
	size_t data_size;
	size_t data_cap;
};
struct audio_audioAudio {
	struct std_stdvector__cgs__4 samples;
	uint32_t sample_rate;
	uint16_t channels;
	uint16_t bits_per_sample;
	size_t num_samples;
	_Bool loaded;
};
struct audio_audioAudioError {
	int __chx__vt_621827;
	union {
		struct {
		} FileNotFound;
		struct {
			struct std_stdstring msg;
		} InvalidFormat;
		struct {
			struct std_stdstring msg;
		} UnsupportedFormat;
		struct {
			struct std_stdstring msg;
		} IoError;
		struct {
		} BufferTooSmall;
	};
};
struct std_stdResult__cgs__37 {
	int __chx__vt_621827;
	union {
		struct {
			struct audio_audioAudio value;
		} Ok;
		struct {
			struct audio_audioAudioError error;
		} Err;
	};
};
void std_stdResult__cgs__37delete(struct std_stdResult__cgs__37* self);
void std_stdvector__cgs__4make(struct std_stdvector__cgs__4* this);
void std_stdvector__cgs__4make_with_capacity(struct std_stdvector__cgs__4* this, size_t init_cap);
void std_stdvector__cgs__4resize(struct std_stdvector__cgs__4* self, size_t new_size);
void std_stdvector__cgs__4reserve(struct std_stdvector__cgs__4* self, size_t new_cap);
void std_stdvector__cgs__4ensure_capacity_for_one_more(struct std_stdvector__cgs__4* self);
void std_stdvector__cgs__4push(struct std_stdvector__cgs__4* self, int16_t value);
void std_stdvector__cgs__4push_back(struct std_stdvector__cgs__4* self, int16_t value);
int16_t std_stdvector__cgs__4get(struct std_stdvector__cgs__4*const self, size_t index);
int16_t* std_stdvector__cgs__4get_ptr(struct std_stdvector__cgs__4*const self, size_t index);
int16_t* std_stdvector__cgs__4get_ref(struct std_stdvector__cgs__4*const self, size_t index);
void std_stdvector__cgs__4set(struct std_stdvector__cgs__4* self, size_t index, int16_t value);
size_t std_stdvector__cgs__4size(struct std_stdvector__cgs__4*const self);
size_t std_stdvector__cgs__4capacity(struct std_stdvector__cgs__4*const self);
const int16_t* std_stdvector__cgs__4data(struct std_stdvector__cgs__4*const self);
int16_t* std_stdvector__cgs__4last_ptr(struct std_stdvector__cgs__4*const self);
void std_stdvector__cgs__4remove(struct std_stdvector__cgs__4* self, size_t index);
void std_stdvector__cgs__4erase(struct std_stdvector__cgs__4* self, size_t index);
void std_stdvector__cgs__4remove_last(struct std_stdvector__cgs__4* self);
void std_stdvector__cgs__4pop_back(struct std_stdvector__cgs__4* self);
int16_t std_stdvector__cgs__4take_last(struct std_stdvector__cgs__4* self);
_Bool std_stdvector__cgs__4empty(struct std_stdvector__cgs__4*const self);
void std_stdvector__cgs__4clear(struct std_stdvector__cgs__4* self);
void std_stdvector__cgs__4set_len(struct std_stdvector__cgs__4* self, size_t new_size);
void std_stdvector__cgs__4delete(struct std_stdvector__cgs__4* self);
struct std_stdResult__cgs__38 {
	int __chx__vt_621827;
	union {
		struct {
			struct std_stdUnit value;
		} Ok;
		struct {
			struct audio_audioAudioError error;
		} Err;
	};
};
void std_stdResult__cgs__38delete(struct std_stdResult__cgs__38* self);
typedef struct __chx_core_coreiterableLinear__cgs__4_vt_t {
	const int16_t*(*data)(void* self);uint64_t(*size)(void* self);
} __chx_core_coreiterableLinear__cgs__4_vt_t;
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/audio/src/wav.ch **/
static uint16_t audio_audioread_u16_le(const uint8_t* data, size_t offset);
static uint32_t audio_audioread_u32_le(const uint8_t* data, size_t offset);
static void audio_audiowrite_u16_le(uint8_t* out, size_t offset, uint16_t val);
static void audio_audiowrite_u32_le(uint8_t* out, size_t offset, uint32_t val);
void audio_audioload_wav(struct std_stdResult__cgs__37* __chx_struct_ret_param_xx, const char* path);
void audio_audioparse_wav(struct std_stdResult__cgs__37* __chx_struct_ret_param_xx, const uint8_t* data, size_t data_len);
void audio_audiosave_wav(struct std_stdResult__cgs__38* __chx_struct_ret_param_xx, struct audio_audioAudio* audio, const char* path);
double audio_audioaudio_duration_ms(struct audio_audioAudio* audio);
void audio_audioaudio_trim(struct audio_audioAudio* __chx_struct_ret_param_xx, struct audio_audioAudio* audio, double start_ms, double end_ms);
void audio_audioaudio_volume(struct audio_audioAudio* audio, float factor);
void audio_audioaudio_mix(struct std_stdResult__cgs__37* __chx_struct_ret_param_xx, struct audio_audioAudio* a, struct audio_audioAudio* b);
void audio_audioaudio_append(struct std_stdResult__cgs__37* __chx_struct_ret_param_xx, struct audio_audioAudio* a, struct audio_audioAudio* b);
void audio_audioaudio_resample(struct audio_audioAudio* __chx_struct_ret_param_xx, struct audio_audioAudio* audio, uint32_t target_rate);
void audio_audioaudio_copy(struct audio_audioAudio* __chx_struct_ret_param_xx, struct audio_audioAudio* audio);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/audio/src/types.ch **/
void audio_audioAudioErrormessage(struct std_stdstring* __chx_struct_ret_param_xx, struct audio_audioAudioError*const self);
void audio_audioAudioErrordelete(struct audio_audioAudioError* self);
void audio_audioAudiomake(struct audio_audioAudio* this);
void audio_audioAudiodelete(struct audio_audioAudio* self);
/** Implement:Generics audio **/
void std_stdResult__cgs__37delete(struct std_stdResult__cgs__37* self){
	__chx__dstctr_clnup_blk__:{
		switch(self->__chx__vt_621827) {
			case 0:
			audio_audioAudiodelete(&self->Ok.value);
			break;
			case 1:
			audio_audioAudioErrordelete(&self->Err.error);
			break;
		}
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
		.data_ptr = ((int16_t*) malloc((sizeof(int16_t) * init_cap))), 
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
		int16_t* start = (self->data_ptr + new_size);
		
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
		self->data_ptr[i] = ((int16_t){0});
		i = (i + 1);
	}
	self->data_size = new_size;
}
void std_stdvector__cgs__4reserve(struct std_stdvector__cgs__4* self, size_t new_cap){
	if((new_cap <= self->data_cap)){
		return;
	}
	int16_t* new_data = ((int16_t*) realloc(self->data_ptr, (sizeof(int16_t) * new_cap)));
	
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
void std_stdvector__cgs__4push(struct std_stdvector__cgs__4* self, int16_t value){
	size_t s = self->data_size;
	if((s >= self->data_cap)){
		std_stdvector__cgs__4ensure_capacity_for_one_more(self);
	}
	memcpy(&self->data_ptr[s], &value, sizeof(int16_t));
	0;
	self->data_size = (s + 1);
}
void std_stdvector__cgs__4push_back(struct std_stdvector__cgs__4* self, int16_t value){
	std_stdvector__cgs__4push(self, value);
}
int16_t std_stdvector__cgs__4get(struct std_stdvector__cgs__4*const self, size_t index){
	return self->data_ptr[index];
}
int16_t* std_stdvector__cgs__4get_ptr(struct std_stdvector__cgs__4*const self, size_t index){
	return &self->data_ptr[index];
}
int16_t* std_stdvector__cgs__4get_ref(struct std_stdvector__cgs__4*const self, size_t index){
	return &self->data_ptr[index];
}
void std_stdvector__cgs__4set(struct std_stdvector__cgs__4* self, size_t index, int16_t value){
	self->data_ptr[index] = ({  value; });
}
size_t std_stdvector__cgs__4size(struct std_stdvector__cgs__4*const self){
	return self->data_size;
}
size_t std_stdvector__cgs__4capacity(struct std_stdvector__cgs__4*const self){
	return self->data_cap;
}
const int16_t* std_stdvector__cgs__4data(struct std_stdvector__cgs__4*const self){
	return self->data_ptr;
}
int16_t* std_stdvector__cgs__4last_ptr(struct std_stdvector__cgs__4*const self){
	return (self->data_ptr + (self->data_size - 1));
}
void std_stdvector__cgs__4remove(struct std_stdvector__cgs__4* self, size_t index){
	size_t s = self->data_size;
	unsigned long last = (s - 1);
	
	if((index == last)){
		self->data_size = last;
	} else {
		memmove(&self->data_ptr[index], &self->data_ptr[(index + 1)], (sizeof(int16_t) * (last - index)));
		self->data_size = last;
	}
}
void std_stdvector__cgs__4erase(struct std_stdvector__cgs__4* self, size_t index){
	std_stdvector__cgs__4remove(self, index);
}
void std_stdvector__cgs__4remove_last(struct std_stdvector__cgs__4* self){
	size_t s = self->data_size;
	unsigned long last = (s - 1);
	int16_t* ptr = std_stdvector__cgs__4get_ptr(self, last);
	
	self->data_size = last;
}
void std_stdvector__cgs__4pop_back(struct std_stdvector__cgs__4* self){
	std_stdvector__cgs__4remove_last(self);
}
int16_t std_stdvector__cgs__4take_last(struct std_stdvector__cgs__4* self){
	unsigned long last = (self->data_size - 1);
	self->data_size = last;
	
	return *std_stdvector__cgs__4get_ptr(self, last);
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
const int16_t* core_coreiterableLinear__cgs__4_vector__cgs__4_data(struct std_stdvector__cgs__4*const self){
	return self->data_ptr;
}
size_t core_coreiterableLinear__cgs__4_vector__cgs__4_size(struct std_stdvector__cgs__4*const self){
	return self->data_size;
}
const __chx_core_coreiterableLinear__cgs__4_vt_t core_coreiterableLinear__cgs__4std_stdvector__cgs__4 = {
	(const int16_t*(*)(void* self)) core_coreiterableLinear__cgs__4_vector__cgs__4_data,
	(uint64_t(*)(void* self)) core_coreiterableLinear__cgs__4_vector__cgs__4_size,
};
void std_stdResult__cgs__38delete(struct std_stdResult__cgs__38* self){
	__chx__dstctr_clnup_blk__:{
		switch(self->__chx__vt_621827) {
			case 0:
			break;
			case 1:
			audio_audioAudioErrordelete(&self->Err.error);
			break;
		}
	}
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/audio/src/wav.ch **/
static uint16_t audio_audioread_u16_le(const uint8_t* data, size_t offset){
	return (((uint16_t) data[offset]) | (((uint16_t) data[(offset + 1)]) << 8));
}
static uint32_t audio_audioread_u32_le(const uint8_t* data, size_t offset){
	return (((((uint32_t) data[offset]) | (((uint32_t) data[(offset + 1)]) << 8)) | (((uint32_t) data[(offset + 2)]) << 16)) | (((uint32_t) data[(offset + 3)]) << 24));
}
static void audio_audiowrite_u16_le(uint8_t* out, size_t offset, uint16_t val){
	out[offset] = ((uint8_t) (val & 255));
	out[(offset + 1)] = ((uint8_t) ((val >> 8) & 255));
}
static void audio_audiowrite_u32_le(uint8_t* out, size_t offset, uint32_t val){
	out[offset] = ((uint8_t) (val & 255));
	out[(offset + 1)] = ((uint8_t) ((val >> 8) & 255));
	out[(offset + 2)] = ((uint8_t) ((val >> 16) & 255));
	out[(offset + 3)] = ((uint8_t) ((val >> 24) & 255));
}
void audio_audioload_wav(struct std_stdResult__cgs__37* __chx_struct_ret_param_xx, const char* path){
	struct std_stdResult__cgs__35 read_result = (*({ struct std_stdResult__cgs__35 __chx__lv__0; fs_fsread_entire_file(&__chx__lv__0, path); &__chx__lv__0; }));
	_Bool __chx__lv__1 = true;
	if((read_result.__chx__vt_621827 == 1)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__37) { .__chx__vt_621827 = 1, 
			.Err.error = (struct audio_audioAudioError) { .__chx__vt_621827 = 0, 
			}
		};
		if(__chx__lv__1) {
			std_stdResult__cgs__35delete(&read_result);
		}
		return;
	}
	struct std_stdResult__cgs__35* __chx__lv__2 = &read_result;
	*__chx_struct_ret_param_xx = (*({ struct std_stdResult__cgs__37 __chx__lv__3; audio_audioparse_wav(&__chx__lv__3, std_stdvector__cgs__3data(&__chx__lv__2->Ok.value), std_stdvector__cgs__3size(&__chx__lv__2->Ok.value)); &__chx__lv__3; }));
	if(__chx__lv__1) {
		std_stdResult__cgs__35delete(&read_result);
	}
	return;
}
void audio_audioparse_wav(struct std_stdResult__cgs__37* __chx_struct_ret_param_xx, const uint8_t* data, size_t data_len){
	if((data_len < 44)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__37) { .__chx__vt_621827 = 1, 
			.Err.error = (struct audio_audioAudioError) { .__chx__vt_621827 = 1, 
				.InvalidFormat.msg = (*({ struct std_stdstring __chx__lv__4; std_stdstringconstructor2(&__chx__lv__4, "WAV file too small", 18, 0); &__chx__lv__4; }))
			}
		};
		return;
	}
	if(((((data[0] != ((uint8_t) 'R')) || (data[1] != ((uint8_t) 'I'))) || (data[2] != ((uint8_t) 'F'))) || (data[3] != ((uint8_t) 'F')))){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__37) { .__chx__vt_621827 = 1, 
			.Err.error = (struct audio_audioAudioError) { .__chx__vt_621827 = 1, 
				.InvalidFormat.msg = (*({ struct std_stdstring __chx__lv__5; std_stdstringconstructor2(&__chx__lv__5, "not a RIFF file", 15, 0); &__chx__lv__5; }))
			}
		};
		return;
	}
	if(((((data[8] != ((uint8_t) 'W')) || (data[9] != ((uint8_t) 'A'))) || (data[10] != ((uint8_t) 'V'))) || (data[11] != ((uint8_t) 'E')))){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__37) { .__chx__vt_621827 = 1, 
			.Err.error = (struct audio_audioAudioError) { .__chx__vt_621827 = 1, 
				.InvalidFormat.msg = (*({ struct std_stdstring __chx__lv__6; std_stdstringconstructor2(&__chx__lv__6, "not a WAVE file", 15, 0); &__chx__lv__6; }))
			}
		};
		return;
	}
	struct audio_audioAudio audio = (*({ struct audio_audioAudio __chx__lv__7; audio_audioAudiomake(&__chx__lv__7); &__chx__lv__7; }));
	_Bool __chx__lv__8 = true;
	size_t pos = 12;
	while(((pos + 8) <= data_len)) {
		uint32_t chunk_id = audio_audioread_u32_le(data, pos);
		size_t chunk_size = ((size_t) audio_audioread_u32_le(data, (pos + 4)));
		if((chunk_id == 1718449184)){
			if(((chunk_size < 16) || ((pos + 24) > data_len))){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__37) { .__chx__vt_621827 = 1, 
					.Err.error = (struct audio_audioAudioError) { .__chx__vt_621827 = 1, 
						.InvalidFormat.msg = (*({ struct std_stdstring __chx__lv__9; std_stdstringconstructor2(&__chx__lv__9, "fmt chunk too small", 19, 0); &__chx__lv__9; }))
					}
				};
				if(__chx__lv__8) {
					audio_audioAudiodelete(&audio);
				}
				return;
			}
			uint16_t audio_format = audio_audioread_u16_le(data, (pos + 8));
			audio.channels = audio_audioread_u16_le(data, (pos + 10));
			audio.sample_rate = audio_audioread_u32_le(data, (pos + 12));
			uint32_t byte_rate = audio_audioread_u32_le(data, (pos + 16));
			uint16_t block_align = audio_audioread_u16_le(data, (pos + 20));
			audio.bits_per_sample = audio_audioread_u16_le(data, (pos + 22));
			if((audio_format != 1)){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__37) { .__chx__vt_621827 = 1, 
					.Err.error = (struct audio_audioAudioError) { .__chx__vt_621827 = 2, 
						.UnsupportedFormat.msg = (*({ struct std_stdstring __chx__lv__10; std_stdstringconstructor2(&__chx__lv__10, "only PCM WAV supported", 22, 0); &__chx__lv__10; }))
					}
				};
				if(__chx__lv__8) {
					audio_audioAudiodelete(&audio);
				}
				return;
			}
			pos += (8 + chunk_size);
			if(((chunk_size % 2) != 0)){
				pos += 1;
			}
		}else if((chunk_id == 1684108385)){
			size_t data_size = chunk_size;
			size_t bytes_per_sample = ((size_t) (audio.bits_per_sample / 8));
			unsigned long total_samples = (data_size / bytes_per_sample);
			std_stdvector__cgs__4resize(&audio.samples, total_samples);
			audio.num_samples = (total_samples / ((size_t) audio.channels));
			unsigned long src_pos = (pos + 8);
			int16_t* sptr = ((int16_t*) std_stdvector__cgs__4data(&audio.samples));
			size_t i = 0;
			while(((i < total_samples) && ((src_pos + bytes_per_sample) <= data_len))) {
				if((audio.bits_per_sample == 16)){
					int16_t lo = ((int16_t) data[src_pos]);
					int16_t hi = (((int16_t) data[(src_pos + 1)]) << 8);
					sptr[i] = (lo | hi);
				}else if((audio.bits_per_sample == 8)){
					sptr[i] = ((((int16_t) data[src_pos]) - 128) << 8);
				}
				src_pos += bytes_per_sample;
				i += 1;
			}
			audio.loaded = 1;
			break;
		} else {
			pos += (8 + chunk_size);
			if(((chunk_size % 2) != 0)){
				pos += 1;
			}
		}
	}
	if(!audio.loaded){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__37) { .__chx__vt_621827 = 1, 
			.Err.error = (struct audio_audioAudioError) { .__chx__vt_621827 = 1, 
				.InvalidFormat.msg = (*({ struct std_stdstring __chx__lv__11; std_stdstringconstructor2(&__chx__lv__11, "no data chunk found", 19, 0); &__chx__lv__11; }))
			}
		};
		if(__chx__lv__8) {
			audio_audioAudiodelete(&audio);
		}
		return;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__37) { .__chx__vt_621827 = 0, 
		.Ok.value = ({ __chx__lv__8 = false; audio; })
	};
	if(__chx__lv__8) {
		audio_audioAudiodelete(&audio);
	}
	return;
}
void audio_audiosave_wav(struct std_stdResult__cgs__38* __chx_struct_ret_param_xx, struct audio_audioAudio* audio, const char* path){
	uint32_t bytes_per_sample = ((uint32_t) (audio->bits_per_sample / 8));
	uint32_t data_size = (((uint32_t) std_stdvector__cgs__4size(&audio->samples)) * bytes_per_sample);
	uint32_t file_size = (36 + data_size);
	struct std_stdvector__cgs__3 file_data = (*({ struct std_stdvector__cgs__3 __chx__lv__12; std_stdvector__cgs__3make(&__chx__lv__12); &__chx__lv__12; }));
	_Bool __chx__lv__13 = true;
	std_stdvector__cgs__3resize(&file_data, ((size_t) (file_size + 8)));
	uint8_t* fptr = ((uint8_t*) std_stdvector__cgs__3data(&file_data));
	fptr[0] = ((uint8_t) 'R');
	fptr[1] = ((uint8_t) 'I');
	fptr[2] = ((uint8_t) 'F');
	fptr[3] = ((uint8_t) 'F');
	audio_audiowrite_u32_le(fptr, 4, file_size);
	fptr[8] = ((uint8_t) 'W');
	fptr[9] = ((uint8_t) 'A');
	fptr[10] = ((uint8_t) 'V');
	fptr[11] = ((uint8_t) 'E');
	fptr[12] = ((uint8_t) 'f');
	fptr[13] = ((uint8_t) 'm');
	fptr[14] = ((uint8_t) 't');
	fptr[15] = ((uint8_t) ' ');
	audio_audiowrite_u32_le(fptr, 16, 16);
	audio_audiowrite_u16_le(fptr, 20, 1);
	audio_audiowrite_u16_le(fptr, 22, audio->channels);
	audio_audiowrite_u32_le(fptr, 24, audio->sample_rate);
	audio_audiowrite_u32_le(fptr, 28, ((audio->sample_rate * ((uint32_t) audio->channels)) * bytes_per_sample));
	audio_audiowrite_u16_le(fptr, 32, (audio->channels * (audio->bits_per_sample / 8)));
	audio_audiowrite_u16_le(fptr, 34, audio->bits_per_sample);
	fptr[36] = ((uint8_t) 'd');
	fptr[37] = ((uint8_t) 'a');
	fptr[38] = ((uint8_t) 't');
	fptr[39] = ((uint8_t) 'a');
	audio_audiowrite_u32_le(fptr, 40, data_size);
	size_t i = 0;
	while((i < std_stdvector__cgs__4size(&audio->samples))) {
		unsigned long offset = (44 + (i * ((size_t) bytes_per_sample)));
		if(((offset + 1) < std_stdvector__cgs__3size(&file_data))){
			int16_t sample = std_stdvector__cgs__4get(&audio->samples, i);
			if((audio->bits_per_sample == 16)){
				fptr[offset] = ((uint8_t) (sample & 255));
				fptr[(offset + 1)] = ((uint8_t) ((sample >> 8) & 255));
			}else if((audio->bits_per_sample == 8)){
				fptr[offset] = ((uint8_t) ((sample >> 8) + 128));
			}
		}
		i += 1;
	}
	struct std_stdResult__cgs__33 write_result = (*({ struct std_stdResult__cgs__33 __chx__lv__14; fs_fswrite_text_file(&__chx__lv__14, path, std_stdvector__cgs__3data(&file_data), std_stdvector__cgs__3size(&file_data)); &__chx__lv__14; }));
	if((write_result.__chx__vt_621827 == 1)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 1, 
			.Err.error = (struct audio_audioAudioError) { .__chx__vt_621827 = 3, 
				.IoError.msg = (*({ struct std_stdstring __chx__lv__15; std_stdstringconstructor2(&__chx__lv__15, "failed to write WAV file", 24, 0); &__chx__lv__15; }))
			}
		};
		if(__chx__lv__13) {
			std_stdvector__cgs__3delete(&file_data);
		}
		return;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 0, 
		.Ok.value = (struct std_stdUnit){ 
		}
	};
	if(__chx__lv__13) {
		std_stdvector__cgs__3delete(&file_data);
	}
	return;
}
double audio_audioaudio_duration_ms(struct audio_audioAudio* audio){
	if((audio->sample_rate == 0)){
		return 0.0;
	}
	return ((((double) audio->num_samples) * 1000.0) / ((double) audio->sample_rate));
}
void audio_audioaudio_trim(struct audio_audioAudio* __chx_struct_ret_param_xx, struct audio_audioAudio* audio, double start_ms, double end_ms){
	size_t start_sample = ((size_t) ((start_ms * ((double) audio->sample_rate)) / 1000.0));
	size_t end_sample = ((size_t) ((end_ms * ((double) audio->sample_rate)) / 1000.0));
	if((start_sample >= audio->num_samples)){
		start_sample = (audio->num_samples - 1);
	}
	if((end_sample > audio->num_samples)){
		end_sample = audio->num_samples;
	}
	struct audio_audioAudio result = (*({ struct audio_audioAudio __chx__lv__16; audio_audioAudiomake(&__chx__lv__16); &__chx__lv__16; }));
	_Bool __chx__lv__17 = true;
	result.sample_rate = audio->sample_rate;
	result.channels = audio->channels;
	result.bits_per_sample = audio->bits_per_sample;
	result.num_samples = (end_sample - start_sample);
	size_t ch = ((size_t) audio->channels);
	unsigned long total = (result.num_samples * ch);
	std_stdvector__cgs__4resize(&result.samples, total);
	unsigned long src_offset = (start_sample * ch);
	int16_t* sptr = ((int16_t*) std_stdvector__cgs__4data(&result.samples));
	size_t i = 0;
	while((i < total)) {
		sptr[i] = std_stdvector__cgs__4get(&audio->samples, (src_offset + i));
		i += 1;
	}
	result.loaded = 1;
	*__chx_struct_ret_param_xx = result;
	return;
}
void audio_audioaudio_volume(struct audio_audioAudio* audio, float factor){
	int16_t* sptr = ((int16_t*) std_stdvector__cgs__4data(&audio->samples));
	size_t i = 0;
	while((i < std_stdvector__cgs__4size(&audio->samples))) {
		int32_t sample = ((int32_t) (((float) std_stdvector__cgs__4get(&audio->samples, i)) * factor));
		if((sample > 32767)){
			sample = 32767;
		}
		if((sample < -32768)){
			sample = -32768;
		}
		sptr[i] = ((int16_t) sample);
		i += 1;
	}
}
void audio_audioaudio_mix(struct std_stdResult__cgs__37* __chx_struct_ret_param_xx, struct audio_audioAudio* a, struct audio_audioAudio* b){
	if((a->sample_rate != b->sample_rate)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__37) { .__chx__vt_621827 = 1, 
			.Err.error = (struct audio_audioAudioError) { .__chx__vt_621827 = 1, 
				.InvalidFormat.msg = (*({ struct std_stdstring __chx__lv__18; std_stdstringconstructor2(&__chx__lv__18, "sample rate mismatch", 20, 0); &__chx__lv__18; }))
			}
		};
		return;
	}
	if((a->channels != b->channels)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__37) { .__chx__vt_621827 = 1, 
			.Err.error = (struct audio_audioAudioError) { .__chx__vt_621827 = 1, 
				.InvalidFormat.msg = (*({ struct std_stdstring __chx__lv__19; std_stdstringconstructor2(&__chx__lv__19, "channel count mismatch", 22, 0); &__chx__lv__19; }))
			}
		};
		return;
	}
	size_t max_samples;
	if((a->num_samples > b->num_samples)){
		max_samples = a->num_samples;
	} else {
		max_samples = b->num_samples;
	}
	size_t ch = ((size_t) a->channels);
	struct audio_audioAudio result = (*({ struct audio_audioAudio __chx__lv__20; audio_audioAudiomake(&__chx__lv__20); &__chx__lv__20; }));
	_Bool __chx__lv__21 = true;
	result.sample_rate = a->sample_rate;
	result.channels = a->channels;
	result.bits_per_sample = a->bits_per_sample;
	result.num_samples = max_samples;
	std_stdvector__cgs__4resize(&result.samples, (max_samples * ch));
	int16_t* rptr = ((int16_t*) std_stdvector__cgs__4data(&result.samples));
	size_t i = 0;
	while((i < (max_samples * ch))) {
		int32_t sa = 0;
		int32_t sb = 0;
		if((i < std_stdvector__cgs__4size(&a->samples))){
			sa = ((int32_t) std_stdvector__cgs__4get(&a->samples, i));
		}
		if((i < std_stdvector__cgs__4size(&b->samples))){
			sb = ((int32_t) std_stdvector__cgs__4get(&b->samples, i));
		}
		int32_t mixed = ((sa + sb) / 2);
		if((mixed > 32767)){
			mixed = 32767;
		}
		if((mixed < -32768)){
			mixed = -32768;
		}
		rptr[i] = ((int16_t) mixed);
		i += 1;
	}
	result.loaded = 1;
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__37) { .__chx__vt_621827 = 0, 
		.Ok.value = ({ __chx__lv__21 = false; result; })
	};
	if(__chx__lv__21) {
		audio_audioAudiodelete(&result);
	}
	return;
}
void audio_audioaudio_append(struct std_stdResult__cgs__37* __chx_struct_ret_param_xx, struct audio_audioAudio* a, struct audio_audioAudio* b){
	if((a->sample_rate != b->sample_rate)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__37) { .__chx__vt_621827 = 1, 
			.Err.error = (struct audio_audioAudioError) { .__chx__vt_621827 = 1, 
				.InvalidFormat.msg = (*({ struct std_stdstring __chx__lv__22; std_stdstringconstructor2(&__chx__lv__22, "sample rate mismatch", 20, 0); &__chx__lv__22; }))
			}
		};
		return;
	}
	if((a->channels != b->channels)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__37) { .__chx__vt_621827 = 1, 
			.Err.error = (struct audio_audioAudioError) { .__chx__vt_621827 = 1, 
				.InvalidFormat.msg = (*({ struct std_stdstring __chx__lv__23; std_stdstringconstructor2(&__chx__lv__23, "channel count mismatch", 22, 0); &__chx__lv__23; }))
			}
		};
		return;
	}
	struct audio_audioAudio result = (*({ struct audio_audioAudio __chx__lv__24; audio_audioAudiomake(&__chx__lv__24); &__chx__lv__24; }));
	_Bool __chx__lv__25 = true;
	result.sample_rate = a->sample_rate;
	result.channels = a->channels;
	result.bits_per_sample = a->bits_per_sample;
	result.num_samples = (a->num_samples + b->num_samples);
	size_t total_a = std_stdvector__cgs__4size(&a->samples);
	size_t total_b = std_stdvector__cgs__4size(&b->samples);
	std_stdvector__cgs__4resize(&result.samples, (total_a + total_b));
	int16_t* rptr = ((int16_t*) std_stdvector__cgs__4data(&result.samples));
	size_t i = 0;
	while((i < total_a)) {
		rptr[i] = std_stdvector__cgs__4get(&a->samples, i);
		i += 1;
	}
	i = 0;
	while((i < total_b)) {
		rptr[(total_a + i)] = std_stdvector__cgs__4get(&b->samples, i);
		i += 1;
	}
	result.loaded = 1;
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__37) { .__chx__vt_621827 = 0, 
		.Ok.value = ({ __chx__lv__25 = false; result; })
	};
	if(__chx__lv__25) {
		audio_audioAudiodelete(&result);
	}
	return;
}
void audio_audioaudio_resample(struct audio_audioAudio* __chx_struct_ret_param_xx, struct audio_audioAudio* audio, uint32_t target_rate){
	if((audio->sample_rate == target_rate)){
		*__chx_struct_ret_param_xx = (*({ struct audio_audioAudio __chx__lv__26; audio_audioaudio_copy(&__chx__lv__26, audio); &__chx__lv__26; }));
		return;
	}
	double ratio = (((double) target_rate) / ((double) audio->sample_rate));
	size_t new_num_samples = ((size_t) (((double) audio->num_samples) * ratio));
	size_t ch = ((size_t) audio->channels);
	struct audio_audioAudio result = (*({ struct audio_audioAudio __chx__lv__27; audio_audioAudiomake(&__chx__lv__27); &__chx__lv__27; }));
	_Bool __chx__lv__28 = true;
	result.sample_rate = target_rate;
	result.channels = audio->channels;
	result.bits_per_sample = audio->bits_per_sample;
	result.num_samples = new_num_samples;
	std_stdvector__cgs__4resize(&result.samples, (new_num_samples * ch));
	int16_t* rptr = ((int16_t*) std_stdvector__cgs__4data(&result.samples));
	size_t i = 0;
	while((i < new_num_samples)) {
		size_t src_pos = ((size_t) (((double) i) / ratio));
		if((src_pos >= audio->num_samples)){
			src_pos = (audio->num_samples - 1);
		}
		size_t j = 0;
		while((j < ch)) {
			rptr[((i * ch) + j)] = std_stdvector__cgs__4get(&audio->samples, ((src_pos * ch) + j));
			j += 1;
		}
		i += 1;
	}
	result.loaded = 1;
	*__chx_struct_ret_param_xx = result;
	return;
}
void audio_audioaudio_copy(struct audio_audioAudio* __chx_struct_ret_param_xx, struct audio_audioAudio* audio){
	struct audio_audioAudio result = (*({ struct audio_audioAudio __chx__lv__29; audio_audioAudiomake(&__chx__lv__29); &__chx__lv__29; }));
	_Bool __chx__lv__30 = true;
	result.sample_rate = audio->sample_rate;
	result.channels = audio->channels;
	result.bits_per_sample = audio->bits_per_sample;
	result.num_samples = audio->num_samples;
	result.loaded = audio->loaded;
	std_stdvector__cgs__4resize(&result.samples, std_stdvector__cgs__4size(&audio->samples));
	int16_t* rptr = ((int16_t*) std_stdvector__cgs__4data(&result.samples));
	size_t i = 0;
	while((i < std_stdvector__cgs__4size(&audio->samples))) {
		rptr[i] = std_stdvector__cgs__4get(&audio->samples, i);
		i += 1;
	}
	*__chx_struct_ret_param_xx = result;
	return;
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/audio/src/types.ch **/
void audio_audioAudioErrormessage(struct std_stdstring* __chx_struct_ret_param_xx, struct audio_audioAudioError*const self){
	switch((self)->__chx__vt_621827) {
		case 0:{
			*__chx_struct_ret_param_xx = (*({ struct std_stdstring __chx__lv__31; std_stdstringconstructor2(&__chx__lv__31, "AudioError: file not found", 26, 0); &__chx__lv__31; }));
			return;
			break;
		}
		case 1:{
			struct std_stdstring s = (*({ struct std_stdstring __chx__lv__32; std_stdstringconstructor2(&__chx__lv__32, "AudioError: ", 12, 0); &__chx__lv__32; }));
			_Bool __chx__lv__33 = true;
			std_stdstringappend_string(&s, &(self)->InvalidFormat.msg);
			*__chx_struct_ret_param_xx = s;
			return;
			break;
		}
		case 2:{
			struct std_stdstring s = (*({ struct std_stdstring __chx__lv__34; std_stdstringconstructor2(&__chx__lv__34, "AudioError: unsupported: ", 25, 0); &__chx__lv__34; }));
			_Bool __chx__lv__35 = true;
			std_stdstringappend_string(&s, &(self)->UnsupportedFormat.msg);
			*__chx_struct_ret_param_xx = s;
			return;
			break;
		}
		case 3:{
			struct std_stdstring s = (*({ struct std_stdstring __chx__lv__36; std_stdstringconstructor2(&__chx__lv__36, "AudioError: IO error: ", 22, 0); &__chx__lv__36; }));
			_Bool __chx__lv__37 = true;
			std_stdstringappend_string(&s, &(self)->IoError.msg);
			*__chx_struct_ret_param_xx = s;
			return;
			break;
		}
		case 4:{
			*__chx_struct_ret_param_xx = (*({ struct std_stdstring __chx__lv__38; std_stdstringconstructor2(&__chx__lv__38, "AudioError: buffer too small", 28, 0); &__chx__lv__38; }));
			return;
			break;
		}
	}
}
void audio_audioAudioErrordelete(struct audio_audioAudioError* self){
	__chx__dstctr_clnup_blk__:{
		switch(self->__chx__vt_621827) {
			case 0:
			break;
			case 1:
			std_stdstringdelete(&self->InvalidFormat.msg);
			break;
			case 2:
			std_stdstringdelete(&self->UnsupportedFormat.msg);
			break;
			case 3:
			std_stdstringdelete(&self->IoError.msg);
			break;
			case 4:
			break;
		}
	}
}
void audio_audioAudiomake(struct audio_audioAudio* this){
	*this = (struct audio_audioAudio){ 
		.samples = (*({ struct std_stdvector__cgs__4 __chx__lv__39; std_stdvector__cgs__4make(&__chx__lv__39); &__chx__lv__39; })), 
		.sample_rate = 44100, 
		.channels = 1, 
		.bits_per_sample = 16, 
		.num_samples = 0, 
		.loaded = 0
	};
	return;
}
void audio_audioAudiodelete(struct audio_audioAudio* self){
	__chx__dstctr_clnup_blk__:{
		std_stdvector__cgs__4delete(&self->samples);
	}
}