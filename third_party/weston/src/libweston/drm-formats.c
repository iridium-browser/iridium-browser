/*
 * Copyright © 2021 Collabora, Ltd.
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

#include <libweston/libweston.h>
#include "libweston-internal.h"
#include "shared/weston-drm-fourcc.h"

/**
 * Initialize a weston_drm_format_array
 *
 * @param formats The weston_drm_format_array to initialize
 */
WL_EXPORT void
weston_drm_format_array_init(struct weston_drm_format_array *formats)
{
	wl_array_init(&formats->arr);
}

/**
 * Finish a weston_drm_format_array
 *
 * It releases the modifiers set for each format and then the
 * formats array itself.
 *
 * @param formats The weston_drm_format_array to finish
 */
WL_EXPORT void
weston_drm_format_array_fini(struct weston_drm_format_array *formats)
{
	struct weston_drm_format *fmt;

	wl_array_for_each(fmt, &formats->arr)
		wl_array_release(&fmt->modifiers);

	wl_array_release(&formats->arr);
}

static int
add_format_and_modifiers(struct weston_drm_format_array *formats,
			 uint32_t format, struct wl_array *modifiers)
{
	struct weston_drm_format *fmt;
	int ret;

	fmt = weston_drm_format_array_add_format(formats, format);
	if (!fmt)
		return -1;

	ret = wl_array_copy(&fmt->modifiers, modifiers);
	if (ret < 0) {
		weston_log("%s: out of memory\n", __func__);
		return -1;
	}

	return 0;
}

/**
 * Replace the content of a weston_drm_format_array
 *
 * Frees the content of the array and then perform a deep copy using
 * source_formats. It duplicates the array of formats and for each format it
 * duplicates the modifiers set as well.
 *
 * @param formats The weston_drm_format_array that gets its content replaced
 * @param source_formats The weston_drm_format_array to copy
 * @return 0 on success, -1 on failure
 */
WL_EXPORT int
weston_drm_format_array_replace(struct weston_drm_format_array *formats,
				const struct weston_drm_format_array *source_formats)
{
	struct weston_drm_format *source_fmt;
	int ret;

	weston_drm_format_array_fini(formats);
	weston_drm_format_array_init(formats);

	wl_array_for_each(source_fmt, &source_formats->arr) {
		ret = add_format_and_modifiers(formats, source_fmt->format,
					       &source_fmt->modifiers);
		if (ret < 0)
			return -1;
	}

	return 0;
}

/**
 * Add format to weston_drm_format_array
 *
 * Adding repeated formats is considered an error.
 *
 * @param formats The weston_drm_format_array that receives the format
 * @param format The format to add to the array
 * @return The weston_drm_format, or NULL on failure
 */
WL_EXPORT struct weston_drm_format *
weston_drm_format_array_add_format(struct weston_drm_format_array *formats,
				   uint32_t format)
{
	struct weston_drm_format *fmt;

	/* We should not try to add repeated formats to an array. */
	assert(!weston_drm_format_array_find_format(formats, format));

	fmt = wl_array_add(&formats->arr, sizeof(*fmt));
	if (!fmt) {
		weston_log("%s: out of memory\n", __func__);
		return NULL;
	}

	fmt->format = format;
	wl_array_init(&fmt->modifiers);

	return fmt;
}

/**
 * Remove latest format added to a weston_drm_format_array
 *
 * Calling this function for an empty array is an error, at least one element
 * must be in the array.
 *
 * @param formats The weston_drm_format_array from which the format is removed
 */
WL_EXPORT void
weston_drm_format_array_remove_latest_format(struct weston_drm_format_array *formats)
{
	struct wl_array *array = &formats->arr;
	struct weston_drm_format *fmt;

	assert(array->size >= sizeof(*fmt));

	array->size -= sizeof(*fmt);

	fmt = array->data + array->size;
	wl_array_release(&fmt->modifiers);
}

/**
 * Find format in a weston_drm_format_array
 *
 * @param formats The weston_drm_format_array where to look for the format
 * @param format The format to look for
 * @return The weston_drm_format if format was found, or NULL otherwise
 */
WL_EXPORT struct weston_drm_format *
weston_drm_format_array_find_format(const struct weston_drm_format_array *formats,
				    uint32_t format)
{
	struct weston_drm_format *fmt;

	wl_array_for_each(fmt, &formats->arr)
		if (fmt->format == format)
			return fmt;

	return NULL;
}

/**
 * Counts the number of format/modifier pairs in a weston_drm_format_array
 *
 * @param formats The weston_drm_format_array
 * @return The number of format/modifier pairs in the array
 */
WL_EXPORT unsigned int
weston_drm_format_array_count_pairs(const struct weston_drm_format_array *formats)
{
	struct weston_drm_format *fmt;
	unsigned int num_pairs = 0;

	wl_array_for_each(fmt, &formats->arr)
		num_pairs += fmt->modifiers.size / sizeof(uint64_t);

	return num_pairs;
}

/**
 * Compare the content of two weston_drm_format_array
 *
 * @param formats_A One of the weston_drm_format_array to compare
 * @param formats_B The other weston_drm_format_array to compare
 * @return True if both sets are equivalent, false otherwise
 */
WL_EXPORT bool
weston_drm_format_array_equal(const struct weston_drm_format_array *formats_A,
			      const struct weston_drm_format_array *formats_B)
{
	struct weston_drm_format *fmt_A, *fmt_B;
	const uint64_t *modifiers_A;
	unsigned num_modifiers_A, num_modifiers_B;
	unsigned int i;

	if (formats_A->arr.size != formats_B->arr.size)
		return false;

	wl_array_for_each(fmt_A, &formats_A->arr) {
		fmt_B = weston_drm_format_array_find_format(formats_B,
							    fmt_A->format);
		if (!fmt_B)
			return false;

		modifiers_A = weston_drm_format_get_modifiers(fmt_A, &num_modifiers_A);
		weston_drm_format_get_modifiers(fmt_B, &num_modifiers_B);
		if (num_modifiers_A != num_modifiers_B)
			return false;
		for (i = 0; i < num_modifiers_A; i++)
			if (!weston_drm_format_has_modifier(fmt_B, modifiers_A[i]))
				return false;
	}

	return true;
}

/**
 * Joins two weston_drm_format_array, keeping the result in A
 *
 * @param formats_A The weston_drm_format_array that receives the formats from B
 * @param formats_B The weston_drm_format_array whose formats are added to A
 * @return 0 on success, -1 on failure
 */
WL_EXPORT int
weston_drm_format_array_join(struct weston_drm_format_array *formats_A,
			     const struct weston_drm_format_array *formats_B)
{
	struct weston_drm_format *fmt_A, *fmt_B;
	const uint64_t *modifiers;
	unsigned int num_modifiers;
	unsigned int i;
	int ret;

	wl_array_for_each(fmt_B, &formats_B->arr) {
		fmt_A = weston_drm_format_array_find_format(formats_A,
							    fmt_B->format);
		if (!fmt_A) {
			fmt_A = weston_drm_format_array_add_format(formats_A,
								   fmt_B->format);
			if (!fmt_A)
				return -1;
		}

		modifiers = weston_drm_format_get_modifiers(fmt_B, &num_modifiers);
		for (i = 0; i < num_modifiers; i++) {
			if (weston_drm_format_has_modifier(fmt_A, modifiers[i]))
				continue;
			ret = weston_drm_format_add_modifier(fmt_A, modifiers[i]);
			if (ret < 0)
				return -1;
		}
	}

	return 0;
}

static int
modifiers_intersect(const struct weston_drm_format *fmt_A,
		    const struct weston_drm_format *fmt_B,
		    struct wl_array *modifiers_result)
{
	const uint64_t *modifiers;
	unsigned int num_modifiers;
	uint64_t *mod;
	unsigned int i;

	modifiers = weston_drm_format_get_modifiers(fmt_A, &num_modifiers);
	for (i = 0; i < num_modifiers; i++) {
		if (!weston_drm_format_has_modifier(fmt_B, modifiers[i]))
			continue;
		mod = wl_array_add(modifiers_result, sizeof(modifiers[i]));
		if (!mod) {
			weston_log("%s: out of memory\n", __func__);
			return -1;
		}
		*mod = modifiers[i];
	}

	return 0;
}

/**
 * Compute the intersection between two DRM-format arrays, keeping the result in A
 *
 * @param formats_A The weston_drm_format_array that keeps the result
 * @param formats_B The other weston_drm_format_array
 * @return 0 on success, -1 on failure
 */
WL_EXPORT int
weston_drm_format_array_intersect(struct weston_drm_format_array *formats_A,
				  const struct weston_drm_format_array *formats_B)
{
	struct weston_drm_format_array formats_result;
	struct weston_drm_format *fmt_result, *fmt_A, *fmt_B;
	int ret;

	weston_drm_format_array_init(&formats_result);

	wl_array_for_each(fmt_A, &formats_A->arr) {
		fmt_B = weston_drm_format_array_find_format(formats_B,
							    fmt_A->format);
		if (!fmt_B)
			continue;

		fmt_result = weston_drm_format_array_add_format(&formats_result,
								fmt_A->format);
		if (!fmt_result)
			goto err;

		ret = modifiers_intersect(fmt_A, fmt_B, &fmt_result->modifiers);
		if (ret < 0)
			goto err;

		if (fmt_result->modifiers.size == 0)
			weston_drm_format_array_remove_latest_format(&formats_result);
	}

	ret = weston_drm_format_array_replace(formats_A, &formats_result);
	if (ret < 0)
		goto err;

	weston_drm_format_array_fini(&formats_result);
	return 0;

err:
	weston_drm_format_array_fini(&formats_result);
	return -1;
}

static int
modifiers_subtract(const struct weston_drm_format *fmt_A,
		   const struct weston_drm_format *fmt_B,
		   struct wl_array *modifiers_result)
{
	const uint64_t *modifiers;
	unsigned int num_modifiers;
	uint64_t *mod;
	unsigned int i;

	modifiers = weston_drm_format_get_modifiers(fmt_A, &num_modifiers);
	for (i = 0; i < num_modifiers; i++) {
		if (weston_drm_format_has_modifier(fmt_B, modifiers[i]))
			continue;
		mod = wl_array_add(modifiers_result, sizeof(modifiers[i]));
		if (!mod) {
			weston_log("%s: out of memory\n", __func__);
			return -1;
		}
		*mod = modifiers[i];
	}

	return 0;
}

/**
 * Compute the subtraction between two DRM-format arrays, keeping the result in A
 *
 * @param formats_A The minuend weston_drm_format_array
 * @param formats_B The subtrahend weston_drm_format_array
 * @return 0 on success, -1 on failure
 */
WL_EXPORT int
weston_drm_format_array_subtract(struct weston_drm_format_array *formats_A,
				 const struct weston_drm_format_array *formats_B)
{
	struct weston_drm_format_array formats_result;
	struct weston_drm_format *fmt_result, *fmt_A, *fmt_B;
	int ret;

	weston_drm_format_array_init(&formats_result);

	wl_array_for_each(fmt_A, &formats_A->arr) {
		fmt_B = weston_drm_format_array_find_format(formats_B,
							    fmt_A->format);
		if (!fmt_B) {
			ret = add_format_and_modifiers(&formats_result, fmt_A->format,
						       &fmt_A->modifiers);
			if (ret < 0)
				goto err;

			continue;
		}

		fmt_result = weston_drm_format_array_add_format(&formats_result,
								fmt_A->format);
		if (!fmt_result)
			goto err;

		ret = modifiers_subtract(fmt_A, fmt_B, &fmt_result->modifiers);
		if (ret < 0)
			goto err;

		if (fmt_result->modifiers.size == 0)
			weston_drm_format_array_remove_latest_format(&formats_result);
	}

	ret = weston_drm_format_array_replace(formats_A, &formats_result);
	if (ret < 0)
		goto err;

	weston_drm_format_array_fini(&formats_result);
	return 0;

err:
	weston_drm_format_array_fini(&formats_result);
	return -1;
}

/**
 * Add modifier to modifier set of a weston_drm_format.
 *
 * Adding repeated modifiers is considered an error.
 *
 * @param format The weston_drm_format that owns the modifier set to which
 * the modifier should be added
 * @param modifier The modifier to add
 * @return 0 on success, -1 on failure
 */
WL_EXPORT int
weston_drm_format_add_modifier(struct weston_drm_format *format,
			       uint64_t modifier)
{
	uint64_t *mod;

	/* We should not try to add repeated modifiers to a set. */
	assert(!weston_drm_format_has_modifier(format, modifier));

	mod = wl_array_add(&format->modifiers, sizeof(*mod));
	if (!mod) {
		weston_log("%s: out of memory\n", __func__);
		return -1;
	}
	*mod = modifier;

	return 0;
}

/**
 * Check if modifier set of a weston_drm_format contains a certain modifier
 *
 * @param format The weston_drm_format that owns the modifier set where to
 * look for the modifier
 * @param modifier The modifier to look for
 * @return True if modifier was found, false otherwise
 */
WL_EXPORT bool
weston_drm_format_has_modifier(const struct weston_drm_format *format,
			       uint64_t modifier)
{
	const uint64_t *modifiers;
	unsigned int num_modifiers;
	unsigned int i;

	modifiers = weston_drm_format_get_modifiers(format, &num_modifiers);
	for (i = 0; i < num_modifiers; i++)
		if (modifiers[i] == modifier)
			return true;

	return false;
}

/**
 * Get array of modifiers and modifiers count from a weston_drm_format
 *
 * @param format The weston_drm_format that contains the modifiers
 * @param count_out Parameter that receives the modifiers count
 * @return The array of modifiers
 */
WL_EXPORT const uint64_t *
weston_drm_format_get_modifiers(const struct weston_drm_format *format,
				unsigned int *count_out)
{
	*count_out = format->modifiers.size / sizeof(uint64_t);
	return format->modifiers.data;
}
