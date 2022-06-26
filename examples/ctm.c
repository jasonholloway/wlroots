#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include "wlr-ctm-unstable-v1-client-protocol.h"

struct output {
	struct wl_output *wl_output;
	struct zwlr_ctm_control_v1 *control;
	double coeffs[9];
	uint64_t *ctm;
	int ctm_fd;
	struct wl_list link;
};

static struct wl_list outputs;
static struct zwlr_ctm_manager_v1 *manager = NULL;

static int create_anonymous_file(off_t size) {
	char template[] = "/tmp/wlroots-shared-XXXXXX";
	int fd = mkstemp(template);
	if (fd < 0) {
		return -1;
	}

	int ret;
	do {
		errno = 0;
		ret = ftruncate(fd, size);
	} while (errno == EINTR);
	if (ret < 0) {
		close(fd);
		return -1;
	}

	unlink(template);
	return fd;
}

static void fill_coeffs(struct output *o, const double saturation) {
	double base = (1.0 - saturation) / 3.0;
	for (int i = 0; i < 9; i++) {
		/* set coefficients. If index is divisible by four (0, 4, 8) add
			* saturation to coeff
			*/
		o->coeffs[i] = base + (i % 4 == 0 ? saturation : 0);
	}
}

static void fill_ctm(struct output *o) {
	for (int i = 0; i < 9; i++) {
		if (o->coeffs[i] < 0) {
			o->ctm[i] = (int64_t) (-(o->coeffs[i]) * ((uint64_t) 1L << 32u));
			o->ctm[i] |= 1ULL << 63u;
		} else {
			o->ctm[i] = (int64_t) (o->coeffs[i] * ((uint64_t) 1L << 32u));
		}
	}
}


static int output_prepare(struct output *o, const double saturation) {
	int size = 18 * sizeof(uint32_t);

	int fd = create_anonymous_file(size);
	if (fd < 0) {
		fprintf(stderr, "failed to create anonymous file\n");
		return -1;
	}

	void *data = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

	if (data == MAP_FAILED) {
		fprintf(stderr, "failed to mmap()\n");
		close(fd);
		return -1;
	}

	o->ctm = data;
	o->ctm_fd = fd;

	fill_coeffs(o, saturation);
	fill_ctm(o);

	return fd;
}


static void ctm_control_handle_failed(void *data, struct zwlr_ctm_control_v1 *control) {
	fprintf(stderr, "failed to set ctm\n");
	exit(EXIT_FAILURE);
}

static const struct zwlr_ctm_control_v1_listener control_listener = {
	.failed = ctm_control_handle_failed,
};



static void registry_handle_global(void *data, struct wl_registry *registry,
		uint32_t name, const char *interface, uint32_t version) {
	if (strcmp(interface, wl_output_interface.name) == 0) {
		struct output *output = calloc(1, sizeof(struct output));
		output->wl_output = wl_registry_bind(registry, name, &wl_output_interface, 1);
		wl_list_insert(&outputs, &output->link);
	} else if (strcmp(interface, zwlr_ctm_manager_v1_interface.name) == 0) {
		manager = wl_registry_bind(registry, name, &zwlr_ctm_manager_v1_interface, 1);
	}
}

static void registry_handle_global_remove(void *data,
		struct wl_registry *registry, uint32_t name) {
	// Who cares?
}

static const struct wl_registry_listener registry_listener = {
	.global = registry_handle_global,
	.global_remove = registry_handle_global_remove,
};

int main(int argc, char *argv[]) {
	wl_list_init(&outputs);

	fprintf(stderr, "Starting...\n");

	struct wl_display *display = wl_display_connect("wayland-0");
	if (display == NULL) {
		fprintf(stderr, "failed to create display\n");
		return -1;
	}

	struct wl_registry *registry = wl_display_get_registry(display);
	wl_registry_add_listener(registry, &registry_listener, NULL);
	wl_display_roundtrip(display);

	if (manager == NULL) {
		fprintf(stderr,
			"compositor doesn't support wlr-ctm-unstable-v1\n");
		return EXIT_FAILURE;
	}

	struct output *output;
	wl_list_for_each(output, &outputs, link) {
		output->control = zwlr_ctm_manager_v1_get_ctm_control(manager, output->wl_output);
		zwlr_ctm_control_v1_add_listener(output->control, &control_listener, output);
	}
	wl_display_roundtrip(display);

	wl_list_for_each(output, &outputs, link) {
		if(output_prepare(output, 2) < 0) {
			exit(EXIT_FAILURE);
		}

		zwlr_ctm_control_v1_set_ctm(output->control, output->ctm_fd);
	}

	fprintf(stderr, "Dispatching...\n");

	while (wl_display_dispatch(display) != -1) {
		// This space is intentionally left blank
	}

	return EXIT_SUCCESS;
}
