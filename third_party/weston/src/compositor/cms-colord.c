/*
 * Copyright © 2013 Richard Hughes
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

#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <colord.h>

#include <libweston/libweston.h>
#include "weston.h"
#include "cms-helper.h"
#include "shared/helpers.h"

struct cms_colord {
	struct weston_compositor	*ec;
	CdClient			*client;
	GHashTable			*devices; /* key = device-id, value = cms_output */
	GHashTable			*pnp_ids; /* key = pnp-id, value = vendor */
	gchar				*pnp_ids_data;
	GMainLoop			*loop;
	GThread				*thread;
	GList				*pending;
	GMutex				 pending_mutex;
	struct wl_event_source		*source;
	int				 readfd;
	int				 writefd;
	struct wl_listener		 destroy_listener;
	struct wl_listener		 output_created_listener;
};

struct cms_output {
	CdDevice			*device;
	guint32				 backlight_value;
	struct cms_colord		*cms;
	struct weston_color_profile	*p;
	struct weston_output		*o;
	struct wl_listener		 destroy_listener;
};

static gint
colord_idle_find_output_cb(gconstpointer a, gconstpointer b)
{
	struct cms_output *ocms = (struct cms_output *) a;
	struct weston_output *o = (struct weston_output *) b;
	return ocms->o == o ? 0 : -1;
}

static void
colord_idle_cancel_for_output(struct cms_colord *cms, struct weston_output *o)
{
	GList *l;

	/* cancel and remove any helpers that match the output */
	g_mutex_lock(&cms->pending_mutex);
	l = g_list_find_custom (cms->pending, o, colord_idle_find_output_cb);
	if (l) {
		struct cms_output *ocms = l->data;
		cms->pending = g_list_remove (cms->pending, ocms);
	}
	g_mutex_unlock(&cms->pending_mutex);
}

static bool
edid_value_valid(const char *str)
{
	if (str == NULL)
		return false;
	if (str[0] == '\0')
		return false;
	if (strcmp(str, "unknown") == 0)
		return false;
	return true;
}

static gchar *
get_output_id(struct cms_colord *cms, struct weston_output *o)
{
	struct weston_head *head;
	const gchar *tmp;
	GString *device_id;

	/* XXX: What to do with multiple heads?
	 * This is potentially unstable, if head configuration is changed
	 * while the output is enabled. */
	head = weston_output_get_first_head(o);

	if (wl_list_length(&o->head_list) > 1) {
		weston_log("colord: WARNING: multiple heads are not supported (output %s).\n",
			   o->name);
	}

	/* see https://github.com/hughsie/colord/blob/master/doc/device-and-profile-naming-spec.txt
	 * for format and allowed values */
	device_id = g_string_new("xrandr");
	if (edid_value_valid(head->make)) {
		tmp = g_hash_table_lookup(cms->pnp_ids, head->make);
		if (tmp == NULL)
			tmp = head->make;
		g_string_append_printf(device_id, "-%s", tmp);
	}
	if (edid_value_valid(head->model))
		g_string_append_printf(device_id, "-%s", head->model);
	if (edid_value_valid(head->serial_number))
		g_string_append_printf(device_id, "-%s", head->serial_number);

	/* no EDID data, so use fallback */
	if (strcmp(device_id->str, "xrandr") == 0)
		g_string_append_printf(device_id, "-drm-%i", o->id);

	return g_string_free(device_id, FALSE);
}

static void
update_device_with_profile_in_idle(struct cms_output *ocms)
{
	gboolean signal_write = FALSE;
	ssize_t rc;
	struct cms_colord *cms = ocms->cms;

	colord_idle_cancel_for_output(cms, ocms->o);
	g_mutex_lock(&cms->pending_mutex);
	if (cms->pending == NULL)
		signal_write = TRUE;
	cms->pending = g_list_prepend(cms->pending, ocms);
	g_mutex_unlock(&cms->pending_mutex);

	/* signal we've got updates to do */
	if (signal_write) {
		gchar tmp = '\0';
		rc = write(cms->writefd, &tmp, 1);
		if (rc == 0)
			weston_log("colord: failed to write to pending fd\n");
	}
}

static void
colord_update_output_from_device (struct cms_output *ocms)
{
	CdProfile *profile;
	const gchar *tmp;
	gboolean ret;
	GError *error = NULL;
	gint percentage;

	/* old profile is no longer valid */
	weston_cms_destroy_profile(ocms->p);
	ocms->p = NULL;

	ret = cd_device_connect_sync(ocms->device, NULL, &error);
	if (!ret) {
		weston_log("colord: failed to connect to device %s: %s\n",
			   cd_device_get_object_path (ocms->device),
			   error->message);
		g_error_free(error);
		goto out;
	}
	profile = cd_device_get_default_profile(ocms->device);
	if (!profile) {
		weston_log("colord: no assigned color profile for %s\n",
			   cd_device_get_id (ocms->device));
		goto out;
	}
	ret = cd_profile_connect_sync(profile, NULL, &error);
	if (!ret) {
		weston_log("colord: failed to connect to profile %s: %s\n",
			   cd_profile_get_object_path (profile),
			   error->message);
		g_error_free(error);
		goto out;
	}

	/* get the calibration brightness level (only set for some profiles) */
	tmp = cd_profile_get_metadata_item(profile, CD_PROFILE_METADATA_SCREEN_BRIGHTNESS);
	if (tmp != NULL) {
		percentage = atoi(tmp);
		if (percentage > 0 && percentage <= 100)
			ocms->backlight_value = percentage * 255 / 100;
	}

	ocms->p = weston_cms_load_profile(cd_profile_get_filename(profile));
	if (ocms->p == NULL) {
		weston_log("colord: warning failed to load profile %s: %s\n",
			   cd_profile_get_object_path (profile),
			   error->message);
		g_error_free(error);
		goto out;
	}
out:
	update_device_with_profile_in_idle(ocms);
}

static void
colord_device_changed_cb(CdDevice *device, struct cms_output *ocms)
{
	weston_log("colord: device %s changed, update output\n",
		   cd_device_get_object_path (ocms->device));
	colord_update_output_from_device(ocms);
}

static void
colord_notifier_output_destroy(struct wl_listener *listener, void *data)
{
	struct cms_output *ocms =
		container_of(listener, struct cms_output, destroy_listener);
	struct weston_output *o = (struct weston_output *) data;
	struct cms_colord *cms = ocms->cms;
	gchar *device_id;

	device_id = get_output_id(cms, o);
	g_hash_table_remove (cms->devices, device_id);
	g_free (device_id);
}

static void
colord_output_created(struct cms_colord *cms, struct weston_output *o)
{
	struct weston_head *head;
	CdDevice *device;
	const gchar *tmp;
	gchar *device_id;
	GError *error = NULL;
	GHashTable *device_props;
	struct cms_output *ocms;

	/* XXX: What to do with multiple heads? */
	head = weston_output_get_first_head(o);

	/* create device */
	device_id = get_output_id(cms, o);
	weston_log("colord: output added %s\n", device_id);
	device_props = g_hash_table_new_full(g_str_hash, g_str_equal,
					     g_free, g_free);
	g_hash_table_insert (device_props,
			     g_strdup(CD_DEVICE_PROPERTY_KIND),
			     g_strdup(cd_device_kind_to_string (CD_DEVICE_KIND_DISPLAY)));
	g_hash_table_insert (device_props,
			     g_strdup(CD_DEVICE_PROPERTY_FORMAT),
			     g_strdup("ColorModel.OutputMode.OutputResolution"));
	g_hash_table_insert (device_props,
			     g_strdup(CD_DEVICE_PROPERTY_COLORSPACE),
			     g_strdup(cd_colorspace_to_string(CD_COLORSPACE_RGB)));
	if (edid_value_valid(head->make)) {
		tmp = g_hash_table_lookup(cms->pnp_ids, head->make);
		if (tmp == NULL)
			tmp = head->make;
		g_hash_table_insert (device_props,
				     g_strdup(CD_DEVICE_PROPERTY_VENDOR),
				     g_strdup(tmp));
	}
	if (edid_value_valid(head->model)) {
		g_hash_table_insert (device_props,
				     g_strdup(CD_DEVICE_PROPERTY_MODEL),
				     g_strdup(head->model));
	}
	if (edid_value_valid(head->serial_number)) {
		g_hash_table_insert (device_props,
				     g_strdup(CD_DEVICE_PROPERTY_SERIAL),
				     g_strdup(head->serial_number));
	}
	if (head->connection_internal) {
		g_hash_table_insert (device_props,
				     g_strdup (CD_DEVICE_PROPERTY_EMBEDDED),
				     NULL);
	}
	device = cd_client_create_device_sync(cms->client,
					      device_id,
					      CD_OBJECT_SCOPE_TEMP,
					      device_props,
					      NULL,
					      &error);
	if (g_error_matches (error,
			     CD_CLIENT_ERROR,
			     CD_CLIENT_ERROR_ALREADY_EXISTS)) {
		g_clear_error(&error);
		device = cd_client_find_device_sync (cms->client,
						     device_id,
						     NULL,
						     &error);
	}
	if (!device) {
		weston_log("colord: failed to create new or "
			   "find existing device: %s\n",
			   error->message);
		g_error_free(error);
		goto out;
	}

	/* create object and watch for the output to be destroyed */
	ocms = g_slice_new0(struct cms_output);
	ocms->cms = cms;
	ocms->o = o;
	ocms->device = g_object_ref(device);
	ocms->destroy_listener.notify = colord_notifier_output_destroy;
	wl_signal_add(&o->destroy_signal, &ocms->destroy_listener);

	/* add to local cache */
	g_hash_table_insert (cms->devices, g_strdup(device_id), ocms);
	g_signal_connect (ocms->device, "changed",
			  G_CALLBACK (colord_device_changed_cb), ocms);

	/* get profiles */
	colord_update_output_from_device (ocms);
out:
	g_hash_table_unref (device_props);
	if (device)
		g_object_unref (device);
	g_free (device_id);
}

static void
colord_notifier_output_created(struct wl_listener *listener, void *data)
{
	struct weston_output *o = (struct weston_output *) data;
	struct cms_colord *cms =
		container_of(listener, struct cms_colord, destroy_listener);
	weston_log("colord: output %s created\n", o->name);
	colord_output_created(cms, o);
}

static gpointer
colord_run_loop_thread(gpointer data)
{
	struct cms_colord *cms = (struct cms_colord *) data;
	struct weston_output *o;

	/* coldplug outputs */
	wl_list_for_each(o, &cms->ec->output_list, link) {
		weston_log("colord: output %s coldplugged\n", o->name);
		colord_output_created(cms, o);
	}

	g_main_loop_run(cms->loop);
	return NULL;
}

static int
colord_dispatch_all_pending(int fd, uint32_t mask, void *data)
{
	gchar tmp;
	GList *l;
	ssize_t rc;
	struct cms_colord *cms = data;
	struct cms_output *ocms;

	weston_log("colord: dispatching events\n");
	g_mutex_lock(&cms->pending_mutex);
	for (l = cms->pending; l != NULL; l = l->next) {
		ocms = l->data;

		/* optionally set backlight to calibration value */
		if (ocms->o->set_backlight && ocms->backlight_value != 0) {
			weston_log("colord: profile calibration backlight to %i/255\n",
				   ocms->backlight_value);
			ocms->o->set_backlight(ocms->o, ocms->backlight_value);
		}

		weston_cms_set_color_profile(ocms->o, ocms->p);
	}
	g_list_free (cms->pending);
	cms->pending = NULL;
	g_mutex_unlock(&cms->pending_mutex);

	/* done */
	rc = read(cms->readfd, &tmp, 1);
	if (rc == 0)
		weston_log("colord: failed to read from pending fd\n");
	return 1;
}

static void
colord_load_pnp_ids(struct cms_colord *cms)
{
	gboolean ret = FALSE;
	gchar *tmp;
	GError *error = NULL;
	guint i;
	const gchar *pnp_ids_fn[] = { "/usr/share/hwdata/pnp.ids",
				      "/usr/share/misc/pnp.ids",
				      NULL };

	/* find and load file */
	for (i = 0; pnp_ids_fn[i] != NULL; i++) {
		if (!g_file_test(pnp_ids_fn[i], G_FILE_TEST_EXISTS))
			continue;
		ret = g_file_get_contents(pnp_ids_fn[i],
					  &cms->pnp_ids_data,
					  NULL,
					  &error);
		if (!ret) {
			weston_log("colord: failed to load %s: %s\n",
				   pnp_ids_fn[i], error->message);
			g_error_free(error);
			return;
		}
		break;
	}
	if (!ret) {
		weston_log("colord: no pnp.ids found\n");
		return;
	}

	/* parse fixed offsets into lines */
	tmp = cms->pnp_ids_data;
	for (i = 0; cms->pnp_ids_data[i] != '\0'; i++) {
		if (cms->pnp_ids_data[i] != '\n')
			continue;
		cms->pnp_ids_data[i] = '\0';
		if (tmp[0] && tmp[1] && tmp[2] && tmp[3] == '\t' && tmp[4]) {
			tmp[3] = '\0';
			g_hash_table_insert(cms->pnp_ids, tmp, tmp+4);
			tmp = &cms->pnp_ids_data[i+1];
		}
	}
}

static void
colord_module_destroy(struct cms_colord *cms)
{
	if (cms->loop) {
		g_main_loop_quit(cms->loop);
		g_main_loop_unref(cms->loop);
	}
	if (cms->thread)
		g_thread_join(cms->thread);

	/* cms->devices must be destroyed before other resources, as
	 * the other resources are needed during output cleanup in
	 * cms->devices unref.
	 */
	if (cms->devices)
		g_hash_table_unref(cms->devices);
	if (cms->client)
		g_object_unref(cms->client);
	if (cms->readfd)
		close(cms->readfd);
	if (cms->writefd)
		close(cms->writefd);

	g_free(cms->pnp_ids_data);
	g_hash_table_unref(cms->pnp_ids);

	wl_list_remove(&cms->destroy_listener.link);
	free(cms);
}

static void
colord_notifier_destroy(struct wl_listener *listener, void *data)
{
	struct cms_colord *cms =
		container_of(listener, struct cms_colord, destroy_listener);
	colord_module_destroy(cms);
}

static void
colord_cms_output_destroy(gpointer data)
{
	struct cms_output *ocms = (struct cms_output *) data;
	struct cms_colord *cms = ocms->cms;
	struct weston_output *o = ocms->o;
	gboolean ret;
	gchar *device_id;
	GError *error = NULL;

	colord_idle_cancel_for_output(cms, o);
	device_id = get_output_id(cms, o);
	weston_log("colord: output unplugged %s\n", device_id);

	wl_list_remove(&ocms->destroy_listener.link);
	g_signal_handlers_disconnect_by_data(ocms->device, ocms);

	ret = cd_client_delete_device_sync (cms->client,
					    ocms->device,
					    NULL,
					    &error);

	if (!ret) {
		weston_log("colord: failed to delete device: %s\n",
			   error->message);
		g_error_free(error);
	}

	g_object_unref(ocms->device);
	g_slice_free(struct cms_output, ocms);
	g_free (device_id);
}

WL_EXPORT int
wet_module_init(struct weston_compositor *ec,
		int *argc, char *argv[])
{
	gboolean ret;
	GError *error = NULL;
	int fd[2];
	struct cms_colord *cms;
	struct wl_event_loop *loop;

	weston_log("colord: initialized\n");

	/* create local state object */
	cms = zalloc(sizeof *cms);
	if (cms == NULL)
		return -1;
	cms->ec = ec;

	if (!weston_compositor_add_destroy_listener_once(ec,
							 &cms->destroy_listener,
							 colord_notifier_destroy)) {
		free(cms);
		return 0;
	}

#if !GLIB_CHECK_VERSION(2,36,0)
	g_type_init();
#endif
	cms->client = cd_client_new();
	ret = cd_client_connect_sync(cms->client, NULL, &error);
	if (!ret) {
		weston_log("colord: failed to contact daemon: %s\n", error->message);
		g_error_free(error);
		colord_module_destroy(cms);
		return -1;
	}
	g_mutex_init(&cms->pending_mutex);
	cms->devices = g_hash_table_new_full(g_str_hash, g_str_equal,
					     g_free, colord_cms_output_destroy);

	/* devices added */
	cms->output_created_listener.notify = colord_notifier_output_created;
	wl_signal_add(&ec->output_created_signal, &cms->output_created_listener);

	/* add all the PNP IDs */
	cms->pnp_ids = g_hash_table_new_full(g_str_hash,
					     g_str_equal,
					     NULL,
					     NULL);
	colord_load_pnp_ids(cms);

	/* setup a thread for the GLib callbacks */
	cms->loop = g_main_loop_new(NULL, FALSE);
	cms->thread = g_thread_new("colord CMS main loop",
				   colord_run_loop_thread, cms);

	/* batch device<->profile updates */
	if (pipe2(fd, O_CLOEXEC) == -1) {
		colord_module_destroy(cms);
		return -1;
	}
	cms->readfd = fd[0];
	cms->writefd = fd[1];
	loop = wl_display_get_event_loop(ec->wl_display);
	cms->source = wl_event_loop_add_fd (loop,
					    cms->readfd,
					    WL_EVENT_READABLE,
					    colord_dispatch_all_pending,
					    cms);
	if (!cms->source) {
		colord_module_destroy(cms);
		return -1;
	}
	return 0;
}
