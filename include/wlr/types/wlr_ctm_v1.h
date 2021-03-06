#ifndef WLR_TYPES_WLR_CTM_V1_H
#define WLR_TYPES_WLR_CTM_V1_H

#include <wayland-server-core.h>

struct wlr_ctm_manager_v1 {
	struct wl_global *global;
	struct wl_list controls; // wlr_gamma_control_v1.link

	struct wl_listener display_destroy;

	struct {
		struct wl_signal destroy;
	} events;

	void *data;
};

struct wlr_ctm_control_v1 {
	struct wl_resource *resource;
	struct wlr_output *output;
	struct wl_list link;

	uint32_t ctm[18];

	struct wl_listener output_commit_listener;
	struct wl_listener output_destroy_listener;

	void *data;
};

struct wlr_ctm_manager_v1 *wlr_ctm_manager_v1_create(
	struct wl_display *display);

#endif
