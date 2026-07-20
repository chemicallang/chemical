
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/encoding/src/types.ch **/
struct encoding_encodingEncodingError;
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/encoding/src/hex.ch **/
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/encoding/src/url.ch **/
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/encoding/src/utf.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/encoding/src/types.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/encoding/src/hex.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/encoding/src/url.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/encoding/src/utf.ch **/
/** FwdDeclare:Generics encoding **/
struct std_stdResult__cgs__29;
struct std_stdResult__cgs__30;
/** Declare:Generics encoding **/
struct encoding_encodingEncodingError {
	int __chx__vt_621827;
	union {
		struct {
			size_t needed;
		} BufferTooSmall;
		struct {
		} InvalidInput;
		struct {
		} Unsupported;
	};
};
struct std_stdResult__cgs__29 {
	int __chx__vt_621827;
	union {
		struct {
			size_t value;
		} Ok;
		struct {
			struct encoding_encodingEncodingError error;
		} Err;
	};
};
struct std_stdResult__cgs__30 {
	int __chx__vt_621827;
	union {
		struct {
			struct std_stdvector__cgs__3 value;
		} Ok;
		struct {
			struct encoding_encodingEncodingError error;
		} Err;
	};
};
void std_stdResult__cgs__30delete(struct std_stdResult__cgs__30* self);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/encoding/src/types.ch **/
void encoding_encodingEncodingErrormessage(struct std_stdstring* __chx_struct_ret_param_xx, struct encoding_encodingEncodingError*const self);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/encoding/src/hex.ch **/
static char* encoding_encodingHEX_DIGITS_LOWER;
static char* encoding_encodingHEX_DIGITS_UPPER;
void encoding_encodinghex_encode(struct std_stdResult__cgs__29* __chx_struct_ret_param_xx, const uint8_t* data, size_t data_len, char* out, size_t out_len);
void encoding_encodinghex_encode_upper(struct std_stdResult__cgs__29* __chx_struct_ret_param_xx, const uint8_t* data, size_t data_len, char* out, size_t out_len);
void encoding_encodinghex_decode(struct std_stdResult__cgs__29* __chx_struct_ret_param_xx, const char* hex, uint8_t* out, size_t out_len);
void encoding_encodinghex_decode_to_vec(struct std_stdResult__cgs__30* __chx_struct_ret_param_xx, struct std_stdstring_view* hex);
static int encoding_encodinghex_to_nibble(char c);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/encoding/src/url.ch **/
static _Bool encoding_encodingis_unreserved(char c);
void encoding_encodingurl_encode(struct std_stdResult__cgs__29* __chx_struct_ret_param_xx, const char* data, size_t data_len, char* out, size_t out_len);
void encoding_encodingurl_encode_query(struct std_stdResult__cgs__29* __chx_struct_ret_param_xx, const char* data, size_t data_len, char* out, size_t out_len);
void encoding_encodingurl_decode(struct std_stdResult__cgs__29* __chx_struct_ret_param_xx, const char* data, size_t data_len, char* out, size_t out_len);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/encoding/src/utf.ch **/
_Bool encoding_encodingutf8_is_valid(const char* data, size_t data_len);
void encoding_encodingutf8_to_utf16(struct std_stdResult__cgs__29* __chx_struct_ret_param_xx, const char* in_utf8, uint16_t* out_w, size_t out_w_len);
void encoding_encodingutf16_to_utf8(struct std_stdResult__cgs__29* __chx_struct_ret_param_xx, const uint16_t* in_w, char* out, size_t out_len);
size_t encoding_encodingutf8_char_len(uint8_t leading);
_Bool encoding_encodingutf8_decode(const char* data, size_t data_len, int* out_codepoint, size_t* out_bytes);
/** Implement:Generics encoding **/
void std_stdResult__cgs__30delete(struct std_stdResult__cgs__30* self){
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
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/encoding/src/types.ch **/
void encoding_encodingEncodingErrormessage(struct std_stdstring* __chx_struct_ret_param_xx, struct encoding_encodingEncodingError*const self){
	switch((self)->__chx__vt_621827) {
		case 0:{
			struct std_stdstring s = (*({ struct std_stdstring __chx__lv__0; std_stdstringconstructor2(&__chx__lv__0, "EncodingError: buffer too small, need ", 38, 0); &__chx__lv__0; }));
			_Bool __chx__lv__1 = true;
			std_stdstringappend_integer(&s, ((int) (self)->BufferTooSmall.needed));
			*__chx_struct_ret_param_xx = s;
			return;
			break;
		}
		case 1:{
			*__chx_struct_ret_param_xx = (*({ struct std_stdstring __chx__lv__2; std_stdstringconstructor2(&__chx__lv__2, "EncodingError: invalid input", 28, 0); &__chx__lv__2; }));
			return;
			break;
		}
		case 2:{
			*__chx_struct_ret_param_xx = (*({ struct std_stdstring __chx__lv__3; std_stdstringconstructor2(&__chx__lv__3, "EncodingError: unsupported operation", 36, 0); &__chx__lv__3; }));
			return;
			break;
		}
	}
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/encoding/src/hex.ch **/
static char* encoding_encodingHEX_DIGITS_LOWER = "0123456789abcdef";
static char* encoding_encodingHEX_DIGITS_UPPER = "0123456789ABCDEF";
void encoding_encodinghex_encode(struct std_stdResult__cgs__29* __chx_struct_ret_param_xx, const uint8_t* data, size_t data_len, char* out, size_t out_len){
	unsigned long needed = (data_len * 2);
	if(((needed + 1) > out_len)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 1, 
			.Err.error = (struct encoding_encodingEncodingError) { .__chx__vt_621827 = 0, 
				.BufferTooSmall.needed = (needed + 1)
			}
		};
		return;
	}
	size_t pos = 0;
	size_t i = 0;
	while((i < data_len)) {
		uint8_t b = data[i];
		out[pos] = ((char) encoding_encodingHEX_DIGITS_LOWER[((size_t) (b >> 4))]);
		pos += 1;
		out[pos] = ((char) encoding_encodingHEX_DIGITS_LOWER[((size_t) (b & 15))]);
		pos += 1;
		i += 1;
	}
	out[pos] = 0;
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 0, 
		.Ok.value = needed
	};
	return;
}
void encoding_encodinghex_encode_upper(struct std_stdResult__cgs__29* __chx_struct_ret_param_xx, const uint8_t* data, size_t data_len, char* out, size_t out_len){
	unsigned long needed = (data_len * 2);
	if(((needed + 1) > out_len)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 1, 
			.Err.error = (struct encoding_encodingEncodingError) { .__chx__vt_621827 = 0, 
				.BufferTooSmall.needed = (needed + 1)
			}
		};
		return;
	}
	size_t pos = 0;
	size_t i = 0;
	while((i < data_len)) {
		uint8_t b = data[i];
		out[pos] = ((char) encoding_encodingHEX_DIGITS_UPPER[((size_t) (b >> 4))]);
		pos += 1;
		out[pos] = ((char) encoding_encodingHEX_DIGITS_UPPER[((size_t) (b & 15))]);
		pos += 1;
		i += 1;
	}
	out[pos] = 0;
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 0, 
		.Ok.value = needed
	};
	return;
}
void encoding_encodinghex_decode(struct std_stdResult__cgs__29* __chx_struct_ret_param_xx, const char* hex, uint8_t* out, size_t out_len){
	size_t hex_len = 0;
	while((hex[hex_len] != 0)) {
		hex_len += 1;
	}
	if(((hex_len % 2) != 0)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 1, 
			.Err.error = (struct encoding_encodingEncodingError) { .__chx__vt_621827 = 1, 
			}
		};
		return;
	}
	unsigned long byte_count = (hex_len / 2);
	if((byte_count > out_len)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 1, 
			.Err.error = (struct encoding_encodingEncodingError) { .__chx__vt_621827 = 0, 
				.BufferTooSmall.needed = byte_count
			}
		};
		return;
	}
	size_t pos = 0;
	size_t i = 0;
	while((i < hex_len)) {
		int high = encoding_encodinghex_to_nibble(hex[i]);
		if((high < 0)){
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 1, 
				.Err.error = (struct encoding_encodingEncodingError) { .__chx__vt_621827 = 1, 
				}
			};
			return;
		}
		int low = encoding_encodinghex_to_nibble(hex[(i + 1)]);
		if((low < 0)){
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 1, 
				.Err.error = (struct encoding_encodingEncodingError) { .__chx__vt_621827 = 1, 
				}
			};
			return;
		}
		out[pos] = ((((uint8_t) high) << 4) | ((uint8_t) low));
		pos += 1;
		i += 2;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 0, 
		.Ok.value = byte_count
	};
	return;
}
void encoding_encodinghex_decode_to_vec(struct std_stdResult__cgs__30* __chx_struct_ret_param_xx, struct std_stdstring_view* hex){
	if(((std_stdstring_viewsize(hex) % 2) != 0)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__30) { .__chx__vt_621827 = 1, 
			.Err.error = (struct encoding_encodingEncodingError) { .__chx__vt_621827 = 1, 
			}
		};
		return;
	}
	unsigned long byte_count = (std_stdstring_viewsize(hex) / 2);
	struct std_stdvector__cgs__3 vec = (*({ struct std_stdvector__cgs__3 __chx__lv__4; std_stdvector__cgs__3make(&__chx__lv__4); &__chx__lv__4; }));
	_Bool __chx__lv__5 = true;
	std_stdvector__cgs__3reserve(&vec, byte_count);
	size_t i = 0;
	while((i < std_stdstring_viewsize(hex))) {
		int high = encoding_encodinghex_to_nibble(std_stdstring_viewdata(hex)[i]);
		if((high < 0)){
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__30) { .__chx__vt_621827 = 1, 
				.Err.error = (struct encoding_encodingEncodingError) { .__chx__vt_621827 = 1, 
				}
			};
			if(__chx__lv__5) {
				std_stdvector__cgs__3delete(&vec);
			}
			return;
		}
		int low = encoding_encodinghex_to_nibble(std_stdstring_viewdata(hex)[(i + 1)]);
		if((low < 0)){
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__30) { .__chx__vt_621827 = 1, 
				.Err.error = (struct encoding_encodingEncodingError) { .__chx__vt_621827 = 1, 
				}
			};
			if(__chx__lv__5) {
				std_stdvector__cgs__3delete(&vec);
			}
			return;
		}
		std_stdvector__cgs__3push(&vec, ((((uint8_t) high) << 4) | ((uint8_t) low)));
		i += 2;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__30) { .__chx__vt_621827 = 0, 
		.Ok.value = ({ __chx__lv__5 = false; vec; })
	};
	if(__chx__lv__5) {
		std_stdvector__cgs__3delete(&vec);
	}
	return;
}
static int encoding_encodinghex_to_nibble(char c){
	if(((c >= '0') && (c <= '9'))){
		return (((int) c) - ((int) '0'));
	}
	if(((c >= 'a') && (c <= 'f'))){
		return ((((int) c) - ((int) 'a')) + 10);
	}
	if(((c >= 'A') && (c <= 'F'))){
		return ((((int) c) - ((int) 'A')) + 10);
	}
	return -1;
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/encoding/src/url.ch **/
static _Bool encoding_encodingis_unreserved(char c){
	if(((c >= 'a') && (c <= 'z'))){
		return 1;
	}
	if(((c >= 'A') && (c <= 'Z'))){
		return 1;
	}
	if(((c >= '0') && (c <= '9'))){
		return 1;
	}
	if(((((c == '-') || (c == '.')) || (c == '_')) || (c == '~'))){
		return 1;
	}
	return 0;
}
void encoding_encodingurl_encode(struct std_stdResult__cgs__29* __chx_struct_ret_param_xx, const char* data, size_t data_len, char* out, size_t out_len){
	size_t pos = 0;
	size_t i = 0;
	while((i < data_len)) {
		char c = data[i];
		if(encoding_encodingis_unreserved(c)){
			if(((pos + 1) >= out_len)){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 1, 
					.Err.error = (struct encoding_encodingEncodingError) { .__chx__vt_621827 = 0, 
						.BufferTooSmall.needed = (pos + 2)
					}
				};
				return;
			}
			out[pos] = c;
			pos += 1;
		} else {
			if(((pos + 3) >= out_len)){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 1, 
					.Err.error = (struct encoding_encodingEncodingError) { .__chx__vt_621827 = 0, 
						.BufferTooSmall.needed = (pos + 4)
					}
				};
				return;
			}
			out[pos] = '%';
			out[(pos + 1)] = ((char) encoding_encodingHEX_DIGITS_UPPER[((size_t) (((uint8_t) c) >> 4))]);
			out[(pos + 2)] = ((char) encoding_encodingHEX_DIGITS_UPPER[((size_t) (((uint8_t) c) & 15))]);
			pos += 3;
		}
		i += 1;
	}
	out[pos] = 0;
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 0, 
		.Ok.value = pos
	};
	return;
}
void encoding_encodingurl_encode_query(struct std_stdResult__cgs__29* __chx_struct_ret_param_xx, const char* data, size_t data_len, char* out, size_t out_len){
	size_t pos = 0;
	size_t i = 0;
	while((i < data_len)) {
		char c = data[i];
		if((c == ' ')){
			if(((pos + 1) >= out_len)){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 1, 
					.Err.error = (struct encoding_encodingEncodingError) { .__chx__vt_621827 = 0, 
						.BufferTooSmall.needed = (pos + 2)
					}
				};
				return;
			}
			out[pos] = '+';
			pos += 1;
		}else if(encoding_encodingis_unreserved(c)){
			if(((pos + 1) >= out_len)){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 1, 
					.Err.error = (struct encoding_encodingEncodingError) { .__chx__vt_621827 = 0, 
						.BufferTooSmall.needed = (pos + 2)
					}
				};
				return;
			}
			out[pos] = c;
			pos += 1;
		} else {
			if(((pos + 3) >= out_len)){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 1, 
					.Err.error = (struct encoding_encodingEncodingError) { .__chx__vt_621827 = 0, 
						.BufferTooSmall.needed = (pos + 4)
					}
				};
				return;
			}
			out[pos] = '%';
			out[(pos + 1)] = ((char) encoding_encodingHEX_DIGITS_UPPER[((size_t) (((uint8_t) c) >> 4))]);
			out[(pos + 2)] = ((char) encoding_encodingHEX_DIGITS_UPPER[((size_t) (((uint8_t) c) & 15))]);
			pos += 3;
		}
		i += 1;
	}
	out[pos] = 0;
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 0, 
		.Ok.value = pos
	};
	return;
}
void encoding_encodingurl_decode(struct std_stdResult__cgs__29* __chx_struct_ret_param_xx, const char* data, size_t data_len, char* out, size_t out_len){
	size_t pos = 0;
	size_t i = 0;
	while((i < data_len)) {
		char c = data[i];
		if(((c == '%') && ((i + 2) < data_len))){
			int high = encoding_encodinghex_to_nibble(data[(i + 1)]);
			int low = encoding_encodinghex_to_nibble(data[(i + 2)]);
			if(((high >= 0) && (low >= 0))){
				if(((pos + 1) >= out_len)){
					*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 1, 
						.Err.error = (struct encoding_encodingEncodingError) { .__chx__vt_621827 = 0, 
							.BufferTooSmall.needed = (pos + 2)
						}
					};
					return;
				}
				out[pos] = ((char) ((((uint8_t) high) << 4) | ((uint8_t) low)));
				pos += 1;
				i += 3;
				continue;
			}
		}
		if((c == '+')){
			if(((pos + 1) >= out_len)){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 1, 
					.Err.error = (struct encoding_encodingEncodingError) { .__chx__vt_621827 = 0, 
						.BufferTooSmall.needed = (pos + 2)
					}
				};
				return;
			}
			out[pos] = ' ';
			pos += 1;
		} else {
			if(((pos + 1) >= out_len)){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 1, 
					.Err.error = (struct encoding_encodingEncodingError) { .__chx__vt_621827 = 0, 
						.BufferTooSmall.needed = (pos + 2)
					}
				};
				return;
			}
			out[pos] = c;
			pos += 1;
		}
		i += 1;
	}
	out[pos] = 0;
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 0, 
		.Ok.value = pos
	};
	return;
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/encoding/src/utf.ch **/
_Bool encoding_encodingutf8_is_valid(const char* data, size_t data_len){
	size_t i = 0;
	while((i < data_len)) {
		uint8_t c = ((uint8_t) data[i]);
		if((c < 128)){
			i += 1;
			continue;
		}
		uint8_t seq_len = 0;
		if(((c & 224) == 192)){
			seq_len = 2;
		}else if(((c & 240) == 224)){
			seq_len = 3;
		}else if(((c & 248) == 240)){
			seq_len = 4;
		} else {
			return 0;
		}
		if(((i + ((size_t) seq_len)) > data_len)){
			return 0;
		}
		size_t j = 1;
		while((j < ((size_t) seq_len))) {
			if(((((uint8_t) data[(i + j)]) & 192) != 128)){
				return 0;
			}
			j += 1;
		}
		i += ((size_t) seq_len);
	}
	return 1;
}
void encoding_encodingutf8_to_utf16(struct std_stdResult__cgs__29* __chx_struct_ret_param_xx, const char* in_utf8, uint16_t* out_w, size_t out_w_len){
	size_t i = 0;
	size_t wpos = 0;
	while((in_utf8[i] != 0)) {
		uint8_t c = ((uint8_t) in_utf8[i]);
		if((c < 128)){
			if(((wpos + 1) >= out_w_len)){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 1, 
					.Err.error = (struct encoding_encodingEncodingError) { .__chx__vt_621827 = 0, 
						.BufferTooSmall.needed = (wpos + 2)
					}
				};
				return;
			}
			out_w[wpos] = ((uint16_t) c);
			wpos += 1;
			i += 1;
			continue;
		}
		if(((c & 224) == 192)){
			if((in_utf8[(i + 1)] == 0)){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 1, 
					.Err.error = (struct encoding_encodingEncodingError) { .__chx__vt_621827 = 1, 
					}
				};
				return;
			}
			uint8_t c2 = ((uint8_t) in_utf8[(i + 1)]);
			if(((c2 & 192) != 128)){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 1, 
					.Err.error = (struct encoding_encodingEncodingError) { .__chx__vt_621827 = 1, 
					}
				};
				return;
			}
			uint32_t code = ((((uint32_t) (c & 31)) << 6) | ((uint32_t) (c2 & 63)));
			if((code < 128)){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 1, 
					.Err.error = (struct encoding_encodingEncodingError) { .__chx__vt_621827 = 1, 
					}
				};
				return;
			}
			if(((wpos + 1) >= out_w_len)){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 1, 
					.Err.error = (struct encoding_encodingEncodingError) { .__chx__vt_621827 = 0, 
						.BufferTooSmall.needed = (wpos + 2)
					}
				};
				return;
			}
			out_w[wpos] = ((uint16_t) code);
			wpos += 1;
			i += 2;
			continue;
		}
		if(((c & 240) == 224)){
			if(((in_utf8[(i + 1)] == 0) || (in_utf8[(i + 2)] == 0))){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 1, 
					.Err.error = (struct encoding_encodingEncodingError) { .__chx__vt_621827 = 1, 
					}
				};
				return;
			}
			uint8_t c2 = ((uint8_t) in_utf8[(i + 1)]);
			uint8_t c3 = ((uint8_t) in_utf8[(i + 2)]);
			if((((c2 & 192) != 128) || ((c3 & 192) != 128))){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 1, 
					.Err.error = (struct encoding_encodingEncodingError) { .__chx__vt_621827 = 1, 
					}
				};
				return;
			}
			uint32_t code = (((((uint32_t) (c & 15)) << 12) | (((uint32_t) (c2 & 63)) << 6)) | ((uint32_t) (c3 & 63)));
			if((code < 2048)){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 1, 
					.Err.error = (struct encoding_encodingEncodingError) { .__chx__vt_621827 = 1, 
					}
				};
				return;
			}
			if(((code >= 55296) && (code <= 57343))){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 1, 
					.Err.error = (struct encoding_encodingEncodingError) { .__chx__vt_621827 = 1, 
					}
				};
				return;
			}
			if(((wpos + 1) >= out_w_len)){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 1, 
					.Err.error = (struct encoding_encodingEncodingError) { .__chx__vt_621827 = 0, 
						.BufferTooSmall.needed = (wpos + 2)
					}
				};
				return;
			}
			out_w[wpos] = ((uint16_t) code);
			wpos += 1;
			i += 3;
			continue;
		}
		if(((c & 248) == 240)){
			if((((in_utf8[(i + 1)] == 0) || (in_utf8[(i + 2)] == 0)) || (in_utf8[(i + 3)] == 0))){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 1, 
					.Err.error = (struct encoding_encodingEncodingError) { .__chx__vt_621827 = 1, 
					}
				};
				return;
			}
			uint8_t c2 = ((uint8_t) in_utf8[(i + 1)]);
			uint8_t c3 = ((uint8_t) in_utf8[(i + 2)]);
			uint8_t c4 = ((uint8_t) in_utf8[(i + 3)]);
			if(((((c2 & 192) != 128) || ((c3 & 192) != 128)) || ((c4 & 192) != 128))){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 1, 
					.Err.error = (struct encoding_encodingEncodingError) { .__chx__vt_621827 = 1, 
					}
				};
				return;
			}
			uint32_t code = ((((((uint32_t) (c & 7)) << 18) | (((uint32_t) (c2 & 63)) << 12)) | (((uint32_t) (c3 & 63)) << 6)) | ((uint32_t) (c4 & 63)));
			if(((code < 65536) || (code > 1114111))){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 1, 
					.Err.error = (struct encoding_encodingEncodingError) { .__chx__vt_621827 = 1, 
					}
				};
				return;
			}
			code -= 65536;
			uint16_t high = (55296 + ((uint16_t) (code >> 10)));
			uint16_t low = (56320 + ((uint16_t) (code & 1023)));
			if(((wpos + 2) >= out_w_len)){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 1, 
					.Err.error = (struct encoding_encodingEncodingError) { .__chx__vt_621827 = 0, 
						.BufferTooSmall.needed = (wpos + 3)
					}
				};
				return;
			}
			out_w[wpos] = high;
			wpos += 1;
			out_w[wpos] = low;
			wpos += 1;
			i += 4;
			continue;
		}
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 1, 
			.Err.error = (struct encoding_encodingEncodingError) { .__chx__vt_621827 = 1, 
			}
		};
		return;
	}
	if((wpos >= out_w_len)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 1, 
			.Err.error = (struct encoding_encodingEncodingError) { .__chx__vt_621827 = 0, 
				.BufferTooSmall.needed = (wpos + 1)
			}
		};
		return;
	}
	out_w[wpos] = 0;
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 0, 
		.Ok.value = wpos
	};
	return;
}
void encoding_encodingutf16_to_utf8(struct std_stdResult__cgs__29* __chx_struct_ret_param_xx, const uint16_t* in_w, char* out, size_t out_len){
	size_t i = 0;
	size_t pos = 0;
	while(1) {
		uint16_t w = in_w[i];
		if((w == 0)){
			break;
		}
		if((w < 128)){
			if(((pos + 1) >= out_len)){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 1, 
					.Err.error = (struct encoding_encodingEncodingError) { .__chx__vt_621827 = 0, 
						.BufferTooSmall.needed = (pos + 2)
					}
				};
				return;
			}
			out[pos] = ((char) w);
			pos += 1;
		}else if((w < 2048)){
			if(((pos + 2) >= out_len)){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 1, 
					.Err.error = (struct encoding_encodingEncodingError) { .__chx__vt_621827 = 0, 
						.BufferTooSmall.needed = (pos + 3)
					}
				};
				return;
			}
			out[pos] = ((char) (192 | ((w >> 6) & 31)));
			out[(pos + 1)] = ((char) (128 | (w & 63)));
			pos += 2;
		}else if(((w >= 55296) && (w <= 56319))){
			uint16_t w2 = in_w[(i + 1)];
			if((((w2 == 0) || (w2 < 56320)) || (w2 > 57343))){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 1, 
					.Err.error = (struct encoding_encodingEncodingError) { .__chx__vt_621827 = 1, 
					}
				};
				return;
			}
			uint32_t code = (65536 + ((((uint32_t) (w & 1023)) << 10) | ((uint32_t) (w2 & 1023))));
			if(((pos + 4) >= out_len)){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 1, 
					.Err.error = (struct encoding_encodingEncodingError) { .__chx__vt_621827 = 0, 
						.BufferTooSmall.needed = (pos + 5)
					}
				};
				return;
			}
			out[pos] = ((char) (240 | ((code >> 18) & 7)));
			out[(pos + 1)] = ((char) (128 | ((code >> 12) & 63)));
			out[(pos + 2)] = ((char) (128 | ((code >> 6) & 63)));
			out[(pos + 3)] = ((char) (128 | (code & 63)));
			pos += 4;
			i += 1;
		} else {
			if(((pos + 3) >= out_len)){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 1, 
					.Err.error = (struct encoding_encodingEncodingError) { .__chx__vt_621827 = 0, 
						.BufferTooSmall.needed = (pos + 4)
					}
				};
				return;
			}
			out[pos] = ((char) (224 | ((w >> 12) & 15)));
			out[(pos + 1)] = ((char) (128 | ((w >> 6) & 63)));
			out[(pos + 2)] = ((char) (128 | (w & 63)));
			pos += 3;
		}
		i += 1;
	}
	if((pos >= out_len)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 1, 
			.Err.error = (struct encoding_encodingEncodingError) { .__chx__vt_621827 = 0, 
				.BufferTooSmall.needed = (pos + 1)
			}
		};
		return;
	}
	out[pos] = 0;
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 0, 
		.Ok.value = pos
	};
	return;
}
size_t encoding_encodingutf8_char_len(uint8_t leading){
	if((leading < 128)){
		return 1;
	}
	if(((leading & 224) == 192)){
		return 2;
	}
	if(((leading & 240) == 224)){
		return 3;
	}
	if(((leading & 248) == 240)){
		return 4;
	}
	return 1;
}
_Bool encoding_encodingutf8_decode(const char* data, size_t data_len, int* out_codepoint, size_t* out_bytes){
	if((data_len == 0)){
		out_bytes[0] = 0;
		out_codepoint[0] = 0;
		return 0;
	}
	uint8_t c = ((uint8_t) data[0]);
	if((c < 128)){
		out_codepoint[0] = ((int) c);
		out_bytes[0] = 1;
		return 1;
	}
	size_t seq_len = encoding_encodingutf8_char_len(c);
	if((seq_len > data_len)){
		out_bytes[0] = data_len;
		out_codepoint[0] = -1;
		return 0;
	}
	if((seq_len == 2)){
		uint8_t c2 = ((uint8_t) data[1]);
		if(((c2 & 192) != 128)){
			out_bytes[0] = 1;
			out_codepoint[0] = -1;
			return 0;
		}
		int code = ((((int) (c & 31)) << 6) | ((int) (c2 & 63)));
		if((code < 128)){
			out_bytes[0] = 1;
			out_codepoint[0] = -1;
			return 0;
		}
		out_codepoint[0] = code;
		out_bytes[0] = 2;
		return 1;
	}
	if((seq_len == 3)){
		uint8_t c2 = ((uint8_t) data[1]);
		uint8_t c3 = ((uint8_t) data[2]);
		if((((c2 & 192) != 128) || ((c3 & 192) != 128))){
			out_bytes[0] = 1;
			out_codepoint[0] = -1;
			return 0;
		}
		int code = (((((int) (c & 15)) << 12) | (((int) (c2 & 63)) << 6)) | ((int) (c3 & 63)));
		if(((code < 2048) || ((code >= 55296) && (code <= 57343)))){
			out_bytes[0] = 1;
			out_codepoint[0] = -1;
			return 0;
		}
		out_codepoint[0] = code;
		out_bytes[0] = 3;
		return 1;
	}
	if((seq_len == 4)){
		uint8_t c2 = ((uint8_t) data[1]);
		uint8_t c3 = ((uint8_t) data[2]);
		uint8_t c4 = ((uint8_t) data[3]);
		if(((((c2 & 192) != 128) || ((c3 & 192) != 128)) || ((c4 & 192) != 128))){
			out_bytes[0] = 1;
			out_codepoint[0] = -1;
			return 0;
		}
		int code = ((((((int) (c & 7)) << 18) | (((int) (c2 & 63)) << 12)) | (((int) (c3 & 63)) << 6)) | ((int) (c4 & 63)));
		if(((code < 65536) || (code > 1114111))){
			out_bytes[0] = 1;
			out_codepoint[0] = -1;
			return 0;
		}
		out_codepoint[0] = code;
		out_bytes[0] = 4;
		return 1;
	}
	out_bytes[0] = 1;
	out_codepoint[0] = -1;
	return 0;
}