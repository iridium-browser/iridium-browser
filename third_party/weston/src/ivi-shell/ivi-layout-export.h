/*
 * Copyright (C) 2013 DENSO CORPORATION
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

/**
 * The ivi-layout library supports API set of controlling properties of
 * surface and layer which groups surfaces. A unique ID whose type is integer is
 * required to create surface and layer. With the unique ID, surface and layer
 * are identified to control them. The API set consists of APIs to control
 * properties of surface and layers about the following:
 * - visibility.
 * - opacity.
 * - clipping (x,y,width,height).
 * - position and size of it to be displayed.
 * - orientation per 90 degree.
 * - add or remove surfaces to a layer.
 * - order of surfaces/layers in layer/screen to be displayed.
 * - commit to apply property changes.
 * - notifications of property change.
 *
 * Management of surfaces and layers grouping these surfaces are common
 * way in In-Vehicle Infotainment system, which integrate several domains
 * in one system. A layer is allocated to a domain in order to control
 * application surfaces grouped to the layer all together.
 *
 * This API and ABI follow following specifications.
 * https://at.projects.genivi.org/wiki/display/PROJ/Wayland+IVI+Extension+Design
 */

#ifndef _IVI_LAYOUT_EXPORT_H_
#define _IVI_LAYOUT_EXPORT_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include <limits.h>

#include "stdbool.h"
#include <libweston/libweston.h>
#include <libweston/plugin-registry.h>

#define IVI_SUCCEEDED (0)
#define IVI_FAILED (-1)
#define IVI_INVALID_ID UINT_MAX

struct ivi_layout_layer;
struct ivi_layout_screen;
struct ivi_layout_surface;

struct ivi_layout_surface_properties
{
	wl_fixed_t opacity;
	int32_t source_x;
	int32_t source_y;
	int32_t source_width;
	int32_t source_height;
	int32_t start_x;
	int32_t start_y;
	int32_t start_width;
	int32_t start_height;
	int32_t dest_x;
	int32_t dest_y;
	int32_t dest_width;
	int32_t dest_height;
	enum wl_output_transform orientation;
	bool visibility;
	int32_t transition_type;
	uint32_t transition_duration;
	uint32_t event_mask;
};

struct ivi_layout_layer_properties
{
	wl_fixed_t opacity;
	int32_t source_x;
	int32_t source_y;
	int32_t source_width;
	int32_t source_height;
	int32_t dest_x;
	int32_t dest_y;
	int32_t dest_width;
	int32_t dest_height;
	enum wl_output_transform orientation;
	bool visibility;
	int32_t transition_type;
	uint32_t transition_duration;
	double start_alpha;
	double end_alpha;
	uint32_t is_fade_in;
	uint32_t event_mask;
};

enum ivi_layout_notification_mask {
	IVI_NOTIFICATION_NONE        = 0,
	IVI_NOTIFICATION_OPACITY     = (1 << 1),
	IVI_NOTIFICATION_SOURCE_RECT = (1 << 2),
	IVI_NOTIFICATION_DEST_RECT   = (1 << 3),
	IVI_NOTIFICATION_DIMENSION   = (1 << 4),
	IVI_NOTIFICATION_POSITION    = (1 << 5),
	IVI_NOTIFICATION_ORIENTATION = (1 << 6),
	IVI_NOTIFICATION_VISIBILITY  = (1 << 7),
	IVI_NOTIFICATION_PIXELFORMAT = (1 << 8),
	IVI_NOTIFICATION_ADD         = (1 << 9),
	IVI_NOTIFICATION_REMOVE      = (1 << 10),
	IVI_NOTIFICATION_CONFIGURE   = (1 << 11),
	IVI_NOTIFICATION_ALL         = 0xFFFF
};

enum ivi_layout_transition_type{
	IVI_LAYOUT_TRANSITION_NONE,
	IVI_LAYOUT_TRANSITION_VIEW_DEFAULT,
	IVI_LAYOUT_TRANSITION_VIEW_DEST_RECT_ONLY,
	IVI_LAYOUT_TRANSITION_VIEW_FADE_ONLY,
	IVI_LAYOUT_TRANSITION_LAYER_FADE,
	IVI_LAYOUT_TRANSITION_LAYER_MOVE,
	IVI_LAYOUT_TRANSITION_LAYER_VIEW_ORDER,
	IVI_LAYOUT_TRANSITION_VIEW_MOVE_RESIZE,
	IVI_LAYOUT_TRANSITION_VIEW_RESIZE,
	IVI_LAYOUT_TRANSITION_VIEW_FADE,
	IVI_LAYOUT_TRANSITION_MAX,
};

#define IVI_LAYOUT_API_NAME "ivi_layout_api_v1"

struct ivi_layout_interface {

	/**
	 * \brief Commit all changes and execute all enqueued commands since
	 * last commit.
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*commit_changes)(void);

	/**
	 * surface controller interface
	 */

	/**
	 * \brief add a listener for notification when ivi_surface is created
	 *
	 * When an ivi_surface is created, a signal is emitted
	 * to the listening controller plugins.
	 * The pointer of the created ivi_surface is sent as the void *data argument
	 * to the wl_listener::notify callback function of the listener.
	 */
	int32_t (*add_listener_create_surface)(struct wl_listener *listener);

	/**
	 * \brief add a listener for notification when ivi_surface is removed
	 *
	 * When an ivi_surface is removed, a signal is emitted
	 * to the listening controller plugins.
	 * The pointer of the removed ivi_surface is sent as the void *data argument
	 * to the wl_listener::notify callback function of the listener.
	 */
	int32_t (*add_listener_remove_surface)(struct wl_listener *listener);

	/**
	 * \brief add a listener for notification when ivi_surface is configured
	 *
	 * When an ivi_surface is configured, a signal is emitted
	 * to the listening controller plugins.
	 * The pointer of the configured ivi_surface is sent as the void *data argument
	 * to the wl_listener::notify callback function of the listener.
	 */
	int32_t (*add_listener_configure_surface)(struct wl_listener *listener);

	/**
	 * \brief add a listener for notification when desktop_surface is configured
	 *
	 * When an desktop_surface is configured, a signal is emitted
	 * to the listening controller plugins.
	 * The pointer of the configured desktop_surface is sent as the void *data argument
	 * to the wl_listener::notify callback function of the listener.
	 */
	int32_t (*add_listener_configure_desktop_surface)(struct wl_listener *listener);

	/**
	 * \brief Get all ivi_surfaces which are currently registered and managed
	 * by the services
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*get_surfaces)(int32_t *pLength, struct ivi_layout_surface ***ppArray);

	/**
	 * \brief get id of ivi_surface from ivi_layout_surface
	 *
	 * \return id of ivi_surface
	 */
	uint32_t (*get_id_of_surface)(struct ivi_layout_surface *ivisurf);

	/**
	 * \brief get ivi_layout_surface from id of ivi_surface
	 *
	 * \return (struct ivi_layout_surface *)
	 *              if the method call was successful
	 * \return NULL if the method call was failed
	 */
	struct ivi_layout_surface *
		(*get_surface_from_id)(uint32_t id_surface);

	/**
	 * \brief get ivi_layout_surface_properties from ivisurf
	 *
	 * \return (struct ivi_layout_surface_properties *)
	 *              if the method call was successful
	 * \return NULL if the method call was failed
	 */
	const struct ivi_layout_surface_properties *
		(*get_properties_of_surface)(struct ivi_layout_surface *ivisurf);

	/**
	 * \brief Get all Surfaces which are currently registered to a given
	 * layer and are managed by the services
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*get_surfaces_on_layer)(struct ivi_layout_layer *ivilayer,
					 int32_t *pLength,
					 struct ivi_layout_surface ***ppArray);

	/**
	 * \brief Set the visibility of a ivi_surface.
	 *
	 * If a surface is not visible it will not be rendered.
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*surface_set_visibility)(struct ivi_layout_surface *ivisurf,
					  bool newVisibility);

	/**
	 * \brief Set the opacity of a surface.
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*surface_set_opacity)(struct ivi_layout_surface *ivisurf,
				       wl_fixed_t opacity);

	/**
	 * \brief Set the area of a ivi_surface which should be used for the rendering.
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*surface_set_source_rectangle)(struct ivi_layout_surface *ivisurf,
						int32_t x, int32_t y,
						int32_t width, int32_t height);

	/**
	 * \brief Set the destination area of a ivi_surface within a ivi_layer
	 * for rendering.
	 *
	 * The surface will be scaled to this rectangle for rendering.
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*surface_set_destination_rectangle)(struct ivi_layout_surface *ivisurf,
						     int32_t x, int32_t y,
						     int32_t width, int32_t height);

	/**
	 * \brief add a listener to listen property changes of ivi_surface
	 *
	 * When a property of the ivi_surface is changed, the property_changed
	 * signal is emitted to the listening controller plugins.
	 * The pointer of the ivi_surface is sent as the void *data argument
	 * to the wl_listener::notify callback function of the listener.
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*surface_add_listener)(struct ivi_layout_surface *ivisurf,
					    struct wl_listener *listener);

	/**
	 * \brief get weston_surface of ivi_surface
	 */
	struct weston_surface *
		(*surface_get_weston_surface)(struct ivi_layout_surface *ivisurf);

	/**
	 * \brief set type of transition animation
	 */
	int32_t (*surface_set_transition)(struct ivi_layout_surface *ivisurf,
					  enum ivi_layout_transition_type type,
					  uint32_t duration);

	/**
	 * \brief set duration of transition animation
	 */
	int32_t (*surface_set_transition_duration)(
					struct ivi_layout_surface *ivisurf,
					uint32_t duration);

	/**
	 * \brief set id of ivi_layout_surface
	 */
	int32_t (*surface_set_id)(struct ivi_layout_surface *ivisurf,
				  uint32_t id_surface);

	/**
	 * layer controller interface
	 */

	/**
	 * \brief add a listener for notification when ivi_layer is created
	 *
	 * When an ivi_layer is created, a signal is emitted
	 * to the listening controller plugins.
	 * The pointer of the created ivi_layer is sent as the void *data argument
	 * to the wl_listener::notify callback function of the listener.
	 */
	int32_t (*add_listener_create_layer)(struct wl_listener *listener);

	/**
	 * \brief add a listener for notification when ivi_layer is removed
	 *
	 * When an ivi_layer is removed, a signal is emitted
	 * to the listening controller plugins.
	 * The pointer of the removed ivi_layer is sent as the void *data argument
	 * to the wl_listener::notify callback function of the listener.
	 */
	int32_t (*add_listener_remove_layer)(struct wl_listener *listener);

	/**
	 * \brief Create a ivi_layer which should be managed by the service
	 *
	 * \return (struct ivi_layout_layer *)
	 *              if the method call was successful
	 * \return NULL if the method call was failed
	 */
	struct ivi_layout_layer *
		(*layer_create_with_dimension)(uint32_t id_layer,
					       int32_t width, int32_t height);

	/**
	 * \brief Removes a ivi_layer which is currently managed by the service
	 */
	void (*layer_destroy)(struct ivi_layout_layer *ivilayer);

	/**
	 * \brief Get all ivi_layers which are currently registered and managed
	 * by the services
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*get_layers)(int32_t *pLength, struct ivi_layout_layer ***ppArray);

	/**
	 * \brief get id of ivi_layer from ivi_layout_layer
	 *
	 *
	 * \return id of ivi_layer
	 */
	uint32_t (*get_id_of_layer)(struct ivi_layout_layer *ivilayer);

	/**
	 * \brief get ivi_layout_layer from id of layer
	 *
	 * \return (struct ivi_layout_layer *)
	 *              if the method call was successful
	 * \return NULL if the method call was failed
	 */
	struct ivi_layout_layer * (*get_layer_from_id)(uint32_t id_layer);

	/**
	 * \brief  Get the ivi_layer properties
	 *
	 * \return (const struct ivi_layout_layer_properties *)
	 *              if the method call was successful
	 * \return NULL if the method call was failed
	 */
	const struct ivi_layout_layer_properties *
		(*get_properties_of_layer)(struct ivi_layout_layer *ivilayer);

	/**
	 * \brief Get all ivi-layers under the given ivi-surface
	 *
	 * This means all the ivi-layers the ivi-surface was added to. It has
	 * no relation to geometric overlaps.
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*get_layers_under_surface)(struct ivi_layout_surface *ivisurf,
					    int32_t *pLength,
					    struct ivi_layout_layer ***ppArray);

	/**
	 * \brief Get all Layers of the given weston_output
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*get_layers_on_screen)(struct weston_output *output,
					int32_t *pLength,
					struct ivi_layout_layer ***ppArray);

	/**
	 * \brief Set the visibility of a ivi_layer. If a ivi_layer is not visible,
	 * the ivi_layer and its ivi_surfaces will not be rendered.
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*layer_set_visibility)(struct ivi_layout_layer *ivilayer,
					bool newVisibility);

	/**
	 * \brief Set the opacity of a ivi_layer.
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*layer_set_opacity)(struct ivi_layout_layer *ivilayer,
				     wl_fixed_t opacity);

	/**
	 * \brief Set the area of a ivi_layer which should be used for the rendering.
	 *
	 * Only this part will be visible.
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*layer_set_source_rectangle)(struct ivi_layout_layer *ivilayer,
					      int32_t x, int32_t y,
					      int32_t width, int32_t height);

	/**
	 * \brief Set the destination area on the display for a ivi_layer.
	 *
	 * The ivi_layer will be scaled and positioned to this rectangle
	 * for rendering
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*layer_set_destination_rectangle)(struct ivi_layout_layer *ivilayer,
						   int32_t x, int32_t y,
						   int32_t width, int32_t height);

	/**
	 * \brief Add a ivi_surface to a ivi_layer which is currently managed by the service
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*layer_add_surface)(struct ivi_layout_layer *ivilayer,
				     struct ivi_layout_surface *addsurf);

	/**
	 * \brief Removes a surface from a layer which is currently managed by the service
	 */
	void (*layer_remove_surface)(struct ivi_layout_layer *ivilayer,
				     struct ivi_layout_surface *remsurf);

	/**
	 * \brief Sets render order of ivi_surfaces within a ivi_layer
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*layer_set_render_order)(struct ivi_layout_layer *ivilayer,
					  struct ivi_layout_surface **pSurface,
					  int32_t number);

	/**
	 * \brief add a listener to listen property changes of ivi_layer
	 *
	 *	When a property of the ivi_layer is changed, the property_changed
	 * signal is emitted to the listening controller plugins.
	 * The pointer of the ivi_layer is sent as the void *data argument
	 * to the wl_listener::notify callback function of the listener.
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*layer_add_listener)(struct ivi_layout_layer *ivilayer,
					  struct wl_listener *listener);

	/**
	 * \brief set type of transition animation
	 */
	int32_t (*layer_set_transition)(struct ivi_layout_layer *ivilayer,
					enum ivi_layout_transition_type type,
					uint32_t duration);

	/**
	 * screen controller interface
	 */

	/**
	 * \brief Get the weston_outputs under the given ivi_layer
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*get_screens_under_layer)(struct ivi_layout_layer *ivilayer,
					   int32_t *pLength,
					   struct weston_output ***ppArray);

	/**
	 * \brief Add a ivi_layer to a weston_output which is currently managed
	 * by the service
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*screen_add_layer)(struct weston_output *output,
				    struct ivi_layout_layer *addlayer);

	/**
	 * \brief Sets render order of ivi_layers on a weston_output
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*screen_set_render_order)(struct weston_output *output,
					   struct ivi_layout_layer **pLayer,
					   const int32_t number);

	/**
	 * transition animation for layer
	 */
	void (*transition_move_layer_cancel)(struct ivi_layout_layer *layer);
	int32_t (*layer_set_fade_info)(struct ivi_layout_layer* ivilayer,
				       uint32_t is_fade_in,
				       double start_alpha, double end_alpha);

	/**
	 * surface content dumping for debugging
	 */
	int32_t (*surface_get_size)(struct ivi_layout_surface *ivisurf,
				    int32_t *width, int32_t *height,
				    int32_t *stride);

	int32_t (*surface_dump)(struct weston_surface *surface,
				void *target, size_t size,
				int32_t x, int32_t y,
				int32_t width, int32_t height);

	/**
	 * Returns the ivi_layout_surface or NULL
	 *
	 * NULL is returned if there is no ivi_layout_surface corresponding
	 * to the given weston_surface.
	 */
	struct ivi_layout_surface *
		(*get_surface)(struct weston_surface *surface);

	/**
	 * \brief Remove a ivi_layer to a weston_output which is currently managed
	 * by the service
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*screen_remove_layer)(struct weston_output *output,
				       struct ivi_layout_layer *removelayer);
};

static inline const struct ivi_layout_interface *
ivi_layout_get_api(struct weston_compositor *compositor)
{
	const void *api;
	api = weston_plugin_api_get(compositor, IVI_LAYOUT_API_NAME,
				    sizeof(struct ivi_layout_interface));

	return (const struct ivi_layout_interface *)api;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _IVI_LAYOUT_EXPORT_H_ */
