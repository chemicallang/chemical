
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/webview/src/main.ch **/
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/webview/src/linux.ch **/
struct webview_webviewGtkWindow;
struct webview_webviewGtkWidget;
struct webview_webviewGtkContainer;
struct webview_webviewGtkBox;
struct webview_webviewWebKitWebView;
struct webview_webviewWebKitSettings;
struct webview_webviewWebKitUserContentManager;
struct webview_webviewWebKitWebContext;
struct webview_webviewWebKitJavascriptResult;
struct webview_webviewGMainLoop;
struct webview_webviewJSCValue;
struct webview_webviewJSGlobalContextRef;
struct webview_webviewWebView;
/** FwdDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/webview/src/types.ch **/
struct webview_webviewWebViewError;
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/webview/src/main.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/webview/src/linux.ch **/
/** TypeAliasDeclare /home/wakaztahir/work/Chemical/chemical/lang/libs/webview/src/types.ch **/
/** FwdDeclare:Generics webview **/
struct std_stdResult__cgs__28;
struct std_stdResult__cgs__29;
/** Declare:Generics webview **/
struct webview_webviewWebView {
	struct webview_webviewGtkWidget* window;
	struct webview_webviewGtkWidget* web_view;
	struct webview_webviewGtkWidget* box;
	struct std_stdstring title;
	int width;
	int height;
	_Bool visible;
	_Bool initialized;
};
struct webview_webviewWebViewError {
	int __chx__vt_621827;
	union {
		struct {
		} PlatformNotSupported;
		struct {
			struct std_stdstring msg;
		} InitFailed;
		struct {
			struct std_stdstring msg;
		} NavigationFailed;
		struct {
			struct std_stdstring msg;
		} JsEvaluationFailed;
		struct {
			struct std_stdstring name;
		} BindFailed;
	};
};
struct std_stdResult__cgs__28 {
	int __chx__vt_621827;
	union {
		struct {
			struct webview_webviewWebView value;
		} Ok;
		struct {
			struct webview_webviewWebViewError error;
		} Err;
	};
};
void std_stdResult__cgs__28delete(struct std_stdResult__cgs__28* self);
struct std_stdResult__cgs__29 {
	int __chx__vt_621827;
	union {
		struct {
			struct std_stdUnit value;
		} Ok;
		struct {
			struct webview_webviewWebViewError error;
		} Err;
	};
};
void std_stdResult__cgs__29delete(struct std_stdResult__cgs__29* self);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/webview/src/main.ch **/
void webview_webviewcreate(struct std_stdResult__cgs__28* __chx_struct_ret_param_xx, const char* title, int width, int height);
void webview_webviewopen_url(struct std_stdResult__cgs__29* __chx_struct_ret_param_xx, const char* url, const char* title, int width, int height);
void webview_webviewopen_html(struct std_stdResult__cgs__29* __chx_struct_ret_param_xx, const char* html, const char* title, int width, int height);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/webview/src/linux.ch **/
struct webview_webviewGtkWindow {
};
struct webview_webviewGtkWidget {
};
struct webview_webviewGtkContainer {
};
struct webview_webviewGtkBox {
};
struct webview_webviewWebKitWebView {
};
struct webview_webviewWebKitSettings {
};
struct webview_webviewWebKitUserContentManager {
};
struct webview_webviewWebKitWebContext {
};
struct webview_webviewWebKitJavascriptResult {
};
static int webview_webviewGTK_WINDOW_TOPLEVEL;
static int webview_webviewGTK_POS_LEFT;
static int webview_webviewTRUE;
static int webview_webviewFALSE;
static int webview_webviewGTK_FILL;
static int webview_webviewGTK_EXPAND;
static int webview_webviewGTK_SHRINK;
static int webview_webviewWEBKIT_LOAD_FINISHED;
extern int gtk_init(int* argc, const char*** argv);
extern void gtk_main();
extern void gtk_main_quit();
extern void gtk_widget_show_all(struct webview_webviewGtkWidget* widget);
extern void gtk_widget_destroy(struct webview_webviewGtkWidget* widget);
extern void gtk_widget_set_size_request(struct webview_webviewGtkWidget* widget, int width, int height);
extern struct webview_webviewGtkWidget* gtk_window_new(int type_);
extern void gtk_window_set_title(struct webview_webviewGtkWindow* window, const char* title);
extern void gtk_window_set_default_size(struct webview_webviewGtkWindow* window, int width, int height);
extern void gtk_window_set_position(struct webview_webviewGtkWindow* window, int position);
extern size_t gtk_window_get_type();
extern struct webview_webviewGtkWidget* gtk_box_new(int orientation, int spacing);
extern void gtk_box_pack_start(struct webview_webviewGtkBox* box, struct webview_webviewGtkWidget* child, int expand, int fill, uint32_t padding);
extern struct webview_webviewGtkWidget* webkit_web_view_new();
extern size_t webkit_web_view_get_type();
extern void webkit_web_view_load_uri(struct webview_webviewWebKitWebView* web_view, const char* uri);
extern void webkit_web_view_load_html(struct webview_webviewWebKitWebView* web_view, const char* content, const char* base_uri);
extern void webkit_web_view_run_javascript(struct webview_webviewWebKitWebView* web_view, const char* script, void* cancellable, void* callback, void* user_data);
extern const char* webkit_web_view_get_title(struct webview_webviewWebKitWebView* web_view);
extern struct webview_webviewWebKitSettings* webkit_web_view_get_settings(struct webview_webviewWebKitWebView* web_view);
extern void webkit_settings_set_javascript_enabled(struct webview_webviewWebKitSettings* settings, int enabled);
extern void webkit_settings_set_allow_file_access_from_file_urls(struct webview_webviewWebKitSettings* settings, int allowed);
extern void webkit_web_view_evaluate_javascript(struct webview_webviewWebKitWebView* web_view, const char* script, isize length, const char* source_uri, void* cancellable, void* callback, void* user_data);
extern uint64_t g_signal_connect(void* instance, const char* signal, void* handler, void* data);
extern uint64_t g_signal_connect_swapped(void* instance, const char* signal, void* handler, void* data);
struct webview_webviewGMainLoop {
};
extern struct webview_webviewGMainLoop* g_main_loop_new(void* context, int is_running);
extern void g_main_loop_quit(struct webview_webviewGMainLoop* loop);
struct webview_webviewJSCValue {
};
struct webview_webviewJSGlobalContextRef {
};
extern const char* jsc_value_to_string(struct webview_webviewJSCValue* value);
extern int jsc_value_is_string(struct webview_webviewJSCValue* value);
void webview_webviewWebViewmake(struct webview_webviewWebView* this);
void webview_webviewWebViewdelete(struct webview_webviewWebView* self);
static struct webview_webviewWebView* webview_webviewg_webview_ref;
static void webview_webviewlinux_on_window_destroy(struct webview_webviewGtkWidget* widget, void* data);
static void webview_webviewlinux_on_navigation_complete(struct webview_webviewWebKitWebView* web_view, int event, void* data);
void webview_webviewwebview_create(struct std_stdResult__cgs__29* __chx_struct_ret_param_xx, struct webview_webviewWebView* wv);
extern void gtk_container_add(struct webview_webviewGtkContainer* container, struct webview_webviewGtkWidget* child);
void webview_webviewwebview_destroy(struct webview_webviewWebView* wv);
void webview_webviewwebview_load_url(struct webview_webviewWebView* wv, const char* url);
void webview_webviewwebview_load_html(struct webview_webviewWebView* wv, const char* html);
void webview_webviewwebview_evaluate_js(struct webview_webviewWebView* wv, const char* script);
void webview_webviewwebview_title(struct std_stdstring* __chx_struct_ret_param_xx, struct webview_webviewWebView* wv);
void webview_webviewwebview_set_title(struct webview_webviewWebView* wv, const char* title);
void webview_webviewwebview_set_size(struct webview_webviewWebView* wv, int width, int height);
void webview_webviewwebview_show(struct webview_webviewWebView* wv);
void webview_webviewwebview_hide(struct webview_webviewWebView* wv);
void webview_webviewwebview_run(struct webview_webviewWebView* wv);
void webview_webviewwebview_stop(struct webview_webviewWebView* wv);
/** Declare /home/wakaztahir/work/Chemical/chemical/lang/libs/webview/src/types.ch **/
void webview_webviewWebViewErrormessage(struct std_stdstring* __chx_struct_ret_param_xx, struct webview_webviewWebViewError*const self);
void webview_webviewWebViewErrordelete(struct webview_webviewWebViewError* self);
/** Implement:Generics webview **/
void std_stdResult__cgs__28delete(struct std_stdResult__cgs__28* self){
	__chx__dstctr_clnup_blk__:{
		switch(self->__chx__vt_621827) {
			case 0:
			webview_webviewWebViewdelete(&self->Ok.value);
			break;
			case 1:
			webview_webviewWebViewErrordelete(&self->Err.error);
			break;
		}
	}
}
void std_stdResult__cgs__29delete(struct std_stdResult__cgs__29* self){
	__chx__dstctr_clnup_blk__:{
		switch(self->__chx__vt_621827) {
			case 0:
			break;
			case 1:
			webview_webviewWebViewErrordelete(&self->Err.error);
			break;
		}
	}
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/webview/src/main.ch **/
void webview_webviewcreate(struct std_stdResult__cgs__28* __chx_struct_ret_param_xx, const char* title, int width, int height){
	struct webview_webviewWebView wv = (*({ struct webview_webviewWebView __chx__lv__0; webview_webviewWebViewmake(&__chx__lv__0); &__chx__lv__0; }));
	_Bool __chx__lv__1 = true;
	{
		struct std_stdstring __chx__lv__2 = (*({ struct std_stdstring __chx__lv__3; std_stdstringconstructor2(&__chx__lv__3, "", 0, 0); &__chx__lv__3; }));
		std_stdstringdelete(&wv.title);
		wv.title = __chx__lv__2;
	};
	std_stdstringappend_char_ptr(&wv.title, title);
	wv.width = width;
	wv.height = height;
	struct std_stdResult__cgs__29 result = (*({ struct std_stdResult__cgs__29 __chx__lv__4; webview_webviewwebview_create(&__chx__lv__4, &wv); &__chx__lv__4; }));
	_Bool __chx__lv__5 = true;
	if((result.__chx__vt_621827 == 1)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__28) { .__chx__vt_621827 = 1, 
			.Err.error = (struct webview_webviewWebViewError) { .__chx__vt_621827 = 1, 
				.InitFailed.msg = (*({ struct std_stdstring __chx__lv__6; std_stdstringconstructor2(&__chx__lv__6, "failed to create webview", 24, 0); &__chx__lv__6; }))
			}
		};
		if(__chx__lv__5) {
			std_stdResult__cgs__29delete(&result);
		}
		if(__chx__lv__1) {
			webview_webviewWebViewdelete(&wv);
		}
		return;
	}
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__28) { .__chx__vt_621827 = 0, 
		.Ok.value = ({ __chx__lv__1 = false; wv; })
	};
	if(__chx__lv__5) {
		std_stdResult__cgs__29delete(&result);
	}
	if(__chx__lv__1) {
		webview_webviewWebViewdelete(&wv);
	}
	return;
}
void webview_webviewopen_url(struct std_stdResult__cgs__29* __chx_struct_ret_param_xx, const char* url, const char* title, int width, int height){
	struct std_stdResult__cgs__28 wv_result = (*({ struct std_stdResult__cgs__28 __chx__lv__7; webview_webviewcreate(&__chx__lv__7, title, width, height); &__chx__lv__7; }));
	_Bool __chx__lv__8 = true;
	if((wv_result.__chx__vt_621827 == 1)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 1, 
			.Err.error = (struct webview_webviewWebViewError) { .__chx__vt_621827 = 1, 
				.InitFailed.msg = (*({ struct std_stdstring __chx__lv__9; std_stdstringconstructor2(&__chx__lv__9, "failed to create webview", 24, 0); &__chx__lv__9; }))
			}
		};
		if(__chx__lv__8) {
			std_stdResult__cgs__28delete(&wv_result);
		}
		return;
	}
	struct std_stdResult__cgs__28* __chx__lv__10 = &wv_result;
	webview_webviewwebview_load_url(&__chx__lv__10->Ok.value, url);
	webview_webviewwebview_show(&__chx__lv__10->Ok.value);
	webview_webviewwebview_run(&__chx__lv__10->Ok.value);
	webview_webviewwebview_destroy(&__chx__lv__10->Ok.value);
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 0, 
		.Ok.value = (struct std_stdUnit){ 
		}
	};
	if(__chx__lv__8) {
		std_stdResult__cgs__28delete(&wv_result);
	}
	return;
}
void webview_webviewopen_html(struct std_stdResult__cgs__29* __chx_struct_ret_param_xx, const char* html, const char* title, int width, int height){
	struct std_stdResult__cgs__28 wv_result = (*({ struct std_stdResult__cgs__28 __chx__lv__11; webview_webviewcreate(&__chx__lv__11, title, width, height); &__chx__lv__11; }));
	_Bool __chx__lv__12 = true;
	if((wv_result.__chx__vt_621827 == 1)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 1, 
			.Err.error = (struct webview_webviewWebViewError) { .__chx__vt_621827 = 1, 
				.InitFailed.msg = (*({ struct std_stdstring __chx__lv__13; std_stdstringconstructor2(&__chx__lv__13, "failed to create webview", 24, 0); &__chx__lv__13; }))
			}
		};
		if(__chx__lv__12) {
			std_stdResult__cgs__28delete(&wv_result);
		}
		return;
	}
	struct std_stdResult__cgs__28* __chx__lv__14 = &wv_result;
	webview_webviewwebview_load_html(&__chx__lv__14->Ok.value, html);
	webview_webviewwebview_show(&__chx__lv__14->Ok.value);
	webview_webviewwebview_run(&__chx__lv__14->Ok.value);
	webview_webviewwebview_destroy(&__chx__lv__14->Ok.value);
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 0, 
		.Ok.value = (struct std_stdUnit){ 
		}
	};
	if(__chx__lv__12) {
		std_stdResult__cgs__28delete(&wv_result);
	}
	return;
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/webview/src/linux.ch **/
static int webview_webviewGTK_WINDOW_TOPLEVEL = 0;
static int webview_webviewGTK_POS_LEFT = 0;
static int webview_webviewTRUE = 1;
static int webview_webviewFALSE = 0;
static int webview_webviewGTK_FILL = 4;
static int webview_webviewGTK_EXPAND = 2;
static int webview_webviewGTK_SHRINK = 1;
static int webview_webviewWEBKIT_LOAD_FINISHED = 4;
void webview_webviewWebViewmake(struct webview_webviewWebView* this){
	*this = (struct webview_webviewWebView){ 
		.window = NULL, 
		.web_view = NULL, 
		.box = NULL, 
		.title = (*({ struct std_stdstring __chx__lv__15; std_stdstringconstructor2(&__chx__lv__15, "Chemical WebView", 16, 0); &__chx__lv__15; })), 
		.width = 800, 
		.height = 600, 
		.visible = 0, 
		.initialized = 0
	};
	return;
}
void webview_webviewWebViewdelete(struct webview_webviewWebView* self){
	__chx__dstctr_clnup_blk__:{
		std_stdstringdelete(&self->title);
	}
}
static struct webview_webviewWebView* webview_webviewg_webview_ref = NULL;
static void webview_webviewlinux_on_window_destroy(struct webview_webviewGtkWidget* widget, void* data){
	gtk_main_quit();
}
static void webview_webviewlinux_on_navigation_complete(struct webview_webviewWebKitWebView* web_view, int event, void* data){
}
void webview_webviewwebview_create(struct std_stdResult__cgs__29* __chx_struct_ret_param_xx, struct webview_webviewWebView* wv){
	int argc = 0;
	const char*** argv = NULL;
	gtk_init(&argc, argv);
	wv->window = gtk_window_new(webview_webviewGTK_WINDOW_TOPLEVEL);
	if((wv->window == NULL)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 1, 
			.Err.error = (struct webview_webviewWebViewError) { .__chx__vt_621827 = 1, 
				.InitFailed.msg = (*({ struct std_stdstring __chx__lv__16; std_stdstringconstructor2(&__chx__lv__16, "failed to create window", 23, 0); &__chx__lv__16; }))
			}
		};
		return;
	}
	gtk_window_set_title(((struct webview_webviewGtkWindow*) wv->window), std_stdstringc_str(&wv->title));
	gtk_window_set_default_size(((struct webview_webviewGtkWindow*) wv->window), wv->width, wv->height);
	gtk_window_set_position(((struct webview_webviewGtkWindow*) wv->window), 1);
	wv->box = gtk_box_new(0, 0);
	wv->web_view = webkit_web_view_new();
	if((wv->web_view == NULL)){
		*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 1, 
			.Err.error = (struct webview_webviewWebViewError) { .__chx__vt_621827 = 1, 
				.InitFailed.msg = (*({ struct std_stdstring __chx__lv__17; std_stdstringconstructor2(&__chx__lv__17, "failed to create web view", 25, 0); &__chx__lv__17; }))
			}
		};
		return;
	}
	struct webview_webviewWebKitSettings* settings = webkit_web_view_get_settings(((struct webview_webviewWebKitWebView*) wv->web_view));
	if((settings != NULL)){
		webkit_settings_set_javascript_enabled(settings, webview_webviewTRUE);
		webkit_settings_set_allow_file_access_from_file_urls(settings, webview_webviewTRUE);
	}
	gtk_box_pack_start(((struct webview_webviewGtkBox*) wv->box), wv->web_view, webview_webviewTRUE, webview_webviewGTK_FILL, 0);
	gtk_container_add(((struct webview_webviewGtkContainer*) wv->window), wv->box);
	g_signal_connect(((void*) wv->window), ((const char*) "destroy\0"), ((void*) webview_webviewlinux_on_window_destroy), NULL);
	g_signal_connect(((void*) wv->web_view), ((const char*) "load-changed\0"), ((void*) webview_webviewlinux_on_navigation_complete), NULL);
	wv->visible = 0;
	wv->initialized = 1;
	webview_webviewg_webview_ref = wv;
	*__chx_struct_ret_param_xx = (struct std_stdResult__cgs__29) { .__chx__vt_621827 = 0, 
		.Ok.value = (struct std_stdUnit){ 
		}
	};
	return;
}
void webview_webviewwebview_destroy(struct webview_webviewWebView* wv){
	if((wv->web_view != NULL)){
		gtk_widget_destroy(wv->web_view);
		wv->web_view = NULL;
	}
	if((wv->window != NULL)){
		gtk_widget_destroy(wv->window);
		wv->window = NULL;
	}
	wv->initialized = 0;
}
void webview_webviewwebview_load_url(struct webview_webviewWebView* wv, const char* url){
	if((wv->web_view != NULL)){
		webkit_web_view_load_uri(((struct webview_webviewWebKitWebView*) wv->web_view), url);
	}
}
void webview_webviewwebview_load_html(struct webview_webviewWebView* wv, const char* html){
	if((wv->web_view != NULL)){
		webkit_web_view_load_html(((struct webview_webviewWebKitWebView*) wv->web_view), html, ((const char*) "about:blank\0"));
	}
}
void webview_webviewwebview_evaluate_js(struct webview_webviewWebView* wv, const char* script){
	if((wv->web_view != NULL)){
		webkit_web_view_run_javascript(((struct webview_webviewWebKitWebView*) wv->web_view), script, NULL, NULL, NULL);
	}
}
void webview_webviewwebview_title(struct std_stdstring* __chx_struct_ret_param_xx, struct webview_webviewWebView* wv){
	if((wv->web_view != NULL)){
		const char* title_ptr = webkit_web_view_get_title(((struct webview_webviewWebKitWebView*) wv->web_view));
		if((title_ptr != NULL)){
			struct std_stdstring result = (*({ struct std_stdstring __chx__lv__18; std_stdstringconstructor2(&__chx__lv__18, "", 0, 0); &__chx__lv__18; }));
			_Bool __chx__lv__19 = true;
			size_t i = 0;
			while((title_ptr[i] != ((char) '\0'))) {
				std_stdstringappend(&result, title_ptr[i]);
				i += 1;
			}
			*__chx_struct_ret_param_xx = result;
			return;
		}
	}
	*__chx_struct_ret_param_xx = wv->title;
	return;
}
void webview_webviewwebview_set_title(struct webview_webviewWebView* wv, const char* title){
	{
		struct std_stdstring __chx__lv__20 = (*({ struct std_stdstring __chx__lv__21; std_stdstringconstructor2(&__chx__lv__21, "", 0, 0); &__chx__lv__21; }));
		std_stdstringdelete(&wv->title);
		wv->title = __chx__lv__20;
	};
	std_stdstringappend_char_ptr(&wv->title, title);
	if((wv->window != NULL)){
		gtk_window_set_title(((struct webview_webviewGtkWindow*) wv->window), title);
	}
}
void webview_webviewwebview_set_size(struct webview_webviewWebView* wv, int width, int height){
	wv->width = width;
	wv->height = height;
	if((wv->window != NULL)){
		gtk_window_set_default_size(((struct webview_webviewGtkWindow*) wv->window), width, height);
	}
}
void webview_webviewwebview_show(struct webview_webviewWebView* wv){
	if((wv->window != NULL)){
		gtk_widget_show_all(wv->window);
		wv->visible = 1;
	}
}
void webview_webviewwebview_hide(struct webview_webviewWebView* wv){
	wv->visible = 0;
}
void webview_webviewwebview_run(struct webview_webviewWebView* wv){
	if((wv->window != NULL)){
		gtk_main();
	}
}
void webview_webviewwebview_stop(struct webview_webviewWebView* wv){
	gtk_main_quit();
}
/** Translate /home/wakaztahir/work/Chemical/chemical/lang/libs/webview/src/types.ch **/
void webview_webviewWebViewErrormessage(struct std_stdstring* __chx_struct_ret_param_xx, struct webview_webviewWebViewError*const self){
	switch((self)->__chx__vt_621827) {
		case 0:{
			*__chx_struct_ret_param_xx = (*({ struct std_stdstring __chx__lv__22; std_stdstringconstructor2(&__chx__lv__22, "WebViewError: platform not supported", 36, 0); &__chx__lv__22; }));
			return;
			break;
		}
		case 1:{
			struct std_stdstring s = (*({ struct std_stdstring __chx__lv__23; std_stdstringconstructor2(&__chx__lv__23, "WebViewError: init failed: ", 27, 0); &__chx__lv__23; }));
			_Bool __chx__lv__24 = true;
			std_stdstringappend_string(&s, &(self)->InitFailed.msg);
			*__chx_struct_ret_param_xx = s;
			return;
			break;
		}
		case 2:{
			struct std_stdstring s = (*({ struct std_stdstring __chx__lv__25; std_stdstringconstructor2(&__chx__lv__25, "WebViewError: navigation failed: ", 33, 0); &__chx__lv__25; }));
			_Bool __chx__lv__26 = true;
			std_stdstringappend_string(&s, &(self)->NavigationFailed.msg);
			*__chx_struct_ret_param_xx = s;
			return;
			break;
		}
		case 3:{
			struct std_stdstring s = (*({ struct std_stdstring __chx__lv__27; std_stdstringconstructor2(&__chx__lv__27, "WebViewError: JS evaluation failed: ", 36, 0); &__chx__lv__27; }));
			_Bool __chx__lv__28 = true;
			std_stdstringappend_string(&s, &(self)->JsEvaluationFailed.msg);
			*__chx_struct_ret_param_xx = s;
			return;
			break;
		}
		case 4:{
			struct std_stdstring s = (*({ struct std_stdstring __chx__lv__29; std_stdstringconstructor2(&__chx__lv__29, "WebViewError: bind failed for: ", 31, 0); &__chx__lv__29; }));
			_Bool __chx__lv__30 = true;
			std_stdstringappend_string(&s, &(self)->BindFailed.name);
			*__chx_struct_ret_param_xx = s;
			return;
			break;
		}
	}
}
void webview_webviewWebViewErrordelete(struct webview_webviewWebViewError* self){
	__chx__dstctr_clnup_blk__:{
		switch(self->__chx__vt_621827) {
			case 0:
			break;
			case 1:
			std_stdstringdelete(&self->InitFailed.msg);
			break;
			case 2:
			std_stdstringdelete(&self->NavigationFailed.msg);
			break;
			case 3:
			std_stdstringdelete(&self->JsEvaluationFailed.msg);
			break;
			case 4:
			std_stdstringdelete(&self->BindFailed.name);
			break;
		}
	}
}