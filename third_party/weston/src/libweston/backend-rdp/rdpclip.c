/*
 * Copyright © 2020 Microsoft
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "config.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>

#include "rdp.h"

#include "libweston-internal.h"

/* From MSDN, RegisterClipboardFormat API.
   Registered clipboard formats are identified by values in the range 0xC000 through 0xFFFF. */
#define CF_PRIVATE_RTF  49309 /* fake format ID for "Rich Text Format". */
#define CF_PRIVATE_HTML 49405 /* fake format ID for "HTML Format".*/

					       /*          1           2           3           4         5         6           7         8      */
					       /*01234567890 1 2345678901234 5 67890123456 7 89012345678901234567890 1 234567890123456789012 3 4*/
static const char rdp_clipboard_html_header[] = "Version:0.9\r\nStartHTML:-1\r\nEndHTML:-1\r\nStartFragment:00000000\r\nEndFragment:00000000\r\n";
#define RDP_CLIPBOARD_FRAGMENT_START_OFFSET (53) //---------------------------------------------------------+                       |
#define RDP_CLIPBOARD_FRAGMENT_END_OFFSET (75) //-----------------------------------------------------------------------------------+

/*
 * https://docs.microsoft.com/en-us/windows/win32/dataxchg/html-clipboard-format
 *
 * The fragment should be preceded and followed by the HTML comments and
 * (no space allowed between the !-- and the text) to conveniently
 * indicate where the fragment starts and ends.
 */
static const char rdp_clipboard_html_fragment_start[] = "<!--StartFragment-->\r\n";
static const char rdp_clipboard_html_fragment_end[] = "<!--EndFragment-->\r\n";

struct rdp_clipboard_data_source;

typedef bool (*pfn_process_data)(struct rdp_clipboard_data_source *source, bool is_send);

struct rdp_clipboard_supported_format {
	uint32_t format_id;
	char *format_name;
	char *mime_type;
	pfn_process_data pfn;
};

static bool
clipboard_process_text_utf8(struct rdp_clipboard_data_source *source, bool is_send);

static bool
clipboard_process_text_raw(struct rdp_clipboard_data_source *source, bool is_send);

static bool
clipboard_process_bmp(struct rdp_clipboard_data_source *source , bool is_send);

static bool
clipboard_process_html(struct rdp_clipboard_data_source *source, bool is_send);

/* TODO: need to support to 1:n or m:n format conversion.
 * For example, CF_UNICODETEXT to "UTF8_STRING" as well as "text/plain;charset=utf-8".
 */
struct rdp_clipboard_supported_format clipboard_supported_formats[] = {
	{ CF_UNICODETEXT,  NULL,               "text/plain;charset=utf-8", clipboard_process_text_utf8 },
	{ CF_TEXT,         NULL,               "STRING",                   clipboard_process_text_raw  },
	{ CF_DIB,          NULL,               "image/bmp",                clipboard_process_bmp       },
	{ CF_PRIVATE_RTF,  "Rich Text Format", "text/rtf",                 clipboard_process_text_raw  },
	{ CF_PRIVATE_HTML, "HTML Format",      "text/html",                clipboard_process_html      },
};
#define RDP_NUM_CLIPBOARD_FORMATS ARRAY_LENGTH(clipboard_supported_formats)

enum rdp_clipboard_data_source_state {
	RDP_CLIPBOARD_SOURCE_ALLOCATED = 0,
	RDP_CLIPBOARD_SOURCE_FORMATLIST_READY, /* format list obtained from provider */
	RDP_CLIPBOARD_SOURCE_PUBLISHED, /* availablity of some or no clipboard data notified to consumer */
	RDP_CLIPBOARD_SOURCE_REQUEST_DATA, /* data request sent to provider */
	RDP_CLIPBOARD_SOURCE_RECEIVED_DATA, /* data was received from provider, waiting data to be dispatched to consumer */
	RDP_CLIPBOARD_SOURCE_TRANSFERING, /* transfering data to consumer */
	RDP_CLIPBOARD_SOURCE_TRANSFERRED, /* completed transfering data to consumer */
	RDP_CLIPBOARD_SOURCE_CANCEL_PENDING, /* data transfer cancel requested */
	RDP_CLIPBOARD_SOURCE_CANCELED, /* data transfer canceled */
	RDP_CLIPBOARD_SOURCE_RETRY, /* retry later */
	RDP_CLIPBOARD_SOURCE_FAILED, /* failure occured */
};

struct rdp_clipboard_data_source {
	struct weston_data_source base;
	struct rdp_loop_task task_base;
	struct wl_event_source *transfer_event_source; /* used for read/write with pipe */
	struct wl_array data_contents;
	void *context;
	int refcount;
	int data_source_fd;
	int format_index;
	enum rdp_clipboard_data_source_state state;
	uint32_t data_response_fail_count;
	uint32_t inflight_write_count;
	void *inflight_data_to_write;
	size_t inflight_data_size;
	bool is_data_processed;
	void *processed_data_start;
	uint32_t processed_data_size;
	bool processed_data_is_send;
	bool is_canceled;
	uint32_t client_format_id_table[RDP_NUM_CLIPBOARD_FORMATS];
};

struct rdp_clipboard_data_request {
	struct rdp_loop_task task_base;
	RdpPeerContext *ctx;
	uint32_t requested_format_index;
};

static char *
clipboard_data_source_state_to_string(struct rdp_clipboard_data_source *source)
{
	if (!source)
		return "null";

	switch (source->state) {
	case RDP_CLIPBOARD_SOURCE_ALLOCATED:
		return "allocated";
	case RDP_CLIPBOARD_SOURCE_FORMATLIST_READY:
		return "format list ready";
	case RDP_CLIPBOARD_SOURCE_PUBLISHED:
		return "published";
	case RDP_CLIPBOARD_SOURCE_REQUEST_DATA:
		return "request data";
	case RDP_CLIPBOARD_SOURCE_RECEIVED_DATA:
		return "received data";
	case RDP_CLIPBOARD_SOURCE_TRANSFERING:
		return "transferring";
	case RDP_CLIPBOARD_SOURCE_TRANSFERRED:
		return "transferred";
	case RDP_CLIPBOARD_SOURCE_CANCEL_PENDING:
		return "cancel pending";
	case RDP_CLIPBOARD_SOURCE_CANCELED:
		return "canceled";
	case RDP_CLIPBOARD_SOURCE_RETRY:
		return "retry";
	case RDP_CLIPBOARD_SOURCE_FAILED:
		return "failed";
	}
	assert(false);
	return "unknown";
}

static bool
clipboard_process_text_utf8(struct rdp_clipboard_data_source *source, bool is_send)
{
	freerdp_peer *client = (freerdp_peer *)source->context;
	RdpPeerContext *ctx = (RdpPeerContext *)client->context;
	struct rdp_backend *b = ctx->rdpBackend;
	struct wl_array data_contents;

	wl_array_init(&data_contents);

	assert(!source->is_data_processed);

	if (is_send) {
		char *data = source->data_contents.data;
		size_t data_size, data_size_in_char;

		/* Linux to Windows (convert utf-8 to UNICODE) */
		/* Include terminating NULL in size */
		assert((source->data_contents.size + 1) <= source->data_contents.alloc);
		data[source->data_contents.size] = '\0';
		source->data_contents.size++;

		/* obtain size in UNICODE */
		data_size = MultiByteToWideChar(CP_UTF8, 0,
						data,
						source->data_contents.size,
						NULL, 0);
		if (data_size < 1)
			goto error_return;

		data_size *= 2; /* convert to size in bytes. */
		if (!wl_array_add(&data_contents, data_size))
			goto error_return;

		/* convert to UNICODE */
		data_size_in_char = MultiByteToWideChar(CP_UTF8, 0,
							data,
							source->data_contents.size,
							data_contents.data,
							data_size);
		assert(data_contents.size == (data_size_in_char * 2));
	} else {
		/* Windows to Linux (UNICODE to utf-8) */
		size_t data_size;
		LPWSTR data = source->data_contents.data;
		size_t data_size_in_char = source->data_contents.size / 2;

		/* Windows's data has trailing chars, which Linux doesn't expect. */
		while (data_size_in_char &&
		       ((data[data_size_in_char-1] == L'\0') || (data[data_size_in_char-1] == L'\n')))
			data_size_in_char -= 1;
		if (!data_size_in_char)
			goto error_return;

		/* obtain size in utf-8 */
		data_size = WideCharToMultiByte(CP_UTF8, 0,
						source->data_contents.data,
						data_size_in_char,
						NULL, 0,
						NULL, NULL);
		if (data_size < 1)
			goto error_return;

		if (!wl_array_add(&data_contents, data_size))
			goto error_return;

		/* convert to utf-8 */
		data_size = WideCharToMultiByte(CP_UTF8, 0,
						source->data_contents.data,
						data_size_in_char,
						data_contents.data,
						data_size,
						NULL, NULL);
		assert(data_contents.size == data_size);
	}

	/* swap the data_contents with new one */
	wl_array_release(&source->data_contents);
	source->data_contents = data_contents;
	source->is_data_processed = true;
	source->processed_data_start = source->data_contents.data;
	source->processed_data_size = source->data_contents.size;
	rdp_debug_clipboard_verbose(b, "RDP %s (%p:%s): %s (%u bytes)\n",
				    __func__, source,
				    clipboard_data_source_state_to_string(source),
				    is_send ? "send" : "receive",
				    (uint32_t)source->data_contents.size);

	return true;

error_return:
	source->state = RDP_CLIPBOARD_SOURCE_FAILED;
	weston_log("RDP %s FAILED (%p:%s): %s (%u bytes)\n",
		   __func__, source,
		   clipboard_data_source_state_to_string(source),
		   is_send ? "send" : "receive",
		   (uint32_t)source->data_contents.size);

	wl_array_release(&data_contents);

	return false;
}

static bool
clipboard_process_text_raw(struct rdp_clipboard_data_source *source, bool is_send)
{
	freerdp_peer *client = (freerdp_peer *)source->context;
	RdpPeerContext *ctx = (RdpPeerContext *)client->context;
	struct rdp_backend *b = ctx->rdpBackend;
	char *data = source->data_contents.data;
	size_t data_size = source->data_contents.size;

	assert(!source->is_data_processed);

	if (is_send) {
		/* Linux to Windows */
		/* Include terminating NULL in size */
		assert(data_size + 1 <= source->data_contents.alloc);
		data[data_size] = '\0';
		source->data_contents.size++;
	} else {
		/* Windows to Linux */
		/* Windows's data has trailing chars, which Linux doesn't expect. */
		while (data_size && ((data[data_size-1] == '\0') || (data[data_size-1] == '\n')))
			data_size -= 1;
		source->data_contents.size = data_size;
	}
	source->is_data_processed = true;
	source->processed_data_start = source->data_contents.data;
	source->processed_data_size = source->data_contents.size;
	rdp_debug_clipboard_verbose(b, "RDP %s (%p): %s (%u bytes)\n",
				    __func__, source,
				    is_send ? "send" : "receive",
				    (uint32_t)source->data_contents.size);

	return true;
}

/* based off sample code at https://docs.microsoft.com/en-us/troubleshoot/cpp/add-html-code-clipboard
   But this missing a lot of corner cases, it must be rewritten with use of proper HTML parser */
/* TODO: This doesn't work for converting HTML from Firefox in Wayland mode to Windows in certain cases,
   because Firefox sends "<meta http-equiv="content-type" content="text/html; charset=utf-8">...", thus
   this needs to property strip meta header and convert to the Windows clipboard style HTML. */
static bool
clipboard_process_html(struct rdp_clipboard_data_source *source, bool is_send)
{
	freerdp_peer *client = (freerdp_peer *)source->context;
	RdpPeerContext *ctx = (RdpPeerContext *)client->context;
	struct rdp_backend *b = ctx->rdpBackend;
	struct wl_array data_contents;
	char *cur = source->data_contents.data;

	assert(!source->is_data_processed);

	/* We're treating the contents as a string for now, so null
	 * terminate it so strstr can't run off the end. However, we
	 * don't increase data_contents.size because we don't want
	 * to affect the content. */
	assert(source->data_contents.size + 1 <= source->data_contents.alloc);
	((char *)(source->data_contents.data))[source->data_contents.size] = '\0';

	wl_array_init(&data_contents);
	cur = strstr(cur, "<html");
	if (!cur)
		goto error_return;

	if (!is_send) {
		/* Windows to Linux */
		size_t data_size = source->data_contents.size -
				   (cur - (char *)source->data_contents.data);

		/* Windows's data has trailing chars, which Linux doesn't expect. */
		while (data_size && ((cur[data_size-1] == '\0') || (cur[data_size-1] == '\n')))
			data_size -= 1;

		if (!data_size)
			goto error_return;

		if (!wl_array_add(&data_contents, data_size+1)) /* +1 for null */
			goto error_return;

		memcpy(data_contents.data, cur, data_size);
		((char *)(data_contents.data))[data_size] = '\0';
		data_contents.size = data_size;
	} else {
		/* Linux to Windows */
		char *last, *buf;
		uint32_t fragment_start, fragment_end;

		if (!wl_array_add(&data_contents, source->data_contents.size+200))
			goto error_return;

		buf = data_contents.data;
		strcpy(buf, rdp_clipboard_html_header);
		last = cur;
		cur = strstr(cur, "<body");
		if (!cur)
			goto error_return;
		cur += 5;
		while (*cur != '>' && *cur != '\0')
			cur++;
		if (*cur == '\0')
			goto error_return;
		cur++; /* include '>' */
		strncat(buf, last, cur-last);
		last = cur;
		fragment_start = strlen(buf);
		strcat(buf, rdp_clipboard_html_fragment_start);
		cur = strstr(cur, "</body");
		if (!cur)
			goto error_return;
		strncat(buf, last, cur-last);
		fragment_end = strlen(buf);
		strcat(buf, rdp_clipboard_html_fragment_end);
		strcat(buf, cur);

		cur = buf + RDP_CLIPBOARD_FRAGMENT_START_OFFSET;
		sprintf(cur, "%08u", fragment_start);
		*(cur+8) = '\r';
		cur = buf + RDP_CLIPBOARD_FRAGMENT_END_OFFSET;
		sprintf(cur, "%08u", fragment_end);
		*(cur+8) = '\r';

		data_contents.size = strlen(buf) + 1; /* +1 to null terminate. */
	}

	/* swap the data_contents with new one */
	wl_array_release(&source->data_contents);
	source->data_contents = data_contents;
	source->is_data_processed = true;
	source->processed_data_start = source->data_contents.data;
	source->processed_data_size = source->data_contents.size;
	rdp_debug_clipboard_verbose(b, "RDP %s (%p:%s): %s (%u bytes)\n",
				    __func__, source, clipboard_data_source_state_to_string(source),
				    is_send ? "send" : "receive", (uint32_t)source->data_contents.size);

	return true;

error_return:
	source->state = RDP_CLIPBOARD_SOURCE_FAILED;
	weston_log("RDP %s FAILED (%p:%s): %s (%u bytes)\n",
		   __func__, source, clipboard_data_source_state_to_string(source),
		   is_send ? "send" : "receive", (uint32_t)source->data_contents.size);

	wl_array_release(&data_contents);

	return false;
}

#define DIB_HEADER_MARKER     ((WORD) ('M' << 8) | 'B')
#define DIB_WIDTH_BYTES(bits) ((((bits) + 31) & ~31) >> 3)

static bool
clipboard_process_bmp(struct rdp_clipboard_data_source *source, bool is_send)
{
	freerdp_peer *client = (freerdp_peer *)source->context;
	RdpPeerContext *ctx = (RdpPeerContext *)client->context;
	struct rdp_backend *b = ctx->rdpBackend;
	BITMAPFILEHEADER *bmfh = NULL;
	BITMAPINFOHEADER *bmih = NULL;
	uint32_t color_table_size = 0;
	struct wl_array data_contents;

	assert(!source->is_data_processed);

	wl_array_init(&data_contents);

	if (is_send) {
		/* Linux to Windows (remove BITMAPFILEHEADER) */
		if (source->data_contents.size <= sizeof(*bmfh))
			goto error_return;

		bmfh = source->data_contents.data;
		bmih = (BITMAPINFOHEADER *)(bmfh + 1);

		source->is_data_processed = true;
		source->processed_data_start = bmih;
		source->processed_data_size = source->data_contents.size - sizeof(*bmfh);
	} else {
		/* Windows to Linux (insert BITMAPFILEHEADER) */
		BITMAPFILEHEADER _bmfh = {};

		if (source->data_contents.size <= sizeof(*bmih))
			goto error_return;

		bmih = source->data_contents.data;
		bmfh = &_bmfh;
		if (bmih->biCompression == BI_BITFIELDS)
			color_table_size = sizeof(RGBQUAD) * 3;
		else
			color_table_size = sizeof(RGBQUAD) * bmih->biClrUsed;

		bmfh->bfType = DIB_HEADER_MARKER;
		bmfh->bfOffBits = sizeof(*bmfh) + bmih->biSize + color_table_size;
		if (bmih->biSizeImage)
			bmfh->bfSize = bmfh->bfOffBits + bmih->biSizeImage;
		else if (bmih->biCompression == BI_BITFIELDS || bmih->biCompression == BI_RGB)
			bmfh->bfSize = bmfh->bfOffBits +
				       (DIB_WIDTH_BYTES(bmih->biWidth * bmih->biBitCount) * abs(bmih->biHeight));
		else
			goto error_return;

		/* source data must have enough size as described in its own bitmap header */
		if (source->data_contents.size < (bmfh->bfSize - sizeof(*bmfh)))
			goto error_return;

		if (!wl_array_add(&data_contents, bmfh->bfSize))
			goto error_return;
		assert(data_contents.size == bmfh->bfSize);

		/* copy generated BITMAPFILEHEADER */
		memcpy(data_contents.data, bmfh, sizeof(*bmfh));
		/* copy rest of bitmap data from source */
		memcpy((char *)data_contents.data + sizeof(*bmfh),
		       source->data_contents.data, bmfh->bfSize - sizeof(*bmfh));

		/* swap the data_contents with new one */
		wl_array_release(&source->data_contents);
		source->data_contents = data_contents;
		source->is_data_processed = true;
		source->processed_data_start = source->data_contents.data;
		source->processed_data_size = source->data_contents.size;
	}

	rdp_debug_clipboard_verbose(b, "RDP %s (%p:%s): %s (%d bytes)\n",
				    __func__, source,
				    clipboard_data_source_state_to_string(source),
				    is_send ? "send" : "receive",
				    (uint32_t)source->data_contents.size);

	return true;

error_return:
	source->state = RDP_CLIPBOARD_SOURCE_FAILED;
	weston_log("RDP %s FAILED (%p:%s): %s (%d bytes)\n",
		   __func__, source, clipboard_data_source_state_to_string(source),
		   is_send ? "send" : "receive", (uint32_t)source->data_contents.size);

	wl_array_release(&data_contents);

	return false;
}

static char *
clipboard_format_id_to_string(UINT32 formatId, bool is_server_format_id)
{
	switch (formatId) {
	case CF_RAW:
		return "CF_RAW";
	case CF_TEXT:
		return "CF_TEXT";
	case CF_BITMAP:
		return "CF_BITMAP";
	case CF_METAFILEPICT:
		return "CF_METAFILEPICT";
	case CF_SYLK:
		return "CF_SYLK";
	case CF_DIF:
		return "CF_DIF";
	case CF_TIFF:
		return "CF_TIFF";
	case CF_OEMTEXT:
		return "CF_OEMTEXT";
	case CF_DIB:
		return "CF_DIB";
	case CF_PALETTE:
		return "CF_PALETTE";
	case CF_PENDATA:
		return "CF_PENDATA";
	case CF_RIFF:
		return "CF_RIFF";
	case CF_WAVE:
		return "CF_WAVE";
	case CF_UNICODETEXT:
		return "CF_UNICODETEXT";
	case CF_ENHMETAFILE:
		return "CF_ENHMETAFILE";
	case CF_HDROP:
		return "CF_HDROP";
	case CF_LOCALE:
		return "CF_LOCALE";
	case CF_DIBV5:
		return "CF_DIBV5";

	case CF_OWNERDISPLAY:
		return "CF_OWNERDISPLAY";
	case CF_DSPTEXT:
		return "CF_DSPTEXT";
	case CF_DSPBITMAP:
		return "CF_DSPBITMAP";
	case CF_DSPMETAFILEPICT:
		return "CF_DSPMETAFILEPICT";
	case CF_DSPENHMETAFILE:
		return "CF_DSPENHMETAFILE";
	}

	if (formatId >= CF_PRIVATEFIRST && formatId <= CF_PRIVATELAST)
		return "CF_PRIVATE";

	if (formatId >= CF_GDIOBJFIRST && formatId <= CF_GDIOBJLAST)
		return "CF_GDIOBJ";

	if (is_server_format_id) {
		if (formatId == CF_PRIVATE_HTML)
			return "CF_PRIVATE_HTML";

		if (formatId == CF_PRIVATE_RTF)
			return "CF_PRIVATE_RTF";
	} else {
		/* From MSDN, RegisterClipboardFormat API.
		   Registered clipboard formats are identified by values in the range 0xC000 through 0xFFFF. */
		if (formatId >= 0xC000 && formatId <= 0xFFFF)
			return "Client side Registered Clipboard Format";
	}

	return "Unknown format";
}

/* find supported index in supported format table by format id from client */
static int
clipboard_find_supported_format_by_format_id(UINT32 format_id)
{
	unsigned int i;

	for (i = 0; i < RDP_NUM_CLIPBOARD_FORMATS; i++) {
		struct rdp_clipboard_supported_format *format = &clipboard_supported_formats[i];

		if (format_id == format->format_id)
			return i;
	}
	return -1;
}

/* find supported index in supported format table by format id and name from client */
static int
clipboard_find_supported_format_by_format_id_and_name(UINT32 format_id, const char *format_name)
{
	unsigned int i;

	for (i = 0; i < RDP_NUM_CLIPBOARD_FORMATS; i++) {
		struct rdp_clipboard_supported_format *format = &clipboard_supported_formats[i];

		/* when our supported format table has format name, only format name must match,
		   format id provided from client is ignored (but it may be saved by caller for future use.
		   When our supported format table doesn't have format name, only format id must match,
		   format name (if provided from client) is ignored */
		if ((format->format_name == NULL && format_id == format->format_id) ||
		    (format->format_name && format_name && strcmp(format_name, format->format_name) == 0))
			return i;
	}
	return -1;
}

/* find supported index in supported format table by mime */
static int
clipboard_find_supported_format_by_mime_type(const char *mime_type)
{
	unsigned int i;

	for (i = 0; i < RDP_NUM_CLIPBOARD_FORMATS; i++) {
		struct rdp_clipboard_supported_format *format = &clipboard_supported_formats[i];

		if (strcmp(mime_type, format->mime_type) == 0)
			return i;
	}
	return -1;
}

static bool
clipboard_process_source(struct rdp_clipboard_data_source *source, bool is_send)
{
	if (source->is_data_processed) {
		assert(source->processed_data_is_send == is_send);
		return true;
	}

	source->processed_data_start = NULL;
	source->processed_data_size = 0;

	if (clipboard_supported_formats[source->format_index].pfn)
		return clipboard_supported_formats[source->format_index].pfn(source, is_send);

	/* No processor, so just set up pointer and length for raw data */
	source->is_data_processed = true;
	source->processed_data_start = source->data_contents.data;
	source->processed_data_size = source->data_contents.size;
	source->processed_data_is_send = is_send;
	return true;
}

static void
clipboard_data_source_unref(struct rdp_clipboard_data_source *source)
{
	freerdp_peer *client = (freerdp_peer *)source->context;
	RdpPeerContext *ctx = (RdpPeerContext *)client->context;
	struct rdp_backend *b = ctx->rdpBackend;
	char **p;

	assert_compositor_thread(b);

	assert(source->refcount);
	source->refcount--;

	rdp_debug_clipboard(b, "RDP %s (%p:%s): refcount:%d\n",
			    __func__, source,
			    clipboard_data_source_state_to_string(source),
			    source->refcount);

	if (source->refcount > 0)
		return;

	if (source->transfer_event_source)
		wl_event_source_remove(source->transfer_event_source);

	if (source->data_source_fd != -1)
		close(source->data_source_fd);

	if (!wl_list_empty(&source->base.destroy_signal.listener_list))
		wl_signal_emit(&source->base.destroy_signal,
			       &source->base);

	wl_array_release(&source->data_contents);

	wl_array_for_each(p, &source->base.mime_types)
		free(*p);

	wl_array_release(&source->base.mime_types);

	free(source);
}

/******************************************\
 * FreeRDP format data response functions *
\******************************************/

/* Inform client data request is succeeded with data */
static void
clipboard_client_send_format_data_response(RdpPeerContext *ctx, struct rdp_clipboard_data_source *source)
{
	struct rdp_backend *b = ctx->rdpBackend;
	CLIPRDR_FORMAT_DATA_RESPONSE formatDataResponse = {};

	assert(source->is_data_processed);
	rdp_debug_clipboard(b, "Client: %s (%p:%s) format_index:%d %s (%d bytes)\n",
			    __func__, source,
			    clipboard_data_source_state_to_string(source),
			    source->format_index,
			    clipboard_supported_formats[source->format_index].mime_type,
			    source->processed_data_size);

	formatDataResponse.msgType = CB_FORMAT_DATA_RESPONSE;
	formatDataResponse.msgFlags = CB_RESPONSE_OK;
	formatDataResponse.dataLen = source->processed_data_size;
	formatDataResponse.requestedFormatData = source->processed_data_start;
	ctx->clipboard_server_context->ServerFormatDataResponse(ctx->clipboard_server_context, &formatDataResponse);
	/* if here failed to send response, what can we do ? */
}

/* Inform client data request has failed */
static void
clipboard_client_send_format_data_response_fail(RdpPeerContext *ctx, struct rdp_clipboard_data_source *source)
{
	struct rdp_backend *b = ctx->rdpBackend;
	CLIPRDR_FORMAT_DATA_RESPONSE formatDataResponse = {};

	rdp_debug_clipboard(b, "Client: %s (%p:%s)\n",
			    __func__, source,
			    clipboard_data_source_state_to_string(source));

	if (source) {
		source->state = RDP_CLIPBOARD_SOURCE_FAILED;
		source->data_response_fail_count++;
	}

	formatDataResponse.msgType = CB_FORMAT_DATA_RESPONSE;
	formatDataResponse.msgFlags = CB_RESPONSE_FAIL;
	formatDataResponse.dataLen = 0;
	formatDataResponse.requestedFormatData = NULL;
	ctx->clipboard_server_context->ServerFormatDataResponse(ctx->clipboard_server_context, &formatDataResponse);
	/* if here failed to send response, what can we do ? */
}

/***************************************\
 * Compositor file descritor callbacks *
\***************************************/

/* Send server clipboard data to client when server side application sent them via pipe. */
static int
clipboard_data_source_read(int fd, uint32_t mask, void *arg)
{
	struct rdp_clipboard_data_source *source = (struct rdp_clipboard_data_source *)arg;
	freerdp_peer *client = (freerdp_peer *)source->context;
	RdpPeerContext *ctx = (RdpPeerContext *)client->context;
	struct rdp_backend *b = ctx->rdpBackend;
	int len;
	bool failed = true;

	rdp_debug_clipboard_verbose(b, "RDP %s (%p:%s) fd:%d\n",
				    __func__, source,
				    clipboard_data_source_state_to_string(source), fd);

	assert_compositor_thread(b);

	assert(source->data_source_fd == fd);
	assert(source->refcount == 1);

	/* event source is not removed here, but it will be removed when read is completed,
	   until it's completed this function will be called whenever next chunk of data is
	   available for read in pipe. */
	assert(source->transfer_event_source);

	source->state = RDP_CLIPBOARD_SOURCE_TRANSFERING;

	len = rdp_wl_array_read_fd(&source->data_contents, fd);
	if (len < 0) {
		source->state = RDP_CLIPBOARD_SOURCE_FAILED;
		weston_log("RDP %s (%p:%s) read failed (%s)\n",
			   __func__, source,
			   clipboard_data_source_state_to_string(source),
			   strerror(errno));
		goto error_exit;
	}

	if (len > 0) {
		rdp_debug_clipboard_verbose(b, "RDP %s (%p:%s) read (%zu bytes)\n",
					    __func__, source,
					    clipboard_data_source_state_to_string(source),
					    source->data_contents.size);
		/* continue to read next batch */
		return 0;
	}

	/* len == 0, all data from source is read, so completed. */
	source->state = RDP_CLIPBOARD_SOURCE_TRANSFERRED;
	rdp_debug_clipboard(b, "RDP %s (%p:%s): read completed (%ld bytes)\n",
			    __func__, source,
			    clipboard_data_source_state_to_string(source),
			    source->data_contents.size);
	if (!source->data_contents.size)
		goto error_exit;
	/* process data before sending to client */
	if (!clipboard_process_source(source, true))
		goto error_exit;

	clipboard_client_send_format_data_response(ctx, source);
	failed = false;

error_exit:
	if (failed)
		clipboard_client_send_format_data_response_fail(ctx, source);

	/* make sure this is the last reference, so event source is removed at unref */
	assert(source->refcount == 1);
	clipboard_data_source_unref(source);
	return 0;
}

/* client's reply with error for data request, clean up */
static int
clipboard_data_source_fail(int fd, uint32_t mask, void *arg)
{
	struct rdp_clipboard_data_source *source = (struct rdp_clipboard_data_source *)arg;
	freerdp_peer *client = (freerdp_peer *)source->context;
	RdpPeerContext *ctx = (RdpPeerContext *)client->context;
	struct rdp_backend *b = ctx->rdpBackend;

	rdp_debug_clipboard_verbose(b, "RDP %s (%p:%s) fd:%d\n", __func__,
				    source, clipboard_data_source_state_to_string(source), fd);

	assert_compositor_thread(b);

	assert(source->data_source_fd == fd);
	/* this data source must be tracked as inflight */
	assert(source == ctx->clipboard_inflight_client_data_source);

	wl_event_source_remove(source->transfer_event_source);
	source->transfer_event_source = NULL;

	/* if data was received, but failed for another reason then keep data
	 * and format index for future request,	otherwise data is purged at
	 * last reference release. */
	if (!source->data_contents.size) {
		/* data has been never received, thus must be empty. */
		assert(source->data_contents.size == 0);
		assert(source->data_contents.alloc == 0);
		assert(source->data_contents.data == NULL);
		/* clear previous requested format so it can be requested later again. */
		source->format_index = -1;
	}

	/* data has never been sent to write(), thus must be no inflight write. */
	assert(source->inflight_write_count == 0);
	assert(source->inflight_data_to_write == NULL);
	assert(source->inflight_data_size == 0);
	/* data never has been sent to write(), so must not be processed. */
	assert(source->is_data_processed == FALSE);
	/* close fd to server clipboard stop pulling data. */
	close(source->data_source_fd);
	source->data_source_fd = -1;
	/* clear inflight data source from client to server. */
	ctx->clipboard_inflight_client_data_source = NULL;
	clipboard_data_source_unref(source);

	return 0;
}

/* Send client's clipboard data to the requesting application at server side */
static int
clipboard_data_source_write(int fd, uint32_t mask, void *arg)
{
	struct rdp_clipboard_data_source *source = (struct rdp_clipboard_data_source *)arg;
	freerdp_peer *client = (freerdp_peer *)source->context;
	RdpPeerContext *ctx = (RdpPeerContext *)client->context;
	struct rdp_backend *b = ctx->rdpBackend;
	void *data_to_write;
	size_t data_size;
	ssize_t size;

	rdp_debug_clipboard_verbose(b, "RDP %s (%p:%s) fd:%d\n", __func__,
				    source,
				    clipboard_data_source_state_to_string(source),
				    fd);

	assert_compositor_thread(b);

	assert(source->data_source_fd == fd);
	/* this data source must be tracked as inflight */
	assert(source == ctx->clipboard_inflight_client_data_source);

	if (source->is_canceled) {
		/* if source is being canceled, this must be the last reference */
		assert(source->refcount == 1);
		source->state = RDP_CLIPBOARD_SOURCE_CANCELED;
		rdp_debug_clipboard_verbose(b, "RDP %s (%p:%s) canceled\n",
					    __func__, source,
					    clipboard_data_source_state_to_string(source));
		goto fail;
	}

	if (!source->data_contents.data || !source->data_contents.size) {
		assert(source->refcount > 1);
		weston_log("RDP %s (%p:%s) no data received from client\n",
			   __func__, source,
			   clipboard_data_source_state_to_string(source));
		goto fail;
	}

	assert(source->refcount > 1);
	if (source->inflight_data_to_write) {
		assert(source->inflight_data_size);
		rdp_debug_clipboard_verbose(b, "RDP %s (%p:%s) transfer in chunck, count:%d\n",
					    __func__, source,
					    clipboard_data_source_state_to_string(source),
					    source->inflight_write_count);
		data_to_write = source->inflight_data_to_write;
		data_size = source->inflight_data_size;
	} else {
		fcntl(source->data_source_fd, F_SETFL, O_WRONLY | O_NONBLOCK);
		clipboard_process_source(source, false);
		data_to_write = source->processed_data_start;
		data_size = source->processed_data_size;
	}
	while (data_to_write && data_size) {
		source->state = RDP_CLIPBOARD_SOURCE_TRANSFERING;
		do {
			size = write(source->data_source_fd, data_to_write, data_size);
		} while (size == -1 && errno == EINTR);

		if (size <= 0) {
			if (errno != EAGAIN) {
				source->state = RDP_CLIPBOARD_SOURCE_FAILED;
				weston_log("RDP %s (%p:%s) write failed %s\n",
					   __func__, source,
					   clipboard_data_source_state_to_string(source),
					   strerror(errno));
				break;
			}
			/* buffer is full, wait until data_source_fd is writable again */
			source->inflight_data_to_write = data_to_write;
			source->inflight_data_size = data_size;
			source->inflight_write_count++;
			return 0;
		} else {
			assert(data_size >= (size_t)size);
			data_size -= size;
			data_to_write = (char *)data_to_write + size;
			rdp_debug_clipboard_verbose(b, "RDP %s (%p:%s) wrote %ld bytes, remaining %ld bytes\n",
						    __func__, source,
						    clipboard_data_source_state_to_string(source),
						    size, data_size);
			if (!data_size) {
				source->state = RDP_CLIPBOARD_SOURCE_TRANSFERRED;
				rdp_debug_clipboard(b, "RDP %s (%p:%s) write completed (%ld bytes)\n",
						    __func__, source,
						    clipboard_data_source_state_to_string(source),
						    source->data_contents.size);
			}
		}
	}

fail:
	/* Here write is either completed, canceled or failed, so close the pipe. */
	close(source->data_source_fd);
	source->data_source_fd = -1;
	/* and remove the event source */
	wl_event_source_remove(source->transfer_event_source);
	source->transfer_event_source = NULL;
	/* and reset the inflight transfer state. */
	source->inflight_write_count = 0;
	source->inflight_data_to_write = NULL;
	source->inflight_data_size = 0;
	ctx->clipboard_inflight_client_data_source = NULL;
	clipboard_data_source_unref(source);

	return 0;
}

/***********************************\
 * Clipboard data-device callbacks *
\***********************************/

/* data-device informs the given data format is accepted */
static void
clipboard_data_source_accept(struct weston_data_source *base,
		   uint32_t time, const char *mime_type)
{
	struct rdp_clipboard_data_source *source = (struct rdp_clipboard_data_source *)base;
	freerdp_peer *client = (freerdp_peer *)source->context;
	RdpPeerContext *ctx = (RdpPeerContext *)client->context;
	struct rdp_backend *b = ctx->rdpBackend;

	rdp_debug_clipboard(b, "RDP %s (%p:%s) mime-type:\"%s\"\n",
			    __func__, source,
			    clipboard_data_source_state_to_string(source),
			    mime_type);
}

/* data-device informs the application requested the specified format data
 * in given data_source (= client's clipboard) */
static void
clipboard_data_source_send(struct weston_data_source *base,
		 const char *mime_type, int32_t fd)
{
	struct rdp_clipboard_data_source *source = (struct rdp_clipboard_data_source *)base;
	freerdp_peer *client = (freerdp_peer *)source->context;
	RdpPeerContext *ctx = (RdpPeerContext *)client->context;
	struct rdp_backend *b = ctx->rdpBackend;
	struct weston_seat *seat = ctx->item.seat;
	struct wl_event_loop *loop = wl_display_get_event_loop(seat->compositor->wl_display);
	CLIPRDR_FORMAT_DATA_REQUEST formatDataRequest = {};
	int index;

	rdp_debug_clipboard(b, "RDP %s (%p:%s) fd:%d, mime-type:\"%s\"\n",
			    __func__, source,
			    clipboard_data_source_state_to_string(source),
			    fd, mime_type);

	assert_compositor_thread(b);

	if (ctx->clipboard_inflight_client_data_source) {
		/* Here means server side (Linux application) request clipboard data,
		   but server hasn't completed with previous request yet.
		   If this happens, punt to idle loop and reattempt. */
		weston_log("\n\n\nRDP %s new (%p:%s:fd %d) vs prev (%p:%s:fd %d): outstanding RDP data request (client to server)\n\n\n",
			   __func__, source, clipboard_data_source_state_to_string(source), fd,
			   ctx->clipboard_inflight_client_data_source,
			   clipboard_data_source_state_to_string(ctx->clipboard_inflight_client_data_source),
			   ctx->clipboard_inflight_client_data_source->data_source_fd);
		if (source == ctx->clipboard_inflight_client_data_source) {
			/* when new source and previous source is same, update fd with new one and retry */
			source->state = RDP_CLIPBOARD_SOURCE_RETRY;
			ctx->clipboard_inflight_client_data_source->data_source_fd = fd;
			return;
		} else {
			source->state = RDP_CLIPBOARD_SOURCE_FAILED;
			goto error_return_close_fd;
		}
	}

	if (source->base.mime_types.size == 0) {
		source->state = RDP_CLIPBOARD_SOURCE_TRANSFERRED;
		rdp_debug_clipboard(b, "RDP %s (%p:%s) source has no data\n",
				    __func__, source, clipboard_data_source_state_to_string(source));
		goto error_return_close_fd;
	}

	index = clipboard_find_supported_format_by_mime_type(mime_type);
	if (index >= 0 &&			/* check supported by this RDP bridge */
	    source->client_format_id_table[index]) {	/* check supported by current data source from client */
		ctx->clipboard_inflight_client_data_source = source;
		source->refcount++; /* reference while request inflight. */
		source->data_source_fd = fd;
		assert(source->inflight_write_count == 0);
		assert(source->inflight_data_to_write == NULL);
		assert(source->inflight_data_size == 0);
		if (index == source->format_index) {
			bool ret;

			/* data is already in data_contents, no need to pull from client */
			assert(source->transfer_event_source == NULL);
			source->state = RDP_CLIPBOARD_SOURCE_RECEIVED_DATA;
			rdp_debug_clipboard_verbose(b, "RDP %s (%p:%s) data in cache \"%s\" index:%d formatId:%d %s\n",
						    __func__, source,
						    clipboard_data_source_state_to_string(source),
						    mime_type, index,
						    source->client_format_id_table[index],
						    clipboard_format_id_to_string(source->client_format_id_table[index],
						    false));

			ret = rdp_event_loop_add_fd(loop, source->data_source_fd, WL_EVENT_WRITABLE,
						    clipboard_data_source_write, source,
						    &source->transfer_event_source);
			if (!ret) {
				source->state = RDP_CLIPBOARD_SOURCE_FAILED;
				weston_log("RDP %s (%p:%s) rdp_event_loop_add_fd failed\n",
					   __func__, source,
					   clipboard_data_source_state_to_string(source));
				goto error_return_unref_source;
			}
		} else {
			/* purge cached data */
			wl_array_release(&source->data_contents);
			wl_array_init(&source->data_contents);
			source->is_data_processed = false;
			/* update requesting format property */
			source->format_index = index;
			/* request clipboard data from client */
			formatDataRequest.msgType = CB_FORMAT_DATA_REQUEST;
			formatDataRequest.dataLen = 4;
			formatDataRequest.requestedFormatId = source->client_format_id_table[index];
			source->state = RDP_CLIPBOARD_SOURCE_REQUEST_DATA;
			rdp_debug_clipboard(b, "RDP %s (%p:%s) request data \"%s\" index:%d formatId:%d %s\n",
					    __func__, source, clipboard_data_source_state_to_string(source), mime_type, index,
					    formatDataRequest.requestedFormatId,
					    clipboard_format_id_to_string(formatDataRequest.requestedFormatId, false));
			if (ctx->clipboard_server_context->ServerFormatDataRequest(ctx->clipboard_server_context, &formatDataRequest) != 0)
				goto error_return_unref_source;
		}
	} else {
		source->state = RDP_CLIPBOARD_SOURCE_FAILED;
		weston_log("RDP %s (%p:%s) specified format \"%s\" index:%d is not supported by client\n",
			   __func__, source, clipboard_data_source_state_to_string(source),
			   mime_type, index);
		goto error_return_close_fd;
	}

	return;

error_return_unref_source:
	source->data_source_fd = -1;
	assert(source->inflight_write_count == 0);
	assert(source->inflight_data_to_write == NULL);
	assert(source->inflight_data_size == 0);
	assert(ctx->clipboard_inflight_client_data_source == source);
	ctx->clipboard_inflight_client_data_source = NULL;
	clipboard_data_source_unref(source);

error_return_close_fd:
	close(fd);
}

/* data-device informs the given data source is not longer referenced by compositor */
static void
clipboard_data_source_cancel(struct weston_data_source *base)
{
	struct rdp_clipboard_data_source *source = (struct rdp_clipboard_data_source *)base;
	freerdp_peer *client = (freerdp_peer *)source->context;
	RdpPeerContext *ctx = (RdpPeerContext *)client->context;
	struct rdp_backend *b = ctx->rdpBackend;

	rdp_debug_clipboard(b, "RDP %s (%p:%s)\n",
			    __func__, source,
			    clipboard_data_source_state_to_string(source));

	assert_compositor_thread(b);

	if (source == ctx->clipboard_inflight_client_data_source) {
		source->is_canceled = true;
		source->state = RDP_CLIPBOARD_SOURCE_CANCEL_PENDING;
		rdp_debug_clipboard(b, "RDP %s (%p:%s): still inflight - refcount:%d\n",
				    __func__, source,
				    clipboard_data_source_state_to_string(source),
				    source->refcount);
		assert(source->refcount > 1);
		return;
	}
	/* everything outside of the base has to be cleaned up */
	source->state = RDP_CLIPBOARD_SOURCE_CANCELED;
	rdp_debug_clipboard_verbose(b, "RDP %s (%p:%s) - refcount:%d\n",
				    __func__, source,
				    clipboard_data_source_state_to_string(source),
				    source->refcount);
	assert(source->refcount == 1);
	assert(source->transfer_event_source == NULL);
	wl_array_release(&source->data_contents);
	wl_array_init(&source->data_contents);
	source->is_data_processed = false;
	source->format_index = -1;
	memset(source->client_format_id_table, 0, sizeof(source->client_format_id_table));
	source->inflight_write_count = 0;
	source->inflight_data_to_write = NULL;
	source->inflight_data_size = 0;
	if (source->data_source_fd != -1) {
		close(source->data_source_fd);
		source->data_source_fd = -1;
	}
}

/**********************************\
 * Compositor idle loop callbacks *
\**********************************/

/* Publish client's available clipboard formats to compositor (make them visible to applications in server) */
static void
clipboard_data_source_publish(bool freeOnly, void *arg)
{
	struct rdp_clipboard_data_source *source = wl_container_of(arg, source, task_base);
	freerdp_peer *client = (freerdp_peer *)source->context;
	RdpPeerContext *ctx = (RdpPeerContext *)client->context;
	struct rdp_backend *b = ctx->rdpBackend;
	struct rdp_clipboard_data_source *source_prev;

	rdp_debug_clipboard(b, "RDP %s (%p:%s)\n",
			    __func__, source, clipboard_data_source_state_to_string(source));

	assert_compositor_thread(b);

	/* here is going to publish new data, if previous data from client is still referenced,
	   unref it after selection */
	source_prev = ctx->clipboard_client_data_source;
	if (!freeOnly) {
		ctx->clipboard_client_data_source = source;
		source->transfer_event_source = NULL;
		source->base.accept = clipboard_data_source_accept;
		source->base.send = clipboard_data_source_send;
		source->base.cancel = clipboard_data_source_cancel;
		source->state = RDP_CLIPBOARD_SOURCE_PUBLISHED;
		weston_seat_set_selection(ctx->item.seat, &source->base,
					  wl_display_next_serial(b->compositor->wl_display));
	} else {
		ctx->clipboard_client_data_source = NULL;
		clipboard_data_source_unref(source);
	}

	if (source_prev)
		clipboard_data_source_unref(source_prev);
}

/* Request the specified clipboard data from data-device at server side */
static void
clipboard_data_source_request(bool freeOnly, void *arg)
{
	struct rdp_clipboard_data_request *request = wl_container_of(arg, request, task_base);
	RdpPeerContext *ctx = request->ctx;
	struct rdp_backend *b = ctx->rdpBackend;
	struct weston_seat *seat = ctx->item.seat;
	struct weston_data_source *selection_data_source = seat->selection_data_source;
	struct wl_event_loop *loop = wl_display_get_event_loop(seat->compositor->wl_display);
	struct rdp_clipboard_data_source *source = NULL;
	int p[2] = {};
	const char *requested_mime_type, **mime_type;
	int index;
	bool found_requested_format;
	bool ret;

	assert_compositor_thread(b);

	if (freeOnly)
		goto error_exit_free_request;

	index = request->requested_format_index;
	assert(index >= 0 && index < (int)RDP_NUM_CLIPBOARD_FORMATS);
	requested_mime_type = clipboard_supported_formats[index].mime_type;
	rdp_debug_clipboard(b, "RDP %s (base:%p) requested mime type:\"%s\"\n",
			    __func__, selection_data_source, requested_mime_type);

	found_requested_format = FALSE;
	wl_array_for_each(mime_type, &selection_data_source->mime_types) {
		rdp_debug_clipboard(b, "RDP %s (base:%p) available formats: %s\n",
				    __func__, selection_data_source, *mime_type);
		if (strcmp(requested_mime_type, *mime_type) == 0) {
			found_requested_format = true;
			break;
		}
	}
	if (!found_requested_format) {
		rdp_debug_clipboard(b, "RDP %s (base:%p) requested format not found format:\"%s\"\n",
				    __func__, selection_data_source, requested_mime_type);
		goto error_exit_response_fail;
	}

	source = zalloc(sizeof *source);
	if (!source)
		goto error_exit_response_fail;

	/* By now, the server side data availablity is already notified
	   to client by clipboard_set_selection(). */
	source->state = RDP_CLIPBOARD_SOURCE_PUBLISHED;
	rdp_debug_clipboard(b, "RDP %s (%p:%s) for (base:%p)\n",
			    __func__, source,
			    clipboard_data_source_state_to_string(source),
			    selection_data_source);
	wl_signal_init(&source->base.destroy_signal);
	wl_array_init(&source->base.mime_types);
	wl_array_init(&source->data_contents);
	source->is_data_processed = false;
	source->context = ctx->item.peer;
	source->refcount = 1; /* decremented when data sent to client. */
	source->data_source_fd = -1;
	source->format_index = index;

	if (pipe2(p, O_CLOEXEC) == -1)
		goto error_exit_free_source;

	source->data_source_fd = p[0];

	rdp_debug_clipboard_verbose(b, "RDP %s (%p:%s) pipe write:%d -> read:%d\n",
				    __func__, source,
				    clipboard_data_source_state_to_string(source),
				    p[1], p[0]);

	/* Request data from data source */
	source->state = RDP_CLIPBOARD_SOURCE_REQUEST_DATA;
	selection_data_source->send(selection_data_source, requested_mime_type, p[1]);
	/* p[1] should be closed by data source */

	ret = rdp_event_loop_add_fd(loop, p[0], WL_EVENT_READABLE,
				    clipboard_data_source_read, source,
				    &source->transfer_event_source);
	if (!ret) {
		source->state = RDP_CLIPBOARD_SOURCE_FAILED;
		weston_log("RDP %s (%p:%s) rdp_event_loop_add_fd failed.\n",
			   __func__, source,
			   clipboard_data_source_state_to_string(source));
		goto error_exit_free_source;
	}

	free(request);

	return;

error_exit_free_source:
	assert(source->refcount == 1);
	clipboard_data_source_unref(source);
error_exit_response_fail:
	clipboard_client_send_format_data_response_fail(ctx, NULL);
error_exit_free_request:
	free(request);
}

/*************************************\
 * Compositor notification callbacks *
\*************************************/

/* Compositor notify new clipboard data is going to be copied to clipboard, and its supported formats */
static void
clipboard_set_selection(struct wl_listener *listener, void *data)
{
	RdpPeerContext *ctx =
		container_of(listener, RdpPeerContext, clipboard_selection_listener);
	struct rdp_backend *b = ctx->rdpBackend;
	struct weston_seat *seat = data;
	struct weston_data_source *selection_data_source = seat->selection_data_source;
	struct rdp_clipboard_data_source *data_source;
	CLIPRDR_FORMAT_LIST formatList = {};
	CLIPRDR_FORMAT format[RDP_NUM_CLIPBOARD_FORMATS] = {};
	const char **mime_type;
	int index, num_supported_format = 0, num_avail_format = 0;

	rdp_debug_clipboard(b, "RDP %s (base:%p)\n", __func__, selection_data_source);

	assert_compositor_thread(b);

	if (selection_data_source == NULL) {
		return;
	}

	if (selection_data_source->accept == clipboard_data_source_accept) {
		/* Callback for our data source. */
		return;
	}

	/* another data source (from server side) gets selected,
	   no longer need previous data from client. */
	if (ctx->clipboard_client_data_source) {
		data_source = ctx->clipboard_client_data_source;
		ctx->clipboard_client_data_source = NULL;
		clipboard_data_source_unref(data_source);
	}

	wl_array_for_each(mime_type, &selection_data_source->mime_types) {
		rdp_debug_clipboard(b, "RDP %s (base:%p) available formats[%d]: %s\n",
				    __func__, selection_data_source, num_avail_format, *mime_type);
		num_avail_format++;
	}

	/* check supported clipboard formats */
	wl_array_for_each(mime_type, &selection_data_source->mime_types) {
		index = clipboard_find_supported_format_by_mime_type(*mime_type);
		if (index >= 0) {
			CLIPRDR_FORMAT *f = &format[num_supported_format];

			f->formatId = clipboard_supported_formats[index].format_id;
			f->formatName = clipboard_supported_formats[index].format_name;
			rdp_debug_clipboard(b, "RDP %s (base:%p) supported formats[%d]: %d: %s\n",
					    __func__,
					    selection_data_source,
					    num_supported_format,
					    f->formatId,
					    f->formatName ?
					    f->formatName :
					    clipboard_format_id_to_string(f->formatId, true));
			num_supported_format++;
		}
	}

	if (num_supported_format) {
		/* let client knows formats are available in server clipboard */
		formatList.msgType = CB_FORMAT_LIST;
		formatList.numFormats = num_supported_format;
		formatList.formats = &format[0];
		ctx->clipboard_server_context->ServerFormatList(ctx->clipboard_server_context, &formatList);
	} else {
		rdp_debug_clipboard(b, "RDP %s (base:%p) no supported formats\n", __func__, selection_data_source);
	}
}

/*********************\
 * FreeRDP callbacks *
\*********************/

/* client reports the path of temp folder */
static UINT
clipboard_client_temp_directory(CliprdrServerContext *context, const CLIPRDR_TEMP_DIRECTORY *tempDirectory)
{
	freerdp_peer *client = (freerdp_peer *)context->custom;
	RdpPeerContext *ctx = (RdpPeerContext *)client->context;
	struct rdp_backend *b = ctx->rdpBackend;

	rdp_debug_clipboard(b, "Client: %s %s\n", __func__, tempDirectory->szTempDir);
	return 0;
}

/* client reports thier clipboard capabilities */
static UINT
clipboard_client_capabilities(CliprdrServerContext *context, const CLIPRDR_CAPABILITIES *capabilities)
{
	freerdp_peer *client = (freerdp_peer *)context->custom;
	RdpPeerContext *ctx = (RdpPeerContext *)client->context;
	struct rdp_backend *b = ctx->rdpBackend;

	rdp_debug_clipboard(b, "Client: clipboard capabilities: cCapabilitiesSet:%d\n", capabilities->cCapabilitiesSets);
	for (uint32_t i = 0; i < capabilities->cCapabilitiesSets; i++) {
		CLIPRDR_CAPABILITY_SET *capabilitySets = &capabilities->capabilitySets[i];
		CLIPRDR_GENERAL_CAPABILITY_SET *generalCapabilitySet = (CLIPRDR_GENERAL_CAPABILITY_SET *)capabilitySets;

		switch (capabilitySets->capabilitySetType) {
		case CB_CAPSTYPE_GENERAL:
			rdp_debug_clipboard(b, "Client: clipboard capabilities[%d]: General\n", i);
			rdp_debug_clipboard(b, "    Version:%d\n", generalCapabilitySet->version);
			rdp_debug_clipboard(b, "    GeneralFlags:0x%x\n", generalCapabilitySet->generalFlags);
			if (generalCapabilitySet->generalFlags & CB_USE_LONG_FORMAT_NAMES)
				rdp_debug_clipboard(b, "        CB_USE_LONG_FORMAT_NAMES\n");
			if (generalCapabilitySet->generalFlags & CB_STREAM_FILECLIP_ENABLED)
				rdp_debug_clipboard(b, "        CB_STREAM_FILECLIP_ENABLED\n");
			if (generalCapabilitySet->generalFlags & CB_FILECLIP_NO_FILE_PATHS)
				rdp_debug_clipboard(b, "        CB_FILECLIP_NO_FILE_PATHS\n");
			if (generalCapabilitySet->generalFlags & CB_CAN_LOCK_CLIPDATA)
				rdp_debug_clipboard(b, "        CB_CAN_LOCK_CLIPDATA\n");
			break;
		default:
			return -1;
		}
	}
	return 0;
}

/* client reports the supported format list in client's clipboard */
static UINT
clipboard_client_format_list(CliprdrServerContext *context, const CLIPRDR_FORMAT_LIST *formatList)
{
	CLIPRDR_FORMAT_LIST_RESPONSE formatListResponse = {};
	freerdp_peer *client = (freerdp_peer *)context->custom;
	RdpPeerContext *ctx = (RdpPeerContext *)client->context;
	struct rdp_backend *b = ctx->rdpBackend;
	struct rdp_clipboard_data_source *source = NULL;
	char **p, *s;

	assert_not_compositor_thread(b);

	rdp_debug_clipboard(b, "Client: %s clipboard format list: numFormats:%d\n", __func__, formatList->numFormats);
	for (uint32_t i = 0; i < formatList->numFormats; i++) {
		CLIPRDR_FORMAT *format = &formatList->formats[i];

		rdp_debug_clipboard(b, "Client: %s clipboard formats[%d]: formatId:%d, formatName:%s\n",
				    __func__, i, format->formatId,
				    format->formatName ? format->formatName : clipboard_format_id_to_string(format->formatId, false));
	}

	source = zalloc(sizeof *source);
	if (!source)
		goto fail;

	source->state = RDP_CLIPBOARD_SOURCE_ALLOCATED;
	rdp_debug_clipboard(b, "Client: %s (%p:%s) allocated\n",
			    __func__, source,
			    clipboard_data_source_state_to_string(source));
	wl_signal_init(&source->base.destroy_signal);
	wl_array_init(&source->base.mime_types);
	wl_array_init(&source->data_contents);
	source->context = client;
	source->refcount = 1; /* decremented when another source is selected. */
	source->data_source_fd = -1;
	source->format_index = -1;

	for (uint32_t i = 0; i < formatList->numFormats; i++) {
		CLIPRDR_FORMAT *format = &formatList->formats[i];
		int index = clipboard_find_supported_format_by_format_id_and_name(format->formatId, format->formatName);

		if (index >= 0) {
			/* save format id given from client, client can handle its own format id for private format. */
			source->client_format_id_table[index] = format->formatId;
			s = strdup(clipboard_supported_formats[index].mime_type);
			if (s) {
				p = wl_array_add(&source->base.mime_types, sizeof *p);
				if (p) {
					rdp_debug_clipboard(b, "Client: %s (%p:%s) mine_type:\"%s\" index:%d formatId:%d\n",
							    __func__, source,
							    clipboard_data_source_state_to_string(source),
							    s, index, format->formatId);
					*p = s;
				} else {
					rdp_debug_clipboard(b, "Client: %s (%p:%s) wl_array_add failed\n",
							    __func__, source,
							    clipboard_data_source_state_to_string(source));
					free(s);
				}
			} else {
				rdp_debug_clipboard(b, "Client: %s (%p:%s) strdup failed\n",
						    __func__, source,
						    clipboard_data_source_state_to_string(source));
			}
		}
	}

	if (formatList->numFormats != 0 &&
	    source->base.mime_types.size == 0) {
		rdp_debug_clipboard(b, "Client: %s (%p:%s) no formats are supported\n",
				    __func__, source,
				    clipboard_data_source_state_to_string(source));
	}

	source->state = RDP_CLIPBOARD_SOURCE_FORMATLIST_READY;
	rdp_dispatch_task_to_display_loop(ctx, clipboard_data_source_publish, &source->task_base);

fail:
	formatListResponse.msgType = CB_FORMAT_LIST_RESPONSE;
	formatListResponse.msgFlags = source ? CB_RESPONSE_OK : CB_RESPONSE_FAIL;
	formatListResponse.dataLen = 0;
	if (ctx->clipboard_server_context->ServerFormatListResponse(ctx->clipboard_server_context, &formatListResponse) != 0) {
		source->state = RDP_CLIPBOARD_SOURCE_FAILED;
		weston_log("Client: %s (%p:%s) ServerFormatListResponse failed\n",
			   __func__, source,
			   clipboard_data_source_state_to_string(source));
		return -1;
	}
	return 0;
}

/* client responded with clipboard data asked by server */
static UINT
clipboard_client_format_data_response(CliprdrServerContext *context, const CLIPRDR_FORMAT_DATA_RESPONSE *formatDataResponse)
{
	freerdp_peer *client = (freerdp_peer *)context->custom;
	RdpPeerContext *ctx = (RdpPeerContext *)client->context;
	struct rdp_backend *b = ctx->rdpBackend;
	struct wl_event_loop *loop = wl_display_get_event_loop(b->compositor->wl_display);
	struct rdp_clipboard_data_source *source = ctx->clipboard_inflight_client_data_source;
	bool success = false;
	bool ret;

	rdp_debug_clipboard(b, "Client: %s (%p:%s) flags:%d dataLen:%d\n",
			    __func__, source,
			    clipboard_data_source_state_to_string(source),
			    formatDataResponse->msgFlags,
			    formatDataResponse->dataLen);

	assert_not_compositor_thread(b);

	if (!source) {
		rdp_debug_clipboard(b, "Client: %s client send data without server asking. protocol error", __func__);
		return -1;
	}

	if (source->transfer_event_source || (source->inflight_write_count != 0)) {
		/* here means client responded more than once for single data request */
		source->state = RDP_CLIPBOARD_SOURCE_FAILED;
		weston_log("Client: %s (%p:%s) middle of write loop:%p, %d\n",
			   __func__, source, clipboard_data_source_state_to_string(source),
			   source->transfer_event_source, source->inflight_write_count);
		return -1;
	}

	if (formatDataResponse->msgFlags == CB_RESPONSE_OK) {
		/* Recieved data from client, cache to data source */
		if (wl_array_add(&source->data_contents, formatDataResponse->dataLen+1)) {
			memcpy(source->data_contents.data,
			       formatDataResponse->requestedFormatData,
			       formatDataResponse->dataLen);
			source->data_contents.size = formatDataResponse->dataLen;
			/* regardless data type, make sure it ends with NULL */
			((char *)source->data_contents.data)[source->data_contents.size] = '\0';
			/* data is ready, waiting to be written to destination */
			source->state = RDP_CLIPBOARD_SOURCE_RECEIVED_DATA;
			success = true;
		} else {
			source->state = RDP_CLIPBOARD_SOURCE_FAILED;
		}
	} else {
		source->state = RDP_CLIPBOARD_SOURCE_FAILED;
		source->data_response_fail_count++;
	}
	rdp_debug_clipboard_verbose(b, "Client: %s (%p:%s) fail count:%d\n",
				    __func__, source,
				    clipboard_data_source_state_to_string(source),
				    source->data_response_fail_count);

	assert(source->transfer_event_source == NULL);
	ret = rdp_event_loop_add_fd(loop, source->data_source_fd, WL_EVENT_WRITABLE,
				    success ? clipboard_data_source_write : clipboard_data_source_fail,
				    source, &source->transfer_event_source);
	if (!ret) {
		source->state = RDP_CLIPBOARD_SOURCE_FAILED;
		weston_log("Client: %s (%p:%s) rdp_event_loop_add_fd failed\n",
			   __func__, source, clipboard_data_source_state_to_string(source));
		return -1;
	}

	return 0;
}

/* client responded on the format list sent by server */
static UINT
clipboard_client_format_list_response(CliprdrServerContext *context,
				      const CLIPRDR_FORMAT_LIST_RESPONSE *formatListResponse)
{
	freerdp_peer *client = (freerdp_peer *)context->custom;
	RdpPeerContext *ctx = (RdpPeerContext *)client->context;
	struct rdp_backend *b = ctx->rdpBackend;

	rdp_debug_clipboard(b, "Client: %s msgFlags:0x%x\n", __func__, formatListResponse->msgFlags);
	assert_not_compositor_thread(b);
	return 0;
}

/* client requested the data of specificed format in server clipboard */
static UINT
clipboard_client_format_data_request(CliprdrServerContext *context,
				     const CLIPRDR_FORMAT_DATA_REQUEST *formatDataRequest)
{
	freerdp_peer *client = (freerdp_peer *)context->custom;
	RdpPeerContext *ctx = (RdpPeerContext *)client->context;
	struct rdp_backend *b = ctx->rdpBackend;
	struct rdp_clipboard_data_request *request;
	int index;

	rdp_debug_clipboard(b, "Client: %s requestedFormatId:%d - %s\n",
			    __func__, formatDataRequest->requestedFormatId,
			    clipboard_format_id_to_string(formatDataRequest->requestedFormatId, true));

	assert_not_compositor_thread(b);

	/* Make sure clients requested the format we knew */
	index = clipboard_find_supported_format_by_format_id(formatDataRequest->requestedFormatId);
	if (index < 0) {
		weston_log("Client: %s client requests data format the server never reported in format list response. protocol error.\n", __func__);
		goto error_return;
	}

	request = zalloc(sizeof(*request));
	if (!request) {
		weston_log("zalloc failed\n");
		goto error_return;
	}
	request->ctx = ctx;
	request->requested_format_index = index;
	rdp_dispatch_task_to_display_loop(ctx, clipboard_data_source_request, &request->task_base);

	return 0;

error_return:
	/* send FAIL response to client */
	clipboard_client_send_format_data_response_fail(ctx, NULL);
	return 0;
}

/********************\
 * Public functions *
\********************/

int
rdp_clipboard_init(freerdp_peer *client)
{
	RdpPeerContext *ctx = (RdpPeerContext *)client->context;
	struct rdp_backend *b = ctx->rdpBackend;
	struct weston_seat *seat = ctx->item.seat;
	CliprdrServerContext *clip_ctx;

	assert(seat);

	assert_compositor_thread(b);

	ctx->clipboard_server_context = cliprdr_server_context_new(ctx->vcm);
	if (!ctx->clipboard_server_context)
		goto error;

	clip_ctx = ctx->clipboard_server_context;
	clip_ctx->custom = (void *)client;
	clip_ctx->TempDirectory = clipboard_client_temp_directory;
	clip_ctx->ClientCapabilities = clipboard_client_capabilities;
	clip_ctx->ClientFormatList = clipboard_client_format_list;
	clip_ctx->ClientFormatListResponse = clipboard_client_format_list_response;
	/* clip_ctx->ClientLockClipboardData */
	/* clip_ctx->ClientUnlockClipboardData */
	clip_ctx->ClientFormatDataRequest = clipboard_client_format_data_request;
	clip_ctx->ClientFormatDataResponse = clipboard_client_format_data_response;
	/* clip_ctxClientFileContentsRequest */
	/* clip_ctx->ClientFileContentsResponse */
	clip_ctx->useLongFormatNames = FALSE; /* ASCII8 format name only (No Windows-style 2 bytes Unicode). */
	clip_ctx->streamFileClipEnabled = FALSE;
	clip_ctx->fileClipNoFilePaths = FALSE;
	clip_ctx->canLockClipData = TRUE;
	if (clip_ctx->Start(ctx->clipboard_server_context) != 0)
		goto error;

	ctx->clipboard_selection_listener.notify = clipboard_set_selection;
	wl_signal_add(&seat->selection_signal,
		      &ctx->clipboard_selection_listener);

	return 0;

error:
	if (ctx->clipboard_server_context) {
		cliprdr_server_context_free(ctx->clipboard_server_context);
		ctx->clipboard_server_context = NULL;
	}

	return -1;
}

void
rdp_clipboard_destroy(RdpPeerContext *ctx)
{
	struct rdp_clipboard_data_source *data_source;
	struct rdp_backend *b = ctx->rdpBackend;

	assert_compositor_thread(b);

	if (ctx->clipboard_selection_listener.notify) {
		wl_list_remove(&ctx->clipboard_selection_listener.link);
		ctx->clipboard_selection_listener.notify = NULL;
	}

	if (ctx->clipboard_inflight_client_data_source) {
		data_source = ctx->clipboard_inflight_client_data_source;
		ctx->clipboard_inflight_client_data_source = NULL;
		clipboard_data_source_unref(data_source);
	}

	if (ctx->clipboard_client_data_source) {
		data_source = ctx->clipboard_client_data_source;
		ctx->clipboard_client_data_source = NULL;
		clipboard_data_source_unref(data_source);
	}

	if (ctx->clipboard_server_context) {
		ctx->clipboard_server_context->Stop(ctx->clipboard_server_context);
		cliprdr_server_context_free(ctx->clipboard_server_context);
		ctx->clipboard_server_context = NULL;
	}
}
