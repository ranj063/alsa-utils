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

#include <alsa/asoundlib.h>
#include "gettext.h"
#include "topology.h"
#include "pre-processor.h"

/*
 * Helper function to find config by id.
 * Topology2.0 object names are constructed with attribute values separated by '.'.
 * So snd_config_search() cannot be used as it interprets the '.' as the node separator.
 */
snd_config_t *tplg_find_config(snd_config_t *config, const char *name)
{
	snd_config_iterator_t i, next;
	snd_config_t *n;
	const char *id;

	snd_config_for_each(i, next, config) {
		n = snd_config_iterator_entry(i);
		if (snd_config_get_id(n, &id) < 0)
			continue;

		if (!strcmp(id, name))
			return n;
	}

	return NULL;
}

/* make a new config and add it to parent */
int tplg_config_make_add(snd_config_t **config, const char *id, snd_config_type_t type,
			 snd_config_t *parent)
{
	int ret;

	ret = snd_config_make(config, id, type);
	if (ret < 0)
		return ret;

	ret = snd_config_add(parent, *config);
	if (ret < 0)
		snd_config_delete(*config);

	return ret;
}

/*
 * The pre-processor will need to concat multiple strings separate by '.' to construct the object
 * name and search for configs with ID's separated by '.'.
 * This function helps concat input strings in the specified input format
 */
char *tplg_snprintf(char *fmt, ...)
{
	char *string;
	int len = 1;

	va_list va;

	va_start(va, fmt);
	len += vsnprintf(NULL, 0, fmt, va);
	va_end(va);

	string = calloc(1, len);
	if (!string)
		return NULL;

	va_start(va, fmt);
	vsnprintf(string, len, fmt, va);
	va_end(va);

	return string;
}

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

void tplg_pp_config_debug(struct tplg_pre_processor *tplg_pp, snd_config_t *cfg)
{
	snd_config_save(cfg, tplg_pp->dbg_output);
}
#else
void tplg_pp_debug(char *fmt, ...) {}
void tplg_pp_config_debug(struct tplg_pre_processor *tplg_pp, snd_config_t *cfg){}
#endif

static int pre_process_config(struct tplg_pre_processor *tplg_pp, snd_config_t *cfg)
{
	snd_config_iterator_t i, next, i2, next2;
	snd_config_t *n, *n2;
	const char *id;
	int err;

	if (snd_config_get_type(cfg) != SND_CONFIG_TYPE_COMPOUND) {
		fprintf(stderr, "compound type expected at top level");
		return -EINVAL;
	}

	/* parse topology objects */
	snd_config_for_each(i, next, cfg) {
		n = snd_config_iterator_entry(i);
		if (snd_config_get_id(n, &id) < 0)
			continue;

		if (strcmp(id, "Object"))
			continue;

		if (snd_config_get_type(n) != SND_CONFIG_TYPE_COMPOUND) {
			fprintf(stderr, "compound type expected for %s", id);
			return -EINVAL;
		}

		snd_config_for_each(i2, next2, n) {
			n2 = snd_config_iterator_entry(i2);

			if (snd_config_get_id(n2, &id) < 0)
				continue;

			if (snd_config_get_type(n2) != SND_CONFIG_TYPE_COMPOUND) {
				fprintf(stderr, "compound type expected for %s", id);
				return -EINVAL;
			}

			/* pre-process Object instance. Top-level object have no parent */
			err = tplg_pre_process_objects(tplg_pp, n2, NULL);
			if (err < 0)
				return err;
		}
	}

	return 0;
}

void free_pre_preprocessor(struct tplg_pre_processor *tplg_pp)
{
	snd_output_close(tplg_pp->output);
	snd_output_close(tplg_pp->dbg_output);
	snd_config_delete(tplg_pp->output_cfg);
	free(tplg_pp->args->names);
	free(tplg_pp->args->values);
	free(tplg_pp->args);
	free(tplg_pp);
}

int init_pre_precessor(struct tplg_pre_processor **tplg_pp, snd_output_type_t type,
		       const char *output_file)
{
	struct tplg_pre_processor *_tplg_pp;
	int ret;

	_tplg_pp = calloc(1, sizeof(struct tplg_pre_processor));
	if (!_tplg_pp)
		return -ENOMEM;

	_tplg_pp->args = calloc(1, sizeof(struct tplg_pre_processor_args));
	if (!_tplg_pp->args) {
		ret = -ENOMEM;
		goto err;
	}

	*tplg_pp = _tplg_pp;

	/* create output top-level config node */
	ret = snd_config_top(&_tplg_pp->output_cfg);
	if (ret < 0)
		goto top_err;

	/* open output based on type */
	if (type == SND_OUTPUT_STDIO) {
		ret = snd_output_stdio_open(&_tplg_pp->output, output_file, "w");
		if (ret < 0) {
			fprintf(stderr, "failed to open file output\n");
			goto open_err;
		}
	} else {
		ret = snd_output_buffer_open(&_tplg_pp->output);
		if (ret < 0) {
			fprintf(stderr, "failed to open buffer output\n");
			goto open_err;
		}
	}

	/* debug output */
	ret = snd_output_stdio_attach(&_tplg_pp->dbg_output, stdout, 0);
	if (ret < 0) {
		fprintf(stderr, "failed to open stdout output\n");
		goto out_close;
	}

	return 0;
out_close:
	snd_output_close(_tplg_pp->output);
open_err:
	snd_config_delete(_tplg_pp->output_cfg);
top_err:
	free(_tplg_pp->args);
err:
	free(_tplg_pp);
	return ret;
}

static int pre_process_args(struct tplg_pre_processor *tplg_pp, const char *pre_processor_defs,
			snd_config_t *top)
{
	snd_config_iterator_t i, next;
	snd_config_t *n, *args;
	int j = tplg_pp->args->count;
	int count = 0;
	int ret;

	ret = snd_config_search(top, "@args", &args);
	if (ret < 0)
		return 0;

	if (snd_config_get_type(args) != SND_CONFIG_TYPE_COMPOUND)
		return 0;

	/* parse topology arguments and get count of number of arguments */
	snd_config_for_each(i, next, args) {
		const char *id;

		n = snd_config_iterator_entry(i);
		if (snd_config_get_id(n, &id) < 0)
			continue;

		count++;
	}

	/* no new arguments */
	if (!count)
		return 0;

	/* allocate memory for arguments */
	if (!tplg_pp->args->count) {
		tplg_pp->args->names = malloc(count * sizeof(*(tplg_pp->args->names)));
		if (!tplg_pp->args->names)
			return -ENOMEM;

		tplg_pp->args->values = malloc(count * sizeof(*(tplg_pp->args->values)));
		if (!tplg_pp->args->values) {
			free(tplg_pp->args->names);
			return -ENOMEM;
		}
	} else {
		size_t size = (tplg_pp->args->count + count) * sizeof(*(tplg_pp->args->names));

		tplg_pp->args->names = realloc(tplg_pp->args->names, size);
		if (!tplg_pp->args->names)
			return -ENOMEM;

		size = (tplg_pp->args->count + count ) * sizeof(*(tplg_pp->args->values));

		tplg_pp->args->values = realloc(tplg_pp->args->values, size);
		if (!tplg_pp->args->values) {
			free(tplg_pp->args->names);
			return -ENOMEM;
		}
	}

	tplg_pp->args->count += count;

	/* parse topology arguments and their default values */
	snd_config_for_each(i, next, args) {
		snd_config_t *type_cfg, *default_cfg;
		const char *type, *default_value;
		char *type_str, *value_str;
		long value;
		const char *id;

		n = snd_config_iterator_entry(i);
		if (snd_config_get_id(n, &id) < 0)
			continue;

		strncpy(tplg_pp->args->names[j], id, TPLG_MAX_ARG_LENGTH);

		ret = snd_config_search(n, "type", &type_cfg);
		if (ret < 0) {
			type_str = tplg_snprintf("%s", "string");
			if (!type_str)
				return -ENOMEM;
		} else {
			snd_config_get_string(type_cfg, &type);
			type_str = tplg_snprintf("%s", type);
			if (!type_str)
				return -ENOMEM;
		}

		ret = snd_config_search(n, "default", &default_cfg);
		if (ret < 0) {
			free(type_str);
			j++;
			continue;
		}

		/* extract the default values based on type */
		if (!strcmp(type, "integer")) {
			snd_config_get_integer(default_cfg, &value);
			value_str = tplg_snprintf("%ld", value);
			if (!value_str) {
				free(type_str);
				return -ENOMEM;
			}
		} else {
			snd_config_get_string(default_cfg, &default_value);
			value_str = tplg_snprintf("%s", default_value);
			if (!value_str) {
				free(type_str);
				return -ENOMEM;
			}
		}

		strncpy(tplg_pp->args->values[j++], value_str, TPLG_MAX_ARG_LENGTH);
		free(value_str);
		free(type_str);
	}

	/* parse the user-defined comma-separated values */
	char *arg;

	if (!pre_processor_defs)
		return 0;

	char *macros = strdup(pre_processor_defs);
	if (!macros)
		return -ENOMEM;

	arg = strtok(macros, ",");

	while (arg != NULL) {
		char *arg_name, *arg_value;

		arg_value = strchr(arg, '=');
		if (!arg_value) {
			fprintf(stderr, "Invalid argument\n");
			free(macros);
			return -EINVAL;
		}

		arg_name = malloc(strlen(arg) - strlen(arg_value) + 1);
		if (!arg_name) {
			free(macros);
			return -ENOMEM;
		}

		snprintf(arg_name, strlen(arg) - strlen(arg_value) + 1, "%s", arg);

		arg_value += 1;

		/* save the parsed values */
		for (j = 0; j < tplg_pp->args->count; j++) {
			if (strcmp(tplg_pp->args->names[j], arg_name))
				continue;

			strncpy(tplg_pp->args->values[j], arg_value, TPLG_MAX_ARG_LENGTH);
		}

		free(arg_name);
		arg = strtok(NULL, ",");
	}

	free(macros);
	return 0;
}

int pre_process(struct tplg_pre_processor *tplg_pp, char *config, size_t config_size,
		const char *pre_processor_defs)
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
		fprintf(stderr, "Unable not load configuration\n");
		goto err;
	}

	/* parse arguments */
	err = pre_process_args(tplg_pp, pre_processor_defs, top);
	if (err < 0) {
		fprintf(stderr, "Failed to parse arguments in input config\n");
		goto err;
	}

	/* expand pre-processor definitions */
	err = snd_config_expand(top, top, pre_processor_defs, NULL, &top);
	if (err < 0) {
		fprintf(stderr, "Failed to expand pre-processor definitions in input config\n");
		goto err;
	}

	tplg_pp->input_cfg = top;

	err = pre_process_config(tplg_pp, top);
	if (err < 0) {
		fprintf(stderr, "Unable to pre-process configuration\n");
		goto err;
	}

	/* save config to output */
	err = snd_config_save(tplg_pp->output_cfg, tplg_pp->output);
	if (err < 0)
		fprintf(stderr, "failed to save pre-processed output file\n");

err:
	snd_config_delete(top);
input_close:
	snd_input_close(in);

	return err;
}
