
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/path/src/types.ch **/
struct path_pathPathError;
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/path/src/path.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/path/src/types.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/path/src/path.ch **/
/** FwdDeclare:Generics path **/
struct std_stdResult__cgs__28;
/** Declare:Generics path **/
struct path_pathPathError {
	int __chx__vt_621827;
	union {
		struct {
		} BufferTooSmall;
		struct {
		} TooManyComponents;
		struct {
			struct std_stdstring msg;
		} InvalidPath;
	};
};
struct std_stdResult__cgs__28 {
	int __chx__vt_621827;
	union {
		struct {
			size_t value;
		} Ok;
		struct {
			struct path_pathPathError error;
		} Err;
	};
};
void std_stdResult__cgs__28delete(struct std_stdResult__cgs__28* self);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/path/src/types.ch **/
void path_pathPathErrormessage(struct std_stdstring* __chx_struct_ret_param_xx, struct path_pathPathError*const self);
void path_pathPathErrordelete(struct path_pathPathError* self);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/path/src/path.ch **/

void path_pathbasename(struct std_stdResult__cgs__28* __chx_struct_ret_param_xx, const char* p, char* out, size_t out_len);
void path_pathdirname(struct std_stdResult__cgs__28* __chx_struct_ret_param_xx, const char* p, char* out, size_t out_len);
void path_pathextension(struct std_stdResult__cgs__28* __chx_struct_ret_param_xx, const char* p, char* out, size_t out_len);
void path_pathstem(struct std_stdResult__cgs__28* __chx_struct_ret_param_xx, const char* p, char* out, size_t out_len);
void path_pathjoin(struct std_stdResult__cgs__28* __chx_struct_ret_param_xx, const char* a, const char* b, char* out, size_t out_len);
void path_pathnormalize(struct std_stdResult__cgs__28* __chx_struct_ret_param_xx, const char* p, char* out, size_t out_len);
_Bool path_pathis_absolute(const char* p);
_Bool path_pathhas_root(const char* p);
static int path_pathfind_last_separator(const char* data, size_t data_len);
void path_pathparent(struct std_stdstring* __chx_struct_ret_param_xx, struct std_stdstring_view* sv);
/** Implement:Generics path **/
void std_stdResult__cgs__28delete(struct std_stdResult__cgs__28* self){
	__chx__dstctr_clnup_blk__:{
		switch(self->__chx__vt_621827) {
			case 0:
			break;
			case 1:
			path_pathPathErrordelete(&self->Err.error);
			break;
		}
	}
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/path/src/types.ch **/
void path_pathPathErrormessage(struct std_stdstring* __chx_struct_ret_param_xx, struct path_pathPathError*const self){
	switch((self)->__chx__vt_621827) {
		case 0:{
			*__chx_struct_ret_param_xx = (*({ struct std_stdstring __chx__lv__0; std_stdstringconstructor2(&__chx__lv__0, "PathError: output buffer too small", 34, 0); &__chx__lv__0; }));
			return;
			break;
		}
		case 1:{
			*__chx_struct_ret_param_xx = (*({ struct std_stdstring __chx__lv__1; std_stdstringconstructor2(&__chx__lv__1, "PathError: too many path components", 35, 0); &__chx__lv__1; }));
			return;
			break;
		}
		case 2:{
			struct std_stdstring s = (*({ struct std_stdstring __chx__lv__2; std_stdstringconstructor2(&__chx__lv__2, "PathError: invalid path: ", 25, 0); &__chx__lv__2; }));
			_Bool __chx__lv__3 = true;
			std_stdstringappend_string(&s, &(self)->InvalidPath.msg);
			*__chx_struct_ret_param_xx = s;
			return;
			break;
		}
	}
}
void path_pathPathErrordelete(struct path_pathPathError* self){
	__chx__dstctr_clnup_blk__:{
		switch(self->__chx__vt_621827) {
			case 0:
			break;
			case 1:
			break;
			case 2:
			std_stdstringdelete(&self->InvalidPath.msg);
			break;
		}
	}
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/path/src/path.ch **/

void path_pathbasename(struct std_stdResult__cgs__28* __chx_struct_ret_param_xx, const char* p, char* out, size_t out_len){
	size_t len = 0;
	while((p[len] != 0)) {
		len += 1;
	}
	if((len == 0)){
		if((out_len < 2)){
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__28) { .__chx__vt_621827 = 1, 
				.Err.error = (struct path_pathPathError) { .__chx__vt_621827 = 0, 
				}
			};
			return;
		}
		out[0] = '.';
		out[1] = 0;
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__28) { .__chx__vt_621827 = 0, 
			.Ok.value = 1
		};
		return;
	}
	size_t end = len;
	while(((end > 0) && ((p[(end - 1)] == '/') || (p[(end - 1)] == '\\')))) {
		end -= 1;
	}
	if((end == 0)){
		if((out_len < 2)){
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__28) { .__chx__vt_621827 = 1, 
				.Err.error = (struct path_pathPathError) { .__chx__vt_621827 = 0, 
				}
			};
			return;
		}
		out[0] = '/';
		out[1] = 0;
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__28) { .__chx__vt_621827 = 0, 
			.Ok.value = 1
		};
		return;
	}
	size_t start = end;
	while((((start > 0) && (p[(start - 1)] != '/')) && (p[(start - 1)] != '\\'))) {
		start -= 1;
	}
	unsigned long comp_len = (end - start);
	if(((comp_len + 1) > out_len)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__28) { .__chx__vt_621827 = 1, 
			.Err.error = (struct path_pathPathError) { .__chx__vt_621827 = 0, 
			}
		};
		return;
	}
	size_t i = 0;
	while((i < comp_len)) {
		out[i] = p[(start + i)];
		i += 1;
	}
	out[i] = 0;
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__28) { .__chx__vt_621827 = 0, 
		.Ok.value = comp_len
	};
	return;
}
void path_pathdirname(struct std_stdResult__cgs__28* __chx_struct_ret_param_xx, const char* p, char* out, size_t out_len){
	size_t len = 0;
	while((p[len] != 0)) {
		len += 1;
	}
	if((len == 0)){
		if((out_len < 2)){
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__28) { .__chx__vt_621827 = 1, 
				.Err.error = (struct path_pathPathError) { .__chx__vt_621827 = 0, 
				}
			};
			return;
		}
		out[0] = '.';
		out[1] = 0;
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__28) { .__chx__vt_621827 = 0, 
			.Ok.value = 1
		};
		return;
	}
	size_t end = len;
	while(((end > 0) && ((p[(end - 1)] == '/') || (p[(end - 1)] == '\\')))) {
		end -= 1;
	}
	if((end == 0)){
		if((out_len < 2)){
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__28) { .__chx__vt_621827 = 1, 
				.Err.error = (struct path_pathPathError) { .__chx__vt_621827 = 0, 
				}
			};
			return;
		}
		out[0] = '/';
		out[1] = 0;
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__28) { .__chx__vt_621827 = 0, 
			.Ok.value = 1
		};
		return;
	}
	size_t start = end;
	while((((start > 0) && (p[(start - 1)] != '/')) && (p[(start - 1)] != '\\'))) {
		start -= 1;
	}
	if((start == 0)){
		if((out_len < 2)){
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__28) { .__chx__vt_621827 = 1, 
				.Err.error = (struct path_pathPathError) { .__chx__vt_621827 = 0, 
				}
			};
			return;
		}
		out[0] = '.';
		out[1] = 0;
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__28) { .__chx__vt_621827 = 0, 
			.Ok.value = 1
		};
		return;
	}
	size_t dir_end = start;
	while(((dir_end > 0) && ((p[(dir_end - 1)] == '/') || (p[(dir_end - 1)] == '\\')))) {
		dir_end -= 1;
	}
	if((dir_end == 0)){
		if((out_len < 2)){
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__28) { .__chx__vt_621827 = 1, 
				.Err.error = (struct path_pathPathError) { .__chx__vt_621827 = 0, 
				}
			};
			return;
		}
		out[0] = '/';
		out[1] = 0;
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__28) { .__chx__vt_621827 = 0, 
			.Ok.value = 1
		};
		return;
	}
	if(((dir_end + 1) > out_len)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__28) { .__chx__vt_621827 = 1, 
			.Err.error = (struct path_pathPathError) { .__chx__vt_621827 = 0, 
			}
		};
		return;
	}
	size_t i = 0;
	while((i < dir_end)) {
		out[i] = p[i];
		i += 1;
	}
	out[i] = 0;
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__28) { .__chx__vt_621827 = 0, 
		.Ok.value = dir_end
	};
	return;
}
void path_pathextension(struct std_stdResult__cgs__28* __chx_struct_ret_param_xx, const char* p, char* out, size_t out_len){
	size_t len = 0;
	while((p[len] != 0)) {
		len += 1;
	}
	size_t end = len;
	while(((end > 0) && ((p[(end - 1)] == '/') || (p[(end - 1)] == '\\')))) {
		end -= 1;
	}
	size_t start = end;
	while((((start > 0) && (p[(start - 1)] != '/')) && (p[(start - 1)] != '\\'))) {
		start -= 1;
	}
	size_t dot_pos = start;
	while(((dot_pos < end) && (p[dot_pos] != '.'))) {
		dot_pos += 1;
	}
	if(((dot_pos >= end) || (dot_pos == start))){
		if((out_len < 1)){
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__28) { .__chx__vt_621827 = 1, 
				.Err.error = (struct path_pathPathError) { .__chx__vt_621827 = 0, 
				}
			};
			return;
		}
		out[0] = 0;
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__28) { .__chx__vt_621827 = 0, 
			.Ok.value = 0
		};
		return;
	}
	unsigned long ext_len = (end - dot_pos);
	if(((ext_len + 1) > out_len)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__28) { .__chx__vt_621827 = 1, 
			.Err.error = (struct path_pathPathError) { .__chx__vt_621827 = 0, 
			}
		};
		return;
	}
	size_t i = 0;
	while((i < ext_len)) {
		out[i] = p[(dot_pos + i)];
		i += 1;
	}
	out[i] = 0;
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__28) { .__chx__vt_621827 = 0, 
		.Ok.value = ext_len
	};
	return;
}
void path_pathstem(struct std_stdResult__cgs__28* __chx_struct_ret_param_xx, const char* p, char* out, size_t out_len){
	size_t len = 0;
	while((p[len] != 0)) {
		len += 1;
	}
	size_t end = len;
	while(((end > 0) && ((p[(end - 1)] == '/') || (p[(end - 1)] == '\\')))) {
		end -= 1;
	}
	size_t start = end;
	while((((start > 0) && (p[(start - 1)] != '/')) && (p[(start - 1)] != '\\'))) {
		start -= 1;
	}
	size_t dot_pos = end;
	while(((dot_pos > start) && (p[(dot_pos - 1)] != '.'))) {
		dot_pos -= 1;
	}
	if((dot_pos <= start)){
		unsigned long stem_len = (end - start);
		if(((stem_len + 1) > out_len)){
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__28) { .__chx__vt_621827 = 1, 
				.Err.error = (struct path_pathPathError) { .__chx__vt_621827 = 0, 
				}
			};
			return;
		}
		size_t i = 0;
		while((i < stem_len)) {
			out[i] = p[(start + i)];
			i += 1;
		}
		out[i] = 0;
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__28) { .__chx__vt_621827 = 0, 
			.Ok.value = stem_len
		};
		return;
	}
	unsigned long stem_len = (dot_pos - start);
	if(((stem_len + 1) > out_len)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__28) { .__chx__vt_621827 = 1, 
			.Err.error = (struct path_pathPathError) { .__chx__vt_621827 = 0, 
			}
		};
		return;
	}
	size_t i = 0;
	while((i < stem_len)) {
		out[i] = p[(start + i)];
		i += 1;
	}
	out[i] = 0;
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__28) { .__chx__vt_621827 = 0, 
		.Ok.value = stem_len
	};
	return;
}
void path_pathjoin(struct std_stdResult__cgs__28* __chx_struct_ret_param_xx, const char* a, const char* b, char* out, size_t out_len){
	size_t a_len = 0;
	while((a[a_len] != 0)) {
		a_len += 1;
	}
	size_t b_len = 0;
	while((b[b_len] != 0)) {
		b_len += 1;
	}
	if((a_len == 0)){
		if(((b_len + 1) > out_len)){
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__28) { .__chx__vt_621827 = 1, 
				.Err.error = (struct path_pathPathError) { .__chx__vt_621827 = 0, 
				}
			};
			return;
		}
		size_t i = 0;
		while((i <= b_len)) {
			out[i] = b[i];
			i += 1;
		}
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__28) { .__chx__vt_621827 = 0, 
			.Ok.value = b_len
		};
		return;
	}
	if(((b_len > 0) && ((b[0] == '/') || ((b_len > 1) && (b[1] == ':'))))){
		if(((b_len + 1) > out_len)){
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__28) { .__chx__vt_621827 = 1, 
				.Err.error = (struct path_pathPathError) { .__chx__vt_621827 = 0, 
				}
			};
			return;
		}
		size_t i = 0;
		while((i <= b_len)) {
			out[i] = b[i];
			i += 1;
		}
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__28) { .__chx__vt_621827 = 0, 
			.Ok.value = b_len
		};
		return;
	}
	_Bool need_sep = ((a[(a_len - 1)] != '/') && (a[(a_len - 1)] != '\\'));
	unsigned long total = ((a_len + need_sep ? ({ 
	1; }) : ({ 
	0; })) + b_len);
	if(((total + 1) > out_len)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__28) { .__chx__vt_621827 = 1, 
			.Err.error = (struct path_pathPathError) { .__chx__vt_621827 = 0, 
			}
		};
		return;
	}
	size_t pos = 0;
	size_t i = 0;
	while((i < a_len)) {
		out[pos] = a[i];
		pos += 1;
		i += 1;
	}
	if(need_sep){
		out[pos] = '/';
		pos += 1;
	}
	i = 0;
	while((i <= b_len)) {
		out[pos] = b[i];
		pos += 1;
		i += 1;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__28) { .__chx__vt_621827 = 0, 
		.Ok.value = total
	};
	return;
}
void path_pathnormalize(struct std_stdResult__cgs__28* __chx_struct_ret_param_xx, const char* p, char* out, size_t out_len){
	size_t len = 0;
	while((p[len] != 0)) {
		len += 1;
	}
	_Bool is_absolute = ((len > 0) && (p[0] == '/'));
	size_t pos = 0;
	if(is_absolute){
		if(((pos + 1) >= out_len)){
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__28) { .__chx__vt_621827 = 1, 
				.Err.error = (struct path_pathPathError) { .__chx__vt_621827 = 0, 
				}
			};
			return;
		}
		out[pos] = '/';
		pos += 1;
	}
	size_t i = 0;
	while((i < len)) {
		while(((i < len) && ((p[i] == '/') || (p[i] == '\\')))) {
			i += 1;
		}
		if((i >= len)){
			break;
		}
		size_t comp_start = i;
		while((((i < len) && (p[i] != '/')) && (p[i] != '\\'))) {
			i += 1;
		}
		unsigned long comp_len = (i - comp_start);
		if(((comp_len == 1) && (p[comp_start] == '.'))){
			continue;
		}
		if((((comp_len == 2) && (p[comp_start] == '.')) && (p[(comp_start + 1)] == '.'))){
			if(((pos == 0) || (is_absolute && (pos == 1)))){
				if(!is_absolute){
					if(((pos + 3) > out_len)){
						*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__28) { .__chx__vt_621827 = 1, 
							.Err.error = (struct path_pathPathError) { .__chx__vt_621827 = 0, 
							}
						};
						return;
					}
					if((pos > 0)){
						out[pos] = '/';
						pos += 1;
					}
					out[pos] = '.';
					pos += 1;
					out[pos] = '.';
					pos += 1;
				}
				continue;
			}
			if((pos > 0)){
				pos -= 1;
			}
			while(((pos > 0) && (out[(pos - 1)] != '/'))) {
				pos -= 1;
			}
			if((pos > 0)){
				pos -= 1;
			}
			continue;
		}
		if(((pos > 0) && !(is_absolute && (pos == 1)))){
			if(((pos + 1) >= out_len)){
				*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__28) { .__chx__vt_621827 = 1, 
					.Err.error = (struct path_pathPathError) { .__chx__vt_621827 = 0, 
					}
				};
				return;
			}
			out[pos] = '/';
			pos += 1;
		}
		if(((pos + comp_len) >= out_len)){
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__28) { .__chx__vt_621827 = 1, 
				.Err.error = (struct path_pathPathError) { .__chx__vt_621827 = 0, 
				}
			};
			return;
		}
		size_t k = 0;
		while((k < comp_len)) {
			out[pos] = p[(comp_start + k)];
			pos += 1;
			k += 1;
		}
	}
	if((pos == 0)){
		if((out_len < 2)){
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__28) { .__chx__vt_621827 = 1, 
				.Err.error = (struct path_pathPathError) { .__chx__vt_621827 = 0, 
				}
			};
			return;
		}
		if(is_absolute){
			out[0] = '/';
			out[1] = 0;
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__28) { .__chx__vt_621827 = 0, 
				.Ok.value = 1
			};
			return;
		} else {
			out[0] = '.';
			out[1] = 0;
			*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__28) { .__chx__vt_621827 = 0, 
				.Ok.value = 1
			};
			return;
		}
	}
	if((pos >= out_len)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__28) { .__chx__vt_621827 = 1, 
			.Err.error = (struct path_pathPathError) { .__chx__vt_621827 = 0, 
			}
		};
		return;
	}
	out[pos] = 0;
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__28) { .__chx__vt_621827 = 0, 
		.Ok.value = pos
	};
	return;
}
_Bool path_pathis_absolute(const char* p){
	if((p[0] == '/')){
		return 1;
	}
	
	return 0;
}
_Bool path_pathhas_root(const char* p){
	return ((p[0] == '/') || (p[0] == '\\'));
}
static int path_pathfind_last_separator(const char* data, size_t data_len){
	if((data_len == 0)){
		return -1;
	}
	int i = (((int) data_len) - 1);
	while((i >= 0)) {
		if(((data[i] == '/') || (data[i] == '\\'))){
			return i;
		}
		i -= 1;
	}
	return -1;
}
void path_pathparent(struct std_stdstring* __chx_struct_ret_param_xx, struct std_stdstring_view* sv){
	struct std_stdstring result = (*({ struct std_stdstring __chx__lv__4; std_stdstringempty_str(&__chx__lv__4); &__chx__lv__4; }));
	_Bool __chx__lv__5 = true;
	int pos = path_pathfind_last_separator(std_stdstring_viewdata(sv), std_stdstring_viewsize(sv));
	if((pos > 0)){
		std_stdstringappend_with_len(&result, std_stdstring_viewdata(sv), ((size_t) (pos + 1)));
	}
	*__chx_struct_ret_param_xx = result;
	return;
}