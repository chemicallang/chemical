
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/image/src/bmp.ch **/
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/image/src/types.ch **/
struct image_imageImageError;
struct image_imageRGBA8;
struct image_imageRGB8;
struct image_imageGray8;
struct image_imageImage;
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/image/src/ppm.ch **/
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/image/src/core.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/image/src/bmp.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/image/src/types.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/image/src/ppm.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/image/src/core.ch **/
/** FwdDeclare:Generics image **/
struct std_stdResult__cgs__37;
struct std_stdResult__cgs__38;
struct std_stdResult__cgs__39;
struct std_stdResult__cgs__40;
/** Declare:Generics image **/
struct image_imageImage {
	struct std_stdvector__cgs__3 pixels;
	int width;
	int height;
	int channels;
};
struct image_imageImageError {
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
			int w;
			int h;
		} InvalidDimensions;
		struct {
			int x;
			int y;
		} PixelOutOfBounds;
	};
};
struct std_stdResult__cgs__37 {
	int __chx__vt_621827;
	union {
		struct {
			struct image_imageImage value;
		} Ok;
		struct {
			struct image_imageImageError error;
		} Err;
	};
};
void std_stdResult__cgs__37delete(struct std_stdResult__cgs__37* self);
struct std_stdResult__cgs__38 {
	int __chx__vt_621827;
	union {
		struct {
			int value;
		} Ok;
		struct {
			struct image_imageImageError error;
		} Err;
	};
};
void std_stdResult__cgs__38delete(struct std_stdResult__cgs__38* self);
struct image_imageRGBA8 {
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
};
struct std_stdResult__cgs__39 {
	int __chx__vt_621827;
	union {
		struct {
			struct image_imageRGBA8 value;
		} Ok;
		struct {
			struct image_imageImageError error;
		} Err;
	};
};
void std_stdResult__cgs__39delete(struct std_stdResult__cgs__39* self);
struct std_stdResult__cgs__40 {
	int __chx__vt_621827;
	union {
		struct {
			struct std_stdUnit value;
		} Ok;
		struct {
			struct image_imageImageError error;
		} Err;
	};
};
void std_stdResult__cgs__40delete(struct std_stdResult__cgs__40* self);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/image/src/bmp.ch **/
void image_imageload_bmp(struct std_stdResult__cgs__37* __chx_struct_ret_param_xx, const char* path);
void image_imageparse_bmp(struct std_stdResult__cgs__37* __chx_struct_ret_param_xx, const uint8_t* data, size_t data_len);
void image_imagesave_bmp(struct std_stdResult__cgs__40* __chx_struct_ret_param_xx, struct image_imageImage* img, const char* path);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/image/src/types.ch **/
void image_imageImageErrormessage(struct std_stdstring* __chx_struct_ret_param_xx, struct image_imageImageError*const self);
void image_imageImageErrordelete(struct image_imageImageError* self);
void image_imageRGBA8make(struct image_imageRGBA8* this, uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_);
struct image_imageRGB8 {
	uint8_t r;
	uint8_t g;
	uint8_t b;
};
struct image_imageGray8 {
	uint8_t v;
};
void image_imageImagemake(struct image_imageImage* this);
void image_imageImagedelete(struct image_imageImage* self);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/image/src/ppm.ch **/
void image_imageload_ppm(struct std_stdResult__cgs__37* __chx_struct_ret_param_xx, const char* path);
static _Bool image_imageis_digit(uint8_t c);
static void image_imageparse_ppm_number(struct std_stdResult__cgs__38* __chx_struct_ret_param_xx, const uint8_t* data, size_t* pos, size_t data_len);
void image_imageparse_ppm(struct std_stdResult__cgs__37* __chx_struct_ret_param_xx, const uint8_t* data, size_t data_len);
void image_imagesave_ppm(struct std_stdResult__cgs__40* __chx_struct_ret_param_xx, struct image_imageImage* img, const char* path);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/image/src/core.ch **/
static uint16_t image_imageread_u16_be(const uint8_t* data, size_t offset);
static uint32_t image_imageread_u32_be(const uint8_t* data, size_t offset);
static uint16_t image_imageread_u16_le(const uint8_t* data, size_t offset);
static uint32_t image_imageread_u32_le(const uint8_t* data, size_t offset);
static int32_t image_imageread_i32_le(const uint8_t* data, size_t offset);
static void image_imagewrite_u16_le(uint8_t* out, size_t offset, uint16_t val);
static void image_imagewrite_u32_le(uint8_t* out, size_t offset, uint32_t val);
void image_imageimage_create(struct image_imageImage* __chx_struct_ret_param_xx, int w, int h, int ch);
void image_imageimage_create_rgba(struct image_imageImage* __chx_struct_ret_param_xx, int w, int h);
void image_imageimage_create_rgb(struct image_imageImage* __chx_struct_ret_param_xx, int w, int h);
void image_imageimage_create_gray(struct image_imageImage* __chx_struct_ret_param_xx, int w, int h);
int image_imageimage_width(struct image_imageImage* img);
int image_imageimage_height(struct image_imageImage* img);
int image_imageimage_channels(struct image_imageImage* img);
uint8_t* image_imageimage_pixels(struct image_imageImage* img);
size_t image_imageimage_pixel_count(struct image_imageImage* img);
size_t image_imageimage_bytes_per_row(struct image_imageImage* img);
size_t image_imageimage_total_bytes(struct image_imageImage* img);
void image_imageimage_get_rgba(struct std_stdResult__cgs__39* __chx_struct_ret_param_xx, struct image_imageImage* img, int x, int y);
void image_imageimage_set_rgba(struct std_stdResult__cgs__40* __chx_struct_ret_param_xx, struct image_imageImage* img, int x, int y, struct image_imageRGBA8* color);
void image_imageimage_fill(struct image_imageImage* img, struct image_imageRGBA8* color);
void image_imageimage_copy(struct image_imageImage* __chx_struct_ret_param_xx, struct image_imageImage* src);
void image_imageimage_crop(struct std_stdResult__cgs__37* __chx_struct_ret_param_xx, struct image_imageImage* img, int x, int y, int w, int h);
void image_imageimage_flip_h(struct image_imageImage* img);
void image_imageimage_flip_v(struct image_imageImage* img);
void image_imageimage_rotate90(struct image_imageImage* __chx_struct_ret_param_xx, struct image_imageImage* img);
void image_imageimage_blit(struct std_stdResult__cgs__40* __chx_struct_ret_param_xx, struct image_imageImage* dst, struct image_imageImage* src, int dx, int dy);
void image_imageimage_line(struct image_imageImage* img, int x0, int y0, int x1, int y1, struct image_imageRGBA8* color);
void image_imageimage_rectangle(struct image_imageImage* img, int x, int y, int w, int h, struct image_imageRGBA8* color);
void image_imageimage_fill_rect(struct image_imageImage* img, int x, int y, int w, int h, struct image_imageRGBA8* color);
void image_imageimage_circle(struct image_imageImage* img, int cx, int cy, int radius, struct image_imageRGBA8* color);
void image_imageimage_fill_circle(struct image_imageImage* img, int cx, int cy, int radius, struct image_imageRGBA8* color);
/** Implement:Generics image **/
void std_stdResult__cgs__37delete(struct std_stdResult__cgs__37* self){
	__chx__dstctr_clnup_blk__:{
		switch(self->__chx__vt_621827) {
			case 0:
			image_imageImagedelete(&self->Ok.value);
			break;
			case 1:
			image_imageImageErrordelete(&self->Err.error);
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
			image_imageImageErrordelete(&self->Err.error);
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
			image_imageImageErrordelete(&self->Err.error);
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
			image_imageImageErrordelete(&self->Err.error);
			break;
		}
	}
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/image/src/bmp.ch **/
void image_imageload_bmp(struct std_stdResult__cgs__37* __chx_struct_ret_param_xx, const char* path){
	struct std_stdResult__cgs__35 read_result = (*({ struct std_stdResult__cgs__35 __chx__lv__0; fs_fsread_entire_file(&__chx__lv__0, path); &__chx__lv__0; }));
	_Bool __chx__lv__1 = true;
	if((read_result.__chx__vt_621827 == 1)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__37) { .__chx__vt_621827 = 1, 
			.Err.error = (struct image_imageImageError) { .__chx__vt_621827 = 0, 
			}
		};
		if(__chx__lv__1) {
			std_stdResult__cgs__35delete(&read_result);
		}
		return;
	}
	struct std_stdResult__cgs__35* __chx__lv__2 = &read_result;
	*__chx_struct_ret_param_xx = (*({ struct std_stdResult__cgs__37 __chx__lv__3; image_imageparse_bmp(&__chx__lv__3, std_stdvector__cgs__3data(&__chx__lv__2->Ok.value), std_stdvector__cgs__3size(&__chx__lv__2->Ok.value)); &__chx__lv__3; }));
	if(__chx__lv__1) {
		std_stdResult__cgs__35delete(&read_result);
	}
	return;
}
void image_imageparse_bmp(struct std_stdResult__cgs__37* __chx_struct_ret_param_xx, const uint8_t* data, size_t data_len){
	if((data_len < 54)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__37) { .__chx__vt_621827 = 1, 
			.Err.error = (struct image_imageImageError) { .__chx__vt_621827 = 1, 
				.InvalidFormat.msg = (*({ struct std_stdstring __chx__lv__4; std_stdstringconstructor2(&__chx__lv__4, "BMP file too small", 18, 0); &__chx__lv__4; }))
			}
		};
		return;
	}
	if(((data[0] != ((uint8_t) 'B')) || (data[1] != ((uint8_t) 'M')))){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__37) { .__chx__vt_621827 = 1, 
			.Err.error = (struct image_imageImageError) { .__chx__vt_621827 = 1, 
				.InvalidFormat.msg = (*({ struct std_stdstring __chx__lv__5; std_stdstringconstructor2(&__chx__lv__5, "invalid BMP signature", 21, 0); &__chx__lv__5; }))
			}
		};
		return;
	}
	size_t pixel_offset = ((size_t) image_imageread_u32_le(data, 10));
	size_t dib_size = ((size_t) image_imageread_u32_le(data, 14));
	if((dib_size < 40)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__37) { .__chx__vt_621827 = 1, 
			.Err.error = (struct image_imageImageError) { .__chx__vt_621827 = 2, 
				.UnsupportedFormat.msg = (*({ struct std_stdstring __chx__lv__6; std_stdstringconstructor2(&__chx__lv__6, "BMP DIB header too small", 24, 0); &__chx__lv__6; }))
			}
		};
		return;
	}
	int32_t width = image_imageread_i32_le(data, 18);
	int32_t height = image_imageread_i32_le(data, 22);
	uint16_t bits_per_pixel = image_imageread_u16_le(data, 28);
	uint32_t compression = image_imageread_u32_le(data, 30);
	if((compression != 0)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__37) { .__chx__vt_621827 = 1, 
			.Err.error = (struct image_imageImageError) { .__chx__vt_621827 = 2, 
				.UnsupportedFormat.msg = (*({ struct std_stdstring __chx__lv__7; std_stdstringconstructor2(&__chx__lv__7, "compressed BMP not supported", 28, 0); &__chx__lv__7; }))
			}
		};
		return;
	}
	int abs_height;
	_Bool top_down;
	if((height < 0)){
		abs_height = -height;
		top_down = 1;
	} else {
		abs_height = height;
		top_down = 0;
	}
	int channels = 0;
	if((bits_per_pixel == 24)){
		channels = 3;
	}else if((bits_per_pixel == 32)){
		channels = 4;
	}else if((bits_per_pixel == 8)){
		channels = 1;
	} else {
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__37) { .__chx__vt_621827 = 1, 
			.Err.error = (struct image_imageImageError) { .__chx__vt_621827 = 2, 
				.UnsupportedFormat.msg = (*({ struct std_stdstring __chx__lv__8; std_stdstringconstructor2(&__chx__lv__8, "unsupported bits per pixel", 26, 0); &__chx__lv__8; }))
			}
		};
		return;
	}
	struct image_imageImage img = (*({ struct image_imageImage __chx__lv__9; image_imageimage_create(&__chx__lv__9, width, abs_height, channels); &__chx__lv__9; }));
	_Bool __chx__lv__10 = true;
	int32_t row_size = ((((((int) bits_per_pixel) * width) + 31) / 32) * 4);
	int row = 0;
	while((row < abs_height)) {
		int src_row;
		if(top_down){
			src_row = row;
		} else {
			src_row = ((abs_height - 1) - row);
		}
		unsigned long src_offset = (pixel_offset + (((size_t) src_row) * ((size_t) row_size)));
		unsigned long dst_offset = ((((size_t) row) * ((size_t) width)) * ((size_t) channels));
		int col = 0;
		while((col < width)) {
			unsigned long px_src = (src_offset + (((size_t) col) * (((size_t) bits_per_pixel) / 8)));
			unsigned long px_dst = (dst_offset + (((size_t) col) * ((size_t) channels)));
			if((((px_src + 2) < data_len) && ((px_dst + 2) < std_stdvector__cgs__3size(&img.pixels)))){
				uint8_t* ptr = ((uint8_t*) std_stdvector__cgs__3data(&img.pixels));
				if((channels == 3)){
					ptr[px_dst] = data[(px_src + 2)];
					ptr[(px_dst + 1)] = data[(px_src + 1)];
					ptr[(px_dst + 2)] = data[px_src];
				}else if((channels == 4)){
					ptr[px_dst] = data[(px_src + 2)];
					ptr[(px_dst + 1)] = data[(px_src + 1)];
					ptr[(px_dst + 2)] = data[px_src];
					ptr[(px_dst + 3)] = data[(px_src + 3)];
				}else if((channels == 1)){
					ptr[px_dst] = data[px_src];
				}
			}
			col += 1;
		}
		row += 1;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__37) { .__chx__vt_621827 = 0, 
		.Ok.value = ({ __chx__lv__10 = false; img; })
	};
	if(__chx__lv__10) {
		image_imageImagedelete(&img);
	}
	return;
}
void image_imagesave_bmp(struct std_stdResult__cgs__40* __chx_struct_ret_param_xx, struct image_imageImage* img, const char* path){
	int bpp;
	if((img->channels == 4)){
		bpp = 32;
	}else if((img->channels == 3)){
		bpp = 24;
	} else {
		bpp = 8;
	}
	int row_size = ((((bpp * img->width) + 31) / 32) * 4);
	int pixel_data_size = (row_size * img->height);
	int file_size = (54 + pixel_data_size);
	struct std_stdvector__cgs__3 file_data = (*({ struct std_stdvector__cgs__3 __chx__lv__11; std_stdvector__cgs__3make(&__chx__lv__11); &__chx__lv__11; }));
	_Bool __chx__lv__12 = true;
	std_stdvector__cgs__3resize(&file_data, ((size_t) file_size));
	uint8_t* fptr = ((uint8_t*) std_stdvector__cgs__3data(&file_data));
	fptr[0] = ((uint8_t) 'B');
	fptr[1] = ((uint8_t) 'M');
	image_imagewrite_u32_le(fptr, 2, ((uint32_t) file_size));
	image_imagewrite_u32_le(fptr, 10, 54);
	image_imagewrite_u32_le(fptr, 14, 40);
	image_imagewrite_u32_le(fptr, 18, ((uint32_t) img->width));
	image_imagewrite_u32_le(fptr, 22, ((uint32_t) img->height));
	image_imagewrite_u16_le(fptr, 26, 1);
	image_imagewrite_u16_le(fptr, 28, ((uint16_t) bpp));
	image_imagewrite_u32_le(fptr, 30, 0);
	const uint8_t* pix_ptr = std_stdvector__cgs__3data(&img->pixels);
	int row = 0;
	while((row < img->height)) {
		int src_row = ((img->height - 1) - row);
		unsigned long src_offset = ((((size_t) src_row) * ((size_t) img->width)) * ((size_t) img->channels));
		unsigned long dst_offset = (54 + (((size_t) row) * ((size_t) row_size)));
		int col = 0;
		while((col < img->width)) {
			unsigned long px_src = (src_offset + (((size_t) col) * ((size_t) img->channels)));
			unsigned long px_dst = (dst_offset + (((size_t) col) * (((size_t) bpp) / 8)));
			if((img->channels >= 3)){
				fptr[px_dst] = pix_ptr[(px_src + 2)];
				fptr[(px_dst + 1)] = pix_ptr[(px_src + 1)];
				fptr[(px_dst + 2)] = pix_ptr[px_src];
				if((img->channels == 4)){
					fptr[(px_dst + 3)] = pix_ptr[(px_src + 3)];
				}
			} else {
				fptr[px_dst] = pix_ptr[px_src];
			}
			col += 1;
		}
		row += 1;
	}
	struct std_stdResult__cgs__33 write_result = (*({ struct std_stdResult__cgs__33 __chx__lv__13; fs_fswrite_text_file(&__chx__lv__13, path, std_stdvector__cgs__3data(&file_data), std_stdvector__cgs__3size(&file_data)); &__chx__lv__13; }));
	if((write_result.__chx__vt_621827 == 1)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__40) { .__chx__vt_621827 = 1, 
			.Err.error = (struct image_imageImageError) { .__chx__vt_621827 = 3, 
				.IoError.msg = (*({ struct std_stdstring __chx__lv__14; std_stdstringconstructor2(&__chx__lv__14, "failed to write BMP file", 24, 0); &__chx__lv__14; }))
			}
		};
		if(__chx__lv__12) {
			std_stdvector__cgs__3delete(&file_data);
		}
		return;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__40) { .__chx__vt_621827 = 0, 
		.Ok.value = (struct std_stdUnit){ 
		}
	};
	if(__chx__lv__12) {
		std_stdvector__cgs__3delete(&file_data);
	}
	return;
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/image/src/types.ch **/
void image_imageImageErrormessage(struct std_stdstring* __chx_struct_ret_param_xx, struct image_imageImageError*const self){
	switch((self)->__chx__vt_621827) {
		case 0:{
			*__chx_struct_ret_param_xx = (*({ struct std_stdstring __chx__lv__15; std_stdstringconstructor2(&__chx__lv__15, "ImageError: file not found", 26, 0); &__chx__lv__15; }));
			return;
			break;
		}
		case 1:{
			struct std_stdstring s = (*({ struct std_stdstring __chx__lv__16; std_stdstringconstructor2(&__chx__lv__16, "ImageError: ", 12, 0); &__chx__lv__16; }));
			_Bool __chx__lv__17 = true;
			std_stdstringappend_string(&s, &(self)->InvalidFormat.msg);
			*__chx_struct_ret_param_xx = s;
			return;
			break;
		}
		case 2:{
			struct std_stdstring s = (*({ struct std_stdstring __chx__lv__18; std_stdstringconstructor2(&__chx__lv__18, "ImageError: unsupported format: ", 32, 0); &__chx__lv__18; }));
			_Bool __chx__lv__19 = true;
			std_stdstringappend_string(&s, &(self)->UnsupportedFormat.msg);
			*__chx_struct_ret_param_xx = s;
			return;
			break;
		}
		case 3:{
			struct std_stdstring s = (*({ struct std_stdstring __chx__lv__20; std_stdstringconstructor2(&__chx__lv__20, "ImageError: IO error: ", 22, 0); &__chx__lv__20; }));
			_Bool __chx__lv__21 = true;
			std_stdstringappend_string(&s, &(self)->IoError.msg);
			*__chx_struct_ret_param_xx = s;
			return;
			break;
		}
		case 4:{
			struct std_stdstring s = (*({ struct std_stdstring __chx__lv__22; std_stdstringconstructor2(&__chx__lv__22, "ImageError: invalid dimensions ", 31, 0); &__chx__lv__22; }));
			_Bool __chx__lv__23 = true;
			std_stdstringappend_integer(&s, (self)->InvalidDimensions.w);
			std_stdstringappend(&s, 'x');
			std_stdstringappend_integer(&s, (self)->InvalidDimensions.h);
			*__chx_struct_ret_param_xx = s;
			return;
			break;
		}
		case 5:{
			struct std_stdstring s = (*({ struct std_stdstring __chx__lv__24; std_stdstringconstructor2(&__chx__lv__24, "ImageError: pixel out of bounds (", 33, 0); &__chx__lv__24; }));
			_Bool __chx__lv__25 = true;
			std_stdstringappend_integer(&s, (self)->PixelOutOfBounds.x);
			std_stdstringappend(&s, ',');
			std_stdstringappend_integer(&s, (self)->PixelOutOfBounds.y);
			std_stdstringappend(&s, ')');
			*__chx_struct_ret_param_xx = s;
			return;
			break;
		}
	}
}
void image_imageImageErrordelete(struct image_imageImageError* self){
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
			case 5:
			break;
		}
	}
}
void image_imageRGBA8make(struct image_imageRGBA8* this, uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_){
	*this = (struct image_imageRGBA8){ 
		.r = r_, 
		.g = g_, 
		.b = b_, 
		.a = a_
	};
	return;
}
void image_imageImagemake(struct image_imageImage* this){
	*this = (struct image_imageImage){ 
		.pixels = (*({ struct std_stdvector__cgs__3 __chx__lv__26; std_stdvector__cgs__3make(&__chx__lv__26); &__chx__lv__26; })), 
		.width = 0, 
		.height = 0, 
		.channels = 0
	};
	return;
}
void image_imageImagedelete(struct image_imageImage* self){
	__chx__dstctr_clnup_blk__:{
		std_stdvector__cgs__3delete(&self->pixels);
	}
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/image/src/ppm.ch **/
void image_imageload_ppm(struct std_stdResult__cgs__37* __chx_struct_ret_param_xx, const char* path){
	struct std_stdResult__cgs__35 read_result = (*({ struct std_stdResult__cgs__35 __chx__lv__27; fs_fsread_entire_file(&__chx__lv__27, path); &__chx__lv__27; }));
	_Bool __chx__lv__28 = true;
	if((read_result.__chx__vt_621827 == 1)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__37) { .__chx__vt_621827 = 1, 
			.Err.error = (struct image_imageImageError) { .__chx__vt_621827 = 0, 
			}
		};
		if(__chx__lv__28) {
			std_stdResult__cgs__35delete(&read_result);
		}
		return;
	}
	struct std_stdResult__cgs__35* __chx__lv__29 = &read_result;
	*__chx_struct_ret_param_xx = (*({ struct std_stdResult__cgs__37 __chx__lv__30; image_imageparse_ppm(&__chx__lv__30, std_stdvector__cgs__3data(&__chx__lv__29->Ok.value), std_stdvector__cgs__3size(&__chx__lv__29->Ok.value)); &__chx__lv__30; }));
	if(__chx__lv__28) {
		std_stdResult__cgs__35delete(&read_result);
	}
	return;
}
static _Bool image_imageis_digit(uint8_t c){
	return ((c >= ((uint8_t) '0')) && (c <= ((uint8_t) '9')));
}
static void image_imageparse_ppm_number(struct std_stdResult__cgs__38* __chx_struct_ret_param_xx, const uint8_t* data, size_t* pos, size_t data_len){
	int val = 0;
	_Bool found = 0;
	while((pos[0] < data_len)) {
		uint8_t c = data[pos[0]];
		if(image_imageis_digit(c)){
			val = ((val * 10) + ((int) (c - ((uint8_t) '0'))));
			found = 1;
			pos[0] += 1;
		}else if(found){
			break;
		} else {
			pos[0] += 1;
		}
	}
	if(!found){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 1, 
			.Err.error = (struct image_imageImageError) { .__chx__vt_621827 = 1, 
				.InvalidFormat.msg = (*({ struct std_stdstring __chx__lv__31; std_stdstringconstructor2(&__chx__lv__31, "expected number in PPM", 22, 0); &__chx__lv__31; }))
			}
		};
		return;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__38) { .__chx__vt_621827 = 0, 
		.Ok.value = val
	};
	return;
}
void image_imageparse_ppm(struct std_stdResult__cgs__37* __chx_struct_ret_param_xx, const uint8_t* data, size_t data_len){
	if((data_len < 3)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__37) { .__chx__vt_621827 = 1, 
			.Err.error = (struct image_imageImageError) { .__chx__vt_621827 = 1, 
				.InvalidFormat.msg = (*({ struct std_stdstring __chx__lv__32; std_stdstringconstructor2(&__chx__lv__32, "PPM file too small", 18, 0); &__chx__lv__32; }))
			}
		};
		return;
	}
	if((data[0] != ((uint8_t) 'P'))){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__37) { .__chx__vt_621827 = 1, 
			.Err.error = (struct image_imageImageError) { .__chx__vt_621827 = 1, 
				.InvalidFormat.msg = (*({ struct std_stdstring __chx__lv__33; std_stdstringconstructor2(&__chx__lv__33, "invalid PPM signature", 21, 0); &__chx__lv__33; }))
			}
		};
		return;
	}
	uint8_t format = data[1];
	if(((format != ((uint8_t) '6')) && (format != ((uint8_t) '3')))){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__37) { .__chx__vt_621827 = 1, 
			.Err.error = (struct image_imageImageError) { .__chx__vt_621827 = 2, 
				.UnsupportedFormat.msg = (*({ struct std_stdstring __chx__lv__34; std_stdstringconstructor2(&__chx__lv__34, "only P3 and P6 PPM supported", 28, 0); &__chx__lv__34; }))
			}
		};
		return;
	}
	size_t pos = 2;
	struct std_stdResult__cgs__38 width = (*({ struct std_stdResult__cgs__38 __chx__lv__35; image_imageparse_ppm_number(&__chx__lv__35, data, &pos, data_len); &__chx__lv__35; }));
	_Bool __chx__lv__36 = true;
	if((width.__chx__vt_621827 == 1)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__37) { .__chx__vt_621827 = 1, 
			.Err.error = (struct image_imageImageError) { .__chx__vt_621827 = 1, 
				.InvalidFormat.msg = (*({ struct std_stdstring __chx__lv__37; std_stdstringconstructor2(&__chx__lv__37, "invalid width in PPM", 20, 0); &__chx__lv__37; }))
			}
		};
		if(__chx__lv__36) {
			std_stdResult__cgs__38delete(&width);
		}
		return;
	}
	struct std_stdResult__cgs__38 height = (*({ struct std_stdResult__cgs__38 __chx__lv__38; image_imageparse_ppm_number(&__chx__lv__38, data, &pos, data_len); &__chx__lv__38; }));
	_Bool __chx__lv__39 = true;
	if((height.__chx__vt_621827 == 1)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__37) { .__chx__vt_621827 = 1, 
			.Err.error = (struct image_imageImageError) { .__chx__vt_621827 = 1, 
				.InvalidFormat.msg = (*({ struct std_stdstring __chx__lv__40; std_stdstringconstructor2(&__chx__lv__40, "invalid height in PPM", 21, 0); &__chx__lv__40; }))
			}
		};
		if(__chx__lv__39) {
			std_stdResult__cgs__38delete(&height);
		}
		if(__chx__lv__36) {
			std_stdResult__cgs__38delete(&width);
		}
		return;
	}
	struct std_stdResult__cgs__38 max_val = (*({ struct std_stdResult__cgs__38 __chx__lv__41; image_imageparse_ppm_number(&__chx__lv__41, data, &pos, data_len); &__chx__lv__41; }));
	_Bool __chx__lv__42 = true;
	if((max_val.__chx__vt_621827 == 1)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__37) { .__chx__vt_621827 = 1, 
			.Err.error = (struct image_imageImageError) { .__chx__vt_621827 = 1, 
				.InvalidFormat.msg = (*({ struct std_stdstring __chx__lv__43; std_stdstringconstructor2(&__chx__lv__43, "invalid max_val in PPM", 22, 0); &__chx__lv__43; }))
			}
		};
		if(__chx__lv__42) {
			std_stdResult__cgs__38delete(&max_val);
		}
		if(__chx__lv__39) {
			std_stdResult__cgs__38delete(&height);
		}
		if(__chx__lv__36) {
			std_stdResult__cgs__38delete(&width);
		}
		return;
	}
	struct std_stdResult__cgs__38* __chx__lv__44 = &width;
	struct std_stdResult__cgs__38* __chx__lv__45 = &height;
	if((format == ((uint8_t) '6'))){
		pos += 1;
		struct image_imageImage img = (*({ struct image_imageImage __chx__lv__46; image_imageimage_create(&__chx__lv__46, __chx__lv__44->Ok.value, __chx__lv__45->Ok.value, 3); &__chx__lv__46; }));
		_Bool __chx__lv__47 = true;
		unsigned long total = ((((size_t) __chx__lv__44->Ok.value) * ((size_t) __chx__lv__45->Ok.value)) * 3);
		uint8_t* ptr = ((uint8_t*) std_stdvector__cgs__3data(&img.pixels));
		size_t i = 0;
		while(((i < total) && ((pos + i) < data_len))) {
			ptr[i] = data[(pos + i)];
			i += 1;
		}
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__37) { .__chx__vt_621827 = 0, 
			.Ok.value = ({ __chx__lv__47 = false; img; })
		};
		if(__chx__lv__47) {
			image_imageImagedelete(&img);
		}
		if(__chx__lv__42) {
			std_stdResult__cgs__38delete(&max_val);
		}
		if(__chx__lv__39) {
			std_stdResult__cgs__38delete(&height);
		}
		if(__chx__lv__36) {
			std_stdResult__cgs__38delete(&width);
		}
		return;
	} else {
		struct image_imageImage img = (*({ struct image_imageImage __chx__lv__48; image_imageimage_create(&__chx__lv__48, __chx__lv__44->Ok.value, __chx__lv__45->Ok.value, 3); &__chx__lv__48; }));
		_Bool __chx__lv__49 = true;
		unsigned long pixel_count = (((size_t) __chx__lv__44->Ok.value) * ((size_t) __chx__lv__45->Ok.value));
		uint8_t* ptr = ((uint8_t*) std_stdvector__cgs__3data(&img.pixels));
		struct std_stdResult__cgs__38* __chx__lv__50 = &max_val;
		size_t i = 0;
		while((i < pixel_count)) {
			struct std_stdResult__cgs__38 r = (*({ struct std_stdResult__cgs__38 __chx__lv__51; image_imageparse_ppm_number(&__chx__lv__51, data, &pos, data_len); &__chx__lv__51; }));
			_Bool __chx__lv__52 = true;
			struct std_stdResult__cgs__38 g = (*({ struct std_stdResult__cgs__38 __chx__lv__53; image_imageparse_ppm_number(&__chx__lv__53, data, &pos, data_len); &__chx__lv__53; }));
			_Bool __chx__lv__54 = true;
			struct std_stdResult__cgs__38 b = (*({ struct std_stdResult__cgs__38 __chx__lv__55; image_imageparse_ppm_number(&__chx__lv__55, data, &pos, data_len); &__chx__lv__55; }));
			_Bool __chx__lv__56 = true;
			if((((r.__chx__vt_621827 == 1) || (g.__chx__vt_621827 == 1)) || (b.__chx__vt_621827 == 1))){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__37) { .__chx__vt_621827 = 1, 
					.Err.error = (struct image_imageImageError) { .__chx__vt_621827 = 1, 
						.InvalidFormat.msg = (*({ struct std_stdstring __chx__lv__57; std_stdstringconstructor2(&__chx__lv__57, "truncated PPM data", 18, 0); &__chx__lv__57; }))
					}
				};
				if(__chx__lv__56) {
					std_stdResult__cgs__38delete(&b);
				}
				if(__chx__lv__54) {
					std_stdResult__cgs__38delete(&g);
				}
				if(__chx__lv__52) {
					std_stdResult__cgs__38delete(&r);
				}
				if(__chx__lv__49) {
					image_imageImagedelete(&img);
				}
				if(__chx__lv__42) {
					std_stdResult__cgs__38delete(&max_val);
				}
				if(__chx__lv__39) {
					std_stdResult__cgs__38delete(&height);
				}
				if(__chx__lv__36) {
					std_stdResult__cgs__38delete(&width);
				}
				return;
			}
			struct std_stdResult__cgs__38* __chx__lv__58 = &r;
			struct std_stdResult__cgs__38* __chx__lv__59 = &g;
			struct std_stdResult__cgs__38* __chx__lv__60 = &b;
			if((__chx__lv__50->Ok.value > 255)){
				__chx__lv__58->Ok.value = ((__chx__lv__58->Ok.value * 255) / __chx__lv__50->Ok.value);
				__chx__lv__59->Ok.value = ((__chx__lv__59->Ok.value * 255) / __chx__lv__50->Ok.value);
				__chx__lv__60->Ok.value = ((__chx__lv__60->Ok.value * 255) / __chx__lv__50->Ok.value);
			}
			ptr[(i * 3)] = ((uint8_t) __chx__lv__58->Ok.value);
			ptr[((i * 3) + 1)] = ((uint8_t) __chx__lv__59->Ok.value);
			ptr[((i * 3) + 2)] = ((uint8_t) __chx__lv__60->Ok.value);
			i += 1;
			if(__chx__lv__56) {
				std_stdResult__cgs__38delete(&b);
			}
			if(__chx__lv__54) {
				std_stdResult__cgs__38delete(&g);
			}
			if(__chx__lv__52) {
				std_stdResult__cgs__38delete(&r);
			}
		}
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__37) { .__chx__vt_621827 = 0, 
			.Ok.value = ({ __chx__lv__49 = false; img; })
		};
		if(__chx__lv__49) {
			image_imageImagedelete(&img);
		}
		if(__chx__lv__42) {
			std_stdResult__cgs__38delete(&max_val);
		}
		if(__chx__lv__39) {
			std_stdResult__cgs__38delete(&height);
		}
		if(__chx__lv__36) {
			std_stdResult__cgs__38delete(&width);
		}
		return;
	}
	if(__chx__lv__42) {
		std_stdResult__cgs__38delete(&max_val);
	}
	if(__chx__lv__39) {
		std_stdResult__cgs__38delete(&height);
	}
	if(__chx__lv__36) {
		std_stdResult__cgs__38delete(&width);
	}
}
void image_imagesave_ppm(struct std_stdResult__cgs__40* __chx_struct_ret_param_xx, struct image_imageImage* img, const char* path){
	if((img->channels != 3)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__40) { .__chx__vt_621827 = 1, 
			.Err.error = (struct image_imageImageError) { .__chx__vt_621827 = 1, 
				.InvalidFormat.msg = (*({ struct std_stdstring __chx__lv__61; std_stdstringconstructor2(&__chx__lv__61, "PPM requires 3 channels (RGB)", 29, 0); &__chx__lv__61; }))
			}
		};
		return;
	}
	struct std_stdstring header = (*({ struct std_stdstring __chx__lv__62; std_stdstringconstructor2(&__chx__lv__62, "", 0, 0); &__chx__lv__62; }));
	_Bool __chx__lv__63 = true;
	std_stdstringappend(&header, 'P');
	std_stdstringappend(&header, '6');
	std_stdstringappend(&header, ' ');
	std_stdstringappend_integer(&header, img->width);
	std_stdstringappend(&header, ' ');
	std_stdstringappend_integer(&header, img->height);
	std_stdstringappend(&header, ' ');
	std_stdstringappend_integer(&header, 255);
	std_stdstringappend(&header, '\n');
	unsigned long total_size = (std_stdstringsize(&header) + std_stdvector__cgs__3size(&img->pixels));
	struct std_stdvector__cgs__3 file_data = (*({ struct std_stdvector__cgs__3 __chx__lv__64; std_stdvector__cgs__3make(&__chx__lv__64); &__chx__lv__64; }));
	_Bool __chx__lv__65 = true;
	std_stdvector__cgs__3resize(&file_data, total_size);
	uint8_t* fptr = ((uint8_t*) std_stdvector__cgs__3data(&file_data));
	size_t i = 0;
	while((i < std_stdstringsize(&header))) {
		fptr[i] = ((uint8_t) std_stdstringget(&header, i));
		i += 1;
	}
	size_t pix_bytes = image_imageimage_total_bytes(img);
	const uint8_t* pix_ptr = std_stdvector__cgs__3data(&img->pixels);
	size_t j = 0;
	while((j < pix_bytes)) {
		fptr[(std_stdstringsize(&header) + j)] = pix_ptr[j];
		j += 1;
	}
	struct std_stdResult__cgs__33 write_result = (*({ struct std_stdResult__cgs__33 __chx__lv__66; fs_fswrite_text_file(&__chx__lv__66, path, std_stdvector__cgs__3data(&file_data), std_stdvector__cgs__3size(&file_data)); &__chx__lv__66; }));
	if((write_result.__chx__vt_621827 == 1)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__40) { .__chx__vt_621827 = 1, 
			.Err.error = (struct image_imageImageError) { .__chx__vt_621827 = 3, 
				.IoError.msg = (*({ struct std_stdstring __chx__lv__67; std_stdstringconstructor2(&__chx__lv__67, "failed to write PPM file", 24, 0); &__chx__lv__67; }))
			}
		};
		if(__chx__lv__65) {
			std_stdvector__cgs__3delete(&file_data);
		}
		if(__chx__lv__63) {
			std_stdstringdelete(&header);
		}
		return;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__40) { .__chx__vt_621827 = 0, 
		.Ok.value = (struct std_stdUnit){ 
		}
	};
	if(__chx__lv__65) {
		std_stdvector__cgs__3delete(&file_data);
	}
	if(__chx__lv__63) {
		std_stdstringdelete(&header);
	}
	return;
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/image/src/core.ch **/
static uint16_t image_imageread_u16_be(const uint8_t* data, size_t offset){
	return ((((uint16_t) data[offset]) << 8) | ((uint16_t) data[(offset + 1)]));
}
static uint32_t image_imageread_u32_be(const uint8_t* data, size_t offset){
	return ((((((uint32_t) data[offset]) << 24) | (((uint32_t) data[(offset + 1)]) << 16)) | (((uint32_t) data[(offset + 2)]) << 8)) | ((uint32_t) data[(offset + 3)]));
}
static uint16_t image_imageread_u16_le(const uint8_t* data, size_t offset){
	return (((uint16_t) data[offset]) | (((uint16_t) data[(offset + 1)]) << 8));
}
static uint32_t image_imageread_u32_le(const uint8_t* data, size_t offset){
	return (((((uint32_t) data[offset]) | (((uint32_t) data[(offset + 1)]) << 8)) | (((uint32_t) data[(offset + 2)]) << 16)) | (((uint32_t) data[(offset + 3)]) << 24));
}
static int32_t image_imageread_i32_le(const uint8_t* data, size_t offset){
	uint32_t val = (((((uint32_t) data[offset]) | (((uint32_t) data[(offset + 1)]) << 8)) | (((uint32_t) data[(offset + 2)]) << 16)) | (((uint32_t) data[(offset + 3)]) << 24));
	return ((int32_t) val);
}
static void image_imagewrite_u16_le(uint8_t* out, size_t offset, uint16_t val){
	out[offset] = ((uint8_t) (val & 255));
	out[(offset + 1)] = ((uint8_t) ((val >> 8) & 255));
}
static void image_imagewrite_u32_le(uint8_t* out, size_t offset, uint32_t val){
	out[offset] = ((uint8_t) (val & 255));
	out[(offset + 1)] = ((uint8_t) ((val >> 8) & 255));
	out[(offset + 2)] = ((uint8_t) ((val >> 16) & 255));
	out[(offset + 3)] = ((uint8_t) ((val >> 24) & 255));
}
void image_imageimage_create(struct image_imageImage* __chx_struct_ret_param_xx, int w, int h, int ch){
	struct image_imageImage img = (struct image_imageImage){ 
		.pixels = (*({ struct std_stdvector__cgs__3 __chx__lv__68; std_stdvector__cgs__3make(&__chx__lv__68); &__chx__lv__68; })), 
		.width = 0, 
		.height = 0, 
		.channels = 0
	};
	_Bool __chx__lv__69 = true;
	img.width = w;
	img.height = h;
	img.channels = ch;
	unsigned long total = ((((size_t) w) * ((size_t) h)) * ((size_t) ch));
	std_stdvector__cgs__3resize(&img.pixels, total);
	uint8_t* ptr = ((uint8_t*) std_stdvector__cgs__3data(&img.pixels));
	size_t i = 0;
	while((i < total)) {
		ptr[i] = 0;
		i += 1;
	}
	*__chx_struct_ret_param_xx = img;
	return;
}
void image_imageimage_create_rgba(struct image_imageImage* __chx_struct_ret_param_xx, int w, int h){
	*__chx_struct_ret_param_xx = (*({ struct image_imageImage __chx__lv__70; image_imageimage_create(&__chx__lv__70, w, h, 4); &__chx__lv__70; }));
	return;
}
void image_imageimage_create_rgb(struct image_imageImage* __chx_struct_ret_param_xx, int w, int h){
	*__chx_struct_ret_param_xx = (*({ struct image_imageImage __chx__lv__71; image_imageimage_create(&__chx__lv__71, w, h, 3); &__chx__lv__71; }));
	return;
}
void image_imageimage_create_gray(struct image_imageImage* __chx_struct_ret_param_xx, int w, int h){
	*__chx_struct_ret_param_xx = (*({ struct image_imageImage __chx__lv__72; image_imageimage_create(&__chx__lv__72, w, h, 1); &__chx__lv__72; }));
	return;
}
int image_imageimage_width(struct image_imageImage* img){
	return img->width;
}
int image_imageimage_height(struct image_imageImage* img){
	return img->height;
}
int image_imageimage_channels(struct image_imageImage* img){
	return img->channels;
}
uint8_t* image_imageimage_pixels(struct image_imageImage* img){
	return ((uint8_t*) std_stdvector__cgs__3data(&img->pixels));
}
size_t image_imageimage_pixel_count(struct image_imageImage* img){
	return (((size_t) img->width) * ((size_t) img->height));
}
size_t image_imageimage_bytes_per_row(struct image_imageImage* img){
	return (((size_t) img->width) * ((size_t) img->channels));
}
size_t image_imageimage_total_bytes(struct image_imageImage* img){
	return (image_imageimage_bytes_per_row(img) * ((size_t) img->height));
}
void image_imageimage_get_rgba(struct std_stdResult__cgs__39* __chx_struct_ret_param_xx, struct image_imageImage* img, int x, int y){
	if(((((x < 0) || (x >= img->width)) || (y < 0)) || (y >= img->height))){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__39) { .__chx__vt_621827 = 1, 
			.Err.error = (struct image_imageImageError) { .__chx__vt_621827 = 5, 
				.PixelOutOfBounds.x = x, 
				.PixelOutOfBounds.y = y
			}
		};
		return;
	}
	if((img->channels < 4)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__39) { .__chx__vt_621827 = 1, 
			.Err.error = (struct image_imageImageError) { .__chx__vt_621827 = 1, 
				.InvalidFormat.msg = (*({ struct std_stdstring __chx__lv__73; std_stdstringconstructor2(&__chx__lv__73, "image does not have alpha channel", 33, 0); &__chx__lv__73; }))
			}
		};
		return;
	}
	unsigned long offset = (((((size_t) y) * ((size_t) img->width)) + ((size_t) x)) * 4);
	const uint8_t* px = (std_stdvector__cgs__3data(&img->pixels) + offset);
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__39) { .__chx__vt_621827 = 0, 
		.Ok.value = (*({ struct image_imageRGBA8 __chx__lv__74; image_imageRGBA8make(&__chx__lv__74, px[0], px[1], px[2], px[3]); &__chx__lv__74; }))
	};
	return;
}
void image_imageimage_set_rgba(struct std_stdResult__cgs__40* __chx_struct_ret_param_xx, struct image_imageImage* img, int x, int y, struct image_imageRGBA8* color){
	if(((((x < 0) || (x >= img->width)) || (y < 0)) || (y >= img->height))){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__40) { .__chx__vt_621827 = 1, 
			.Err.error = (struct image_imageImageError) { .__chx__vt_621827 = 5, 
				.PixelOutOfBounds.x = x, 
				.PixelOutOfBounds.y = y
			}
		};
		return;
	}
	if((img->channels < 4)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__40) { .__chx__vt_621827 = 1, 
			.Err.error = (struct image_imageImageError) { .__chx__vt_621827 = 1, 
				.InvalidFormat.msg = (*({ struct std_stdstring __chx__lv__75; std_stdstringconstructor2(&__chx__lv__75, "image does not have alpha channel", 33, 0); &__chx__lv__75; }))
			}
		};
		return;
	}
	unsigned long offset = (((((size_t) y) * ((size_t) img->width)) + ((size_t) x)) * 4);
	uint8_t* ptr = ((uint8_t*) std_stdvector__cgs__3data(&img->pixels));
	ptr[offset] = color->r;
	ptr[(offset + 1)] = color->g;
	ptr[(offset + 2)] = color->b;
	ptr[(offset + 3)] = color->a;
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__40) { .__chx__vt_621827 = 0, 
		.Ok.value = (struct std_stdUnit){ 
		}
	};
	return;
}
void image_imageimage_fill(struct image_imageImage* img, struct image_imageRGBA8* color){
	size_t bpp = ((size_t) img->channels);
	uint8_t* ptr = ((uint8_t*) std_stdvector__cgs__3data(&img->pixels));
	size_t i = 0;
	while((i < std_stdvector__cgs__3size(&img->pixels))) {
		if((bpp >= 1)){
			ptr[i] = color->r;
		}
		if((bpp >= 3)){
			ptr[(i + 1)] = color->g;
			ptr[(i + 2)] = color->b;
		}
		if((bpp >= 4)){
			ptr[(i + 3)] = color->a;
		}
		i += bpp;
	}
}
void image_imageimage_copy(struct image_imageImage* __chx_struct_ret_param_xx, struct image_imageImage* src){
	struct image_imageImage dst = (*({ struct image_imageImage __chx__lv__76; image_imageimage_create(&__chx__lv__76, src->width, src->height, src->channels); &__chx__lv__76; }));
	_Bool __chx__lv__77 = true;
	size_t total = image_imageimage_total_bytes(src);
	uint8_t* dst_ptr = ((uint8_t*) std_stdvector__cgs__3data(&dst.pixels));
	const uint8_t* src_ptr = std_stdvector__cgs__3data(&src->pixels);
	size_t i = 0;
	while((i < total)) {
		dst_ptr[i] = src_ptr[i];
		i += 1;
	}
	*__chx_struct_ret_param_xx = dst;
	return;
}
void image_imageimage_crop(struct std_stdResult__cgs__37* __chx_struct_ret_param_xx, struct image_imageImage* img, int x, int y, int w, int h){
	if(((((x < 0) || (y < 0)) || ((x + w) > img->width)) || ((y + h) > img->height))){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__37) { .__chx__vt_621827 = 1, 
			.Err.error = (struct image_imageImageError) { .__chx__vt_621827 = 4, 
				.InvalidDimensions.w = w, 
				.InvalidDimensions.h = h
			}
		};
		return;
	}
	struct image_imageImage cropped = (*({ struct image_imageImage __chx__lv__78; image_imageimage_create(&__chx__lv__78, w, h, img->channels); &__chx__lv__78; }));
	_Bool __chx__lv__79 = true;
	size_t bpp = ((size_t) img->channels);
	unsigned long row_bytes = (((size_t) w) * bpp);
	const uint8_t* src_ptr = std_stdvector__cgs__3data(&img->pixels);
	uint8_t* dst_ptr = ((uint8_t*) std_stdvector__cgs__3data(&cropped.pixels));
	int dst_y = 0;
	while((dst_y < h)) {
		unsigned long src_offset = (((((size_t) (y + dst_y)) * ((size_t) img->width)) + ((size_t) x)) * bpp);
		unsigned long dst_offset = (((size_t) dst_y) * row_bytes);
		size_t col = 0;
		while((col < row_bytes)) {
			dst_ptr[(dst_offset + col)] = src_ptr[(src_offset + col)];
			col += 1;
		}
		dst_y += 1;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__37) { .__chx__vt_621827 = 0, 
		.Ok.value = ({ __chx__lv__79 = false; cropped; })
	};
	if(__chx__lv__79) {
		image_imageImagedelete(&cropped);
	}
	return;
}
void image_imageimage_flip_h(struct image_imageImage* img){
	size_t bpp = ((size_t) img->channels);
	unsigned long row_bytes = (((size_t) img->width) * bpp);
	uint8_t* ptr = ((uint8_t*) std_stdvector__cgs__3data(&img->pixels));
	int row = 0;
	while((row < img->height)) {
		unsigned long row_start = (((size_t) row) * row_bytes);
		size_t left = 0;
		unsigned long right = (row_bytes - bpp);
		while((left < right)) {
			size_t i = 0;
			while((i < bpp)) {
				uint8_t tmp = ptr[((row_start + left) + i)];
				ptr[((row_start + left) + i)] = ptr[((row_start + right) + i)];
				ptr[((row_start + right) + i)] = tmp;
				i += 1;
			}
			left += bpp;
			right -= bpp;
		}
		row += 1;
	}
}
void image_imageimage_flip_v(struct image_imageImage* img){
	size_t bpp = ((size_t) img->channels);
	unsigned long row_bytes = (((size_t) img->width) * bpp);
	int half_h = (img->height / 2);
	uint8_t* ptr = ((uint8_t*) std_stdvector__cgs__3data(&img->pixels));
	int row = 0;
	while((row < half_h)) {
		unsigned long top_start = (((size_t) row) * row_bytes);
		unsigned long bot_start = (((size_t) ((img->height - 1) - row)) * row_bytes);
		size_t col = 0;
		while((col < row_bytes)) {
			uint8_t tmp = ptr[(top_start + col)];
			ptr[(top_start + col)] = ptr[(bot_start + col)];
			ptr[(bot_start + col)] = tmp;
			col += 1;
		}
		row += 1;
	}
}
void image_imageimage_rotate90(struct image_imageImage* __chx_struct_ret_param_xx, struct image_imageImage* img){
	struct image_imageImage rotated = (*({ struct image_imageImage __chx__lv__80; image_imageimage_create(&__chx__lv__80, img->height, img->width, img->channels); &__chx__lv__80; }));
	_Bool __chx__lv__81 = true;
	size_t bpp = ((size_t) img->channels);
	size_t src_w = ((size_t) img->width);
	size_t src_h = ((size_t) img->height);
	const uint8_t* src_ptr = std_stdvector__cgs__3data(&img->pixels);
	uint8_t* dst_ptr = ((uint8_t*) std_stdvector__cgs__3data(&rotated.pixels));
	size_t y = 0;
	while((y < src_h)) {
		size_t x = 0;
		while((x < src_w)) {
			unsigned long src_off = (((y * src_w) + x) * bpp);
			unsigned long dst_off = (((x * src_h) + ((src_h - 1) - y)) * bpp);
			size_t i = 0;
			while((i < bpp)) {
				dst_ptr[(dst_off + i)] = src_ptr[(src_off + i)];
				i += 1;
			}
			x += 1;
		}
		y += 1;
	}
	*__chx_struct_ret_param_xx = rotated;
	return;
}
void image_imageimage_blit(struct std_stdResult__cgs__40* __chx_struct_ret_param_xx, struct image_imageImage* dst, struct image_imageImage* src, int dx, int dy){
	size_t bpp = ((size_t) src->channels);
	if((dst->channels != src->channels)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__40) { .__chx__vt_621827 = 1, 
			.Err.error = (struct image_imageImageError) { .__chx__vt_621827 = 1, 
				.InvalidFormat.msg = (*({ struct std_stdstring __chx__lv__82; std_stdstringconstructor2(&__chx__lv__82, "channel count mismatch for blit", 31, 0); &__chx__lv__82; }))
			}
		};
		return;
	}
	const uint8_t* src_ptr = std_stdvector__cgs__3data(&src->pixels);
	uint8_t* dst_ptr = ((uint8_t*) std_stdvector__cgs__3data(&dst->pixels));
	int y = 0;
	while((y < src->height)) {
		int x = 0;
		while((x < src->width)) {
			int dst_x = (dx + x);
			int dst_y = (dy + y);
			if(((((dst_x >= 0) && (dst_x < dst->width)) && (dst_y >= 0)) && (dst_y < dst->height))){
				unsigned long src_off = (((((size_t) y) * ((size_t) src->width)) + ((size_t) x)) * bpp);
				unsigned long dst_off = (((((size_t) dst_y) * ((size_t) dst->width)) + ((size_t) dst_x)) * bpp);
				size_t i = 0;
				while((i < bpp)) {
					dst_ptr[(dst_off + i)] = src_ptr[(src_off + i)];
					i += 1;
				}
			}
			x += 1;
		}
		y += 1;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__40) { .__chx__vt_621827 = 0, 
		.Ok.value = (struct std_stdUnit){ 
		}
	};
	return;
}
void image_imageimage_line(struct image_imageImage* img, int x0, int y0, int x1, int y1, struct image_imageRGBA8* color){
	int dx = (x1 - x0);
	int dy = (y1 - y0);
	int sx = 1;
	int sy = 1;
	if((dx < 0)){
		dx = -dx;
		sx = -1;
	}
	if((dy < 0)){
		dy = -dy;
		sy = -1;
	}
	int err = (dx - dy);
	int x = x0;
	int y = y0;
	while(1) {
		if(((((x >= 0) && (x < img->width)) && (y >= 0)) && (y < img->height))){
			struct std_stdResult__cgs__40 __chx__lv__83 = (*({ struct std_stdResult__cgs__40 __chx__lv__84; image_imageimage_set_rgba(&__chx__lv__84, img, x, y, ({ struct image_imageRGBA8 __chx__lv__85 = *color; &__chx__lv__85; })); &__chx__lv__84; }));
			std_stdResult__cgs__40delete(&__chx__lv__83);
		}
		if(((x == x1) && (y == y1))){
			break;
		}
		int e2 = (err * 2);
		if((e2 > -dy)){
			err -= dy;
			x += sx;
		}
		if((e2 < dx)){
			err += dx;
			y += sy;
		}
	}
}
void image_imageimage_rectangle(struct image_imageImage* img, int x, int y, int w, int h, struct image_imageRGBA8* color){
	image_imageimage_line(img, x, y, ((x + w) - 1), y, ({ struct image_imageRGBA8 __chx__lv__86 = *color; &__chx__lv__86; }));
	image_imageimage_line(img, ((x + w) - 1), y, ((x + w) - 1), ((y + h) - 1), ({ struct image_imageRGBA8 __chx__lv__87 = *color; &__chx__lv__87; }));
	image_imageimage_line(img, ((x + w) - 1), ((y + h) - 1), x, ((y + h) - 1), ({ struct image_imageRGBA8 __chx__lv__88 = *color; &__chx__lv__88; }));
	image_imageimage_line(img, x, ((y + h) - 1), x, y, ({ struct image_imageRGBA8 __chx__lv__89 = *color; &__chx__lv__89; }));
}
void image_imageimage_fill_rect(struct image_imageImage* img, int x, int y, int w, int h, struct image_imageRGBA8* color){
	size_t bpp = ((size_t) img->channels);
	uint8_t* ptr = ((uint8_t*) std_stdvector__cgs__3data(&img->pixels));
	int row = y;
	while((row < (y + h))) {
		if(((row >= 0) && (row < img->height))){
			int col = x;
			while((col < (x + w))) {
				if(((col >= 0) && (col < img->width))){
					unsigned long offset = (((((size_t) row) * ((size_t) img->width)) + ((size_t) col)) * bpp);
					if((bpp >= 1)){
						ptr[offset] = color->r;
					}
					if((bpp >= 3)){
						ptr[(offset + 1)] = color->g;
						ptr[(offset + 2)] = color->b;
					}
					if((bpp >= 4)){
						ptr[(offset + 3)] = color->a;
					}
				}
				col += 1;
			}
		}
		row += 1;
	}
}
void image_imageimage_circle(struct image_imageImage* img, int cx, int cy, int radius, struct image_imageRGBA8* color){
	int x = radius;
	int y = 0;
	int err = (1 - radius);
	while((x >= y)) {
		struct std_stdResult__cgs__40 __chx__lv__90 = (*({ struct std_stdResult__cgs__40 __chx__lv__91; image_imageimage_set_rgba(&__chx__lv__91, img, (cx + x), (cy + y), ({ struct image_imageRGBA8 __chx__lv__92 = *color; &__chx__lv__92; })); &__chx__lv__91; }));
		struct std_stdResult__cgs__40 __chx__lv__93 = (*({ struct std_stdResult__cgs__40 __chx__lv__94; image_imageimage_set_rgba(&__chx__lv__94, img, (cx + y), (cy + x), ({ struct image_imageRGBA8 __chx__lv__95 = *color; &__chx__lv__95; })); &__chx__lv__94; }));
		struct std_stdResult__cgs__40 __chx__lv__96 = (*({ struct std_stdResult__cgs__40 __chx__lv__97; image_imageimage_set_rgba(&__chx__lv__97, img, (cx - y), (cy + x), ({ struct image_imageRGBA8 __chx__lv__98 = *color; &__chx__lv__98; })); &__chx__lv__97; }));
		struct std_stdResult__cgs__40 __chx__lv__99 = (*({ struct std_stdResult__cgs__40 __chx__lv__100; image_imageimage_set_rgba(&__chx__lv__100, img, (cx - x), (cy + y), ({ struct image_imageRGBA8 __chx__lv__101 = *color; &__chx__lv__101; })); &__chx__lv__100; }));
		struct std_stdResult__cgs__40 __chx__lv__102 = (*({ struct std_stdResult__cgs__40 __chx__lv__103; image_imageimage_set_rgba(&__chx__lv__103, img, (cx - x), (cy - y), ({ struct image_imageRGBA8 __chx__lv__104 = *color; &__chx__lv__104; })); &__chx__lv__103; }));
		struct std_stdResult__cgs__40 __chx__lv__105 = (*({ struct std_stdResult__cgs__40 __chx__lv__106; image_imageimage_set_rgba(&__chx__lv__106, img, (cx - y), (cy - x), ({ struct image_imageRGBA8 __chx__lv__107 = *color; &__chx__lv__107; })); &__chx__lv__106; }));
		struct std_stdResult__cgs__40 __chx__lv__108 = (*({ struct std_stdResult__cgs__40 __chx__lv__109; image_imageimage_set_rgba(&__chx__lv__109, img, (cx + y), (cy - x), ({ struct image_imageRGBA8 __chx__lv__110 = *color; &__chx__lv__110; })); &__chx__lv__109; }));
		struct std_stdResult__cgs__40 __chx__lv__111 = (*({ struct std_stdResult__cgs__40 __chx__lv__112; image_imageimage_set_rgba(&__chx__lv__112, img, (cx + x), (cy - y), ({ struct image_imageRGBA8 __chx__lv__113 = *color; &__chx__lv__113; })); &__chx__lv__112; }));
		y += 1;
		if((err < 0)){
			err += ((2 * y) + 1);
		} else {
			x -= 1;
			err += ((2 * (y - x)) + 1);
		}
		std_stdResult__cgs__40delete(&__chx__lv__111);
		std_stdResult__cgs__40delete(&__chx__lv__108);
		std_stdResult__cgs__40delete(&__chx__lv__105);
		std_stdResult__cgs__40delete(&__chx__lv__102);
		std_stdResult__cgs__40delete(&__chx__lv__99);
		std_stdResult__cgs__40delete(&__chx__lv__96);
		std_stdResult__cgs__40delete(&__chx__lv__93);
		std_stdResult__cgs__40delete(&__chx__lv__90);
	}
}
void image_imageimage_fill_circle(struct image_imageImage* img, int cx, int cy, int radius, struct image_imageRGBA8* color){
	int y = -radius;
	while((y <= radius)) {
		int x = -radius;
		while((x <= radius)) {
			if((((x * x) + (y * y)) <= (radius * radius))){
				struct std_stdResult__cgs__40 __chx__lv__114 = (*({ struct std_stdResult__cgs__40 __chx__lv__115; image_imageimage_set_rgba(&__chx__lv__115, img, (cx + x), (cy + y), ({ struct image_imageRGBA8 __chx__lv__116 = *color; &__chx__lv__116; })); &__chx__lv__115; }));
				std_stdResult__cgs__40delete(&__chx__lv__114);
			}
			x += 1;
		}
		y += 1;
	}
}