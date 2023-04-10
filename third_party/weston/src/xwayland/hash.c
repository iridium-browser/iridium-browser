/*
 * Copyright © 2009 Intel Corporation
 * Copyright © 1988-2004 Keith Packard and Bart Massey.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Except as contained in this notice, the names of the authors
 * or their institutions shall not be used in advertising or
 * otherwise to promote the sale, use or other dealings in this
 * Software without prior written authorization from the
 * authors.
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *    Keith Packard <keithp@keithp.com>
 */

#include "config.h"

#include <stdlib.h>
#include <stdint.h>

#include "hash.h"

struct hash_entry {
	uint32_t hash;
	void *data;
};

struct hash_table {
	struct hash_entry *table;
	uint32_t size;
	uint32_t rehash;
	uint32_t max_entries;
	uint32_t size_index;
	uint32_t entries;
	uint32_t deleted_entries;
};

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

/*
 * From Knuth -- a good choice for hash/rehash values is p, p-2 where
 * p and p-2 are both prime.  These tables are sized to have an extra 10%
 * free to avoid exponential performance degradation as the hash table fills
 */

static const uint32_t deleted_data;

static const struct {
   uint32_t max_entries, size, rehash;
} hash_sizes[] = {
    { 2,		5,		3	  },
    { 4,		7,		5	  },
    { 8,		13,		11	  },
    { 16,		19,		17	  },
    { 32,		43,		41        },
    { 64,		73,		71        },
    { 128,		151,		149       },
    { 256,		283,		281       },
    { 512,		571,		569       },
    { 1024,		1153,		1151      },
    { 2048,		2269,		2267      },
    { 4096,		4519,		4517      },
    { 8192,		9013,		9011      },
    { 16384,		18043,		18041     },
    { 32768,		36109,		36107     },
    { 65536,		72091,		72089     },
    { 131072,		144409,		144407    },
    { 262144,		288361,		288359    },
    { 524288,		576883,		576881    },
    { 1048576,		1153459,	1153457   },
    { 2097152,		2307163,	2307161   },
    { 4194304,		4613893,	4613891   },
    { 8388608,		9227641,	9227639   },
    { 16777216,		18455029,	18455027  },
    { 33554432,		36911011,	36911009  },
    { 67108864,		73819861,	73819859  },
    { 134217728,	147639589,	147639587 },
    { 268435456,	295279081,	295279079 },
    { 536870912,	590559793,	590559791 },
    { 1073741824,	1181116273,	1181116271},
    { 2147483648ul,	2362232233ul,	2362232231ul}
};

static int
entry_is_free(struct hash_entry *entry)
{
	return entry->data == NULL;
}

static int
entry_is_deleted(struct hash_entry *entry)
{
	return entry->data == &deleted_data;
}

static int
entry_is_present(struct hash_entry *entry)
{
	return entry->data != NULL && entry->data != &deleted_data;
}

struct hash_table *
hash_table_create(void)
{
	struct hash_table *ht;

	ht = malloc(sizeof(*ht));
	if (ht == NULL)
		return NULL;

	ht->size_index = 0;
	ht->size = hash_sizes[ht->size_index].size;
	ht->rehash = hash_sizes[ht->size_index].rehash;
	ht->max_entries = hash_sizes[ht->size_index].max_entries;
	ht->table = calloc(ht->size, sizeof(*ht->table));
	ht->entries = 0;
	ht->deleted_entries = 0;

	if (ht->table == NULL) {
		free(ht);
		return NULL;
	}

	return ht;
}

/**
 * Frees the given hash table.
 */
void
hash_table_destroy(struct hash_table *ht)
{
	if (!ht)
		return;

	free(ht->table);
	free(ht);
}

/**
 * Finds a hash table entry with the given key and hash of that key.
 *
 * Returns NULL if no entry is found.  Note that the data pointer may be
 * modified by the user.
 */
static void *
hash_table_search(struct hash_table *ht, uint32_t hash)
{
	uint32_t hash_address;

	hash_address = hash % ht->size;
	do {
		uint32_t double_hash;

		struct hash_entry *entry = ht->table + hash_address;

		if (entry_is_free(entry)) {
			return NULL;
		} else if (entry_is_present(entry) && entry->hash == hash) {
			return entry;
		}

		double_hash = 1 + hash % ht->rehash;

		hash_address = (hash_address + double_hash) % ht->size;
	} while (hash_address != hash % ht->size);

	return NULL;
}

void
hash_table_for_each(struct hash_table *ht,
		    hash_table_iterator_func_t func, void *data)
{
	struct hash_entry *entry;
	uint32_t i;

	for (i = 0; i < ht->size; i++) {
		entry = ht->table + i;
		if (entry_is_present(entry))
			func(entry->data, data);
	}
}

void *
hash_table_lookup(struct hash_table *ht, uint32_t hash)
{
	struct hash_entry *entry;

	entry = hash_table_search(ht, hash);
	if (entry != NULL)
		return entry->data;

	return NULL;
}

static void
hash_table_rehash(struct hash_table *ht, unsigned int new_size_index)
{
	struct hash_table old_ht;
	struct hash_entry *table, *entry;

	if (new_size_index >= ARRAY_SIZE(hash_sizes))
		return;

	table = calloc(hash_sizes[new_size_index].size, sizeof(*ht->table));
	if (table == NULL)
		return;

	old_ht = *ht;

	ht->table = table;
	ht->size_index = new_size_index;
	ht->size = hash_sizes[ht->size_index].size;
	ht->rehash = hash_sizes[ht->size_index].rehash;
	ht->max_entries = hash_sizes[ht->size_index].max_entries;
	ht->entries = 0;
	ht->deleted_entries = 0;

	for (entry = old_ht.table;
	     entry != old_ht.table + old_ht.size;
	     entry++) {
		if (entry_is_present(entry)) {
			hash_table_insert(ht, entry->hash, entry->data);
		}
	}

	free(old_ht.table);
}

/**
 * Inserts the data with the given hash into the table.
 *
 * Note that insertion may rearrange the table on a resize or rehash,
 * so previously found hash_entries are no longer valid after this function.
 */
int
hash_table_insert(struct hash_table *ht, uint32_t hash, void *data)
{
	uint32_t hash_address;

	if (ht->entries >= ht->max_entries) {
		hash_table_rehash(ht, ht->size_index + 1);
	} else if (ht->deleted_entries + ht->entries >= ht->max_entries) {
		hash_table_rehash(ht, ht->size_index);
	}

	hash_address = hash % ht->size;
	do {
		struct hash_entry *entry = ht->table + hash_address;
		uint32_t double_hash;

		if (!entry_is_present(entry)) {
			if (entry_is_deleted(entry))
				ht->deleted_entries--;
			entry->hash = hash;
			entry->data = data;
			ht->entries++;
			return 0;
		}

		double_hash = 1 + hash % ht->rehash;

		hash_address = (hash_address + double_hash) % ht->size;
	} while (hash_address != hash % ht->size);

	/* We could hit here if a required resize failed. An unchecked-malloc
	 * application could ignore this result.
	 */
	return -1;
}

/**
 * This function deletes the given hash table entry.
 *
 * Note that deletion doesn't otherwise modify the table, so an iteration over
 * the table deleting entries is safe.
 */
void
hash_table_remove(struct hash_table *ht, uint32_t hash)
{
	struct hash_entry *entry;

	entry = hash_table_search(ht, hash);
	if (entry != NULL) {
		entry->data = (void *) &deleted_data;
		ht->entries--;
		ht->deleted_entries++;
	}
}
