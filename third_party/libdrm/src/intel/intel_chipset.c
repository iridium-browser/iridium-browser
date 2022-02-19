/*
 * Copyright (C) 2018 Intel Corporation
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
 */
#include "intel_chipset.h"

#include <inttypes.h>
#include <stdbool.h>

#include "i915_pciids.h"

#undef INTEL_VGA_DEVICE
#define INTEL_VGA_DEVICE(id, gen) { id, gen }

static const struct pci_device {
	uint16_t device;
	uint16_t gen;
} pciids[] = {
	/* Keep ids sorted by gen; latest gen first */
	INTEL_TGL_12_IDS(12),
	INTEL_EHL_IDS(11),
	INTEL_ICL_11_IDS(11),
	INTEL_CNL_IDS(10),
	INTEL_CFL_IDS(9),
	INTEL_GLK_IDS(9),
	INTEL_KBL_IDS(9),
	INTEL_BXT_IDS(9),
	INTEL_SKL_IDS(9),
};

drm_private bool intel_is_genx(unsigned int devid, int gen)
{
	const struct pci_device *p,
		  *pend = pciids + sizeof(pciids) / sizeof(pciids[0]);

	for (p = pciids; p < pend; p++) {
		/* PCI IDs are sorted */
		if (p->gen < gen)
			break;

		if (p->device != devid)
			continue;

		if (gen == p->gen)
			return true;

		break;
	}

	return false;
}

drm_private bool intel_get_genx(unsigned int devid, int *gen)
{
	const struct pci_device *p,
		  *pend = pciids + sizeof(pciids) / sizeof(pciids[0]);

	for (p = pciids; p < pend; p++) {
		if (p->device != devid)
			continue;

		if (gen)
			*gen = p->gen;

		return true;
	}

	return false;
}
