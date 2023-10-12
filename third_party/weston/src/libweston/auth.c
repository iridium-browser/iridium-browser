/*
 * Copyright © 2022 Philipp Zabel
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

#include <shared/xalloc.h>
#include <stdbool.h>
#include "libweston-internal.h"

#ifdef HAVE_PAM

#include <security/pam_appl.h>
#include <security/pam_misc.h>

static int
weston_pam_conv(int num_msg, const struct pam_message **msg,
		struct pam_response **resp, void *appdata_ptr)
{
	const char *password = appdata_ptr;
	struct pam_response *rsp;
	int i;

	if (!num_msg)
		return PAM_CONV_ERR;

	rsp = calloc(num_msg, sizeof(*rsp));
	if (!rsp)
		return PAM_CONV_ERR;

	for (i = 0; i < num_msg; i++) {
		switch (msg[i]->msg_style) {
		case PAM_PROMPT_ECHO_OFF:
			rsp[i].resp = strdup(password);
			break;
		case PAM_PROMPT_ECHO_ON:
			break;
		case PAM_ERROR_MSG:
			weston_log("PAM error message: %s\n", msg[i]->msg);
			break;
		case PAM_TEXT_INFO:
			weston_log("PAM info text: %s\n", msg[i]->msg);
			break;
		default:
			free(rsp);
			return PAM_CONV_ERR;
		}
	}

	*resp = rsp;
	return PAM_SUCCESS;
}

#endif

WL_EXPORT bool
weston_authenticate_user(const char *username, const char *password)
{
	bool authenticated = false;
#ifdef HAVE_PAM
	struct pam_conv conv = {
		.conv = weston_pam_conv,
		.appdata_ptr = strdup(password),
	};
	struct pam_handle *pam;
	int ret;

	conv.appdata_ptr = strdup(password);

	ret = pam_start("weston-remote-access", username, &conv, &pam);
	if (ret != PAM_SUCCESS) {
		weston_log("PAM: start failed\n");
		goto out;
	}

	ret = pam_authenticate(pam, 0);
	if (ret != PAM_SUCCESS) {
		weston_log("PAM: authentication failed\n");
		goto out;
	}

	ret = pam_acct_mgmt(pam, 0);
	if (ret != PAM_SUCCESS) {
		weston_log("PAM: account check failed\n");
		goto out;
	}

	authenticated = true;
out:
	ret = pam_end(pam, ret);
	assert(ret == PAM_SUCCESS);
	free(conv.appdata_ptr);
#endif
	return authenticated;
}
