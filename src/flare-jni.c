#include <jni.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32) && defined(_UNICODE)
	#include <windows.h>
#endif

#include "errors.h"
#include <libavutil/mem.h>
#include "flare.h"

JNIEXPORT jintArray JNICALL Java_com_amanoteam_flare_Flare_findBands(JNIEnv* env, jclass caller, jstring path) {

	jintArray result = NULL;

	jboolean copied = JNI_FALSE;

	const char* utf = NULL;

	int* bands = NULL;
	int count = 0;

	(void) caller;

	if (path == NULL) {
		return NULL;
	}

	utf = (*env)->GetStringUTFChars(env, path, &copied);

	if (utf == NULL) {
		return NULL;
	}

	if (flare_bands(utf, &bands, &count) != FLAREERR_SUCCESS) {
		(*env)->ReleaseStringUTFChars(env, path, utf);

		av_free(bands);

		return NULL;
	}

	(*env)->ReleaseStringUTFChars(env, path, utf);

	result = (*env)->NewIntArray(env, count);

	if (result == NULL || count == 0) {
		av_free(bands);

		return NULL;
	}

	(*env)->SetIntArrayRegion(env, result, 0, count, (const jint*) bands);

	av_free(bands);

	return result;
}

JNIEXPORT jint JNICALL Java_com_amanoteam_flare_Flare_flareMain(JNIEnv* env, jclass caller, jobjectArray arguments) {

	static const char FLARE_JNI_PROGRAM_NAME[] = "flare";

	jsize count = 0;
	jsize index = 0;

	size_t size = 0;

	argv_t** argv = NULL;
	const char* utf = NULL;

	jstring item = NULL;

	jint code = 0;

	(void) caller;

	if (arguments == NULL) {
		return -1;
	}

	count = (*env)->GetArrayLength(env, arguments);

	argv = calloc((size_t) (count + 1), sizeof(*argv));

	if (argv == NULL) {
		return -1;
	}

	#if !defined(_WIN32) || !defined(_UNICODE)
		argv[0] = (argv_t*) FLARE_JNI_PROGRAM_NAME;
	#endif

	while (index < count) {
		item = (jstring) (*env)->GetObjectArrayElement(env, arguments, index);

		utf = (*env)->GetStringUTFChars(env, item, NULL);

		if (utf == NULL) {
			code = -1;
			goto end;
		}

		size = strlen(utf);

		#if defined(_WIN32) && defined(_UNICODE)
			{
				int wsize = MultiByteToWideChar(CP_UTF8, 0, utf, (int) size, NULL, 0);

				if (wsize == 0) {
					(*env)->ReleaseStringUTFChars(env, item, utf);

					code = -1;
					goto end;
				}

				argv[index + 1] = malloc(((size_t) wsize + 1) * sizeof(wchar_t));

				if (argv[index + 1] == NULL) {
					(*env)->ReleaseStringUTFChars(env, item, utf);

					code = -1;
					goto end;
				}

				MultiByteToWideChar(CP_UTF8, 0, utf, (int) size, (wchar_t*) argv[index + 1], wsize);

				((wchar_t*) argv[index + 1])[wsize] = L'\0';
			}
		#else
			argv[index + 1] = malloc(size + 1);

			if (argv[index + 1] == NULL) {
				(*env)->ReleaseStringUTFChars(env, item, utf);

				code = -1;
				goto end;
			}

			memcpy(argv[index + 1], utf, size + 1);
		#endif

		(*env)->ReleaseStringUTFChars(env, item, utf);

		index++;
	}

	code = (jint) flare_main((int) (count + 1), argv);

	end:;

	while (index > 0) {
		free(argv[index]);
		index--;
	}

	free(argv);

	return code;

}
