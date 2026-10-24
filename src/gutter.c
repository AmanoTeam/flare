#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 199901L
#define inline __inline__
#endif
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(__ANDROID__)
	#include <android/log.h>
#endif
#include <stdarg.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/log.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>

#include "argparse.h"
#include "errors.h"
#include "flare.h"
#include "program_help.h"

#define WHITE 240
#define BLACK 15

static const AVRational BAND   = {199, 200};
static const AVRational MARGIN = {999, 1000};
static const AVRational TB     = {1, 25};

static const char KOPT_I[] = "i";
static const char KOPT_INPUT[] = "input";

static const char KOPT_O[] = "o";
static const char KOPT_OUTPUT[] = "output";

static const char KOPT_M[] = "m";
static const char KOPT_MINIMUM[] = "minimum";

static const char KOPT_S[] = "s";
static const char KOPT_SIDES[] = "sides";

static const char KOPT_H[] = "h";
static const char KOPT_HELP[] = "help";

static const char KOPT_V[] = "v";
static const char KOPT_VERSION[] = "version";

#define ACTION_UNKNOWN (0x00)
#define ACTION_INPUT (0x01)
#define ACTION_OUTPUT (0x02)
#define ACTION_SIDES (0x03)
#define ACTION_MINIMUM (0x04)
#define ACTION_HELP (0x10)
#define ACTION_VERSION (0x11)

static int get_action(const arg_t* const arg) {

	int status = 0;

	status = (strcmp(arg->key, KOPT_I) == 0 || strcmp(arg->key, KOPT_INPUT) == 0);

	if (status) {
		return ACTION_INPUT;
	}

	status = (strcmp(arg->key, KOPT_O) == 0 || strcmp(arg->key, KOPT_OUTPUT) == 0);

	if (status) {
		return ACTION_OUTPUT;
	}

	status = (strcmp(arg->key, KOPT_M) == 0 || strcmp(arg->key, KOPT_MINIMUM) == 0);

	if (status) {
		return ACTION_MINIMUM;
	}

	status = (strcmp(arg->key, KOPT_S) == 0 || strcmp(arg->key, KOPT_SIDES) == 0);

	if (status) {
		return ACTION_SIDES;
	}

	status = (strcmp(arg->key, KOPT_H) == 0 || strcmp(arg->key, KOPT_HELP) == 0);

	if (status) {
		return ACTION_HELP;
	}

	status = (strcmp(arg->key, KOPT_V) == 0 || strcmp(arg->key, KOPT_VERSION) == 0);

	if (status) {
		return ACTION_VERSION;
	}

	return ACTION_UNKNOWN;

}

static AVRational mkq(const int num, const int den) {

	AVRational r = {0, 1};

	r.num = num;
	r.den = den;

	return r;

}

static uint8_t* decode(const char* const path, int* W, int* H, int* S, int* err) {

	AVFormatContext* ic = NULL;
	const AVCodec* c = NULL;
	AVCodecContext* dc = NULL;
	AVPacket* pk = NULL;
	AVFrame* fr = NULL;
	struct SwsContext* sw = NULL;
	uint8_t* d[4] = {0};
	int l[4] = {0};
	int v = 0;
	int got = 0;

	if (avformat_open_input(&ic, path, NULL, NULL) < 0 || avformat_find_stream_info(ic, NULL) < 0) {
		*err = FLAREERR_INPUT_OPEN_FAILURE;
		goto fail;
	}

	v = av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);

	if (v < 0) {
		*err = FLAREERR_VIDEO_STREAM_MISSING;
		goto fail;
	}

	c = avcodec_find_decoder(ic->streams[v]->codecpar->codec_id);

	if (c == NULL) {
		*err = FLAREERR_DECODER_OPEN_FAILURE;
		goto fail;
	}

	dc = avcodec_alloc_context3(c);

	if (dc == NULL) {
		*err = FLAREERR_MEM_ALLOC_FAILURE;
		goto fail;
	}

	avcodec_parameters_to_context(dc, ic->streams[v]->codecpar);

	if (avcodec_open2(dc, c, NULL) < 0) {
		*err = FLAREERR_DECODER_OPEN_FAILURE;
		goto fail;
	}

	pk = av_packet_alloc();
	fr = av_frame_alloc();

	if (pk == NULL || fr == NULL) {
		*err = FLAREERR_MEM_ALLOC_FAILURE;
		goto fail;
	}

	while (!got && av_read_frame(ic, pk) >= 0) {
		if (pk->stream_index == v) {
			avcodec_send_packet(dc, pk);

			while (avcodec_receive_frame(dc, fr) >= 0) {
				got = 1;
				break;
			}
		}

		av_packet_unref(pk);
	}

	if (!got) {
		avcodec_send_packet(dc, NULL);

		while (avcodec_receive_frame(dc, fr) >= 0) {
			got = 1;
			break;
		}
	}

	if (!got) {
		*err = FLAREERR_DECODE_FAILED;
		goto fail;
	}

	if (av_image_alloc(d, l, fr->width, fr->height, AV_PIX_FMT_RGB24, 1) < 0) {
		*err = FLAREERR_MEM_ALLOC_FAILURE;
		goto fail;
	}

	sw = sws_getContext(fr->width, fr->height, fr->format, fr->width, fr->height, AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);

	if (sw == NULL) {
		av_freep(&d[0]);

		*err = FLAREERR_MEM_ALLOC_FAILURE;
		goto fail;
	}

	sws_scale(sw, (const uint8_t* const*) fr->data, fr->linesize, 0, fr->height, d, l);

	*W = fr->width;
	*H = fr->height;
	*S = l[0];

	sws_freeContext(sw);
	av_frame_free(&fr);
	av_packet_free(&pk);
	avcodec_free_context(&dc);
	avformat_close_input(&ic);

	return d[0];

	fail:;
		sws_freeContext(sw);
		av_frame_free(&fr);
		av_packet_free(&pk);
		avcodec_free_context(&dc);
		avformat_close_input(&ic);

		return NULL;

}

static int save(const char* const path, uint8_t* const src, const int S, const int W, const int H) {

	AVFormatContext* oc = NULL;
	const AVCodec* e = NULL;
	AVCodecContext* ec = NULL;
	AVStream* os = NULL;
	AVFrame* f = NULL;
	AVPacket* pk = NULL;
	const uint8_t* sd[4] = {0};
	int sl[4] = {0};
	int dl[4] = {0};
	int err = FLAREERR_SUCCESS;

	if (avformat_alloc_output_context2(&oc, NULL, NULL, path) < 0) {
		err = FLAREERR_OUTPUT_CONTEXT_FAILURE;
		goto end;
	}

	if (oc->priv_data != NULL) {
		av_opt_set(oc->priv_data, "update", "1", AV_OPT_SEARCH_CHILDREN);
	}

	e = avcodec_find_encoder(AV_CODEC_ID_PNG);

	if (e == NULL) {
		err = FLAREERR_ENCODER_OPEN_FAILURE;
		goto end;
	}

	ec = avcodec_alloc_context3(e);

	if (ec == NULL) {
		err = FLAREERR_MEM_ALLOC_FAILURE;
		goto end;
	}

	ec->width = W;
	ec->height = H;
	ec->pix_fmt = AV_PIX_FMT_RGB24;
	ec->time_base = TB;

	if (avcodec_open2(ec, e, NULL) < 0) {
		err = FLAREERR_ENCODER_OPEN_FAILURE;
		goto end;
	}

	os = avformat_new_stream(oc, NULL);

	if (os == NULL) {
		err = FLAREERR_MEM_ALLOC_FAILURE;
		goto end;
	}

	avcodec_parameters_from_context(os->codecpar, ec);
	os->time_base = TB;

	if (avio_open(&oc->pb, path, AVIO_FLAG_WRITE) < 0) {
		err = FLAREERR_OUTPUT_FILE_OPEN_FAILURE;
		goto end;
	}

	if (avformat_write_header(oc, NULL) < 0) {
		err = FLAREERR_WRITE_HEADER_FAILURE;
		goto end;
	}

	f = av_frame_alloc();

	if (f == NULL) {
		err = FLAREERR_MEM_ALLOC_FAILURE;
		goto end;
	}

	f->format = AV_PIX_FMT_RGB24;
	f->width = W;
	f->height = H;

	if (av_frame_get_buffer(f, 0) < 0) {
		err = FLAREERR_MEM_ALLOC_FAILURE;
		goto end;
	}

	sd[0] = src;
	sl[0] = S;
	dl[0] = f->linesize[0];

	av_image_copy(f->data, dl, sd, sl, AV_PIX_FMT_RGB24, W, H);

	f->pts = 0;
	avcodec_send_frame(ec, f);
	avcodec_send_frame(ec, NULL);

	pk = av_packet_alloc();

	if (pk == NULL) {
		err = FLAREERR_MEM_ALLOC_FAILURE;
		goto end;
	}

	while (avcodec_receive_packet(ec, pk) >= 0) {
		av_packet_rescale_ts(pk, ec->time_base, os->time_base);
		av_interleaved_write_frame(oc, pk);
		av_packet_unref(pk);
	}

	av_write_trailer(oc);

	end:;

	av_packet_free(&pk);
	av_frame_free(&f);
	avcodec_free_context(&ec);

	if (oc != NULL) {
		avio_closep(&oc->pb);
		avformat_free_context(oc);
	}

	return err;

}

static int wh(const uint8_t* const p) {
	return p[0] >= WHITE && p[1] >= WHITE && p[2] >= WHITE;
}

static int bl(const uint8_t* const p) {
	return p[0] <= BLACK && p[1] <= BLACK && p[2] <= BLACK;
}

static void copy_row(uint8_t* const dst, const uint8_t* const src, const int pad_left, const int copy, const int pad_right, const uint8_t left_fill, const uint8_t right_fill) {

	int offset = 0;

	if (pad_left > 0) {
		memset(dst, left_fill, pad_left * 3);
	}

	offset = pad_left * 3;

	memcpy(dst + offset, src, copy * 3);

	if (pad_right > 0) {
		memset(dst + offset + copy * 3, right_fill, pad_right * 3);
	}

}

#if defined(__ANDROID__)
	static void flare_log(const int priority, const char* const format, ...) {
		va_list arguments;

		va_start(arguments, format);

		__android_log_vprint(priority, PROJECT_NAME, format, arguments);

		va_end(arguments);
	}

	static void flare_av_log_callback(void* context, const int level, const char* const format, va_list arguments) {
		(void) context;
		(void) level;

		__android_log_vprint(ANDROID_LOG_ERROR, PROJECT_NAME, format, arguments);
	}
#else
	#define ANDROID_LOG_INFO 0
	#define ANDROID_LOG_ERROR 1

	static void flare_log(const int priority, const char* const format, ...) {
		va_list arguments;

		va_start(arguments, format);

		if (priority == ANDROID_LOG_ERROR) {
			vfprintf(stderr, format, arguments);
		}
		else {
			vprintf(format, arguments);
		}

		va_end(arguments);
	}
#endif

int flare_main(int argc, argv_t** argv) {

	int err = FLAREERR_SUCCESS;
	int action = 0;

	int sides = 0;
	int minimum = 0;
	int W = 0;
	int H = 0;
	int S = 0;
	int KR = 0;
	int lb = 0;
	int rb = 0;
	int OW = 0;
	int OH = 0;
	int OL = 0;
	int y = 0;
	int c = 0;
	int bc = 0;

	int runs = 0;
	int start = 0;
	int gsize = 0;
	int kept = 0;
	int pad = 0;

	int kept_left = 0;
	int kept_right = 0;
	int pad_left = 0;
	int pad_right = 0;

	uint8_t left_fill = 255;
	uint8_t right_fill = 255;
	uint8_t run_fill = 255;

	int cs = 0;
	int cols = 0;
	int span = 0;

	const char* in = NULL;
	const char* out = NULL;

	char outb[1024] = {0};

	char* endptr = NULL;

	uint8_t* rgb = NULL;
	uint8_t* ob = NULL;
	uint8_t* dst = NULL;
	uint8_t* gutter = NULL;
	long* cw = NULL;
	long* cb = NULL;

	long value = 0;

	const uint8_t* p = NULL;
	const uint8_t* e = NULL;
	const uint8_t* q = NULL;
	long* cx = NULL;
	long* bx = NULL;

	argparse_t argparse = {0};
	const arg_t* arg = NULL;

	#if defined(FLARE_LIBRARY)
		#if defined(__ANDROID__)
			av_log_set_callback(flare_av_log_callback);
		#endif

		av_log_set_level(AV_LOG_ERROR);
	#endif

	err = argparse_init(&argparse, argc, argv);

	if (err != ARGPARSE_ERR_SUCCESS) {
		switch (err) {
			case ARGPARSE_ERR_VALUE_UNEXPECTED:
				err = FLAREERR_ARGPARSE_VALUE_UNEXPECTED;
				break;
			case ARGPARSE_ERR_ARGUMENT_EMPTY:
				err = FLAREERR_ARGPARSE_ARGUMENT_EMPTY;
				break;
			case ARGPARSE_ERR_MEM_ALLOC_FAILURE:
				err = FLAREERR_MEM_ALLOC_FAILURE;
				break;
			default:
				break;
		}

		goto end;
	}

	while ((arg = argparse_getnext(&argparse)) != NULL) {
		action = get_action(arg);

		switch (action) {
			case ACTION_INPUT:
			case ACTION_OUTPUT:
			case ACTION_MINIMUM: {
				if (arg->value == NULL) {
					err = FLAREERR_ARGPARSE_ARGUMENT_VALUE_MISSING;
					goto end;
				}

				break;
			}
			default: {
				break;
			}
		}

		switch (action) {
			case ACTION_INPUT: {
				in = arg->value;
				break;
			}
			case ACTION_OUTPUT: {
				out = arg->value;
				break;
			}
			case ACTION_MINIMUM: {
				errno = 0;
				value = strtol(arg->value, &endptr, 10);

				if (errno == ERANGE || endptr == arg->value || *endptr != '\0' || value < 0 || value > INT_MAX) {
					err = FLAREERR_ARGPARSE_INVALID_UINT;
					goto end;
				}

				minimum = (int) value;

				break;
			}
			case ACTION_SIDES: {
				sides = 1;
				break;
			}
			case ACTION_HELP: {
				printf("%s", PROGRAM_HELP);
				goto end;
			}
			case ACTION_VERSION: {
				printf("%s v%s\n", PROJECT_NAME, PROJECT_VERSION);
				goto end;
			}
			case ACTION_UNKNOWN: {
				err = FLAREERR_ARGPARSE_ARGUMENT_INVALID;
				goto end;
			}
		}
	}

	if (in == NULL) {
		err = FLAREERR_INPUT_MISSING;
		goto end;
	}

	if (out == NULL) {
		out = outb;
		snprintf(outb, sizeof outb, "%.*s_nogutter.png", (int) (strrchr(in, '.') ? strrchr(in, '.') - in : (int) strlen(in)), in);
	}

	rgb = decode(in, &W, &H, &S, &err);

	if (rgb == NULL) {
		goto end;
	}

	gutter = av_mallocz(H);

	if (gutter == NULL) {
		err = FLAREERR_MEM_ALLOC_FAILURE;
		goto end;
	}

	if (sides) {
		cw = av_calloc(W, sizeof(long));
		cb = av_calloc(W, sizeof(long));

		if (cw == NULL || cb == NULL) {
			err = FLAREERR_MEM_ALLOC_FAILURE;
			goto end;
		}
	}

	for (y = 0; y < H; y++) {
		p = rgb + y * S;
		e = p + 3 * W;
		c = 0;
		bc = 0;

		for (q = p; q < e; q += 3) {
			if (wh(q)) {
				c++;
			}
			else if (bl(q)) {
				bc++;
			}
		}

		gutter[y] = 0;

		if (av_cmp_q(mkq(c, W), BAND) >= 0) {
			gutter[y] = 1;
		}
		else if (av_cmp_q(mkq(bc, W), BAND) >= 0) {
			gutter[y] = 2;
		}

		if (sides) {
			for (cx = cw, bx = cb, q = p; q < e; q += 3, cx++, bx++) {
				if (wh(q)) {
					*cx += 1;
				}
				else if (bl(q)) {
					*bx += 1;
				}
			}
		}

		if (gutter[y] != 0) {
			continue;
		}

		KR++;
	}

	if (!KR) {
		err = FLAREERR_IMAGE_NO_CONTENT;
		goto end;
	}

	runs = 0;
	y = 0;

	while (y < H) {
		if (!gutter[y]) {
			y++;
			continue;
		}

		runs++;

		while (y < H && gutter[y]) {
			y++;
		}
	}

	rb = W - 1;

	if (sides) {
		while (lb < W && (av_cmp_q(mkq((int) cw[lb], H), MARGIN) >= 0 || av_cmp_q(mkq((int) cb[lb], H), MARGIN) >= 0)) {
			lb++;
		}

		while (rb > lb && (av_cmp_q(mkq((int) cw[rb], H), MARGIN) >= 0 || av_cmp_q(mkq((int) cb[rb], H), MARGIN) >= 0)) {
			rb--;
		}
	}

	if (KR == H && lb == 0 && rb == W - 1) {
		flare_log(ANDROID_LOG_INFO, "no gutters found\n");

		goto end;
	}

	if (sides) {
		if (lb > 0) {
			kept_left = ((lb < minimum) ? lb : minimum);
			pad_left = minimum - kept_left;

			if (av_cmp_q(mkq((int) cb[0], H), MARGIN) >= 0) {
				left_fill = 0;
			}
		}

		if (W - 1 - rb > 0) {
			kept_right = ((W - 1 - rb < minimum) ? (W - 1 - rb) : minimum);
			pad_right = minimum - kept_right;

			if (av_cmp_q(mkq((int) cb[W - 1], H), MARGIN) >= 0) {
				right_fill = 0;
			}
		}

		cs = lb - kept_left;
		cols = rb - lb + 1;
	}
	else {
		cs = 0;
		cols = W;
	}

	OW = cols + kept_left + kept_right + pad_left + pad_right;
	OH = KR + runs * minimum;
	span = OW - pad_left - pad_right;
	OL = av_image_get_linesize(AV_PIX_FMT_RGB24, OW, 0);
	ob = av_malloc(av_image_get_buffer_size(AV_PIX_FMT_RGB24, OW, OH, 1));

	if (ob == NULL) {
		err = FLAREERR_MEM_ALLOC_FAILURE;
		goto end;
	}

	dst = ob;

	y = 0;

	while (y < H) {
		if (!gutter[y]) {
			copy_row(dst, rgb + y * S + cs * 3, pad_left, span, pad_right, left_fill, right_fill);
			dst += OL;
			y++;
			continue;
		}

		start = y;

		while (y < H && gutter[y]) {
			y++;
		}

		gsize = y - start;
		kept = ((gsize < minimum) ? gsize : minimum);
		pad = minimum - kept;

		run_fill = ((gutter[start] == 2) ? 0 : 255);

		for (c = start; c < start + kept; c++) {
			copy_row(dst, rgb + c * S + cs * 3, pad_left, span, pad_right, left_fill, right_fill);
			dst += OL;
		}

		if (pad > 0) {
			memset(dst, run_fill, pad * OL);
			dst += pad * OL;
		}
	}

	err = save(out, ob, OL, OW, OH);

	if (err == FLAREERR_SUCCESS) {
		flare_log(ANDROID_LOG_INFO, "%dx%d -> %dx%d  %s\n", W, H, OW, OH, out);
	}

	end:;

	av_freep(&rgb);
	av_freep(&gutter);
	av_freep(&cw);
	av_freep(&cb);
	av_freep(&ob);

	if (err != FLAREERR_SUCCESS) {
		flare_log(ANDROID_LOG_ERROR, "fatal error: (%i) %s", -err, flare_getmessage(err));

		switch (err) {
			case FLAREERR_ARGPARSE_ARGUMENT_VALUE_MISSING:
			case FLAREERR_ARGPARSE_ARGUMENT_INVALID: {
				fprintf(stderr, ": %.*s%s", 1 + (strlen(arg->key) > 1), "--", arg->key);
				break;
			}
			case FLAREERR_ARGPARSE_INVALID_UINT: {
				fprintf(stderr, ": %s", arg->value);
				break;
			}
		}

		fprintf(stderr, "\n");
	}

	argparse_free(&argparse);

	#if defined(FLARE_LIBRARY)
		return ((err == FLAREERR_SUCCESS) ? EXIT_SUCCESS : err);
	#else
		return ((err == FLAREERR_SUCCESS) ? EXIT_SUCCESS : EXIT_FAILURE);
	#endif

}

#if !defined(FLARE_LIBRARY)
	int main(const int argc, char** const argv) {
		return flare_main(argc, (argv_t**) argv);
	}
#endif
