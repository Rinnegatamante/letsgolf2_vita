/* main.c -- Let's Golf 2 .so loader
 *
 * Copyright (C) 2025 Alessio Tosto
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.	See the LICENSE file for details.
 */

#include <vitasdk.h>
#include <kubridge.h>
#include <vitaGL.h>

#include <pthread.h>

#include <malloc.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <locale.h>
#include <wctype.h>
#include <ctype.h>

#include <poll.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include <math.h>

#include <sys/time.h>

#include "main.h"
#include "config.h"
#include "dialog.h"
#include "so_util.h"

//#define DEBUG

int _newlib_heap_size_user = MEMORY_NEWLIB_MB * 1024 * 1024;

so_module main_mod;

void *__wrap_memcpy(void *dest, const void *src, size_t n) {
	return sceClibMemcpy(dest, src, n);
}

void *__wrap_memmove(void *dest, const void *src, size_t n) {
	return sceClibMemmove(dest, src, n);
}

void *__wrap_memset(void *s, int c, size_t n) {
	return sceClibMemset(s, c, n);
}

int __android_log_write(int prio, const char *tag, const char *text) {
#ifdef DEBUG
	sceClibPrintf("%s: %s\n", tag, text);
#endif
	return 0;
}


int __android_log_print(int prio, const char *tag, const char *fmt, ...) {
#ifdef DEBUG
	va_list list;
	static char string[0x8000];

	va_start(list, fmt);
	vsprintf(string, fmt, list);
	va_end(list);

	sceClibPrintf("[LOG] %s: %s\n", tag, string);
#endif
	return 0;
}

int ret0(void) {
	return 0;
}

int GetWidthOfScreen() {
	return SCREEN_W;
}

int GetHeightOfScreen() {
	return SCREEN_H;
}

int ret1() {
	return 1;
}

int nativeOpenBrowser(char *url) {
	return 0;
}

int nativeExit() {
	return sceKernelExitProcess(0);
}

int nativeLoadMovie() {
	//sceClibPrintf("nativeLoadMovie\n");
	return 1;
}

int nativeIsVideoCompleted() {
	//sceClibPrintf("nativeIsVideoCompleted\n");
	return 1;
}

void GFX_InitializeAndroid() {
	glViewport(0, 0, 960, 544);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrthof(0.0f, 960.0f, 0.0f, 544.0f, -1.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glRotatef(0.0f, 0.0f, 0.0f, 1.0f);
	glRotatef(180.0f, 1.0f, 0.0f, 0.0f);
	glTranslatef(0.0f, -544.0f, 0.0f);
}

void *audio_player_play(char *path, uint8_t loop, float vol, uint8_t no_autostart);
int audio_player_is_playing(void *m);
void audio_player_stop(void *m);
void audio_player_set_pause(void *m, int val);
void audio_player_init();
void audio_player_stop_all_sounds();
void audio_player_set_pause_all_sounds(int val);
void audio_player_instance(void *m, uint8_t loop, float vol);
void audio_player_set_volume(void *m, float vol);
void audio_player_change_bgm_volume(float vol);
void audio_player_change_sfx_volume(float vol);
float audio_player_get_volume(void *m);

#define AUDIO_SOURCES_NUM (2048)
void *audio_sources[AUDIO_SOURCES_NUM] = {};

void nativePlaySound(int id, float vol, int loop) {
	char path[256];
	sprintf(path, "ux0:data/letsgolf2/raw/raw_%03d.ogg", id);
	//sceClibPrintf("nativePlaySound %s %f %d\n", path, vol, loop);
	
	if (audio_sources[id] == (void *)0xDEADBEEF) {
		audio_sources[id] = audio_player_play(path, loop, vol, 0);
	} else {
		audio_player_instance(audio_sources[id], loop, vol);
	}
}

void nativeStopAllSounds() {
	//sceClibPrintf("nativeStopAllSounds\n");
#if 0
	audio_player_stop_all_sounds();
	for (int i = 0; i < AUDIO_SOURCES_NUM; i++) {
		audio_sources[i] = (void *)0xDEADBEEF;
	}
#else
	for (int i = 0; i < AUDIO_SOURCES_NUM; i++) {
		if (audio_sources[i] != (void *)0xDEADBEEF) {
			audio_player_stop(audio_sources[i]);
			audio_sources[i] = (void *)0xDEADBEEF;
		}
	}
#endif
}

void nativePauseAllSounds() {
	//sceClibPrintf("nativePauseAllSounds\n");
	audio_player_set_pause_all_sounds(1);
}

void nativeResumeAllSounds() {
	//sceClibPrintf("nativeResumeAllSounds\n");
	audio_player_set_pause_all_sounds(0);
}

int nativeIsMediaPlaying(int id) {
	if (audio_sources[id] == (void *)0xDEADBEEF)
		return 0;
	return audio_player_is_playing(audio_sources[id]);
}

void nativeKillSound(int id) {
	//sceClibPrintf("nativeKill/StopSound %s\n", sounds[id]);
	if (audio_sources[id] != (void *)0xDEADBEEF) {
		audio_player_stop(audio_sources[id]);
		audio_sources[id] = (void *)0xDEADBEEF;
	}
}

void nativePauseSound(int id) {
	//sceClibPrintf("nativePauseSound %s\n", sounds[id]);
	if (audio_sources[id] != (void *)0xDEADBEEF) {
		audio_player_set_pause(audio_sources[id], 1);
	}
}

void nativeResumeSound(int id) {
	//sceClibPrintf("nativeResumeSound %s\n", sounds[id]);
	if (audio_sources[id] != (void *)0xDEADBEEF) {
		audio_player_set_pause(audio_sources[id], 0);
	}
}

void nativeSetSoundVolume(int id, float vol) {
	//sceClibPrintf("nativeSetSoundVolume %d %f\n", id, vol);
	if (audio_sources[id] != (void *)0xDEADBEEF) {
		audio_player_set_volume(audio_sources[id], vol);
	}
}

void patch_game(void) {
	audio_player_init();
	for (int i = 0; i < AUDIO_SOURCES_NUM; i++) {
		audio_sources[i] = (void *)0xDEADBEEF;
	}
	
	hook_addr(so_symbol(&main_mod, "nativeExit"), (uintptr_t)&nativeExit);
	hook_addr(so_symbol(&main_mod, "nativeLoadMovie"), (uintptr_t)&nativeLoadMovie);
	hook_addr(so_symbol(&main_mod, "nativeIsVideoCompleted"), (uintptr_t)&nativeIsVideoCompleted);
	hook_addr(so_symbol(&main_mod, "nativeSetIntroVideoState"), (uintptr_t)&ret0);
	hook_addr(so_symbol(&main_mod, "nativeRegisterSoundFile"), (uintptr_t)&ret0);
	//hook_addr(so_symbol(&main_mod, "nativeIsWifiEnabled"), (uintptr_t)&ret1);	
	
	hook_addr(so_symbol(&main_mod, "nativePlaySoundBig"), (uintptr_t)&nativePlaySound);
	hook_addr(so_symbol(&main_mod, "nativePauseSoundBig"), (uintptr_t)&nativePauseSound);
	hook_addr(so_symbol(&main_mod, "nativeResumeSoundBig"), (uintptr_t)&nativeResumeSound);
	hook_addr(so_symbol(&main_mod, "nativeStopAllMusic"), (uintptr_t)&nativeStopAllSounds);
	hook_addr(so_symbol(&main_mod, "nativeStopAllSounds"), (uintptr_t)&nativeStopAllSounds);
	hook_addr(so_symbol(&main_mod, "nativeIsMediaPlaying"), (uintptr_t)&nativeIsMediaPlaying);
	hook_addr(so_symbol(&main_mod, "nativeStopSoundBig"), (uintptr_t)&nativeKillSound);
	hook_addr(so_symbol(&main_mod, "nativeSetVolumeBig"), (uintptr_t)&nativeSetSoundVolume);
	hook_addr(so_symbol(&main_mod, "nativeResumeAllSoundBig"), (uintptr_t)&nativeResumeAllSounds);
	hook_addr(so_symbol(&main_mod, "nativePauseAllSoundBig"), (uintptr_t)&nativePauseAllSounds);
	//hook_addr(so_symbol(&main_mod, "_Z21GFX_InitializeAndroidv"), (uintptr_t)&GFX_InitializeAndroid);
}

FILE *fopen_hook(char *fname, char *mode) {
	char new_path[1024];
	if (strncmp(fname, "/sdcard", 7) == 0) {
		snprintf(new_path, sizeof(new_path), "%s%s", DATA_PATH, fname + 7);
		fname = new_path;
	} else if (strncmp(fname, "/data/", 6) == 0) {
		snprintf(new_path, sizeof(new_path), "%s%s", DATA_PATH, strrchr(fname, '/'));
		fname = new_path;		
	}
	//sceClibPrintf("fopen %s\n", fname);
	return fopen(fname, mode);
}

extern void *__aeabi_atexit;
extern void *__aeabi_d2f;
extern void *__aeabi_d2iz;
extern void *__aeabi_d2uiz;
extern void *__aeabi_dadd;
extern void *__aeabi_dcmpeq;
extern void *__aeabi_dcmpge;
extern void *__aeabi_dcmpgt;
extern void *__aeabi_dcmple;
extern void *__aeabi_dcmplt;
extern void *__aeabi_ddiv;
extern void *__aeabi_dmul;
extern void *__aeabi_dsub;
extern void *__aeabi_f2d;
extern void *__aeabi_f2iz;
extern void *__aeabi_fadd;
extern void *__aeabi_fcmpeq;
extern void *__aeabi_fcmpge;
extern void *__aeabi_fcmpgt;
extern void *__aeabi_fcmple;
extern void *__aeabi_fcmplt;
extern void *__aeabi_fcmpun;
extern void *__aeabi_fdiv;
extern void *__aeabi_fmul;
extern void *__aeabi_fsub;
extern void *__aeabi_i2d;
extern void *__aeabi_i2f;
extern void *__aeabi_idiv;
extern void *__aeabi_idivmod;
extern void *__aeabi_ldivmod;
extern void *__aeabi_lmul;
extern void *__aeabi_ui2d;
extern void *__aeabi_ui2f;
extern void *__aeabi_uidiv;
extern void *__aeabi_uidivmod;
extern void *__cxa_guard_acquire;
extern void *__cxa_guard_release;
extern void *__dso_handle;
extern void *__stack_chk_fail;
extern void *__aeabi_l2d;
extern void *__aeabi_d2lz;
extern void *__aeabi_l2f;
extern void *__aeabi_f2lz;
extern void *_Znaj;
extern void *_ZdaPv;
extern void *_Znwj;
extern void *_ZdlPv;

static int __stack_chk_guard_fake = 0x42424242;

static FILE __sF_fake[0x100][3];

void glEnable_hook(GLenum v) {
	glEnable(v);
}

int pthread_mutex_init_fake(pthread_mutex_t **uid, const pthread_mutexattr_t *mutexattr) {
	pthread_mutex_t *m = calloc(1, sizeof(pthread_mutex_t));
	if (!m)
		return -1;

	const int recursive = (mutexattr && *(const int *)mutexattr == 1);
	*m = recursive ? PTHREAD_RECURSIVE_MUTEX_INITIALIZER
								 : PTHREAD_MUTEX_INITIALIZER;

	int ret = pthread_mutex_init(m, mutexattr);
	if (ret < 0) {
		free(m);
		return -1;
	}

	*uid = m;

	return 0;
}

int pthread_mutex_destroy_fake(pthread_mutex_t **uid) {
	if (uid && *uid && (uintptr_t)*uid > 0x8000) {
		pthread_mutex_destroy(*uid);
		free(*uid);
		*uid = NULL;
	}
	return 0;
}

int pthread_mutex_lock_fake(pthread_mutex_t **uid) {
	int ret = 0;
	if (!*uid) {
		ret = pthread_mutex_init_fake(uid, NULL);
	} else if ((uintptr_t)*uid == 0x4000) {
		pthread_mutexattr_t attr;
		pthread_mutexattr_init(&attr);
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
		ret = pthread_mutex_init_fake(uid, &attr);
		pthread_mutexattr_destroy(&attr);
	} else if ((uintptr_t)*uid == 0x8000) {
		pthread_mutexattr_t attr;
		pthread_mutexattr_init(&attr);
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
		ret = pthread_mutex_init_fake(uid, &attr);
		pthread_mutexattr_destroy(&attr);
	}
	if (ret < 0)
		return ret;
	return pthread_mutex_lock(*uid);
}

int pthread_mutex_unlock_fake(pthread_mutex_t **uid) {
	int ret = 0;
	if (!*uid) {
		ret = pthread_mutex_init_fake(uid, NULL);
	} else if ((uintptr_t)*uid == 0x4000) {
		pthread_mutexattr_t attr;
		pthread_mutexattr_init(&attr);
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
		ret = pthread_mutex_init_fake(uid, &attr);
		pthread_mutexattr_destroy(&attr);
	} else if ((uintptr_t)*uid == 0x8000) {
		pthread_mutexattr_t attr;
		pthread_mutexattr_init(&attr);
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
		ret = pthread_mutex_init_fake(uid, &attr);
		pthread_mutexattr_destroy(&attr);
	}
	if (ret < 0)
		return ret;
	return pthread_mutex_unlock(*uid);
}

int pthread_cond_init_fake(pthread_cond_t **cnd, const int *condattr) {
	pthread_cond_t *c = calloc(1, sizeof(pthread_cond_t));
	if (!c)
		return -1;

	*c = PTHREAD_COND_INITIALIZER;

	int ret = pthread_cond_init(c, NULL);
	if (ret < 0) {
		free(c);
		return -1;
	}

	*cnd = c;

	return 0;
}

int pthread_cond_broadcast_fake(pthread_cond_t **cnd) {
	if (!*cnd) {
		if (pthread_cond_init_fake(cnd, NULL) < 0)
			return -1;
	}
	return pthread_cond_broadcast(*cnd);
}

int pthread_cond_signal_fake(pthread_cond_t **cnd) {
	if (!*cnd) {
		if (pthread_cond_init_fake(cnd, NULL) < 0)
			return -1;
	}
	return pthread_cond_signal(*cnd);
}

int pthread_cond_destroy_fake(pthread_cond_t **cnd) {
	if (cnd && *cnd) {
		pthread_cond_destroy(*cnd);
		free(*cnd);
		*cnd = NULL;
	}
	return 0;
}

int pthread_cond_wait_fake(pthread_cond_t **cnd, pthread_mutex_t **mtx) {
	if (!*cnd) {
		if (pthread_cond_init_fake(cnd, NULL) < 0)
			return -1;
	}
	return pthread_cond_wait(*cnd, *mtx);
}

int pthread_cond_timedwait_fake(pthread_cond_t **cnd, pthread_mutex_t **mtx, const struct timespec *t) {
	if (!*cnd) {
		if (pthread_cond_init_fake(cnd, NULL) < 0)
			return -1;
	}
	return pthread_cond_timedwait(*cnd, *mtx, t);
}

int pthread_create_fake(pthread_t *thread, const void *unused, void *entry, void *arg) {
	pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 512 * 1024);
	return pthread_create(thread, &attr, entry, arg);
}

int pthread_once_fake(volatile int *once_control, void (*init_routine)(void)) {
	if (!once_control || !init_routine)
		return -1;
	if (__sync_lock_test_and_set(once_control, 1) == 0)
		(*init_routine)();
	return 0;
}

GLuint screen_fbo;
void glBindFramebuffer_hook(GLenum target, GLuint framebuffer) {
	if (framebuffer == 0)
		framebuffer = screen_fbo;
	glBindFramebuffer(target, framebuffer);
}

extern void *__cxa_pure_virtual;
extern void *_ZnajRKSt9nothrow_t;
extern const char *BIONIC_ctype_;
extern const short *BIONIC_tolower_tab_;
extern const short *BIONIC_toupper_tab_;

/*
00:37:17	Unresolved import: bind
00:37:17	Unresolved import: gethostbyname
00:37:17	Unresolved import: strlcpy
00:37:17	Unresolved import: glGetUniformfv
00:37:17	Unresolved import: usleep
00:37:17	Unresolved import: connect
00:37:17	Unresolved import: getprotobyname
00:37:17	Unresolved import: send
00:37:17	Unresolved import: getsockname
00:37:17	Unresolved import: __gnu_Unwind_Find_exidx
00:37:17	Unresolved import: ioctl
00:37:17	Unresolved import: __cxa_begin_cleanup
00:37:17	Unresolved import: recv
00:37:17	Unresolved import: sendto
00:37:17	Unresolved import: select
00:37:17	Unresolved import: strncasecmp
00:37:17	Unresolved import: __assert
00:37:17	Unresolved import: setjmp
00:37:17	Unresolved import: mktime
00:37:17	Unresolved import: setsockopt
00:37:17	Unresolved import: gethostname
00:37:17	Unresolved import: getsockopt
00:37:17	Unresolved import: longjmp
00:37:17	Unresolved import: localtime
00:37:17	Unresolved import: close
00:37:17	Unresolved import: sigaction
00:37:17	Unresolved import: fputs
00:37:17	Unresolved import: inet_ntoa
00:37:17	Unresolved import: inet_addr
00:37:17	Unresolved import: recvfrom
00:37:17	Unresolved import: socket
00:37:17	Unresolved import: __cxa_type_match
00:37:17	Unresolved import: glPointSizePointerOES
00:37:17	Unresolved import: __errno
00:37:17	Unresolved import: accept
00:37:17	Unresolved import: listen
00:37:17	Unresolved import: pthread_join
*/

static so_default_dynlib default_dynlib[] = {
	// Online related
	{ "accept", (uintptr_t)&accept},
	{ "bind", (uintptr_t)&bind},
	{ "close", (uintptr_t)&close},
	{ "connect", (uintptr_t)&connect},
	{ "getsockopt", (uintptr_t)&getsockopt},
	{ "inet_addr", (uintptr_t)&inet_addr},
	{ "inet_ntoa", (uintptr_t)&inet_ntoa},
	{ "listen", (uintptr_t)&listen},
	{ "recv", (uintptr_t)&recv},
	{ "recvfrom", (uintptr_t)&recvfrom},
	{ "select", (uintptr_t)&select},
	{ "sendto", (uintptr_t)&sendto},
	{ "setsockopt", (uintptr_t)&setsockopt},
	{ "sendto", (uintptr_t)&sendto},
	{ "socket", (uintptr_t)&socket},
	// Normal symbols
	{ "_ctype_", (uintptr_t)&BIONIC_ctype_},
	{ "_tolower_tab_", (uintptr_t)&BIONIC_tolower_tab_},
	{ "_toupper_tab_", (uintptr_t)&BIONIC_toupper_tab_},
	{ "_Znaj", (uintptr_t)&_Znaj },
	{ "_ZnajRKSt9nothrow_t", (uintptr_t)&_ZnajRKSt9nothrow_t },
	{ "_ZdaPv", (uintptr_t)&_ZdaPv },
	{ "_Znwj", (uintptr_t)&_Znwj },
	{ "_ZdlPv", (uintptr_t)&_ZdlPv },
	{ "__aeabi_atexit", (uintptr_t)&__aeabi_atexit },
	{ "__aeabi_f2lz", (uintptr_t)&__aeabi_f2lz },
	{ "__aeabi_d2lz", (uintptr_t)&__aeabi_d2lz },
	{ "__aeabi_l2f", (uintptr_t)&__aeabi_l2f },
	{ "__aeabi_l2d", (uintptr_t)&__aeabi_l2d },
	{ "__aeabi_d2f", (uintptr_t)&__aeabi_d2f },
	{ "__aeabi_d2iz", (uintptr_t)&__aeabi_d2iz },
	{ "__aeabi_d2uiz", (uintptr_t)&__aeabi_d2uiz },
	{ "__aeabi_dadd", (uintptr_t)&__aeabi_dadd },
	{ "__aeabi_dcmpeq", (uintptr_t)&__aeabi_dcmpeq },
	{ "__aeabi_dcmpge", (uintptr_t)&__aeabi_dcmpge },
	{ "__aeabi_dcmpgt", (uintptr_t)&__aeabi_dcmpgt },
	{ "__aeabi_dcmple", (uintptr_t)&__aeabi_dcmple },
	{ "__aeabi_dcmplt", (uintptr_t)&__aeabi_dcmplt },
	{ "__aeabi_ddiv", (uintptr_t)&__aeabi_ddiv },
	{ "__aeabi_dmul", (uintptr_t)&__aeabi_dmul },
	{ "__aeabi_dsub", (uintptr_t)&__aeabi_dsub },
	{ "__aeabi_f2d", (uintptr_t)&__aeabi_f2d },
	{ "__aeabi_f2iz", (uintptr_t)&__aeabi_f2iz },
	{ "__aeabi_fadd", (uintptr_t)&__aeabi_fadd },
	{ "__aeabi_fcmpeq", (uintptr_t)&__aeabi_fcmpeq },
	{ "__aeabi_fcmpge", (uintptr_t)&__aeabi_fcmpge },
	{ "__aeabi_fcmpgt", (uintptr_t)&__aeabi_fcmpgt },
	{ "__aeabi_fcmple", (uintptr_t)&__aeabi_fcmple },
	{ "__aeabi_fcmplt", (uintptr_t)&__aeabi_fcmplt },
	{ "__aeabi_fcmpun", (uintptr_t)&__aeabi_fcmpun },
	{ "__aeabi_fdiv", (uintptr_t)&__aeabi_fdiv },
	{ "__aeabi_fmul", (uintptr_t)&__aeabi_fmul },
	{ "__aeabi_fsub", (uintptr_t)&__aeabi_fsub },
	{ "__aeabi_i2d", (uintptr_t)&__aeabi_i2d },
	{ "__aeabi_i2f", (uintptr_t)&__aeabi_i2f },
	{ "__aeabi_idiv", (uintptr_t)&__aeabi_idiv },
	{ "__aeabi_idivmod", (uintptr_t)&__aeabi_idivmod },
	{ "__aeabi_ldivmod", (uintptr_t)&__aeabi_ldivmod },
	{ "__aeabi_lmul", (uintptr_t)&__aeabi_lmul },
	{ "__aeabi_ui2d", (uintptr_t)&__aeabi_ui2d },
	{ "__aeabi_ui2f", (uintptr_t)&__aeabi_ui2f },
	{ "__aeabi_uidiv", (uintptr_t)&__aeabi_uidiv },
	{ "__aeabi_uidivmod", (uintptr_t)&__aeabi_uidivmod },
	{ "__android_log_write", (uintptr_t)&__android_log_write },
	{ "__android_log_print", (uintptr_t)&__android_log_print },
	{ "__cxa_guard_acquire", (uintptr_t)&__cxa_guard_acquire },
	{ "__cxa_guard_release", (uintptr_t)&__cxa_guard_release },
	{ "__cxa_pure_virtual", (uintptr_t)&__cxa_pure_virtual },
	{ "__dso_handle", (uintptr_t)&__dso_handle },
	{ "__sF", (uintptr_t)&__sF_fake },
	{ "__stack_chk_fail", (uintptr_t)&__stack_chk_fail },
	{ "__stack_chk_guard", (uintptr_t)&__stack_chk_guard_fake },
	{ "abort", (uintptr_t)&abort },
	{ "acos", (uintptr_t)&acos },
	{ "acosf", (uintptr_t)&acosf },
	{ "asin", (uintptr_t)&asin },
	{ "asinf", (uintptr_t)&asinf },
	{ "atan", (uintptr_t)&atan },
	{ "atan2", (uintptr_t)&atan2 },
	{ "atan2f", (uintptr_t)&atan2f },
	{ "atanf", (uintptr_t)&atanf },
	{ "atoi", (uintptr_t)&atoi },
	{ "calloc", (uintptr_t)&calloc },
	{ "ceil", (uintptr_t)&ceil },
	{ "ceilf", (uintptr_t)&ceilf },
	// { "chdir", (uintptr_t)&chdir },
	// { "close", (uintptr_t)&close },
	// { "connect", (uintptr_t)&connect },
	{ "cos", (uintptr_t)&cos },
	{ "cosf", (uintptr_t)&cosf },
	{ "cosh", (uintptr_t)&cosh },
	{ "exit", (uintptr_t)&exit },
	{ "exp", (uintptr_t)&exp },
	{ "expf", (uintptr_t)&expf },
	{ "fclose", (uintptr_t)&fclose },
	{ "fcntl", (uintptr_t)&ret0 },
	{ "fgetc", (uintptr_t)&fgetc },
	{ "fgets", (uintptr_t)&fgets },
	{ "floor", (uintptr_t)&floor },
	{ "floorf", (uintptr_t)&floorf },
	{ "fmod", (uintptr_t)&fmod },
	{ "fmodf", (uintptr_t)&fmodf },
	{ "fopen", (uintptr_t)&fopen_hook },
	{ "fprintf", (uintptr_t)&fprintf },
	{ "fputc", (uintptr_t)&fputc },
	{ "fread", (uintptr_t)&fread },
	{ "fflush", (uintptr_t)&fflush },
	{ "free", (uintptr_t)&free },
	{ "fseek", (uintptr_t)&fseek },
	{ "ftell", (uintptr_t)&ftell },
	{ "fwrite", (uintptr_t)&fwrite },
	// { "gethostbyname", (uintptr_t)&gethostbyname },
	{ "gettimeofday", (uintptr_t)&gettimeofday },
	{ "glActiveTexture", (uintptr_t)&glActiveTexture },
	{ "glAlphaFunc", (uintptr_t)&glAlphaFunc },
	{ "glBindBuffer", (uintptr_t)&glBindBuffer },
	{ "glBindBuffer", (uintptr_t)&glBindBuffer },
	{ "glBindFramebuffer", (uintptr_t)&glBindFramebuffer_hook },
	{ "glBindFramebufferOES", (uintptr_t)&glBindFramebuffer_hook },
	{ "glBindTexture", (uintptr_t)&glBindTexture },
	{ "glBlendFunc", (uintptr_t)&glBlendFunc },
	{ "glBufferData", (uintptr_t)&glBufferData },
	{ "glBufferSubData", (uintptr_t)&glBufferSubData },
	{ "glClear", (uintptr_t)&glClear },
	{ "glClearColor", (uintptr_t)&glClearColor },
	{ "glClearDepthf", (uintptr_t)&glClearDepthf },
	{ "glClientActiveTexture", (uintptr_t)&glClientActiveTexture },
	{ "glClipPlanef", (uintptr_t)&glClipPlanef },
	{ "glColor4f", (uintptr_t)&glColor4f },
	{ "glColor4ub", (uintptr_t)&glColor4ub },
	{ "glColorMask", (uintptr_t)&glColorMask },
	{ "glColorPointer", (uintptr_t)&glColorPointer },
	{ "glCompressedTexImage2D", (uintptr_t)&glCompressedTexImage2D },
	{ "glCopyTexImage2D", (uintptr_t)&glCopyTexImage2D },
	{ "glCullFace", (uintptr_t)&glCullFace },
	{ "glDeleteBuffers", (uintptr_t)&glDeleteBuffers },
	{ "glDeleteTextures", (uintptr_t)&glDeleteTextures },
	{ "glDepthFunc", (uintptr_t)&glDepthFunc },
	{ "glDepthMask", (uintptr_t)&glDepthMask },
	{ "glDepthRangef", (uintptr_t)&glDepthRangef },
	{ "glDisable", (uintptr_t)&glDisable },
	{ "glDisableClientState", (uintptr_t)&glDisableClientState },
	{ "glDrawArrays", (uintptr_t)&glDrawArrays },
	{ "glDrawElements", (uintptr_t)&glDrawElements },
	{ "glEnable", (uintptr_t)&glEnable_hook },
	{ "glEnableClientState", (uintptr_t)&glEnableClientState },
	{ "glFogf", (uintptr_t)&glFogf },
	{ "glFogfv", (uintptr_t)&glFogfv },
	{ "glFrontFace", (uintptr_t)&glFrontFace },
	{ "glFrustumf", (uintptr_t)&glFrustumf },
	{ "glGenBuffers", (uintptr_t)&glGenBuffers },
	{ "glGenTextures", (uintptr_t)&glGenTextures },
	{ "glGetBooleanv", (uintptr_t)&glGetBooleanv },
	{ "glGetError", (uintptr_t)&glGetError },
	{ "glGetFloatv", (uintptr_t)&glGetFloatv },
	{ "glGetIntegerv", (uintptr_t)&glGetIntegerv },
	{ "glGetPointerv", (uintptr_t)&glGetPointerv },
	{ "glGetString", (uintptr_t)&glGetString },
	{ "glGetTexEnviv", (uintptr_t)&glGetTexEnviv },
	{ "glHint", (uintptr_t)&ret0 },
	{ "glIsEnabled", (uintptr_t)&glIsEnabled },
	{ "glLightModelfv", (uintptr_t)&glLightModelfv },
	{ "glLightf", (uintptr_t)&ret0 },
	{ "glLightfv", (uintptr_t)&glLightfv },
	{ "glLineWidth", (uintptr_t)&glLineWidth },
	{ "glLoadIdentity", (uintptr_t)&glLoadIdentity },
	{ "glLoadMatrixf", (uintptr_t)&glLoadMatrixf },
	{ "glMaterialf", (uintptr_t)&ret0 },
	{ "glMaterialfv", (uintptr_t)&glMaterialfv },
	{ "glMatrixMode", (uintptr_t)&glMatrixMode },
	{ "glMultMatrixf", (uintptr_t)&glMultMatrixf },
	{ "glNormal3f", (uintptr_t)&glNormal3f },
	{ "glNormalPointer", (uintptr_t)&glNormalPointer },
	{ "glOrthof", (uintptr_t)&glOrthof },
	{ "glOrthox", (uintptr_t)&glOrthox },
	{ "glPixelStorei", (uintptr_t)&ret0 },
	{ "glPointParameterf", (uintptr_t)&ret0 },
	{ "glPointSize", (uintptr_t)&glPointSize },
	{ "glPolygonOffset", (uintptr_t)&glPolygonOffset },
	{ "glPopMatrix", (uintptr_t)&glPopMatrix },
	{ "glPushMatrix", (uintptr_t)&glPushMatrix },
	{ "glReadPixels", (uintptr_t)&glReadPixels },
	{ "glRotatef", (uintptr_t)&glRotatef },
	{ "glScalef", (uintptr_t)&glScalef },
	{ "glScissor", (uintptr_t)&glScissor },
	{ "glShadeModel", (uintptr_t)&ret0 },
	{ "glStencilFunc", (uintptr_t)&glStencilFunc },
	{ "glStencilMask", (uintptr_t)&glStencilMask },
	{ "glStencilOp", (uintptr_t)&glStencilOp },
	{ "glTexCoordPointer", (uintptr_t)&glTexCoordPointer },
	{ "glTexEnvf", (uintptr_t)&glTexEnvf },
	{ "glTexEnvfv", (uintptr_t)&glTexEnvfv },
	{ "glTexEnvi", (uintptr_t)&glTexEnvi },
	{ "glTexImage2D", (uintptr_t)&glTexImage2D },
	{ "glTexParameterf", (uintptr_t)&glTexParameterf },
	{ "glTexParameteri", (uintptr_t)&glTexParameteri },
	{ "glTexParameterx", (uintptr_t)&glTexParameterx },
	{ "glTexSubImage2D", (uintptr_t)&glTexSubImage2D },
	{ "glTranslatef", (uintptr_t)&glTranslatef },
	{ "glVertexPointer", (uintptr_t)&glVertexPointer },
	{ "glViewport", (uintptr_t)&glViewport },
	{ "isalnum", (uintptr_t)&isalnum },
	{ "isalpha", (uintptr_t)&isalpha },
	{ "isspace", (uintptr_t)&isspace },
	{ "iswupper", (uintptr_t)&iswupper },
	{ "iswxdigit", (uintptr_t)&iswxdigit },
	{ "ldexp", (uintptr_t)&ldexp },
	{ "localtime", (uintptr_t)&localtime },
	{ "log", (uintptr_t)&log },
	{ "logf", (uintptr_t)&logf },
	{ "log10", (uintptr_t)&log10 },
	{ "log10f", (uintptr_t)&log10f },
	{ "lrand48", (uintptr_t)&lrand48 },
	{ "malloc", (uintptr_t)&malloc },
	{ "memchr", (uintptr_t)&memchr },
	{ "memcmp", (uintptr_t)&memcmp },
	{ "memcpy", (uintptr_t)&memcpy },
	{ "memmove", (uintptr_t)&memmove },
	{ "memset", (uintptr_t)&memset },
	{ "modf", (uintptr_t)&modf },
	// { "nanosleep", (uintptr_t)&nanosleep },
	{ "pow", (uintptr_t)&pow },
	{ "powf", (uintptr_t)&powf },
	{ "pthread_attr_destroy", (uintptr_t)&ret0 },
	{ "pthread_attr_init", (uintptr_t)&ret0 },
	{ "pthread_attr_setschedparam", (uintptr_t)&ret0 },
	{ "pthread_attr_setdetachstate", (uintptr_t)&ret0 },
	{ "pthread_attr_setstacksize", (uintptr_t)&ret0 },
	{ "pthread_attr_setschedpolicy", (uintptr_t)&ret0 },
	{ "pthread_cond_signal", (uintptr_t)&pthread_cond_signal_fake},
	{ "pthread_cond_broadcast", (uintptr_t)&pthread_cond_broadcast_fake},
	{ "pthread_cond_wait", (uintptr_t)&pthread_cond_wait_fake},
	{ "pthread_create", (uintptr_t)&pthread_create_fake },
	{ "pthread_getschedparam", (uintptr_t)&pthread_getschedparam },
	{ "pthread_getspecific", (uintptr_t)&pthread_getspecific },
	{ "pthread_key_create", (uintptr_t)&pthread_key_create },
	{ "pthread_key_delete", (uintptr_t)&pthread_key_delete },
	{ "pthread_mutex_destroy", (uintptr_t)&pthread_mutex_destroy_fake },
	{ "pthread_mutex_init", (uintptr_t)&pthread_mutex_init_fake },
	{ "pthread_mutex_lock", (uintptr_t)&pthread_mutex_lock_fake },
	{ "pthread_mutex_unlock", (uintptr_t)&pthread_mutex_unlock_fake },
	{ "pthread_once", (uintptr_t)&pthread_once_fake },
	{ "pthread_self", (uintptr_t)&pthread_self },
	{ "pthread_setschedparam", (uintptr_t)&pthread_setschedparam },
	{ "pthread_setspecific", (uintptr_t)&pthread_setspecific },
	{ "printf", (uintptr_t)&printf },
	{ "puts", (uintptr_t)&ret0 },
	{ "qsort", (uintptr_t)&qsort },
	{ "realloc", (uintptr_t)&realloc },
	// { "recv", (uintptr_t)&recv },
	{ "remove", (uintptr_t)&remove },
	{ "rename", (uintptr_t)&rename },
	{ "sin", (uintptr_t)&sin },
	{ "sinf", (uintptr_t)&sinf },
	{ "sinh", (uintptr_t)&sinh },
	{ "snprintf", (uintptr_t)&snprintf },
	// { "socket", (uintptr_t)&socket },
	{ "sprintf", (uintptr_t)&sprintf },
	{ "sqrt", (uintptr_t)&sqrt },
	{ "sqrtf", (uintptr_t)&sqrtf },
	{ "srand48", (uintptr_t)&srand48 },
	{ "sscanf", (uintptr_t)&sscanf },
	{ "strcasecmp", (uintptr_t)&strcasecmp },
	{ "strcat", (uintptr_t)&strcat },
	{ "strchr", (uintptr_t)&strchr },
	{ "strcmp", (uintptr_t)&strcmp },
	{ "strcpy", (uintptr_t)&strcpy },
	{ "strdup", (uintptr_t)&strdup },
	{ "strftime", (uintptr_t)&strftime },
	{ "strlcpy", (uintptr_t)&strlcpy },
	{ "strlen", (uintptr_t)&strlen },
	{ "strncasecmp", (uintptr_t)&strncasecmp },
	{ "strncat", (uintptr_t)&strncat },
	{ "strncmp", (uintptr_t)&strncmp },
	{ "strncpy", (uintptr_t)&strncpy },
	{ "strpbrk", (uintptr_t)&strpbrk },
	{ "strrchr", (uintptr_t)&strrchr },
	{ "strstr", (uintptr_t)&strstr },
	{ "strtod", (uintptr_t)&strtod },
	{ "strtol", (uintptr_t)&strtol },
	{ "strtoul", (uintptr_t)&strtoul },
	{ "tan", (uintptr_t)&tan },
	{ "tanf", (uintptr_t)&tanf },
	{ "tanh", (uintptr_t)&tanh },
	{ "time", (uintptr_t)&time },
	{ "tolower", (uintptr_t)&tolower },
	{ "towlower", (uintptr_t)&towlower },
	{ "ungetc", (uintptr_t)&ungetc },
	{ "vsprintf", (uintptr_t)&vsprintf },
	{ "vsnprintf", (uintptr_t)&vsnprintf },
	{ "wcscmp", (uintptr_t)&wcscmp },
	{ "wcscpy", (uintptr_t)&wcscpy },
	{ "wcslen", (uintptr_t)&wcslen },
	{ "wmemcpy", (uintptr_t)&wmemcpy },
	{ "wmemmove", (uintptr_t)&wmemmove },
	{ "wmemset", (uintptr_t)&wmemset },
	// { "write", (uintptr_t)&write },
};

static char fake_vm[0x1000];
static char fake_env[0x1000];

enum MethodIDs {
	UNKNOWN = 0,
} MethodIDs;

typedef struct {
	char *name;
	enum MethodIDs id;
} NameToMethodID;

static NameToMethodID name_to_method_ids[] = {
};

int GetStaticMethodID(void *env, void *class, const char *name, const char *sig) {
	sceClibPrintf("GetStaticMethodID %s\n", name);
	for (int i = 0; i < sizeof(name_to_method_ids) / sizeof(NameToMethodID); i++) {
		if (strcmp(name, name_to_method_ids[i].name) == 0)
			return name_to_method_ids[i].id;
	}

	return UNKNOWN;
}

int CallStaticIntMethod(void *env, void *obj, int methodID) {
	return 0;
}

float CallStaticFloatMethod(void *env, void *obj, int methodID) {
	return 0.0f;
}

void CallStaticVoidMethod(void *env, void *obj, int methodID) {
}

void *NewGlobalRef(void) {
	return (void *)0x42424242;
}

int check_kubridge(void) {
	int search_unk[2];
	return _vshKernelSearchModuleByName("kubridge", search_unk);
}

int file_exists(const char *path) {
	SceIoStat stat;
	return sceIoGetstat(path, &stat) >= 0;
}

enum {
	KEYCODE_CIRCLE = 4,
	KEYCODE_DPAD_UP = 19,
	KEYCODE_DPAD_DOWN = 20,
	KEYCODE_DPAD_LEFT = 21,
	KEYCODE_DPAD_RIGHT = 22,
	KEYCODE_CROSS = 23,
	KEYCODE_SQUARE = 99,
	KEYCODE_TRIANGLE = 100,
	KEYCODE_BUTTON_L1 = 102,
	KEYCODE_BUTTON_R1 = 103,
	KEYCODE_BUTTON_START = 108,
	KEYCODE_BUTTON_SELECT = 109,
};

enum {
	SCANCODE_UNK = 0,
	SCANCODE_LEFT = 103,
	SCANCODE_DOWN = 105,
	SCANCODE_UP = 106,
	SCANCODE_RIGHT = 108,
	SCANCODE_START = 139,
	SCANCODE_CROSS = 304,
	SCANCODE_CIRCLE = 305,
	SCANCODE_SQUARE = 307,
	SCANCODE_TRIANGLE = 308,
	SCANCODE_L1 = 310,
	SCANCODE_R1 = 311,
	SCANCODE_SELECT = 314
};

enum {
	SOURCE_TOUCHSCREEN = 0x00001002,
	SOURCE_TOUCHPAD = 0x00100008,
};

char *GetStringUTFChars(void *env, char *string, int *isCopy) {
	if (isCopy)
		*isCopy = 0;
	return string;
}

int GetEnv(void *vm, void **env, int r2) {
	*env = fake_env;
	return 0;
}

int AttachCurrentThread(void *vm, void **p_env, void *thr_args) {
	GetEnv(vm, p_env, 0);
	return 0;
}

void video_open(const char *path);
int video_draw();
void video_stop();
void video_draw_generic(GLuint tex);

void *pthread_main(void *arg);

int main(int argc, char *argv[]) {
	pthread_t t;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 2 * 1024 * 1024);
	pthread_create(&t, &attr, pthread_main, NULL);

	return sceKernelExitDeleteThread(0);
}

void *pthread_main(void *arg) {
	sceSysmoduleLoadModule(SCE_SYSMODULE_AVPLAYER);
	sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
	sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);

	scePowerSetArmClockFrequency(444);
	scePowerSetBusClockFrequency(222);
	scePowerSetGpuClockFrequency(222);
	scePowerSetGpuXbarClockFrequency(166);
	sceAppUtilInit(&(SceAppUtilInitParam){}, &(SceAppUtilBootParam){});

	if (check_kubridge() < 0)
		fatal_error("Error kubridge.skprx is not installed.");

	if (!file_exists("ur0:/data/libshacccg.suprx") && !file_exists("ur0:/data/external/libshacccg.suprx"))
		fatal_error("Error libshacccg.suprx is not installed.");

	if (so_file_load(&main_mod, SO_PATH, LOAD_ADDRESS) < 0)
		fatal_error("Error could not load %s.", SO_PATH);

	so_relocate(&main_mod);
	so_resolve(&main_mod, default_dynlib, sizeof(default_dynlib), 0);
	
	vglInitExtended(0, SCREEN_W, SCREEN_H, MEMORY_VITAGL_THRESHOLD_MB * 1024 * 1024, SCE_GXM_MULTISAMPLE_4X);

	// Play logo videos
	SceCtrlData pad;
	video_open("ux0:data/letsgolf2/gameloft/games/lgolf2/intro.mp4");
	while (!video_draw()) {
		sceCtrlPeekBufferPositive(0, &pad, 1);
		if (pad.buttons)
			video_stop();
		vglSwapBuffers(GL_FALSE);
	}
	
	patch_game();
	so_flush_caches(&main_mod);
	so_initialize(&main_mod);

	memset(fake_vm, 'A', sizeof(fake_vm));
	*(uintptr_t *)(fake_vm + 0x00) = (uintptr_t)fake_vm; // just point to itself...
	*(uintptr_t *)(fake_vm + 0x10) = (uintptr_t)AttachCurrentThread;
	*(uintptr_t *)(fake_vm + 0x18) = (uintptr_t)GetEnv;

	int (* JNI_OnLoad)(void *vm, void *reserved) = (void *)so_symbol(&main_mod, "JNI_OnLoad");
	if (JNI_OnLoad)
		JNI_OnLoad(fake_vm, NULL);
	
	memset(fake_env, 'A', sizeof(fake_env));
	*(uintptr_t *)(fake_env + 0x00) = (uintptr_t)fake_env; // just point to itself...
	*(uintptr_t *)(fake_env + 0x54) = (uintptr_t)NewGlobalRef;
	*(uintptr_t *)(fake_env + 0x1C4) = (uintptr_t)GetStaticMethodID;
	*(uintptr_t *)(fake_env + 0x204) = (uintptr_t)CallStaticIntMethod;
	*(uintptr_t *)(fake_env + 0x21C) = (uintptr_t)CallStaticFloatMethod;
	*(uintptr_t *)(fake_env + 0x234) = (uintptr_t)CallStaticVoidMethod;
	*(uintptr_t *)(fake_env + 0x2A4) = (uintptr_t)GetStringUTFChars;
	
	int (* Game_nativeInit)(void *env, void *obj, int is_demo);
	int (* GameRenderer_nativeInit)(void *env, void *obj, int unused, int ctx_lost, int w, int h, char *lang);
	int (* nativeRender)();
	int (* GLMediaPlayer_nativeInit)(void *env, void *obj);
	int (* nativeResume)();
	int (* nativeOnTouchPressed)(void *env, void *obj, int id, int x, int y, int unk);
	int (* nativeOnTouchReleased)(void *env, void *obj, int id, int x, int y, int unk);
	int (* nativeOnTouchMoved)(void *env, void *obj, int id, int x, int y, int unk);
	int (* appKeyPressed)(int id, int scancode) = (void *)so_symbol(&main_mod, "notifyKeyPressed");
	int (* appKeyReleased)(int id, int scancode) = (void *)so_symbol(&main_mod, "notifyKeyReleased");
	int (* GetJNIEnv)(void *env);

	GetJNIEnv = (void *)so_symbol(&main_mod, "Java_com_gameloft_android_GAND_GloftLG2P_ML_GameRenderer_nativeGetJNIEnv");
	Game_nativeInit = (void *)so_symbol(&main_mod, "Java_com_gameloft_android_GAND_GloftLG2P_ML_LGOLF2_nativeInit");
	GameRenderer_nativeInit = (void *)so_symbol(&main_mod, "Java_com_gameloft_android_GAND_GloftLG2P_ML_GameRenderer_nativeInit");
	nativeRender = (void *)so_symbol(&main_mod, "Java_com_gameloft_android_GAND_GloftLG2P_ML_GameRenderer_nativeRender");
	GLMediaPlayer_nativeInit = (void *)so_symbol(&main_mod, "Java_com_gameloft_android_GAND_GloftLG2P_ML_GLMediaPlayer_nativeInit");
	nativeResume = (void *)so_symbol(&main_mod, "Java_com_gameloft_android_GAND_GloftLG2P_ML_GameGLSurfaceView_nativeResume");
	nativeOnTouchPressed = (void *)so_symbol(&main_mod, "Java_com_gameloft_android_GAND_GloftLG2P_ML_LGOLF2_nativeTouchPressed");
	nativeOnTouchMoved = (void *)so_symbol(&main_mod, "Java_com_gameloft_android_GAND_GloftLG2P_ML_LGOLF2_nativeTouchMoved");
	nativeOnTouchReleased = (void *)so_symbol(&main_mod, "Java_com_gameloft_android_GAND_GloftLG2P_ML_LGOLF2_nativeTouchReleased");
	
	// Detect language
	char lang[3];
	int _lang;
	sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_LANG, &_lang);
	switch (_lang) {
	case SCE_SYSTEM_PARAM_LANG_ITALIAN:
		strcpy(lang, "it");
		break;
	case SCE_SYSTEM_PARAM_LANG_SPANISH:
		strcpy(lang, "es");
		break;
	case SCE_SYSTEM_PARAM_LANG_GERMAN:
		strcpy(lang, "de");
		break;
	case SCE_SYSTEM_PARAM_LANG_JAPANESE:
		strcpy(lang, "jp");
		break;
	case SCE_SYSTEM_PARAM_LANG_FRENCH:
		strcpy(lang, "fr");
		break;
	case SCE_SYSTEM_PARAM_LANG_CHINESE_T:
	case SCE_SYSTEM_PARAM_LANG_CHINESE_S:
		strcpy(lang, "zh");
		break;
	default:
		strcpy(lang, "en");
		break;
	}
	
	GetJNIEnv(fake_env);
	GameRenderer_nativeInit(fake_env, NULL, 0, 0, XPERIA_W, XPERIA_H, lang);
	Game_nativeInit(fake_env, NULL, 0);
	GLMediaPlayer_nativeInit(fake_env, NULL);

	int lastX[2] = { -1, -1 };
	int lastY[2] = { -1, -1 };
	
	#define physicalInput(btn, id, code) \
		if ((pad.buttons & btn) == btn) { \
			appKeyPressed(id, code); \
		} else if ((oldpad & btn) == btn) { \
			appKeyReleased(id, code); \
		}		
	
	GLuint screen_tex;
	glGenFramebuffers(1, &screen_fbo);
	glGenTextures(1, &screen_tex);
	glBindTexture(GL_TEXTURE_2D, screen_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, XPERIA_W, XPERIA_H, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glBindFramebuffer(GL_FRAMEBUFFER, screen_fbo);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, screen_tex, 0);
	float TOUCHSCREEN_MULT_X = (float)XPERIA_W / (float)SCREEN_W;
	float TOUCHSCREEN_MULT_Y = (float)XPERIA_H / (float)SCREEN_H;		
	
	uint32_t oldpad = 0;
	sceClibPrintf("Entering main loop\n");
	nativeResume();
	while (1) {
		SceTouchData touch;
		sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch, 1);
		SceCtrlData pad;
		sceCtrlPeekBufferPositive(0, &pad, 1);

		physicalInput(SCE_CTRL_CROSS, KEYCODE_CROSS, SCANCODE_CROSS)
		physicalInput(SCE_CTRL_CIRCLE, KEYCODE_CIRCLE, SCANCODE_CIRCLE)
		physicalInput(SCE_CTRL_SQUARE, KEYCODE_SQUARE, SCANCODE_SQUARE)
		physicalInput(SCE_CTRL_TRIANGLE, KEYCODE_TRIANGLE, SCANCODE_TRIANGLE)
		physicalInput(SCE_CTRL_UP, KEYCODE_DPAD_UP, SCANCODE_UP)
		physicalInput(SCE_CTRL_LEFT, KEYCODE_DPAD_LEFT, SCANCODE_LEFT)
		physicalInput(SCE_CTRL_RIGHT, KEYCODE_DPAD_RIGHT, SCANCODE_RIGHT)
		physicalInput(SCE_CTRL_DOWN, KEYCODE_DPAD_DOWN, SCANCODE_DOWN)
		physicalInput(SCE_CTRL_LTRIGGER, KEYCODE_BUTTON_L1, SCANCODE_L1)
		physicalInput(SCE_CTRL_RTRIGGER, KEYCODE_BUTTON_R1, SCANCODE_R1)
		physicalInput(SCE_CTRL_SELECT, KEYCODE_BUTTON_SELECT, SCANCODE_SELECT)
		physicalInput(SCE_CTRL_START, KEYCODE_BUTTON_START, SCANCODE_START)
		oldpad = pad.buttons;
		
		for (int i = 0; i < 2; i++) {
			if (i < touch.reportNum) {
				int x = (int)((float)touch.report[i].x * 0.5f * TOUCHSCREEN_MULT_X);
				int y = (int)((float)touch.report[i].y * 0.5f * TOUCHSCREEN_MULT_Y);

				if (lastX[i] == -1 || lastY[i] == -1) {
					nativeOnTouchPressed(fake_env, NULL, i, x, y, 0);
				} else if (lastX[i] != x || lastY[i] != y) {
					nativeOnTouchMoved(fake_env, NULL, i, x, y, 0);
				}
				lastX[i] = x;
				lastY[i] = y;
			} else {
				if (lastX[i] != -1 || lastY[i] != -1) {
					nativeOnTouchReleased(fake_env, NULL, i, lastX[i], lastY[i], 0);
				}
				lastX[i] = -1;
				lastY[i] = -1;
			}
		}
		
		glBindFramebuffer(GL_FRAMEBUFFER, screen_fbo);
		glViewport(0, 0, XPERIA_W, XPERIA_H);
		//glEnable(GL_SCISSOR_TEST);
		nativeRender();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, SCREEN_W, SCREEN_H);
		glDisable(GL_SCISSOR_TEST);
		glClear(GL_COLOR_BUFFER_BIT);
		video_draw_generic(screen_tex);
		vglSwapBuffers(GL_FALSE);
	}

	return 0;
}
