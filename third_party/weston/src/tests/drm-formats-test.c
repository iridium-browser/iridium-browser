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
#include <libweston-internal.h>
#include "shared/weston-drm-fourcc.h"

#include "weston-test-client-helper.h"
#include "weston-test-fixture-compositor.h"

/* Add multiple formats to weston_drm_format_array and add the same set of
 * modifiers to each format. */
#define ADD_FORMATS_AND_MODS(dest, formats, mods) \
do { \
        unsigned int i; \
        for (i = 0; i < ARRAY_LENGTH(formats); i++) \
                format_array_add_format_and_modifiers(dest, (formats)[i], \
                                                mods, ARRAY_LENGTH(mods)); \
} while (0)

/* Same as ADD_FORMATS_AND_MODS, but add the formats in reverse order. */
#define ADD_FORMATS_AND_MODS_REVERSE(dest, formats, mods) \
do { \
        int i; \
        for (i = ARRAY_LENGTH(formats) - 1; i >= 0; i--) \
                format_array_add_format_and_modifiers(dest, (formats)[i], \
                                                mods, ARRAY_LENGTH(mods)); \
} while (0)

static void
format_array_add_format_and_modifiers(struct weston_drm_format_array *formats,
                                      uint32_t format, uint64_t *modifiers,
                                      unsigned int num_modifiers)
{
        struct weston_drm_format *fmt;
        unsigned int i;
        int ret;

        fmt = weston_drm_format_array_add_format(formats, format);
        assert(fmt);

        for (i = 0; i < num_modifiers; i++) {
                ret = weston_drm_format_add_modifier(fmt, modifiers[i]);
                assert(ret == 0);
        }
}

TEST(basic_operations)
{
        struct weston_drm_format_array format_array;
        struct weston_drm_format *fmt;
        uint32_t formats[] = {1, 2, 3, 4, 5};
        uint64_t modifiers[] = {11, 12, 13, 14, 15};
        unsigned int i, j;

        weston_drm_format_array_init(&format_array);

        assert(weston_drm_format_array_count_pairs(&format_array) == 0);

        ADD_FORMATS_AND_MODS(&format_array, formats, modifiers);

        for (i = 0; i < ARRAY_LENGTH(formats); i++) {
                fmt = weston_drm_format_array_find_format(&format_array, formats[i]);
                assert(fmt && fmt->format == formats[i]);
                for (j = 0; j < ARRAY_LENGTH(modifiers); j++)
                        assert(weston_drm_format_has_modifier(fmt, modifiers[j]));
        }

        assert(weston_drm_format_array_count_pairs(&format_array) ==
               ARRAY_LENGTH(formats) * ARRAY_LENGTH(modifiers));

        weston_drm_format_array_fini(&format_array);
}

TEST(compare_arrays_same_content)
{
        struct weston_drm_format_array format_array_A, format_array_B;
        uint32_t formats[] = {1, 2, 3, 4, 5};
        uint64_t modifiers[] = {11, 12, 13, 14, 15};

        weston_drm_format_array_init(&format_array_A);
        weston_drm_format_array_init(&format_array_B);

        /* Both are empty arrays, so they have the same content. */
        assert(weston_drm_format_array_equal(&format_array_A, &format_array_B));

        /* Test non-empty arrays with same content. */
        ADD_FORMATS_AND_MODS(&format_array_A, formats, modifiers);
        ADD_FORMATS_AND_MODS(&format_array_B, formats, modifiers);
        assert(weston_drm_format_array_equal(&format_array_A, &format_array_B));

        /* Test non-empty arrays with same content, but add elements to B in
         * reverse order. This is important as in the future we may keep
         * DRM-format arrays ordered to improve performance. */
        weston_drm_format_array_fini(&format_array_B);
        weston_drm_format_array_init(&format_array_B);
        ADD_FORMATS_AND_MODS_REVERSE(&format_array_B, formats, modifiers);
        assert(weston_drm_format_array_equal(&format_array_A, &format_array_B));

        weston_drm_format_array_fini(&format_array_A);
        weston_drm_format_array_fini(&format_array_B);
}

TEST(compare_arrays_exclusive_content)
{
        struct weston_drm_format_array format_array_A, format_array_B;
        uint32_t formats_A[] = {1, 2, 3, 4, 5};
        uint32_t formats_B[] = {6, 7, 8, 9, 10};
        uint64_t modifiers_A[] = {11, 12, 13, 14, 15};
        uint64_t modifiers_B[] = {16, 17, 18, 19, 20};

        weston_drm_format_array_init(&format_array_A);
        weston_drm_format_array_init(&format_array_B);

        /* Arrays with formats that are mutually exclusive. */
        ADD_FORMATS_AND_MODS(&format_array_A, formats_A, modifiers_A);
        ADD_FORMATS_AND_MODS(&format_array_B, formats_B, modifiers_B);
        assert(!weston_drm_format_array_equal(&format_array_A, &format_array_B));

        weston_drm_format_array_fini(&format_array_A);
        weston_drm_format_array_fini(&format_array_B);
}

TEST(replace_array)
{
        struct weston_drm_format_array format_array_A, format_array_B;
        uint32_t formats[] = {1, 2, 3, 4, 5};
        uint64_t modifiers[] = {11, 12, 13, 14, 15};
        int ret;

        weston_drm_format_array_init(&format_array_A);
        weston_drm_format_array_init(&format_array_B);

        /* Replace content of B with the content of A, so they should
         * have the same content. */
        ADD_FORMATS_AND_MODS(&format_array_A, formats, modifiers);
        ret = weston_drm_format_array_replace(&format_array_B, &format_array_A);
        assert(ret == 0);
        assert(weston_drm_format_array_equal(&format_array_A, &format_array_B));

        weston_drm_format_array_fini(&format_array_A);
        weston_drm_format_array_fini(&format_array_B);
}

TEST(remove_from_array)
{
        struct weston_drm_format_array format_array_A, format_array_B, format_array_C;
        uint32_t formats_A[] = {1, 2, 3, 4, 5};
        uint32_t formats_B[] = {1, 2, 3, 4};
        uint32_t formats_C[] = {1, 2, 3, 4, 6};
        uint64_t modifiers[] = {11, 12, 13, 14, 15};

        weston_drm_format_array_init(&format_array_A);
        weston_drm_format_array_init(&format_array_B);
        weston_drm_format_array_init(&format_array_C);

        /* After removing latest added format from array A, it should
         * be equal to B. */
        ADD_FORMATS_AND_MODS(&format_array_A, formats_A, modifiers);
        ADD_FORMATS_AND_MODS(&format_array_B, formats_B, modifiers);
        weston_drm_format_array_remove_latest_format(&format_array_A);
        assert(weston_drm_format_array_equal(&format_array_A, &format_array_B));

        /* Add 6 to the format array A, so it should be equal to C. */
        ADD_FORMATS_AND_MODS(&format_array_A, (uint32_t[]){6}, modifiers);
        ADD_FORMATS_AND_MODS(&format_array_C, formats_C, modifiers);
        assert(weston_drm_format_array_equal(&format_array_A, &format_array_C));

        weston_drm_format_array_fini(&format_array_A);
        weston_drm_format_array_fini(&format_array_B);
        weston_drm_format_array_fini(&format_array_C);
}

TEST(join_arrays)
{
        struct weston_drm_format_array format_array_A, format_array_B;
        struct weston_drm_format_array format_array_C;
        uint32_t formats_A[] = {1, 2, 6, 9, 10};
        uint32_t formats_B[] = {2, 5, 7, 9, 10};
        uint64_t modifiers_A[] = {1, 2, 3, 4, 7};
        uint64_t modifiers_B[] = {0, 2, 3, 5, 6};
        uint64_t modifiers_join[] = {0, 1, 2, 3, 4, 5, 6, 7};
        int ret;

        weston_drm_format_array_init(&format_array_A);
        weston_drm_format_array_init(&format_array_B);
        weston_drm_format_array_init(&format_array_C);

        ADD_FORMATS_AND_MODS(&format_array_A, formats_A, modifiers_A);
        ADD_FORMATS_AND_MODS(&format_array_B, formats_B, modifiers_B);
        ret = weston_drm_format_array_join(&format_array_A, &format_array_B);
        assert(ret == 0);

        /* The result of the joint (which is saved in A) should have
         * the same content as C. */
        ADD_FORMATS_AND_MODS(&format_array_C, (uint32_t[]){1}, modifiers_A);
        ADD_FORMATS_AND_MODS(&format_array_C, (uint32_t[]){2}, modifiers_join);
        ADD_FORMATS_AND_MODS(&format_array_C, (uint32_t[]){5}, modifiers_B);
        ADD_FORMATS_AND_MODS(&format_array_C, (uint32_t[]){6}, modifiers_A);
        ADD_FORMATS_AND_MODS(&format_array_C, (uint32_t[]){7}, modifiers_B);
        ADD_FORMATS_AND_MODS(&format_array_C, (uint32_t[]){9}, modifiers_join);
        ADD_FORMATS_AND_MODS(&format_array_C, (uint32_t[]){10}, modifiers_join);
        assert(weston_drm_format_array_equal(&format_array_A, &format_array_C));

        weston_drm_format_array_fini(&format_array_A);
        weston_drm_format_array_fini(&format_array_B);
        weston_drm_format_array_fini(&format_array_C);
}

TEST(join_arrays_same_content)
{
        struct weston_drm_format_array format_array_A, format_array_B;
        uint32_t formats[] = {1, 2, 3, 4, 5};
        uint64_t modifiers[] = {11, 12, 13, 14, 15};
        int ret;

        weston_drm_format_array_init(&format_array_A);
        weston_drm_format_array_init(&format_array_B);

        /* Joint of empty arrays must be empty. */
        ret = weston_drm_format_array_join(&format_array_A, &format_array_B);
        assert(ret == 0);
        assert(format_array_A.arr.size == 0);

        /* Join B, which is empty, with A, which is non-empty. The joint (which
         * is saved in B) should have the same content as A. */
        ADD_FORMATS_AND_MODS(&format_array_A, formats, modifiers);
        ret = weston_drm_format_array_join(&format_array_B, &format_array_A);
        assert(ret == 0);
        assert(weston_drm_format_array_equal(&format_array_A, &format_array_B));

        /* Now A and B are non-empty and have the same content. The joint (which
         * is saved in A) should not change its content. */
        ret = weston_drm_format_array_join(&format_array_A, &format_array_B);
        assert(ret == 0);
        assert(weston_drm_format_array_equal(&format_array_A, &format_array_B));

        weston_drm_format_array_fini(&format_array_A);
        weston_drm_format_array_fini(&format_array_B);
}

TEST(join_arrays_exclusive_content)
{
        struct weston_drm_format_array format_array_A, format_array_B;
        struct weston_drm_format_array format_array_C;
        uint32_t formats_A[] = {1, 2, 3, 4, 5};
        uint32_t formats_B[] = {6, 7, 8, 9, 10};
        uint32_t formats_C[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        uint64_t modifiers[] = {11, 12, 13, 14, 15};
        int ret;

        weston_drm_format_array_init(&format_array_A);
        weston_drm_format_array_init(&format_array_B);
        weston_drm_format_array_init(&format_array_C);

        /* The joint of DRM-format arrays A and B should be equal to C. */
        ADD_FORMATS_AND_MODS(&format_array_A, formats_A, modifiers);
        ADD_FORMATS_AND_MODS(&format_array_B, formats_B, modifiers);
        ADD_FORMATS_AND_MODS(&format_array_C, formats_C, modifiers);
        ret = weston_drm_format_array_join(&format_array_A, &format_array_B);
        assert(ret == 0);
        assert(weston_drm_format_array_equal(&format_array_A, &format_array_C));

        weston_drm_format_array_fini(&format_array_A);
        weston_drm_format_array_fini(&format_array_B);
        weston_drm_format_array_fini(&format_array_C);
}

TEST(join_arrays_modifier_invalid)
{
        struct weston_drm_format_array format_array_A, format_array_B;
        struct weston_drm_format_array format_array_C;
        uint64_t regular_modifiers[] = {1, 2, 3, 4, 5};
        uint64_t modifier_invalid[] = {DRM_FORMAT_MOD_INVALID};
        uint64_t regular_modifiers_plus_invalid[] = {1, 2, 3, 4, 5, DRM_FORMAT_MOD_INVALID};
        int ret;

        weston_drm_format_array_init(&format_array_A);
        weston_drm_format_array_init(&format_array_B);
        weston_drm_format_array_init(&format_array_C);

        /* DRM-format array A has only one format with MOD_INVALID, and B has
         * the same format but with a regular set of formats. The joint should
         * contain both MOD_INVALID and the regular modifiers. */
        ADD_FORMATS_AND_MODS(&format_array_A, (uint32_t[]){1}, modifier_invalid);
        ADD_FORMATS_AND_MODS(&format_array_B, (uint32_t[]){1}, regular_modifiers);
        ADD_FORMATS_AND_MODS(&format_array_C, (uint32_t[]){1}, regular_modifiers_plus_invalid);
        ret = weston_drm_format_array_join(&format_array_A, &format_array_B);
        assert(ret == 0);
        assert(weston_drm_format_array_equal(&format_array_A, &format_array_C));

        weston_drm_format_array_fini(&format_array_A);
        weston_drm_format_array_fini(&format_array_B);
        weston_drm_format_array_fini(&format_array_C);
}

TEST(intersect_arrays)
{
        struct weston_drm_format_array format_array_A, format_array_B;
        struct weston_drm_format_array format_array_C;
        uint32_t formats_A[] = {1, 2, 6, 9, 10};
        uint32_t formats_B[] = {2, 5, 7, 9, 10};
        uint64_t modifiers_A[] = {1, 2, 3, 4, 7};
        uint64_t modifiers_B[] = {0, 2, 3, 5, 6};
        uint64_t modifiers_intersect[] = {2, 3};
        int ret;

        weston_drm_format_array_init(&format_array_A);
        weston_drm_format_array_init(&format_array_B);
        weston_drm_format_array_init(&format_array_C);

        ADD_FORMATS_AND_MODS(&format_array_A, formats_A, modifiers_A);
        ADD_FORMATS_AND_MODS(&format_array_B, formats_B, modifiers_B);
        ret = weston_drm_format_array_intersect(&format_array_A, &format_array_B);
        assert(ret == 0);

        /* The result of the intersection (stored in A) should have the same
         * content as C. */
        ADD_FORMATS_AND_MODS(&format_array_C, (uint32_t[]){2}, modifiers_intersect);
        ADD_FORMATS_AND_MODS(&format_array_C, (uint32_t[]){9}, modifiers_intersect);
        ADD_FORMATS_AND_MODS(&format_array_C, (uint32_t[]){10}, modifiers_intersect);
        assert(weston_drm_format_array_equal(&format_array_A, &format_array_C));

        weston_drm_format_array_fini(&format_array_A);
        weston_drm_format_array_fini(&format_array_B);
        weston_drm_format_array_fini(&format_array_C);
}

TEST(intersect_arrays_same_content)
{
        struct weston_drm_format_array format_array_A, format_array_B;
        uint32_t formats[] = {1, 2, 3, 4, 5};
        uint64_t modifiers[] = {11, 12, 13, 14, 15};
        int ret;

        weston_drm_format_array_init(&format_array_A);
        weston_drm_format_array_init(&format_array_B);

        /* The intersection between two empty arrays must be an
         * empty array. */
        ret = weston_drm_format_array_intersect(&format_array_A, &format_array_B);
        assert(ret == 0);
        assert(format_array_A.arr.size == 0);

        /* DRM-format arrays A and B have the same content, so the intersection
         * should be equal to them. A keeps the result of the intersection, and B
         * does not change. So we compare them. */
        ADD_FORMATS_AND_MODS(&format_array_A, formats, modifiers);
        ret = weston_drm_format_array_replace(&format_array_B, &format_array_A);
        assert(ret == 0);
        ret = weston_drm_format_array_intersect(&format_array_A, &format_array_B);
        assert(ret == 0);
        assert(weston_drm_format_array_equal(&format_array_A, &format_array_B));

        weston_drm_format_array_fini(&format_array_A);
        weston_drm_format_array_fini(&format_array_B);
}

TEST(intersect_arrays_exclusive_formats)
{
        struct weston_drm_format_array format_array_A, format_array_B;
        uint64_t formats_A[] = {1, 2, 3, 4, 5};
        uint64_t formats_B[] = {6, 7, 8, 9, 10};
        uint64_t modifiers[] = {11, 12, 13, 14, 15};
        int ret;

        weston_drm_format_array_init(&format_array_A);
        weston_drm_format_array_init(&format_array_B);

        /* DRM-format arrays A and B have formats that are mutually exclusive,
         * so the intersection (which is stored in A) must be empty. */
        ADD_FORMATS_AND_MODS(&format_array_A, formats_A, modifiers);
        ADD_FORMATS_AND_MODS(&format_array_B, formats_B, modifiers);
        ret = weston_drm_format_array_intersect(&format_array_A, &format_array_B);
        assert(ret ==  0);
        assert(format_array_A.arr.size == 0);

        weston_drm_format_array_fini(&format_array_A);
        weston_drm_format_array_fini(&format_array_B);
}

TEST(intersect_arrays_exclusive_modifiers)
{
        struct weston_drm_format_array format_array_A, format_array_B;
        uint64_t modifiers_A[] = {1, 2, 3, 4, 5};
        uint64_t modifiers_B[] = {6, 7, 8, 9, 10};
        int ret;

        weston_drm_format_array_init(&format_array_A);
        weston_drm_format_array_init(&format_array_B);

        /* Both DRM-format arrays A and B have the same format but with modifier
         * sets that are mutually exclusive. The intersection (which is stored
         * in A) between mutually exclusive modifier must be empty, and so the
         * format should not be added to the array. So the array must also be
         * empty. */
        ADD_FORMATS_AND_MODS(&format_array_A, (uint32_t[]){1}, modifiers_A);
        ADD_FORMATS_AND_MODS(&format_array_B, (uint32_t[]){1}, modifiers_B);
        ret = weston_drm_format_array_intersect(&format_array_A, &format_array_B);
        assert(ret == 0);
        assert(format_array_A.arr.size == 0);

        weston_drm_format_array_fini(&format_array_A);
        weston_drm_format_array_fini(&format_array_B);
}

TEST(subtract_arrays)
{
        struct weston_drm_format_array format_array_A, format_array_B;
        struct weston_drm_format_array format_array_C;
        uint32_t formats_A[] = {1, 2, 6, 9, 10};
        uint32_t formats_B[] = {2, 5, 7, 9, 10};
        uint64_t modifiers_A[] = {1, 2, 3, 4, 7};
        uint64_t modifiers_B[] = {0, 2, 3, 5, 6};
        uint64_t modifiers_subtract[] = {1, 4, 7};
        int ret;

        weston_drm_format_array_init(&format_array_A);
        weston_drm_format_array_init(&format_array_B);
        weston_drm_format_array_init(&format_array_C);

        ADD_FORMATS_AND_MODS(&format_array_A, formats_A, modifiers_A);
        ADD_FORMATS_AND_MODS(&format_array_B, formats_B, modifiers_B);
        ret = weston_drm_format_array_subtract(&format_array_A, &format_array_B);
        assert(ret == 0);

        /* The result of the subtraction (which is saved in A) should have
         * the same content as C. */
        ADD_FORMATS_AND_MODS(&format_array_C, (uint32_t[]){1}, modifiers_A);
        ADD_FORMATS_AND_MODS(&format_array_C, (uint32_t[]){2}, modifiers_subtract);
        ADD_FORMATS_AND_MODS(&format_array_C, (uint32_t[]){6}, modifiers_A);
        ADD_FORMATS_AND_MODS(&format_array_C, (uint32_t[]){9}, modifiers_subtract);
        ADD_FORMATS_AND_MODS(&format_array_C, (uint32_t[]){10}, modifiers_subtract);
        assert(weston_drm_format_array_equal(&format_array_A, &format_array_C));

        weston_drm_format_array_fini(&format_array_A);
        weston_drm_format_array_fini(&format_array_B);
        weston_drm_format_array_fini(&format_array_C);
}

TEST(subtract_arrays_same_content)
{
        struct weston_drm_format_array format_array_A, format_array_B;
        uint32_t formats[] = {1, 2, 3, 4, 5};
        uint64_t modifiers[] = {11, 12, 13, 14, 15};
        int ret;

        weston_drm_format_array_init(&format_array_A);
        weston_drm_format_array_init(&format_array_B);

        /* Minuend and subtrahend have the same content. The subtraction
         * (which is saved in A) should be an empty array. */
        ADD_FORMATS_AND_MODS(&format_array_A, formats, modifiers);
        ret = weston_drm_format_array_replace(&format_array_B, &format_array_A);
        assert(ret == 0);
        ret = weston_drm_format_array_subtract(&format_array_A, &format_array_B);
        assert(ret == 0);
        assert(format_array_A.arr.size == 0);

        weston_drm_format_array_fini(&format_array_A);
        weston_drm_format_array_fini(&format_array_B);
}

TEST(subtract_arrays_exclusive_formats)
{
        struct weston_drm_format_array format_array_A, format_array_B;
        struct weston_drm_format_array format_array_C;
        uint32_t formats_A[] = {1, 2, 3, 4, 5};
        uint32_t formats_B[] = {6, 7, 8, 9, 10};
        uint64_t modifiers[] = {11, 12, 13, 14, 15};
        int ret;

        weston_drm_format_array_init(&format_array_A);
        weston_drm_format_array_init(&format_array_B);
        weston_drm_format_array_init(&format_array_C);

        /* Minuend and subtrahend have mutually exclusive formats. The
         * subtraction (which is saved in A) should be equal the minuend. */
        ADD_FORMATS_AND_MODS(&format_array_A, formats_A, modifiers);
        ADD_FORMATS_AND_MODS(&format_array_B, formats_B, modifiers);
        ret = weston_drm_format_array_replace(&format_array_C, &format_array_A);
        assert(ret == 0);

        ret = weston_drm_format_array_subtract(&format_array_A, &format_array_B);
        assert(ret == 0);
        assert(weston_drm_format_array_equal(&format_array_A, &format_array_C));

        weston_drm_format_array_fini(&format_array_A);
        weston_drm_format_array_fini(&format_array_B);
        weston_drm_format_array_fini(&format_array_C);
}

TEST(subtract_arrays_exclusive_modifiers)
{
        struct weston_drm_format_array format_array_A, format_array_B;
        struct weston_drm_format_array format_array_C;
        uint64_t modifiers_A[] = {1, 2, 3, 4, 5};
        uint64_t modifiers_B[] = {6, 7, 8, 9, 10};
        int ret;

        weston_drm_format_array_init(&format_array_A);
        weston_drm_format_array_init(&format_array_B);
        weston_drm_format_array_init(&format_array_C);

        /* Minuend and subtrahend have the same format but with modifiers that
         * are mutually exclusive. The subtraction (which is saved in A) should
         * contain the format and the modifier set of the minuend. */
        ADD_FORMATS_AND_MODS(&format_array_A, (uint32_t[]){1}, modifiers_A);
        ADD_FORMATS_AND_MODS(&format_array_B, (uint32_t[]){1}, modifiers_B);
        ret = weston_drm_format_array_replace(&format_array_C, &format_array_A);
        assert(ret == 0);

        ret = weston_drm_format_array_subtract(&format_array_A, &format_array_B);
        assert(ret == 0);
        assert(weston_drm_format_array_equal(&format_array_A, &format_array_C));

        weston_drm_format_array_fini(&format_array_A);
        weston_drm_format_array_fini(&format_array_B);
        weston_drm_format_array_fini(&format_array_C);
}

TEST(subtract_arrays_modifier_invalid)
{
        struct weston_drm_format_array format_array_A, format_array_B;
        uint64_t modifier_invalid[] = {DRM_FORMAT_MOD_INVALID};
        uint64_t regular_modifiers_plus_invalid[] = {1, 2, 3, 4, 5, DRM_FORMAT_MOD_INVALID};
        int ret;

        weston_drm_format_array_init(&format_array_A);
        weston_drm_format_array_init(&format_array_B);

        /* The minuend has a format with modifier set that contains MOD_INVALID
         * and the subtrahend contains the same format but with a regular set of
         * modifiers + MOD_INVALID. So the subtraction between the modifiers
         * sets results in empty, and so the format should not be included to
         * the result. As it is the only format in the minuend, the resulting
         * array must be empty. */
        ADD_FORMATS_AND_MODS(&format_array_A, (uint32_t[]){1}, modifier_invalid);
        ADD_FORMATS_AND_MODS(&format_array_B, (uint32_t[]){1}, regular_modifiers_plus_invalid);
        ret = weston_drm_format_array_subtract(&format_array_A, &format_array_B);
        assert(ret == 0);
        assert(format_array_A.arr.size == 0);

        weston_drm_format_array_fini(&format_array_A);
        weston_drm_format_array_fini(&format_array_B);
}
