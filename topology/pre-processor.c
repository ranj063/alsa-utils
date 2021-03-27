/*
  Copyright(c) 2021 Intel Corporation
  All rights reserved.

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
  The full GNU General Public License is included in this distribution
  in the file called LICENSE.GPL.
*/

#include <stdarg.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>

#include <alsa/input.h>
#include <alsa/output.h>
#include <alsa/conf.h>
#include "gettext.h"
#include "topology.h"
#include "pre-processor.h"

#ifdef TPLG_DEBUG
void tplg_pp_debug(char *fmt, ...)
{
	char msg[DEBUG_MAX_LENGTH];
	va_list va;

	va_start(va, fmt);
	vsnprintf(msg, DEBUG_MAX_LENGTH, fmt, va);
	va_end(va);

	fprintf(stdout, "%s\n", msg);
}

void print_pre_processed_config(struct tplg_pre_processor *tplg_pp)
{
	int i;

	for (i = tplg_pp->dbg_print_len; i < tplg_pp->buf.dst_len; i++)
		fprintf(stdout, "%c", tplg_pp->buf.dst[i]);

	tplg_pp->dbg_print_len = tplg_pp->buf.dst_len;
}
#else
void tplg_pp_debug(char *fmt, ...) {}
void print_pre_processed_config(struct tplg_pre_processor *tplg_pp) {}
#endif


/*
 * Parse compound config nodes
 */
static int pre_process_compound(struct tplg_pre_processor *tplg_pp, snd_config_t *cfg,
				int (*fcn)(struct tplg_pre_processor *, snd_config_t *))
{
	const char *id;
	snd_config_iterator_t i, next;
	snd_config_t *n;
	int err = -EINVAL;

	if (snd_config_get_id(cfg, &id) < 0)
		return -EINVAL;

	if (snd_config_get_type(cfg) != SND_CONFIG_TYPE_COMPOUND) {
		fprintf(stderr, "compound type expected for %s", id);
		return -EINVAL;
	}

	/* parse compound */
	snd_config_for_each(i, next, cfg) {
		n = snd_config_iterator_entry(i);

		if (snd_config_get_type(cfg) != SND_CONFIG_TYPE_COMPOUND) {
			fprintf(stderr, _("compound type expected for %s, is %d"),
				id, snd_config_get_type(cfg));
			return -EINVAL;
		}

		err = fcn(tplg_pp, n);
		if (err < 0)
			return err;
	}

	return err;
}

static int pre_process_config(struct tplg_pre_processor *tplg_pp, snd_config_t *cfg)
{
	int (*parser)(struct tplg_pre_processor *tplg_pp, snd_config_t *cfg);
	snd_config_iterator_t i, next;
	snd_config_t *n;
	const char *id;
	int err;

	if (snd_config_get_type(cfg) != SND_CONFIG_TYPE_COMPOUND) {
		fprintf(stderr, "compound type expected at top level");
		return -EINVAL;
	}

	/* parse topology config sections */
	snd_config_for_each(i, next, cfg) {
		n = snd_config_iterator_entry(i);
		if (snd_config_get_id(n, &id) < 0)
			continue;

		parser = NULL;
		if (!strcmp(id, "Class")) {
			/* TODO: set object parser */
		}

		if (!strcmp(id, "Object")) {
			/* TODO: set object parser */
		}

		if (!parser)
			continue;

		err = pre_process_compound(tplg_pp, n, parser);
		if (err < 0)
			return err;

	}

	return 0;
}

void free_pre_preprocessor(struct tplg_pre_processor *tplg_pp)
{
	snd_tplg_buf_free(&tplg_pp->buf);
	free(tplg_pp);
}

void init_pre_precessor(struct tplg_pre_processor *tplg_pp)
{
	snd_tplg_buf_init(&tplg_pp->buf);
}

int pre_process(struct tplg_pre_processor *tplg_pp, char *config, size_t config_size)
{
	snd_input_t *in;
	snd_config_t *top;
	int err;

	/* create input buffer */
	err = snd_input_buffer_open(&in, config, config_size);
	if (err < 0) {
		fprintf(stderr, "Unable to open input buffer\n");
		return err;
	}

	/* create top-level config node */
	err = snd_config_top(&top);
	if (err < 0)
		goto input_close;

	/* load config */
	err = snd_config_load(top, in);
	if (err < 0) {
		fprintf(stderr, "Unable not load configuration");
		goto err;
	}

	err = pre_process_config(tplg_pp, top);
	if (err < 0)
		fprintf(stderr, "Unable to pre-process configuration");

err:
	snd_config_delete(top);
input_close:
	snd_input_close(in);

	return err;
}
