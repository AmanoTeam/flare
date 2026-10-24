#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 199901L
#define inline __inline__
#endif
#include <jni.h>
#include <math.h>
#include <stdlib.h>

typedef struct {
	unsigned int x;
	unsigned int y;
	unsigned int width;
	unsigned int height;
} CropBordersRect;

static const float CROPBORDERS_FILLED_RATIO_LIMIT = 0.0025;

static const unsigned char CROPBORDERS_BLACK_THRESHOLD = (unsigned char) (255.0 * 0.75);
static const unsigned char CROPBORDERS_WHITE_THRESHOLD = (unsigned char) (255.0 - 255.0 * 0.75);

static inline int is_black_pixel(const unsigned char* const pixels, const unsigned int width, const int x, const int y) {
	return pixels[y * width + x] < CROPBORDERS_BLACK_THRESHOLD;
}

static inline int is_white_pixel(const unsigned char* const pixels, const unsigned int width, const int x, const int y) {
	return pixels[y * width + x] > CROPBORDERS_WHITE_THRESHOLD;
}

static unsigned int find_border_top(const unsigned char* const pixels, const unsigned int width, const unsigned int height) {
	const int limit = (int) round(width * CROPBORDERS_FILLED_RATIO_LIMIT / 2);

	int (*detect)(const unsigned char* const, const unsigned int, const int, const int) = is_black_pixel;

	int x = 0;
	int y = 0;
	int count = 0;

	unsigned int black = 0;
	unsigned int white = 0;

	for (x = 0; x < (int) width; x += 2) {
		if (is_black_pixel(pixels, width, x, 0)) {
			black++;
		}
		else if (is_white_pixel(pixels, width, x, 0)) {
			white++;
		}
	}

	if (white > (unsigned int) limit && black > (unsigned int) limit) {
		return 0;
	}
	else if (black > (unsigned int) limit) {
		detect = is_white_pixel;
	}

	for (y = 1; y < (int) height; y++) {
		count = 0;

		for (x = 0; x < (int) width; x += 2) {
			if (detect(pixels, width, x, y)) {
				count++;
			}
		}

		if (count > limit) {
			return (unsigned int) y;
		}
	}

	return 0;
}

static unsigned int find_border_bottom(const unsigned char* const pixels, const unsigned int width, const unsigned int height) {
	const int limit = (int) round(width * CROPBORDERS_FILLED_RATIO_LIMIT / 2);

	int (*detect)(const unsigned char* const, const unsigned int, const int, const int) = is_black_pixel;

	int x = 0;
	int y = 0;
	int count = 0;

	unsigned int black = 0;
	unsigned int white = 0;

	for (x = 0; x < (int) width; x += 2) {
		if (is_black_pixel(pixels, width, x, (int) height - 1)) {
			black++;
		}
		else if (is_white_pixel(pixels, width, x, (int) height - 1)) {
			white++;
		}
	}

	if (white > (unsigned int) limit && black > (unsigned int) limit) {
		return height;
	}
	else if (black > (unsigned int) limit) {
		detect = is_white_pixel;
	}

	for (y = (int) height - 2; y > 0; y--) {
		count = 0;

		for (x = 0; x < (int) width; x += 2) {
			if (detect(pixels, width, x, y)) {
				count++;
			}
		}

		if (count > limit) {
			return (unsigned int) y + 1;
		}
	}

	return height;
}

static unsigned int find_border_left(const unsigned char* const pixels, const unsigned int width, const unsigned int height, const unsigned int top, const unsigned int bottom) {
	const int limit = (int) round(height * CROPBORDERS_FILLED_RATIO_LIMIT / 2);

	int (*detect)(const unsigned char* const, const unsigned int, const int, const int) = is_black_pixel;

	int x = 0;
	int y = 0;
	int count = 0;

	unsigned int black = 0;
	unsigned int white = 0;

	for (y = (int) top; y < (int) bottom; y += 2) {
		if (is_black_pixel(pixels, width, 0, y)) {
			black++;
		}
		else if (is_white_pixel(pixels, width, 0, y)) {
			white++;
		}
	}

	if (white > (unsigned int) limit && black > (unsigned int) limit) {
		return 0;
	}
	else if (black > (unsigned int) limit) {
		detect = is_white_pixel;
	}

	for (x = 1; x < (int) width; x++) {
		count = 0;

		for (y = (int) top; y < (int) bottom; y += 2) {
			if (detect(pixels, width, x, y)) {
				count++;
			}
		}

		if (count > limit) {
			return (unsigned int) x;
		}
	}

	return 0;
}

static unsigned int find_border_right(const unsigned char* const pixels, const unsigned int width, const unsigned int height, const unsigned int top, const unsigned int bottom) {
	const int limit = (int) round(height * CROPBORDERS_FILLED_RATIO_LIMIT / 2);

	int (*detect)(const unsigned char* const, const unsigned int, const int, const int) = is_black_pixel;

	int x = 0;
	int y = 0;
	int count = 0;

	unsigned int black = 0;
	unsigned int white = 0;

	for (y = (int) top; y < (int) bottom; y += 2) {
		if (is_black_pixel(pixels, width, (int) width - 1, y)) {
			black++;
		}
		else if (is_white_pixel(pixels, width, (int) width - 1, y)) {
			white++;
		}
	}

	if (white > (unsigned int) limit && black > (unsigned int) limit) {
		return width;
	}
	else if (black > (unsigned int) limit) {
		detect = is_white_pixel;
	}

	for (x = (int) width - 2; x > 0; x--) {
		count = 0;

		for (y = (int) top; y < (int) bottom; y += 2) {
			if (detect(pixels, width, x, y)) {
				count++;
			}
		}

		if (count > limit) {
			return (unsigned int) x + 1;
		}
	}

	return width;
}

static CropBordersRect find_borders(const unsigned char* const pixels, const unsigned int width, const unsigned int height) {
	const unsigned int top = find_border_top(pixels, width, height);
	const unsigned int bottom = find_border_bottom(pixels, width, height);
	const unsigned int left = find_border_left(pixels, width, height, top, bottom);
	const unsigned int right = find_border_right(pixels, width, height, top, bottom);

	CropBordersRect rect = {0, 0, 0, 0};

	rect.x = left;
	rect.y = top;
	rect.width = right - left;
	rect.height = bottom - top;

	return rect;
}

JNIEXPORT jintArray JNICALL Java_com_davemorrissey_labs_subscaleview_CropBorders_findCropBorders(JNIEnv* env, jclass caller, jbyteArray data, jint width, jint height) {
	jbyte* rgba = NULL;
	unsigned char* gray = NULL;

	jintArray result = NULL;
	jint rect[4] = {0, 0, width, height};

	const unsigned long size = (unsigned long) ((width > 0 && height > 0) ? width : 0) * (unsigned long) ((width > 0 && height > 0) ? height : 0);

	(void) caller;

	if (size == 0 || data == NULL) {
		gray = NULL;
	}
	else {
		gray = malloc(size);
	}

	if (gray == NULL) {
		rgba = NULL;
	}
	else {
		rgba = (*env)->GetByteArrayElements(env, data, NULL);
	}

	if (rgba != NULL) {
		unsigned long index = 0;

		for (index = 0; index < size; index++) {
			const unsigned int r = (unsigned char) rgba[index * 4 + 0];
			const unsigned int g = (unsigned char) rgba[index * 4 + 1];
			const unsigned int b = (unsigned char) rgba[index * 4 + 2];

			gray[index] = (unsigned char) ((r * 77 + g * 150 + b * 29) >> 8);
		}

		(*env)->ReleaseByteArrayElements(env, data, rgba, JNI_ABORT);

		{
			const CropBordersRect cropped = find_borders(gray, width, height);

			rect[0] = (jint) cropped.x;
			rect[1] = (jint) cropped.y;
			rect[2] = (jint) cropped.width;
			rect[3] = (jint) cropped.height;
		}

		free(gray);
	}
	else {
		free(gray);
	}

	result = (*env)->NewIntArray(env, 4);

	if (result != NULL) {
		(*env)->SetIntArrayRegion(env, result, 0, 4, rect);
	}

	return result;

}
