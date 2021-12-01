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
#include <errno.h>
#include <stdio.h>
#include <alsa/asoundlib.h>
#include "topology.h"
#include "pre-processor.h"

int tplg_build_base_object(struct tplg_pre_processor *tplg_pp, snd_config_t *obj_cfg,
			   snd_config_t *parent, bool skip_name)
{
	snd_config_t *top, *parent_obj, *cfg, *dest;
	const char *parent_name;

	/* find parent section config */
	top = tplg_object_get_section(tplg_pp, parent);
	if (!top)
		return -EINVAL;

	parent_obj = tplg_object_get_instance_config(tplg_pp, parent);

	/* get parent name */
	parent_name = tplg_object_get_name(tplg_pp, parent_obj);
	if (!parent_name)
		return 0;

	/* find parent config with name */
	dest = tplg_find_config(top, parent_name);
	if (!dest) {
		SNDERR("Cannot find parent config %s\n", parent_name);
		return -EINVAL;
	}

	/* build config from template and add to parent */
	return tplg_build_object_from_template(tplg_pp, obj_cfg, &cfg, dest, skip_name);
}

int tplg_build_scale_object(struct tplg_pre_processor *tplg_pp, snd_config_t *obj_cfg,
			      snd_config_t *parent)
{
	return tplg_build_base_object(tplg_pp, obj_cfg, parent, true);
}

int tplg_build_ops_object(struct tplg_pre_processor *tplg_pp, snd_config_t *obj_cfg,
			      snd_config_t *parent)
{
	return tplg_build_base_object(tplg_pp, obj_cfg, parent, false);
}

int tplg_build_channel_object(struct tplg_pre_processor *tplg_pp, snd_config_t *obj_cfg,
			      snd_config_t *parent)
{
	return tplg_build_base_object(tplg_pp, obj_cfg, parent, false);
}

int tplg_build_tlv_object(struct tplg_pre_processor *tplg_pp, snd_config_t *obj_cfg,
			      snd_config_t *parent)
{
	snd_config_t *cfg;
	const char *name;
	int ret;

	cfg = tplg_object_get_instance_config(tplg_pp, obj_cfg);

	name = tplg_object_get_name(tplg_pp, cfg);
	if (!name)
		return -EINVAL;

	ret = tplg_build_object_from_template(tplg_pp, obj_cfg, &cfg, NULL, false);
	if (ret < 0)
		return ret;

	return tplg_parent_update(tplg_pp, parent, "tlv", name);
}

static int tplg_build_control(struct tplg_pre_processor *tplg_pp, snd_config_t *obj_cfg,
			      snd_config_t *parent, char *type)
{
	snd_config_t *cfg, *obj;
	const char *name;
	int ret;

	obj = tplg_object_get_instance_config(tplg_pp, obj_cfg);

	/* get control name */
	ret = snd_config_search(obj, "name", &cfg);
	if (ret < 0)
		return 0;

	ret = snd_config_get_string(cfg, &name);
	if (ret < 0)
		return ret;

	ret = tplg_build_object_from_template(tplg_pp, obj_cfg, &cfg, NULL, false);
	if (ret < 0)
		return ret;

	ret = tplg_add_object_data(tplg_pp, obj_cfg, cfg, NULL);
	if (ret < 0)
		SNDERR("Failed to add data section for %s\n", name);

	return tplg_parent_update(tplg_pp, parent, type, name);
}

int tplg_build_mixer_control(struct tplg_pre_processor *tplg_pp, snd_config_t *obj_cfg,
			      snd_config_t *parent)
{
	return tplg_build_control(tplg_pp, obj_cfg, parent, "mixer");
}

int tplg_build_bytes_control(struct tplg_pre_processor *tplg_pp, snd_config_t *obj_cfg,
			      snd_config_t *parent)
{
	return tplg_build_control(tplg_pp, obj_cfg, parent, "bytes");
}

/*
 * Widget names for pipeline endpoints can be of the following type:
 * "class.<constructor args separated by .> ex: pga.0.1, buffer.1.1 etc
 * Optionally, the index argument for a widget can be omitted and will be substituted with
 * the index from the route: ex: pga..0, host..playback etc
 */
static int tplg_pp_get_widget_name(struct tplg_pre_processor *tplg_pp,
				      const char *string, long index, char **widget)
{
	snd_config_iterator_t i, next;
	snd_config_t *temp_cfg, *child, *class_cfg, *n;
	char *class_name, *args, *widget_name;
	int ret;

	/* get class name */
	args = strchr(string, '.');
	if (!args) {
		SNDERR("Error getting class name for %s\n", string);
		return -EINVAL;
	}

	class_name = calloc(1, strlen(string) - strlen(args) + 1);
	if (!class_name)
		return -ENOMEM;

	snprintf(class_name, strlen(string) - strlen(args) + 1, "%s", string);

	/* create config with Widget class type */
	ret = snd_config_make(&temp_cfg, "Widget", SND_CONFIG_TYPE_COMPOUND);
	if (ret < 0) {
		free(class_name);
		return ret;
	}

	/* create config with class name and add it to the Widget config */
	ret = tplg_config_make_add(&child, class_name, SND_CONFIG_TYPE_COMPOUND, temp_cfg);
	if (ret < 0) {
		free(class_name);
		return ret;
	}

	/* get class definition for widget */
	class_cfg = tplg_class_lookup(tplg_pp, temp_cfg);
	snd_config_delete(temp_cfg);
	if (!class_cfg) {
		free(class_name);
		return -EINVAL;
	}

	/* get constructor for class */
	ret = snd_config_search(class_cfg, "attributes.constructor", &temp_cfg);
	if (ret < 0) {
		SNDERR("No arguments in class for widget %s\n", string);
		free(class_name);
		return ret;
	}

	widget_name = strdup(class_name);
	free(class_name);
	if (!widget_name)
		return -ENOMEM;

	/* construct widget name using the constructor argument values */
	snd_config_for_each(i, next, temp_cfg) {
		const char *id;
		char *arg, *remaining, *temp;

		n = snd_config_iterator_entry(i);
		if (snd_config_get_string(n, &id) < 0)
			continue;

		if (!args) {
			SNDERR("insufficient arugments for widget %s\n", string);
			ret = -EINVAL;
			goto err;
		}

		remaining = strchr(args + 1, '.');
		if (remaining) {
			arg = calloc(1, strlen(args + 1) - strlen(remaining) + 1);
			if (!arg) {
				ret = -ENOMEM;
				goto err;
			}
			snprintf(arg, strlen(args + 1) - strlen(remaining) + 1, "%s", args + 1);
		} else {
			arg = calloc(1, strlen(args + 1) + 1);
			if (!arg) {
				ret = -ENOMEM;
				goto err;
			}

			snprintf(arg, strlen(args + 1) + 1, "%s", args + 1);
		}

		/* if no index provided, substitue with route index */
		if (!strcmp(arg, "") && !strcmp(id, "index")) {
			free(arg);
			arg = tplg_snprintf("%ld", index);
			if (!arg) {
				ret = -ENOMEM;
				free(arg);
				goto err;
			}
		}

		temp = tplg_snprintf("%s.%s", widget_name, arg);
		if (!temp) {
			ret = -ENOMEM;
			free(arg);
			goto err;
		}

		free(widget_name);
		widget_name = temp;
		free(arg);
		if (remaining)
			args = remaining;
		else
			args = NULL;
	}

	*widget = widget_name;
	return 0;

err:
	free(widget_name);
	return ret;
}

int tplg_build_dapm_route_object(struct tplg_pre_processor *tplg_pp, snd_config_t *obj_cfg,
			      snd_config_t *parent)
{
	snd_config_t *top, *obj, *cfg, *route, *child, *parent_obj;
	const char *name, *wname;
	const char *parent_name = "Endpoint";
	char *src_widget_name, *sink_widget_name, *line_str, *route_name;
	const char *control = "";
	long index = 0;
	int ret;

	obj = tplg_object_get_instance_config(tplg_pp, obj_cfg);

	ret = snd_config_get_id(obj, &name);
	if (ret < 0)
		return -EINVAL;

	/* endpoint connections at the top-level conf have no parent */
	if (parent) {
		parent_obj = tplg_object_get_instance_config(tplg_pp, parent);

		ret = snd_config_get_id(parent_obj, &parent_name);
		if (ret < 0)
			return -EINVAL;
	}

	tplg_pp_debug("Building DAPM route object: '%s' ...", name);

	ret = snd_config_search(tplg_pp->output_cfg, "SectionGraph", &top);
	if (ret < 0) {
		ret = tplg_config_make_add(&top, "SectionGraph",
					  SND_CONFIG_TYPE_COMPOUND, tplg_pp->output_cfg);
		if (ret < 0) {
			SNDERR("Error creating 'SectionGraph' config\n");
			return ret;
		}
	}

	/* get route index */
	ret = snd_config_search(obj, "index", &cfg);
	if (ret >= 0) {
		ret = snd_config_get_integer(cfg, &index);
		if (ret < 0) {
			SNDERR("Invalid index route %s\n", name);
			return ret;
		}
	}

	/* get source widget name */
	ret = snd_config_search(obj, "source", &cfg);
	if (ret < 0) {
		SNDERR("No source for route %s\n", name);
		return ret;
	}

	ret = snd_config_get_string(cfg, &wname);
	if (ret < 0) {
		SNDERR("Invalid name for source in route %s\n", name);
		return ret;
	}

	ret = tplg_pp_get_widget_name(tplg_pp, wname, index, &src_widget_name);
	if (ret < 0) {
		SNDERR("error getting widget name for %s\n", wname);
		return ret;
	}

	/* get sink widget name */
	ret = snd_config_search(obj, "sink", &cfg);
	if (ret < 0) {
		SNDERR("No sink for route %s\n", name);
		free(src_widget_name);
		return ret;
	}

	ret = snd_config_get_string(cfg, &wname);
	if (ret < 0) {
		SNDERR("Invalid name for sink in route %s\n", name);
		free(src_widget_name);
		return ret;
	}

	ret = tplg_pp_get_widget_name(tplg_pp, wname, index, &sink_widget_name);
	if (ret < 0) {
		SNDERR("error getting widget name for %s\n", wname);
		free(src_widget_name);
		return ret;
	}

	/* get control name */
	ret = snd_config_search(obj, "control", &cfg);
	if (ret >= 0) {
		ret = snd_config_get_string(cfg, &control);
		if (ret < 0) {
			SNDERR("Invalid control name for route %s\n", name);
			goto err;
		}
	}

	/* add route */
	route_name = tplg_snprintf("%s.%s", parent_name, name);
	if (!route_name) {
		ret = -ENOMEM;
		goto err;
	}

	ret = snd_config_make(&route, route_name, SND_CONFIG_TYPE_COMPOUND);
	free(route_name);
	if (ret < 0) {
		SNDERR("Error creating route config for %s %d\n", name, ret);
		goto err;
	}

	ret = snd_config_add(top, route);
	if (ret < 0) {
		SNDERR("Error adding route config for %s %d\n", name, ret);
		goto err;
	}

	/* add index */
	ret = tplg_config_make_add(&child, "index", SND_CONFIG_TYPE_INTEGER, route);
	if (ret < 0) {
		SNDERR("Error creating index config for %s\n", name);
		goto err;
	}

	ret = snd_config_set_integer(child, index);
	if (ret < 0) {
		SNDERR("Error setting index config for %s\n", name);
		goto err;
	}

	/* add lines */
	ret = tplg_config_make_add(&cfg, "lines", SND_CONFIG_TYPE_COMPOUND, route);
	if (ret < 0) {
		SNDERR("Error creating lines config for %s\n", name);
		goto err;
	}

	/* add route string */
	ret = tplg_config_make_add(&child, "0", SND_CONFIG_TYPE_STRING, cfg);
	if (ret < 0) {
		SNDERR("Error creating lines config for %s\n", name);
		goto err;
	}

	line_str = tplg_snprintf("%s, %s, %s", sink_widget_name, control, src_widget_name);
	if (!line_str) {
		ret = -ENOMEM;
		goto err;
	}

	/* set the line string */
	ret = snd_config_set_string(child, line_str);
	free(line_str);
	if (ret < 0)
		SNDERR("Error creating lines config for %s\n", name);
err:
	free(src_widget_name);
	free(sink_widget_name);
	return ret;
}

static int tplg_get_sample_size_from_format(const char *format)
{
	if (!strcmp(format, "s32le") || !strcmp(format, "s24le") || !strcmp(format, "float"))
		return 4;

	if (!strcmp(format, "s16le"))
		return 2;

	SNDERR("Unsupported format: %s\n", format);
	return -EINVAL;
}

int tplg_update_buffer_auto_attr(struct tplg_pre_processor *tplg_pp,
				 snd_config_t *buffer_cfg, snd_config_t *parent)
{
	snd_config_iterator_t i, next;
	snd_config_t *n, *pipeline_cfg, *child;
	const char *buffer_id, *format;
	long periods, channels, sample_size;
	long sched_period, rate, frames;
	long buffer_size;
	int err;

	if (snd_config_get_id(buffer_cfg, &buffer_id) < 0)
		return -EINVAL;

	if (!parent) {
		SNDERR("No parent for buffer %s\n", buffer_id);
		return -EINVAL;
	}

	/* acquire attributes from buffer config */
	snd_config_for_each(i, next, buffer_cfg) {
		const char *id;

		n = snd_config_iterator_entry(i);
		if (snd_config_get_id(n, &id) < 0)
			continue;

		if (!strcmp(id, "periods")) {
			if (snd_config_get_integer(n, &periods)) {
				SNDERR("Invalid number of periods for buffer %s\n", buffer_id);
				return -EINVAL;
			}
		}

		if (!strcmp(id, "channels")) {
			if (snd_config_get_integer(n, &channels)) {
				SNDERR("Invalid number of channels for buffer %s\n", buffer_id);
				return -EINVAL;
			}
		}

		if (!strcmp(id, "format")) {
			if (snd_config_get_string(n, &format)) {
				SNDERR("Invalid format for buffer %s\n", buffer_id);
				return -EINVAL;
			}
		}
	}

	pipeline_cfg = tplg_object_get_instance_config(tplg_pp, parent);

	/* acquire some other attributes from parent pipeline config */
	snd_config_for_each(i, next, pipeline_cfg) {
		const char *id;

		n = snd_config_iterator_entry(i);
		if (snd_config_get_id(n, &id) < 0)
			continue;

		if (!strcmp(id, "period")) {
			if (snd_config_get_integer(n, &sched_period)) {
				SNDERR("Invalid period for buffer %s\n", buffer_id);
				return -EINVAL;
			}
		}

		if (!strcmp(id, "rate")) {
			if (snd_config_get_integer(n, &rate)) {
				SNDERR("Invalid rate for buffer %s\n", buffer_id);
				return -EINVAL;
			}
		}
	}

	/* calculate buffer size */
	sample_size = tplg_get_sample_size_from_format(format);
	if (sample_size < 0) {
		SNDERR("Invalid sample size value for %s\n", buffer_id);
		return sample_size;
	}
	frames = (rate * sched_period) / 1000000;
	buffer_size = periods * sample_size * channels * frames;

	/* add size child config to buffer config */
	err = tplg_config_make_add(&child, "size", SND_CONFIG_TYPE_INTEGER, buffer_cfg);
	if (err < 0) {
		SNDERR("Error creating size config for %s\n", buffer_id);
		return err;
	}

	err = snd_config_set_integer(child, buffer_size);
	if (err < 0)
		SNDERR("Error setting size config for %s\n", buffer_id);

	return err;
}

int tplg_update_audio_format_auto_attr(struct tplg_pre_processor *tplg_pp,
				       snd_config_t *audio_fmt_cfg, snd_config_t *parent)
{
	enum channel_config {
		/* one channel only. */
		CHANNEL_CONFIG_MONO,
		/* L & R. */
		CHANNEL_CONFIG_STEREO,
		/* L, R & LFE; PCM only. */
		CHANNEL_CONFIG_2_POINT_1,
		/* L, C & R; MP3 & AAC only. */
		CHANNEL_CONFIG_3_POINT_0,
		/* L, C, R & LFE; PCM only. */
		CHANNEL_CONFIG_3_POINT_1,
		/* L, R, Ls & Rs; PCM only. */
		CHANNEL_CONFIG_QUATRO,
		/* L, C, R & Cs; MP3 & AAC only. */
		CHANNEL_CONFIG_4_POINT_0,
		/* L, C, R, Ls & Rs. */
		CHANNEL_CONFIG_5_POINT_0,
		/* L, C, R, Ls, Rs & LFE. */
		CHANNEL_CONFIG_5_POINT_1,
		/* one channel replicated in two. */
		CHANNEL_CONFIG_DUAL_MONO,
		/* Stereo (L,R) in 4 slots, 1st stream: [ L, R, -, - ] */
		CHANNEL_CONFIG_I2S_DUAL_STEREO_0,
		/* Stereo (L,R) in 4 slots, 2nd stream: [ -, -, L, R ] */
		CHANNEL_CONFIG_I2S_DUAL_STEREO_1,
		/* L, C, R, Ls, Rs & LFE., LS, RS */
		CHANNEL_CONFIG_7_POINT_1,
		/* invalid */
		CHANNEL_CONFIG_INVALID,
	};
	struct sample_type {
		const char *type;
		int value;
	};
	const struct sample_type sample_types[] = {
		{"MSB_Integer", 0},
		{"LSB_Integer", 1},
		{"Signed_Integer", 2},
		{"Unsigned_Integer", 3},
		{"Float", 4},
	};
	struct channel_map_table {
		int ch_count;
		enum channel_config config;
		unsigned int ch_map;
	};
	const struct channel_map_table ch_map_table[] = {
	{ 1, CHANNEL_CONFIG_MONO, 0xFFFFFFF0 },
	{ 2, CHANNEL_CONFIG_STEREO, 0xFFFFFF10 },
	{ 3, CHANNEL_CONFIG_2_POINT_1, 0xFFFFF210 },
	{ 4, CHANNEL_CONFIG_3_POINT_1, 0xFFFF3210 },
	{ 5, CHANNEL_CONFIG_5_POINT_0, 0xFFF43210 },
	{ 6, CHANNEL_CONFIG_5_POINT_1, 0xFF543210 },
	{ 7, CHANNEL_CONFIG_INVALID, 0xFFFFFFFF },
	{ 8, CHANNEL_CONFIG_7_POINT_1, 0x76543210 },
	};
	snd_config_iterator_t i, next;
	snd_config_t *n, *parent_obj, *dir_cfg, *type_cfg;
	const char *audio_fmt_id, *parent_name;
	const char *copier_dir, *copier_type;
	const char *in_sample_type, *out_sample_type;
	int input_sample_type = 0, output_sample_type = 0;
	long in_ch_count, out_ch_count;
	long in_valid_bit_depth, out_valid_bit_depth;
	long in_rate, out_rate;
	long in_bit_depth, out_bit_depth;
	int err, fmt_cfg, j;
	int in_buffer_size, out_buffer_size, dma_buffer_size;

	if (snd_config_get_id(audio_fmt_cfg, &audio_fmt_id) < 0)
		return -EINVAL;

	if (!parent) {
		SNDERR("No parent for audio_fmt %s\n", audio_fmt_id);
		return -EINVAL;
	}

	parent_obj = tplg_object_get_instance_config(tplg_pp, parent);

	parent_name = tplg_object_get_name(tplg_pp, parent_obj);
	if (!parent_name) {
		err = snd_config_get_id(parent_obj, &parent_name);
                if (err < 0) {
                        SNDERR("Invalid parent name for %s\n", audio_fmt_id);
                        return err;
                }
	}

	printf("audio_fmt auto attr %s for copier %s\n", audio_fmt_id, parent_name);

	/* get all required attribute values from the audio_format object */
	snd_config_for_each(i, next, audio_fmt_cfg) {
		const char *id;

		n = snd_config_iterator_entry(i);
		if (snd_config_get_id(n, &id) < 0)
			continue;

		if (!strcmp(id, "in_channels")) {
			if (snd_config_get_integer(n, &in_ch_count)) {
				SNDERR("Invalid intput channel count in audio_format object %s\n",
				       audio_fmt_id);
				return -EINVAL;
			}
			continue;
		}

		if (!strcmp(id, "out_channels")) {
			if (snd_config_get_integer(n, &out_ch_count)) {
				SNDERR("Invalid output channel count in audio_format object %s\n",
				       audio_fmt_id);
				return -EINVAL;
			}
			continue;
		}

		if (!strcmp(id, "in_sample_type")) {
			if (snd_config_get_string(n, &in_sample_type)) {
				SNDERR("Invalid input sample type in audio_format object %s\n",
				       audio_fmt_id);
				return -EINVAL;
			}
			continue;
		}

		if (!strcmp(id, "out_sample_type")) {
			if (snd_config_get_string(n, &out_sample_type)) {
				SNDERR("Invalid output sample type in audio_format object %s\n",
				       audio_fmt_id);
				return -EINVAL;
			}
			continue;
		}

		if (!strcmp(id, "in_valid_bit_depth")) {
			if (snd_config_get_integer(n, &in_valid_bit_depth)) {
				SNDERR("Invalid input valid bit depth in audio_format object %s\n",
				       audio_fmt_id);
				return -EINVAL;
			}
			continue;
		}

		if (!strcmp(id, "out_valid_bit_depth")) {
			if (snd_config_get_integer(n, &out_valid_bit_depth)) {
				SNDERR("Invalid output valid bit depth in audio_format object %s\n",
				       audio_fmt_id);
				return -EINVAL;
			}
			continue;
		}

		if (!strcmp(id, "in_rate")) {
			if (snd_config_get_integer(n, &in_rate)) {
				SNDERR("Invalid output sampling rate in audio_format object %s\n",
				       audio_fmt_id);
				return -EINVAL;
			}
			continue;
		}

		if (!strcmp(id, "out_rate")) {
			if (snd_config_get_integer(n, &out_rate)) {
				SNDERR("Invalid output sampling rate in audio_format object %s\n",
				       audio_fmt_id);
				return -EINVAL;
			}
			continue;
		}

		if (!strcmp(id, "in_bit_depth")) {
			if (snd_config_get_integer(n, &in_bit_depth)) {
				SNDERR("Invalid output bit depth in audio_format object %s\n",
				       audio_fmt_id);
				return -EINVAL;
			}
			continue;
		}

		if (!strcmp(id, "out_bit_depth")) {
			if (snd_config_get_integer(n, &out_bit_depth)) {
				SNDERR("Invalid output bit depth in audio_format object %s\n",
				       audio_fmt_id);
				return -EINVAL;
			}
			continue;
		}
	}

	/* add in_ch_cfg config to audio_format object config */
	err = tplg_config_make_add(&n, "in_ch_cfg", SND_CONFIG_TYPE_INTEGER, audio_fmt_cfg);
	if (err < 0) {
		SNDERR("Error creating in_ch_cfg config for %s\n", audio_fmt_id);
		return err;
	}

	err = snd_config_set_integer(n, ch_map_table[in_ch_count - 1].config);
	if (err < 0) {
		SNDERR("Error setting in_ch_cfg config for %s\n", audio_fmt_id);
		return err;
	}


	/* add in_ch_map config to audio_format object config */
	err = tplg_config_make_add(&n, "in_ch_map", SND_CONFIG_TYPE_INTEGER, audio_fmt_cfg);
	if (err < 0) {
		SNDERR("Error creating in_ch_cfg config for %s\n", audio_fmt_id);
		return err;
	}

	err = snd_config_set_integer(n, ch_map_table[in_ch_count - 1].ch_map);
	if (err < 0) {
		SNDERR("Error setting in_ch_map config for %s\n", audio_fmt_id);
		return err;
	}


	/* add in_fmt_cfg config to audio_format object config */
	err = tplg_config_make_add(&n, "in_fmt_cfg", SND_CONFIG_TYPE_INTEGER, audio_fmt_cfg);
	if (err < 0) {
		SNDERR("Error creating in_fmt_cfg config for %s\n", audio_fmt_id);
		return err;
	}

	for (j = 0; j < ARRAY_SIZE(sample_types); j++) {
		if (!strcmp(in_sample_type, sample_types[j].type))
			input_sample_type = sample_types[j].value;

		if (!strcmp(out_sample_type, sample_types[j].type))
			output_sample_type = sample_types[j].value;

	}

	fmt_cfg = (in_ch_count & 0xFF);
	fmt_cfg |= (in_valid_bit_depth & 0xFF) << 8;
	fmt_cfg |= (input_sample_type & 0xFF) << 16;

	err = snd_config_set_integer(n, fmt_cfg);
	if (err < 0) {
		SNDERR("Error setting in_fmt_cfg size config for %s\n", audio_fmt_id);
		return err;
	}

	printf("printing input audio format for copier %s\n", parent_name);
	printf("ch_cfg %d, ch_map %#x, fmt_cfg %#x\n", ch_map_table[in_ch_count - 1].config,
	       ch_map_table[in_ch_count - 1].ch_map, fmt_cfg);


	/* add out_ch_cfg config to audio_format object config */
	err = tplg_config_make_add(&n, "out_ch_cfg", SND_CONFIG_TYPE_INTEGER, audio_fmt_cfg);
	if (err < 0) {
		SNDERR("Error creating out_ch_cfg config for %s\n", audio_fmt_id);
		return err;
	}

	err = snd_config_set_integer(n, ch_map_table[out_ch_count - 1].config);
	if (err < 0) {
		SNDERR("Error setting out_ch_cfg config for %s\n", audio_fmt_id);
		return err;
	}

	/* add in_ch_map config to audio_format object config */
	err = tplg_config_make_add(&n, "out_ch_map", SND_CONFIG_TYPE_INTEGER, audio_fmt_cfg);
	if (err < 0) {
		SNDERR("Error creating out_ch_cfg config for %s\n", audio_fmt_id);
		return err;
	}

	err = snd_config_set_integer(n, ch_map_table[out_ch_count - 1].ch_map);
	if (err < 0) {
		SNDERR("Error setting out_ch_map config for %s\n", audio_fmt_id);
		return err;
	}


	/* add in_fmt_cfg config to audio_format object config */
	err = tplg_config_make_add(&n, "out_fmt_cfg", SND_CONFIG_TYPE_INTEGER, audio_fmt_cfg);
	if (err < 0) {
		SNDERR("Error creating out_fmt_cfg config for %s\n", audio_fmt_id);
		return err;
	}

	fmt_cfg = (out_ch_count & 0xFF);
	fmt_cfg |= (out_valid_bit_depth & 0xFF) << 8;
	fmt_cfg |= (output_sample_type & 0xFF) << 16;

	err = snd_config_set_integer(n, fmt_cfg);
	if (err < 0) {
		SNDERR("Error setting in_fmt_cfg size config for %s\n", audio_fmt_id);
		return err;
	}

	printf("printing output audio format for copier %s\n", parent_name);
	printf("ch_cfg %d, ch_map %#x, fmt_cfg %#x\n", ch_map_table[out_ch_count - 1].config,
	       ch_map_table[out_ch_count - 1].ch_map, fmt_cfg);

	/* add input buffer size config to audio_format object config */
	err = tplg_config_make_add(&n, "ibs", SND_CONFIG_TYPE_INTEGER, audio_fmt_cfg);
	if (err < 0) {
		SNDERR("Error creating ibs config for %s\n", audio_fmt_id);
		return err;
	}

	in_buffer_size = in_ch_count * (in_rate / 1000) * (in_bit_depth >> 3);

	err = snd_config_set_integer(n, in_buffer_size);
	if (err < 0) {
		SNDERR("Error setting ibs size config for %s\n", audio_fmt_id);
		return err;
	}

	printf("ibs %#x\n", in_buffer_size);

	/* add output buffer size config to audio_format object config */
	err = tplg_config_make_add(&n, "obs", SND_CONFIG_TYPE_INTEGER, audio_fmt_cfg);
	if (err < 0) {
		SNDERR("Error creating obs config for %s\n", audio_fmt_id);
		return err;
	}

	out_buffer_size = out_ch_count * (out_rate / 1000) * (out_bit_depth >> 3);

	err = snd_config_set_integer(n, out_buffer_size);
	if (err < 0) {
		SNDERR("Error setting obs size config for %s\n", audio_fmt_id);
		return err;
	}

	printf("obs %#x\n", out_buffer_size);

	if (strncmp(parent_name, "copier", 6))
		return err;

	/* get copier direction */
	err = snd_config_search(parent_obj, "direction", &dir_cfg);
	if (err < 0)
		return err;

	err = snd_config_get_string(dir_cfg, &copier_dir);
	if (err < 0)
		return err;

	/* get copier type */
	err = snd_config_search(parent_obj, "type", &type_cfg);
	if (err < 0)
		return err;

	err = snd_config_get_string(type_cfg, &copier_type);
	if (err < 0)
		return err;

	/* nothing to do for module copiers */
	if (!strcmp(copier_type, "module"))
		return 0;

	/* now update the parent copier's dma_buffer_size */	
	err = tplg_config_make_add(&n, "dma_buffer_size", SND_CONFIG_TYPE_INTEGER, audio_fmt_cfg);
	if (err < 0) {
		SNDERR("Error creating dma_buffer_size config for %s\n", audio_fmt_id);
		return err;
	}

	if (!strcmp(copier_type, "host")) {
		if (!strcmp(copier_dir, "playback"))
			dma_buffer_size = in_buffer_size * 2;
		else
			dma_buffer_size = out_buffer_size * 2;
	} else {
		if (!strcmp(copier_dir, "playback"))
			dma_buffer_size = out_buffer_size * 2;
		else
			dma_buffer_size = in_buffer_size * 2;
	}

	printf("dma_buffer_size %#x\n", dma_buffer_size);

	err = snd_config_set_integer(n, dma_buffer_size);
	if (err < 0) {
		SNDERR("Error setting dma_buffer_size config for %s\n", parent_name);
		return err;
	}

	return err;
}

int tplg_update_copier_auto_attr(struct tplg_pre_processor *tplg_pp, snd_config_t *copier_cfg,
				 snd_config_t *parent)
{
	enum sof_node_type {
		//HD/A host output (-> DSP).
		nHdaHostOutputClass = 0,
		//HD/A host input (<- DSP).
		nHdaHostInputClass = 1,
		//HD/A host input/output (rsvd for future use).
		nHdaHostInoutClass = 2,

		//HD/A link output (DSP ->).
		nHdaLinkOutputClass = 8,
		//HD/A link input (DSP <-).
		nHdaLinkInputClass = 9,
		//HD/A link input/output (rsvd for future use).
		nHdaLinkInoutClass = 10,

		//DMIC link input (DSP <-).
		nDmicLinkInputClass = 11,

		//I2S link output (DSP ->).
		nI2sLinkOutputClass = 12,
		//I2S link input (DSP <-).
		nI2sLinkInputClass = 13,

		//ALH link output, legacy for SNDW (DSP ->).
		nALHLinkOutputClass = 16,
		//ALH link input, legacy for SNDW (DSP <-).
		nALHLinkInputClass = 17,

		//SNDW link output (DSP ->).
		nAlhSndWireStreamLinkOutputClass = 16,
		//SNDW link input (DSP <-).
		nAlhSndWireStreamLinkInputClass = 17,

		//UAOL link output (DSP ->).
		nAlhUAOLStreamLinkOutputClass = 18,
		//UAOL link input (DSP <-).
		nAlhUAOLStreamLinkInputClass = 19,

		//IPC output (DSP ->).
		nIPCOutputClass = 20,
		//IPC input (DSP <-).
		nIPCInputClass = 21,

		//I2S Multi gtw output (DSP ->).
		nI2sMultiLinkOutputClass = 22,
		//I2S Multi gtw input (DSP <-).
		nI2sMultiLinkInputClass = 23,
		//GPIO
		nGpioClass = 24,
		//SPI
		nSpiOutputClass = 25,
		nSpiInputClass = 26,
	};
	snd_config_iterator_t i, next;
	snd_config_t *n;
	const char *copier_id, *direction, *type;
	long bss_size;
	int err, is_pages;
	int node_type = -EINVAL;

	if (snd_config_get_id(copier_cfg, &copier_id) < 0)
		return -EINVAL;

	printf("copier auto attr %s\n", copier_id);

	/* acquire attributes from buffer config */
	snd_config_for_each(i, next, copier_cfg) {
		const char *id;

		n = snd_config_iterator_entry(i);
		if (snd_config_get_id(n, &id) < 0)
			continue;

		if (!strcmp(id, "bss_size")) {
			if (snd_config_get_integer(n, &bss_size)) {
				SNDERR("Invalid number of periods for copier %s\n", copier_id);
				return -EINVAL;
			}
		}

		if (!strcmp(id, "direction")) {
			if (snd_config_get_string(n, &direction)) {
				SNDERR("Invalid direction for copier %s\n", copier_id);
				return -EINVAL;
			}
		}

		if (!strcmp(id, "copier_type")) {
			if (snd_config_get_string(n, &type)) {
				SNDERR("Invalid direction for copier %s\n", copier_id);
				return -EINVAL;
			}
		}
	}

	is_pages = ((bss_size + (1 << 12) - 1) & ~((1 << 12) - 1)) >> 12;

	printf("mem_size for %s is %d\n", copier_id, is_pages);

	/* add in_fmt_cfg config to audio_format object config */
	err = tplg_config_make_add(&n, "is_pages", SND_CONFIG_TYPE_INTEGER, copier_cfg);
	if (err < 0) {
		SNDERR("Error creating mem_size config for %s\n", copier_id);
		return err;
	}

	err = snd_config_set_integer(n, is_pages);
	if (err < 0) {
		SNDERR("Error setting mem_size size config for %s\n", copier_id);
		return err;
	}

	/* add node_type config to audio_format object config */
	err = tplg_config_make_add(&n, "node_type", SND_CONFIG_TYPE_INTEGER, copier_cfg);
	if (err < 0) {
		SNDERR("Error creating bode_type config for %s\n", copier_id);
		return err;
	}

	if (!strcmp(type, "host")) {
		if (!strcmp(direction, "playback"))
			node_type = nHdaHostOutputClass;
		else
			node_type = nHdaHostInputClass;
		goto set_node_type;
	}

	if (!strcmp(type, "HDA")) {
		if (!strcmp(direction, "playback"))
			node_type = nHdaLinkOutputClass;
		else
			node_type = nHdaLinkInputClass;
		goto set_node_type;
	}

	if (!strcmp(type, "ALH")) {
		if (!strcmp(direction, "playback"))
			node_type = nALHLinkOutputClass;
		else
			node_type = nALHLinkInputClass;
		goto set_node_type;
	}

	if (!strcmp(type, "SSP")) {
		if (!strcmp(direction, "playback"))
			node_type = nI2sLinkOutputClass;
		else
			node_type = nI2sLinkInputClass;
		goto set_node_type;
	}

	if (!strcmp(type, "DMIC"))
		node_type = nDmicLinkInputClass;

set_node_type:
	if (node_type < 0) {
		SNDERR("Invalid node_type size config for %s\n", copier_id);
		return node_type;
	}

	err = snd_config_set_integer(n, node_type);
	if (err < 0) {
		SNDERR("Error setting node_type size config for %s\n", copier_id);
		return err;
	}

	return 0;
}
