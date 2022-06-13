#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <wayland-server-core.h>
#include <wlr/interfaces/wlr_output.h>
#include <wlr/types/wlr_ctm_v1.h>
#include <wlr/types/wlr_output.h>
#include <wlr/util/log.h>
#include "util/signal.h"
#include "wlr-ctm-unstable-v1-protocol.h"

#define CTM_MANAGER_V1_VERSION 1

static void ctm_control_handle_destroy(struct wl_client *client,
		struct wl_resource *resource) {
	wl_resource_destroy(resource);
}

static void ctm_control_destroy(struct wlr_ctm_control_v1 *control) {
	if (control == NULL) {
		return;
	}

	// set up application here
	//wlr_output_set_gamma(control->output, 0, NULL, NULL, NULL);

	// will be applied on next output commit
	wlr_output_schedule_frame(control->output);

	wl_resource_set_user_data(control->resource, NULL);
	wl_list_remove(&control->output_destroy_listener.link);
	wl_list_remove(&control->output_commit_listener.link);
	wl_list_remove(&control->link);
	free(control->table);
	free(control);
}

/* static void ctm_control_send_failed(struct wlr_ctm_control_v1 *control) { */
/* 	zwlr_ctm_control_v1_send_failed(control->resource); */
/* 	ctm_control_destroy(control); */
/* } */

static void ctm_control_apply(struct wlr_ctm_control_v1 *control) {

	/* uint16_t *r = control->table; */
	/* uint16_t *g = control->table + control->ramp_size; */
	/* uint16_t *b = control->table + 2 * control->ramp_size; */

	/* wlr_output_set_gamma(control->output, control->ramp_size, r, g, b); */

	/* if (!wlr_output_test(control->output)) { */
	/* 	wlr_output_rollback(control->output); */
	/* 	ctm_control_send_failed(control); */
	/* 	return; */
	/* } */

	// Gamma LUT will be applied on next output commit
	wlr_output_schedule_frame(control->output);
}


/* ------------------------------------------------------------------  */

static const struct zwlr_ctm_control_v1_interface ctm_control_impl;

static struct wlr_ctm_control_v1 *ctm_control_from_resource(struct wl_resource *resource) {
	assert(wl_resource_instance_of(resource, &zwlr_ctm_control_v1_interface, &ctm_control_impl));
	return wl_resource_get_user_data(resource);
}

static void ctm_control_handle_resource_destroy(struct wl_resource *resource) {
	struct wlr_ctm_control_v1 *control = ctm_control_from_resource(resource);
	ctm_control_destroy(control);
}

static void ctm_control_handle_output_destroy(struct wl_listener *listener, void *data) {
	struct wlr_ctm_control_v1 *control =
		wl_container_of(listener, control, output_destroy_listener);
	ctm_control_destroy(control);
}

static void ctm_control_handle_output_commit(struct wl_listener *listener, void *data) {
	struct wlr_ctm_control_v1 *control =
		wl_container_of(listener, control, output_commit_listener);
	struct wlr_output_event_commit *event = data;
	if ((event->committed & WLR_OUTPUT_STATE_ENABLED) && control->output->enabled) {
		ctm_control_apply(control);
	}
}

static void ctm_control_handle_set_ctm(struct wl_client *client,
		struct wl_resource *resource, int fd) {

	struct wlr_ctm_control_v1 *control = ctm_control_from_resource(resource);
	if (control == NULL) {
		goto error_fd;
	}

	fprintf(stderr, "RECEIVED CTM!");

	/* uint32_t ramp_size = wlr_output_get_gamma_size(control->output); */
	/* size_t table_size = ramp_size * 3 * sizeof(uint16_t); */

	/* // Refuse to block when reading */
	/* int fd_flags = fcntl(fd, F_GETFL, 0); */
	/* if (fd_flags == -1) { */
	/* 	wlr_log_errno(WLR_ERROR, "failed to get FD flags"); */
	/* 	ctm_control_send_failed(control); */
	/* 	goto error_fd; */
	/* } */
	/* if (fcntl(fd, F_SETFL, fd_flags | O_NONBLOCK) == -1) { */
	/* 	wlr_log_errno(WLR_ERROR, "failed to set FD flags"); */
	/* 	ctm_control_send_failed(control); */
	/* 	goto error_fd; */
	/* } */

	/* // Use the heap since gamma tables can be large */
	/* uint16_t *table = malloc(table_size); */
	/* if (table == NULL) { */
	/* 	wl_resource_post_no_memory(resource); */
	/* 	goto error_fd; */
	/* } */

	/* ssize_t n_read = read(fd, table, table_size); */
	/* if (n_read < 0) { */
	/* 	wlr_log_errno(WLR_ERROR, "failed to read gamma table"); */
	/* 	ctm_control_send_failed(control); */
	/* 	goto error_table; */
	/* } else if ((size_t)n_read != table_size) { */
	/* 	wl_resource_post_error(resource, */
	/* 		ZWLR_CTM_CONTROL_V1_ERROR_INVALID_CTM, */
	/* 		"The gamma ramps don't have the correct size"); */
	/* 	goto error_table; */
	/* } */
	/* close(fd); */
	/* fd = -1; */

	/* free(control->table); */
	/* control->table = table; */
	/* control->ramp_size = ramp_size; */

	/* if (control->output->enabled) { */
	/* 	ctm_control_apply(control); */
	/* } */

	return;

/* error_table: */
/* 	free(table); */
error_fd:
/* 	close(fd); */
}

static const struct zwlr_ctm_control_v1_interface ctm_control_impl = {
	.destroy = ctm_control_handle_destroy,
	.set_ctm = ctm_control_handle_set_ctm,
};


/* ------------------------------------------------------------------  */

static const struct zwlr_ctm_manager_v1_interface ctm_manager_impl;

static struct wlr_ctm_manager_v1 *ctm_manager_from_resource(struct wl_resource *resource) {
	assert(wl_resource_instance_of(resource,&zwlr_ctm_manager_v1_interface, &ctm_manager_impl));
	return wl_resource_get_user_data(resource);
}

static void ctm_manager_get_control(struct wl_client *client,
		struct wl_resource *manager_resource, uint32_t id,
		struct wl_resource *output_resource) {

	struct wlr_ctm_manager_v1 *manager =
		ctm_manager_from_resource(manager_resource);
	struct wlr_output *output = wlr_output_from_resource(output_resource);

	struct wlr_ctm_control_v1 *control =
		calloc(1, sizeof(struct wlr_ctm_control_v1));
	if (control == NULL) {
		wl_client_post_no_memory(client);
		return;
	}
	control->output = output;

	uint32_t version = wl_resource_get_version(manager_resource);
	control->resource = wl_resource_create(client,
		&zwlr_ctm_control_v1_interface, version, id);
	if (control->resource == NULL) { 
		free(control);
		wl_client_post_no_memory(client);
		return;
	}
	wl_resource_set_implementation(control->resource, &ctm_control_impl,
		control, ctm_control_handle_resource_destroy);

	if (output == NULL) {
		wl_resource_set_user_data(control->resource, NULL);
		zwlr_ctm_control_v1_send_failed(control->resource);
		free(control);
		return;
	}

	wl_signal_add(&output->events.destroy, &control->output_destroy_listener);
	control->output_destroy_listener.notify = ctm_control_handle_output_destroy;

	wl_signal_add(&output->events.commit, &control->output_commit_listener);
	control->output_commit_listener.notify = ctm_control_handle_output_commit;

	wl_list_init(&control->link);

	/* size_t gamma_size = wlr_output_get_gamma_size(output); */
	/* if (gamma_size == 0) { */
	/* 	zwlr_gamma_control_v1_send_failed(gamma_control->resource); */
	/* 	gamma_control_destroy(gamma_control); */
	/* 	return; */
	/* } */

	struct wlr_ctm_control_v1 *gc;
	wl_list_for_each(gc, &manager->controls, link) {
		if (gc->output == output) {
			zwlr_ctm_control_v1_send_failed(gc->resource);
			ctm_control_destroy(gc);
			return;
		}
	}

	wl_list_remove(&control->link);
	wl_list_insert(&manager->controls, &control->link);
	/* zwlr_ctm_control_v1_send_ctm_size(control->resource, gamma_size); */
}

static void ctm_manager_destroy(struct wl_client *client,
		struct wl_resource *manager_resource) {
	wl_resource_destroy(manager_resource);
}

static const struct zwlr_ctm_manager_v1_interface ctm_manager_impl = {
	.get_ctm_control = ctm_manager_get_control,
	.destroy = ctm_manager_destroy,
};


/* ------------------------------------------------------------------  */

static void ctm_manager_bind(struct wl_client *client, void *data,
		uint32_t version, uint32_t id) {
	struct wlr_ctm_manager_v1 *manager = data;

	struct wl_resource *resource = wl_resource_create(client,
		&zwlr_ctm_manager_v1_interface, version, id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return;
	}
	wl_resource_set_implementation(resource, &ctm_manager_impl,
		manager, NULL);
}

static void handle_display_destroy(struct wl_listener *listener, void *data) {
	struct wlr_ctm_manager_v1 *manager =
		wl_container_of(listener, manager, display_destroy);
	wlr_signal_emit_safe(&manager->events.destroy, manager);
	wl_list_remove(&manager->display_destroy.link);
	wl_global_destroy(manager->global);
	free(manager);
}

struct wlr_ctm_manager_v1 *wlr_ctm_manager_v1_create(struct wl_display *display) {

	struct wlr_ctm_manager_v1 *manager =
		calloc(1, sizeof(struct wlr_ctm_manager_v1));
	if (!manager) {
		return NULL;
	}

	manager->global = wl_global_create(display,
		&zwlr_ctm_manager_v1_interface,
		CTM_MANAGER_V1_VERSION, manager, ctm_manager_bind);
	if (manager->global == NULL) {
		free(manager);
		return NULL;
	}

	wl_signal_init(&manager->events.destroy);
	wl_list_init(&manager->controls);

	manager->display_destroy.notify = handle_display_destroy;
	wl_display_add_destroy_listener(display, &manager->display_destroy);

	return manager;
}
