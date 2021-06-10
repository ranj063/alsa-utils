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
#define TPLG_DEBUG

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

struct tplg_widget_name_map {
	char *tplg1_name;
	char *tplg2_name;
};

struct tplg_token_map {
	char *tplg1_name;
	char *tplg2_name;
};

struct tplg_widget_name_map widget_map[] = {
	{"BUF", "buffer."},
	{"PGA", "pga."},
	{"MUXDEMUX", "muxdemux."},
	{"Dmic", "pga."},// this is a speacial case
	{"SSP", "dai.SSP."},
	{"ALH", "dai.ALH."},
	{"SAI", "dai.SAI."},
	{"ESAI", "dai.ESAI."},
	{"HDA", "dai.HDA."},
	{"DMIC", "dai.DMIC."},
	{"PCM", "host."},
	{"PIPELINE", "pipeline."},
	{"EQIIR", "eqiir."},
};

struct tplg_token_map token_map[] = {
	{"SOF_TKN_BUF_SIZE", "size"},
	{"SOF_TKN_BUF_CAPS", "caps"},
	{"SOF_TKN_DAI_DMAC_CONFIG", "dmac_config"},
	{"SOF_TKN_DAI_TYPE", "dai_type"},
	{"SOF_TKN_DAI_INDEX", "dai_index"},
	{"SOF_TKN_DAI_DIRECTION", "direction"},
	{"SOF_TKN_SCHED_PERIOD", "period"},
	{"SOF_TKN_SCHED_PRIORITY", "priority"},
	{"SOF_TKN_SCHED_MIPS", "mips"},
	{"SOF_TKN_SCHED_CORE", "core"},
	{"SOF_TKN_SCHED_FRAMES", "frames"},
	{"SOF_TKN_SCHED_TIME_DOMAIN", "time_domain"},
	{"SOF_TKN_SCHED_DYNAMIC_PIPELINE", "dynamic_pipeline"},
	{"SOF_TKN_VOLUME_RAMP_STEP_TYPE", "step_type"},
	{"SOF_TKN_VOLUME_RAMP_STEP_MS", "step_ms"},
	{"SOF_TKN_SRC_RATE_IN", "rate_in"},
	{"SOF_TKN_SRC_RATE_OUT", "rate_out"},
	{"SOF_TKN_ASRC_RATE_IN", "rate_in"},
	{"SOF_TKN_ASRC_RATE_OUT", "rate_out"},
	{"SOF_TKN_ASRC_ASYNCHRONOUS_MODE", "asynchronous_mode"},
	{"SOF_TKN_ASRC_OPERATION_MODE", "operation_mode"},
	{"SOF_TKN_PCM_DMAC_CONFIG", "dmac_config"},
	{"SOF_TKN_COMP_PERIOD_SINK_COUNT", "period_sink_count"},
	{"SOF_TKN_COMP_PERIOD_SOURCE_COUNT", "period_source_count"},
	{"SOF_TKN_COMP_FORMAT", "format"},
	{"SOF_TKN_COMP_PRELOAD_COUNT", "preload_count"},
	{"SOF_TKN_COMP_CORE_ID", "core_id"},
	{"SOF_TKN_COMP_UUID", 	"uuid"},
	{"SOF_TKN_INTEL_SSP_CLKS_CONTROL", "clks_control"},
	{"SOF_TKN_INTEL_SSP_MCLK_ID",	"mclk_id"},
	{"SOF_TKN_INTEL_SSP_SAMPLE_BITS", "sample_bits"},
	{"SOF_TKN_INTEL_SSP_FRAME_PULSE_WIDTH", "frame_pulse_width"},
	{"SOF_TKN_INTEL_SSP_QUIRKS", "quirks"},
	{"SOF_TKN_INTEL_SSP_TDM_PADDING_PER_SLOT",  "tdm_padding_per_slot"},
	{"SOF_TKN_INTEL_SSP_BCLK_DELAY", "bclk_delay"},
	{"SOF_TKN_INTEL_DMIC_DRIVER_VERSION",	"driver_version"},
	{"SOF_TKN_INTEL_DMIC_CLK_MIN", "clk_min"},
	{"SOF_TKN_INTEL_DMIC_CLK_MAX", "clk_max"},
	{"SOF_TKN_INTEL_DMIC_DUTY_MIN", "duty_min"},
	{"SOF_TKN_INTEL_DMIC_DUTY_MAX", "duty_max"},
	{"SOF_TKN_INTEL_DMIC_NUM_PDM_ACTIVE",	"num_pdm_active"},
	{"SOF_TKN_INTEL_DMIC_SAMPLE_RATE", "sample_rate"},
	{"SOF_TKN_INTEL_DMIC_FIFO_WORD_LENGTH", "fifo_word_length"},
	{"SOF_TKN_INTEL_DMIC_UNMUTE_RAMP_TIME_MS", "unmute_ramp_time_ms"},
	{"SOF_TKN_INTEL_DMIC_PDM_CTRL_ID", "ctrl_id"},
	{"SOF_TKN_INTEL_DMIC_PDM_MIC_A_Enable", "mic_a_enable"},
	{"SOF_TKN_INTEL_DMIC_PDM_MIC_B_Enable", "mic_b_enable"},
	{"SOF_TKN_INTEL_DMIC_PDM_POLARITY_A",	"polarity_a"},
	{"SOF_TKN_INTEL_DMIC_PDM_POLARITY_B",	"polarity_b"},
	{"SOF_TKN_INTEL_DMIC_PDM_CLK_EDGE", "clk_edge"},
	{"SOF_TKN_INTEL_DMIC_PDM_SKEW", "skew"},
	{"SOF_TKN_TONE_SAMPLE_RATE", "sample_rate"},
	{"SOF_TKN_PROCESS_TYPE", "process_type"},
	{"SOF_TKN_INTEL_HDA_RATE", "rate"},
	{"SOF_TKN_INTEL_HDA_CH", "ch"},
	{"SOF_TKN_INTEL_ALH_RATE", "rate"},
	{"SOF_TKN_INTEL_ALH_CH", "ch"},
	{"SOF_TKN_MUTE_LED_USE", "mute_led_use"},
	{"SOF_TKN_MUTE_LED_DIRECTION", "mute_led_direction"},
	
};

/* make a new config and add it to parent */
int tplg_config_make_add_join(snd_config_t **config, const char *id, snd_config_type_t type,
			 snd_config_t *parent, bool join)
{
	int ret;

	if (type != SND_CONFIG_TYPE_COMPOUND) {
		ret = snd_config_make(config, id, type);
		if (ret < 0)
			return ret;
	} else {
		ret = snd_config_make_compound(config, id, join);
		if (ret < 0)
			return ret;
	}

	ret = snd_config_add(parent, *config);
	if (ret < 0)
		snd_config_delete(*config);

	return ret;
}

int tplg_create_and_add_config(snd_config_t *parent, char *str, bool join)
{
	snd_config_t *child;
	char *temp;
	int err;

	temp = strchr(str, '.');

	while(temp) {
		char *new_config_id;

		new_config_id = calloc(1, strlen(str) - strlen(temp) + 1);
		if (!new_config_id)
			return -ENOMEM;

		snprintf(new_config_id, strlen(str) - strlen(temp) + 1, "%s", str);

		/* first try to find it */
		err = snd_config_search(parent, new_config_id, &child);
		if (err < 0) {
			/* create it if not found */
			err = tplg_config_make_add_join(&child, new_config_id, SND_CONFIG_TYPE_COMPOUND, parent, join);
			free(new_config_id);
			if (err < 0)
				return -ENOMEM;
		}

		str = temp + 1;
		temp = strchr(str, '.');
		parent = child;
	}


	return tplg_config_make_add(&child, str, SND_CONFIG_TYPE_COMPOUND, parent);
}

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

	*tplg_pp = _tplg_pp;

	/* create output top-level config node */
	ret = snd_config_top(&_tplg_pp->output_cfg);
	if (ret < 0)
		goto err;

	/* open output based on type */
	if (type == SND_OUTPUT_STDIO) {
		ret = snd_output_stdio_open(&_tplg_pp->output, output_file, "a");
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
err:
	free(_tplg_pp);
	return ret;
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
		fprintf(stderr, "Unable not load configuration\n");
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

static int tplg_string_config_set_and_add(snd_config_t *parent, char *id, char *value)
{
	snd_config_t *new;
	int err;

	/* is it a number */
	if (strchr(value, ' '))
		goto string;

	if (value[0] >= '0' && value[0] <= '9')
		goto integer;

string:
	err = tplg_config_make_add(&new, id, SND_CONFIG_TYPE_STRING, parent);
	if (err < 0) {
		fprintf(stderr, "Error creating string config %s\n", id);
		return err;
	}

	return snd_config_set_string(new, value);

integer:
	err = tplg_config_make_add(&new, id, SND_CONFIG_TYPE_INTEGER, parent);
	if (err < 0) {
		fprintf(stderr, "Error creating integer config %s error %d\n", id, err);
		return err;
	}

	return snd_config_set_integer(new, atoi(value));
}

static int tplg_integer_config_set_and_add(snd_config_t *parent, char *id, char *value)
{
	snd_config_t *new;
	int err;

	err = tplg_config_make_add(&new, id, SND_CONFIG_TYPE_INTEGER, parent);
	if (err < 0) {
		fprintf(stderr, "Error creating string config %s\n", id);
		return err;
	}

	return snd_config_set_integer(new, atoi(value));
}

static int tplg_copy_all_configs(snd_config_t *src, snd_config_t *dest)
{
	snd_config_iterator_t i, next;
	snd_config_t *n;
	int ret;

	snd_config_for_each(i, next, src) {
		const char *str, *id;
		char *val;

		n = snd_config_iterator_entry(i);

		if (snd_config_get_type(n) == SND_CONFIG_TYPE_COMPOUND)
			continue;

		if (snd_config_get_id(n, &id) < 0)
			continue;

		if (snd_config_get_string(n, &str) < 0)
			continue;

		/* skip empty values */
		if (!strcmp(str, ""))
			continue;

		if (!strcmp(str, "codec_slave"))
			val = "codec_consumer";
		else
			val = (char *)str;

		/* check if value is a number */
		ret = tplg_string_config_set_and_add(dest, (char *)id, val);
		if (ret < 0) {
			fprintf(stderr, "error adding %s\n", id);
			return ret;
		}
	}

	return 0;
}

static snd_config_t *tplg_find_hw_config(struct tplg_pre_processor *tplg_pp, const char *name)
{
	snd_config_iterator_t i, next;
	snd_config_t *section_hwcfg, *n;
	int err;

	err = snd_config_search(tplg_pp->input_cfg, "SectionHWConfig", &section_hwcfg);
	if (err < 0) {
		fprintf(stderr, "no section hwcfg found\n");
		return NULL;
	}

	snd_config_for_each(i, next, section_hwcfg) {
		const char *id;

		n = snd_config_iterator_entry(i);

		if (snd_config_get_id(n, &id) < 0)
			continue;

		if (!strcmp(id, name))
			return n;
	}

	return NULL;
}

static snd_config_t *tplg_find_tuple_config_item(snd_config_t *config, char *tuple_id)
{
	snd_config_iterator_t i, next;
	snd_config_t *n;

	snd_config_for_each(i, next, config) {
		const char *id;

		n = snd_config_iterator_entry(i);

		if (snd_config_get_id(n, &id) < 0)
			continue;

		if (!strcmp(id, tuple_id))
			return n;
	}

	return NULL;
}

static char *tplg_get_tplg2_token_name(const char *tplg1_name)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(token_map); i++) {
		if (!strcmp(token_map[i].tplg1_name, tplg1_name))
			return token_map[i].tplg2_name;
	}

	return NULL;
}

static char *tplg_get_tplg2_widget_classname(const char *tplg1_name)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(widget_map); i++) {
		if (strstr(tplg1_name, widget_map[i].tplg1_name))
			return widget_map[i].tplg2_name;
	}

	return NULL;
}

static char *tplg_get_tplg2_widget_name(char *tplg1_name)
{
	char *new_str;
	int i;

	for (i = 0; i < ARRAY_SIZE(widget_map); i++) {
		if (strstr(tplg1_name, widget_map[i].tplg1_name)) {
			char *instance_str, *temp;
			int len, instance, temp_len;

			instance_str = strchr(tplg1_name, '.');
			if (!instance_str) {
				fprintf(stderr, "error getting instance id from %s\n", tplg1_name);
				return NULL;
			}

			temp_len = strlen(tplg1_name + strlen(widget_map[i].tplg1_name)) - strlen(instance_str) + 1;
			temp = calloc(1, temp_len);
			if (!temp)
				return NULL;
			snprintf(temp, strlen(tplg1_name + strlen(widget_map[i].tplg1_name)) - strlen(instance_str) + 1,
				"%s", tplg1_name + strlen(widget_map[i].tplg1_name));

			instance_str += 1;

			if (instance_str[0] >= '0' && instance_str[0] <= '9') {

				len = strlen(widget_map[i].tplg2_name) + strlen(temp) + strlen(instance_str) + 2;
				len += strlen(instance_str);

				new_str = calloc(1, len);
				if (!new_str) {
					free(temp);
					return NULL;
				}
				instance = atoi(instance_str) + 1;
				snprintf(new_str, len, "%s%s.%d", widget_map[i].tplg2_name, temp, instance);
			} else {
				len = strlen(widget_map[i].tplg2_name) + strlen(temp) + 2;
				len += strlen(instance_str);
				new_str = calloc(1, len);
				if (!new_str) {
					free(temp);
					return NULL;
				}
				snprintf(new_str, len, "%s%s.%s", widget_map[i].tplg2_name, temp, instance_str);
			}
			free(temp);

			if (strstr(new_str, "OUT")) {
				char *temp1, *temp;

				temp1 = calloc(1, strlen(new_str) - 2);
				if (!temp1) {
					free(new_str);
					return NULL;
				}

				temp = calloc(1, strlen(new_str) + 6);
				if (!temp) {
					free(temp1);
					free(new_str);
					return NULL;
				}

				snprintf(temp1, strlen(new_str) - 2, "%s", new_str);
				snprintf(temp, strlen(new_str) + 6, "%s%s\n", temp1, "playback");
				free(new_str);
				free(temp1);
				new_str = temp;
			}
			if (strstr(new_str, "IN")) {
				char *temp1, *temp;

				temp1 = calloc(1, strlen(new_str) - 1);
				if (!temp1) {
					free(new_str);
					return NULL;
				}

				temp = calloc(1, strlen(new_str) + 6);
				if (!temp) {
					free(temp1);
					free(new_str);
					return NULL;
				}

				snprintf(temp1, strlen(new_str) - 1, "%s", new_str);
				snprintf(temp, strlen(new_str) + 6, "%s%s\n", temp1, "capture");
				free(new_str);
				free(temp1);
				new_str = temp;
			}

			return new_str;
		}
	}

	fprintf(stderr, "no tplg2 name found for %s\n", tplg1_name);
	return NULL;
}

static int tplg_add_all_tuple_items(snd_config_t *config, snd_config_t *dest)
{
	snd_config_iterator_t i, next;
	snd_config_t *n, *new;
	int err;

	snd_config_for_each(i, next, config) {
		const char *id, *str;
		char *tplg2_name;

		n = snd_config_iterator_entry(i);

		if (snd_config_get_id(n, &id) < 0)
			continue;

		if (snd_config_get_string(n, &str) < 0)
			continue;

		/* skip empty tuples */
		if (!strcmp(str, ""))
			continue;

		if (!strcmp(id, "SOF_TKN_COMP_UUID") || !strcmp(id, "SOF_TKN_DAI_TYPE"))
			continue;

		if (!strcmp(id, "SOF_TKN_INTEL_SSP_QUIRKS")) {
			if (!strcmp(str, "64"))
				str = "lbm_mode";
			else
				str = "timer";
		}

		if (!strcmp(id, "SOF_TKN_SCHED_TIME_DOMAIN")) {
			if (!strcmp(str, "0"))
				str = "dma";
			else
				str = "timer";
		}

		if (!strcmp(id, "SOF_TKN_DAI_DIRECTION")) {
			if (!strcmp(str, "0"))
				str = "playback";
			else
				str = "capture";
		}

		if (!strcmp(id, "SOF_TKN_SCHED_DYNAMIC_PIPELINE"))
				str = "@DYNAMIC_PIPELINE@";

		tplg2_name = tplg_get_tplg2_token_name(id);
		if (!tplg2_name) {
			fprintf(stderr, "invalid token name %s\n", id);
			return -EINVAL;
		}

		err = snd_config_search(dest, tplg2_name, &new);
		if (err < 0) {
			err = tplg_string_config_set_and_add(dest, tplg2_name, (char *) str);
			if (err < 0)
				return err;
			continue;
		}

		if (snd_config_get_type(new) == SND_CONFIG_TYPE_STRING) {
			err = snd_config_set_string(new, str);
			if(err < 0) {
				fprintf(stderr, "Error setting tuple config %s\n", tplg2_name);
				return err;
			}
		} else {
			err = snd_config_set_integer(new, atoi(str));
			if(err < 0) {
				fprintf(stderr, "Error setting tuple config %s\n", tplg2_name);
				return err;
			}
		}
	}

	return 0;
}

static int tplg_add_all_tuples(struct tplg_pre_processor *tplg_pp, snd_config_t *src, snd_config_t *dest)
{
	snd_config_iterator_t i, next;
	snd_config_t *data_cfg, *n;
	int err;

	err = snd_config_search(src, "data", &data_cfg);
	if (err < 0) {
		fprintf(stderr, "no data found\n");
		return 0;
	}

	snd_config_for_each(i, next, data_cfg) {
		snd_config_t *section_data_cfg, *data_name_cfg, *section_tuples_cfg, *tuples_cfg, *item, *pdm, *pdm_obj, *tuples;
		const char *tuples_name, *data_str;

		n = snd_config_iterator_entry(i);

		if (snd_config_get_string(n, &data_str) < 0)
			continue;

		err = snd_config_search(tplg_pp->input_cfg, "SectionData", &section_data_cfg);
		if (err < 0) {
			fprintf(stderr, "no section data found\n");
			continue;
		}

		data_name_cfg = tplg_find_config(section_data_cfg, data_str);
		if (!data_name_cfg)
			continue;

		err = snd_config_search(data_name_cfg, "tuples", &tuples_cfg);
		if (err < 0) {
			fprintf(stderr, "no tuples found\n");
			continue;
		}

		if (snd_config_get_string(tuples_cfg, &tuples_name) < 0)
			continue;

		err = snd_config_search(tplg_pp->input_cfg, "SectionVendorTuples", &section_tuples_cfg);
		if (err < 0) {
			fprintf(stderr, "no section vendor tuples found\n");
			continue;
		}

		tuples_cfg = tplg_find_config(section_tuples_cfg, tuples_name);
		if (!tuples_cfg)
			continue;

		/* iterate through the tuple arrays */
		err = snd_config_search(tuples_cfg, "tuples.word", &item);
		if (err < 0)
			goto short_tuples;

		err = tplg_add_all_tuple_items(item, dest);
		if (err < 0)
			return err;
short_tuples:
		err = snd_config_search(tuples_cfg, "tuples.short", &item);
		if (err < 0)
			goto pdm0;

		err = tplg_add_all_tuple_items(item, dest);
		if (err < 0)
			return err;

pdm0:
		err = snd_config_search(tuples_cfg, "tuples", &tuples);
		if (err < 0)
			goto pdm1;

		pdm = tplg_find_config(tuples, "short.pdm0");
		if (!pdm)
			goto pdm1;

		/* add pdm object */
		err = tplg_create_and_add_config(dest, "Object.Base.pdm_config.0", true);
		if (err < 0)
			return err;

		err = snd_config_search(dest, "Object.Base.pdm_config.0", &pdm_obj);
		if (err < 0)
			return err;

		err = tplg_add_all_tuple_items(pdm, pdm_obj);
		if (err < 0)
			return err;

pdm1:
		pdm = tplg_find_config(tuples, "short.pdm1");
		if (!pdm)
			goto string_tuples;

		err = tplg_create_and_add_config(dest, "Object.Base.pdm_config.1", true);
		if (err < 0)
			return err;

		err = snd_config_search(dest, "Object.Base.pdm_config.1", &pdm_obj);
		if (err < 0)
			return err;

		err = tplg_add_all_tuple_items(pdm, pdm_obj);
		if (err < 0)
			return err;

string_tuples:
		err = snd_config_search(tuples_cfg, "tuples.string", &item);
		if (err < 0)
			goto bool_tuples;

		err = tplg_add_all_tuple_items(item, dest);
		if (err < 0)
			return err;
bool_tuples:
		err = snd_config_search(tuples_cfg, "tuples.bool", &item);
		if (err < 0)
			goto uuid_tuples;

		err = tplg_add_all_tuple_items(item, dest);
		if (err < 0)
			return err;
uuid_tuples:
		err = snd_config_search(tuples_cfg, "tuples.uuid", &item);
		if (err < 0)
			continue;

		err = tplg_add_all_tuple_items(item, dest);
		if (err < 0)
			return err;
	}

	return 0;	
}

static snd_config_t *tplg_find_tuple_config(struct tplg_pre_processor *tplg_pp, snd_config_t *config, char *tuple_id)
{
	snd_config_iterator_t i, next;
	snd_config_t *data_cfg, *n, *out_cfg;
	int err;

	err = snd_config_search(config, "data", &data_cfg);
	if (err < 0) {
		fprintf(stderr, "no data found\n");
		return NULL;
	}

	snd_config_for_each(i, next, data_cfg) {
		snd_config_t *section_data_cfg, *data_name_cfg, *section_tuples_cfg, *tuples_cfg, *item;
		const char *tuples_name, *data_str;

		n = snd_config_iterator_entry(i);

		if (snd_config_get_string(n, &data_str) < 0)
			continue;

		err = snd_config_search(tplg_pp->input_cfg, "SectionData", &section_data_cfg);
		if (err < 0) {
			fprintf(stderr, "no section data found\n");
			continue;
		}

		data_name_cfg = tplg_find_config(section_data_cfg, data_str);
		if (!data_name_cfg)
			continue;

		err = snd_config_search(data_name_cfg, "tuples", &tuples_cfg);
		if (err < 0) {
			fprintf(stderr, "no tuples found\n");
			continue;
		}

		if (snd_config_get_string(tuples_cfg, &tuples_name) < 0)
			continue;

		err = snd_config_search(tplg_pp->input_cfg, "SectionVendorTuples", &section_tuples_cfg);
		if (err < 0) {
			fprintf(stderr, "no section vendor tuples found\n");
			continue;
		}

		tuples_cfg = tplg_find_config(section_tuples_cfg, tuples_name);
		if (!tuples_cfg)
			continue;

		/* iterate through the tuple arrays */
		err = snd_config_search(tuples_cfg, "tuples.word", &item);
		if (err < 0)
			goto short_tuples;

		out_cfg = tplg_find_tuple_config_item(item, tuple_id);
		if (out_cfg)
			return out_cfg;
short_tuples:
		err = snd_config_search(tuples_cfg, "tuples.short", &item);
		if (err < 0)
			goto string_tuples;

		out_cfg = tplg_find_tuple_config_item(item, tuple_id);
		if (out_cfg)
			return out_cfg;

string_tuples:
		err = snd_config_search(tuples_cfg, "tuples.string", &item);
		if (err < 0)
			goto bool_tuples;
		out_cfg = tplg_find_tuple_config_item(item, tuple_id);
		if (out_cfg)
			return out_cfg;
bool_tuples:
		err = snd_config_search(tuples_cfg, "tuples.bool", &item);
		if (err < 0)
			goto uuid_tuples;

		out_cfg = tplg_find_tuple_config_item(item, tuple_id);
		if (out_cfg)
			return out_cfg;
uuid_tuples:
		err = snd_config_search(tuples_cfg, "tuples.uuid", &item);
		if (err < 0)
			continue;

		out_cfg = tplg_find_tuple_config_item(item, tuple_id);
		if (out_cfg)
			return out_cfg;
	}

	return NULL;
}

static int tplg_add_hw_config_objects(struct tplg_pre_processor *tplg_pp, snd_config_t *hw_cfg, snd_config_t *dest)
{
	snd_config_iterator_t i, next;
	snd_config_t *n, *hw_cfg_item, *hw_cfg_obj;
	int err;

	snd_config_for_each(i, next, hw_cfg) {
		const char *name;
		char *str;

		n = snd_config_iterator_entry(i);

		if (snd_config_get_string(n, &name) < 0)
			continue;

		hw_cfg_item = tplg_find_hw_config(tplg_pp, name);
		if (!hw_cfg_item)
			return -EINVAL;

		str = calloc(1, strlen("Object.Base.hw_config.") + strlen(name) + 1);
		if (!str)
			return -ENOMEM;

		snprintf(str, strlen("Object.Base.hw_config.") + strlen(name) + 1, "Object.Base.hw_config.%s", name);
		err = tplg_create_and_add_config(dest, str, true);
		if (err < 0) {
			fprintf(stderr, "Cannot create new hw_cfg object %s\n", str);
			free(str);
			return err;
		}

		/* copy all hw_cfg attributes */
		err = snd_config_search(dest, str, &hw_cfg_obj);
		free(str);
		err = tplg_copy_all_configs(hw_cfg_item, hw_cfg_obj);
		if (err < 0) {
			fprintf(stderr, "error copying configs for hw_cfg %s\n", name);
			return err;
		}
	}

	return 0;
}

static int tplg_add_dai_widgets(struct tplg_pre_processor *tplg_pp, snd_config_t *dest, const char *sname);

static int parse_be(struct tplg_pre_processor *tplg_pp, snd_config_t *n)
{
	snd_config_iterator_t i, next;
	snd_config_t *n2, *dai_cfg, *dai_obj, *hw_cfg;
	int err;

	snd_config_for_each(i, next, n) {
		snd_config_t *type_cfg, *index_cfg;
		const char *id, *type, *index;
		char *str;

		n2 = snd_config_iterator_entry(i);

		if (snd_config_get_id(n2, &id) < 0)
			continue;

		if (snd_config_get_type(n2) != SND_CONFIG_TYPE_COMPOUND) {
			fprintf(stderr, "compound type expected for %s", id);
			return -EINVAL;
		}

		err = snd_config_search(tplg_pp->output_cfg, "Object.Dai", &dai_cfg);
		if (err < 0) {
			fprintf(stderr, "Error finding Object.DAI config\n");
			return err;
		}

		/* find type and dai index */
		type_cfg = tplg_find_tuple_config(tplg_pp, n2, "SOF_TKN_DAI_TYPE");
		if (!type_cfg) {
			fprintf(stderr, "no type for DAI %s\n", id);
			return -EINVAL;
		}

		if (snd_config_get_string(type_cfg, &type) < 0) {
			fprintf(stderr, "Invalid type for DAI %s\n", id);
			return -EINVAL;
		}

		index_cfg = tplg_find_tuple_config(tplg_pp, n2, "SOF_TKN_DAI_INDEX");
		if (!index_cfg) {
			fprintf(stderr, "no index for DAI %s\n", id);
			return -EINVAL;
		}

		if (snd_config_get_string(index_cfg, &index) < 0) {
			fprintf(stderr, "Invalid index for DAI %s\n", id);
			return -EINVAL;
		}

		str = calloc(1, strlen(type) + strlen(index) + 2);
		if (!str)
			return -ENOMEM;
		snprintf(str, strlen(type) + strlen(index) + 2, "%s.%s", type, index);

		/* Add type.index config for DAI object */
		err = tplg_create_and_add_config(dai_cfg, str, true);
		if (err < 0) {
			fprintf(stderr, "Cannot create new DAI object %s\n", str);
			free(str);
			return err;
		}

		err = snd_config_search(dai_cfg, str, &dai_obj);
		free(str);
		if (err < 0) {
			fprintf(stderr, "Error finding DAI obj config\n");
			return err;
		}

		/* add stream name */
		err = tplg_string_config_set_and_add(dai_obj, "name", (char *)id);
		if (err < 0)
			return err;

		err = tplg_copy_all_configs(n2, dai_obj);
		if (err < 0) {
			fprintf(stderr, "error copying configs for DAI %s\n", id);
			return err;
		}

		err = tplg_add_all_tuples(tplg_pp, n2, dai_obj);
		if (err < 0) {
			fprintf(stderr, "error copying tuples for DAI %s\n", id);
			return err;
		}

		/* find hwcfg */
		err = snd_config_search(n2, "hw_configs", &hw_cfg);
		if (err < 0) {
			fprintf(stderr, " no hw_cfgs found for %s\n", id);
			return err;
		}

		/* all all hw_cfg objects */
		err = tplg_add_hw_config_objects(tplg_pp, hw_cfg, dai_obj);
		if (err < 0) {
			fprintf(stderr, "failed to add hw_cfg objects for %s\n", id);
			return err;
		}

		/* add DAI widget objects */
		err = tplg_add_dai_widgets(tplg_pp, dai_obj, id);
		if (err < 0)
			return err;

		/* TODO: add direction config */

	}

	return 0;
}

static int parse_pcm(struct tplg_pre_processor *tplg_pp, snd_config_t *n)
{
	snd_config_iterator_t i, next;
	snd_config_t *n2, *pipe_obj, *pipe_cfg, *pipe_class, *id_cfg, *child, *new, *caps_cfg, *section_caps;
	const char *id;
	int err;

	err = snd_config_search(tplg_pp->input_cfg, "SectionPCMCapabilities", &section_caps);
	if (err < 0) {
		fprintf(stderr, "no section pcm caps config\n");
		return err;
	}

	snd_config_for_each(i, next, n) {
		const char *pcm_id;
		char *new_id, *fe_dai;
		bool playback = false, capture = false;

		n2 = snd_config_iterator_entry(i);

		if (snd_config_get_id(n2, &id) < 0)
			continue;

		if (snd_config_get_type(n2) != SND_CONFIG_TYPE_COMPOUND) {
			fprintf(stderr, "compound type expected for %s", id);
			return -EINVAL;
		}

		err = snd_config_search(tplg_pp->output_cfg, "Object.PCM", &pipe_cfg);
		if (err < 0) {
			fprintf(stderr, "Error finding Object.PCM config\n");
			return err;
		}

		/* find pcm class */
		err = snd_config_search(pipe_cfg, "pcm", &pipe_class);
		if (err < 0) {
			err = tplg_config_make_add_join(&pipe_class, "pcm", SND_CONFIG_TYPE_COMPOUND, pipe_cfg, true);
			if (err < 0) {
				fprintf(stderr, "Error creating pcm class config\n");
				return err;
			}
		}

		/* get PCM id */
		err = snd_config_search(n2, "id", &id_cfg);
		if (err < 0) {
			fprintf(stderr, "No ID for PCM %s\n", id);
			return err;
		}

		if (snd_config_get_string(id_cfg, &pcm_id) < 0) {
			fprintf(stderr, "Invalid ID for PCM %s\n", id);
			return -EINVAL;
		}

		new_id = calloc(1, strlen(pcm_id) + 1);
		if (!new_id)
			return -ENOMEM;

		snprintf(new_id, strlen(pcm_id) + 1, "%s", pcm_id);

		err = tplg_config_make_add(&pipe_obj, new_id, SND_CONFIG_TYPE_COMPOUND, pipe_class);
		if (err < 0) {
			fprintf(stderr, "Error creating new PCM Object config %s\n", pcm_id);
			return err;
		}

		/* add name for PCM */
		err = tplg_config_make_add(&child, "name", SND_CONFIG_TYPE_STRING, pipe_obj);
		if (err < 0) {
			fprintf(stderr, "Error creating new PCM Object config %s\n", pcm_id);
			return err;
		}

		err = snd_config_set_string(child, id);
		if(err < 0)
			return err;

		fe_dai = calloc(1, strlen("Object.Base.fe_dai") + strlen(id) + 2);
		if (!fe_dai)
			return -ENOMEM;

		snprintf(fe_dai, strlen("Object.Base.fe_dai") + strlen(id) + 2, "%s.%s", "Object.Base.fe_dai", id);

		/* add object config */
		err = tplg_create_and_add_config(pipe_obj, fe_dai, true);
		free(fe_dai);
		if (err < 0) {
			fprintf(stderr, "Error creating fe_dai object config\n");
			return err;
		}

		/* check if playback caps exists */
		err = snd_config_search(n2, "pcm.playback", &child);
		if (err < 0)
			goto capture;

		playback = true;

		/* get playback caps name */
		err = snd_config_search(child, "capabilities", &id_cfg);
		if (err < 0) {
			fprintf(stderr, "No capabilities for PCM playback\n");
			return err;
		}

		if (snd_config_get_string(id_cfg, &pcm_id) < 0) {
			fprintf(stderr, "Invalid name for PCM playback caps\n");
			return -EINVAL;
		}

		/* add PCM config */
		err = tplg_create_and_add_config(pipe_obj, "Object.PCM.pcm_caps.playback", true);
		if (err < 0) {
			fprintf(stderr, "Error creating PCM playback caps config\n");
			return err;
		}

		/* get playback config */
		err = snd_config_search(pipe_obj, "Object.PCM.pcm_caps.playback", &new);
		if (err < 0) {
			fprintf(stderr, "No playback caps for PCM\n");
			return err;
		}

		err = tplg_config_make_add(&child, "name", SND_CONFIG_TYPE_STRING, new);
		if (err < 0) {
			fprintf(stderr, "Error creating name config\n");
			return err;
		}

		/* set playback caps name */
		err = snd_config_set_string(child, pcm_id);
		if(err < 0)
			return err;

		/* find pcm caps and add all attributes */
		err = snd_config_search(section_caps, pcm_id, &caps_cfg);
		if (err < 0) {
			fprintf(stderr, "cannot find pcm caps config %s\n", pcm_id);
			return err;
		}

		err = tplg_copy_all_configs(caps_cfg, new);
		if (err < 0) {
			fprintf(stderr, "cannot copy pcm caps config %s\n", pcm_id);
			return err;
		}

capture:
		/* check if capture caps exists */
		err = snd_config_search(n2, "pcm.capture", &child);
		if (err < 0)
			goto direction;

		capture = true;

		/* get capture caps name */
		err = snd_config_search(child, "capabilities", &id_cfg);
		if (err < 0) {
			fprintf(stderr, "No capabilities for PCM capture\n");
			return err;
		}

		if (snd_config_get_string(id_cfg, &pcm_id) < 0) {
			fprintf(stderr, "Invalid name for PCM playback caps\n");
			return -EINVAL;
		}

		/* add PCM config */
		err = tplg_create_and_add_config(pipe_obj, "Object.PCM.pcm_caps.capture", true);
		if (err < 0) {
			fprintf(stderr, "Error creating PCM capture caps config\n");
			return err;
		}

		/* get capture config */
		err = snd_config_search(pipe_obj, "Object.PCM.pcm_caps.capture", &new);
		if (err < 0) {
			fprintf(stderr, "No capture caps for PCM\n");
			return err;
		}

		err = tplg_config_make_add(&child, "name", SND_CONFIG_TYPE_STRING, new);
		if (err < 0) {
			fprintf(stderr, "Error creating name config\n");
			return err;
		}

		/* set capture caps name */
		err = snd_config_set_string(child, pcm_id);
		if(err < 0)
			return err;

		/* find pcm caps and add all attributes */
		err = snd_config_search(section_caps, pcm_id, &caps_cfg);
		if (err < 0) {
			fprintf(stderr, "cannot find pcm caps config %s\n", pcm_id);
			return err;
		}

		err = tplg_copy_all_configs(caps_cfg, new);
		if (err < 0) {
			fprintf(stderr, "cannot copy pcm caps config %s\n", pcm_id);
			return err;
		}
direction:
		/* add direction */
		err = tplg_config_make_add(&new, "direction", SND_CONFIG_TYPE_STRING, pipe_obj);
		if (err < 0)
			return err;

		if (playback && capture)
			err = snd_config_set_string(new, "duplex");
		else if (!playback)
			err = snd_config_set_string(new, "capture");
		else
			err = snd_config_set_string(new, "playback");
	}

	return 0;
}

static int tplg_add_dai_widgets(struct tplg_pre_processor *tplg_pp, snd_config_t *dest, const char *sname)
{
	snd_config_iterator_t i, next;
	snd_config_t *section_widget, *n, *stream_name_cfg, *type_cfg, *dai_obj;
	bool playback = false, capture = false;
	int count = 0;
	int err;

	err = snd_config_search(tplg_pp->input_cfg, "SectionWidget", &section_widget);
	if (err < 0) {
		fprintf(stderr, "no section widget found\n");
		return -EINVAL;
	}

	snd_config_for_each(i, next, section_widget) {
		const char *id, *stream_name, *type;

		n = snd_config_iterator_entry(i);

		if (snd_config_get_id(n, &id) < 0)
			continue;

		err = snd_config_search(n, "stream_name", &stream_name_cfg);
		if (err < 0)
			continue;

		if (snd_config_get_string(stream_name_cfg, &stream_name) < 0)
			continue;

		err = snd_config_search(n, "type", &type_cfg);
		if (err < 0)
			continue;

		if (snd_config_get_string(type_cfg, &type) < 0)
			continue;

		if (strcmp(stream_name, sname))
			continue;

		if (!strcmp(type, "dai_in")) {
			char *dai_widget = tplg_snprintf("%s%d", "Object.Widget.dai.", count++);
			err = tplg_create_and_add_config(dest, dai_widget, true);
			if (err < 0) {
				fprintf(stderr, "error creating new playback DAi widget %s\n", dai_widget);
				free(dai_widget);
				return err;
			}
			err = snd_config_search(dest, dai_widget, &dai_obj);
			playback = true;
			err = tplg_string_config_set_and_add(dai_obj, "direction", "playback");
			if (err < 0) {
				fprintf(stderr, "error creating direction for playback DAI widget %s\n", dai_widget);
				free(dai_widget);
				return err;
			}
			free(dai_widget);
		} else {
			char *dai_widget = tplg_snprintf("%s%d", "Object.Widget.dai.", count++);
			err = tplg_create_and_add_config(dest, dai_widget, true);
			if (err < 0) {
				fprintf(stderr, "error creating new capture DAi widget %s\n", dai_widget);
				free(dai_widget);
				return err;
			}
			err = snd_config_search(dest, dai_widget, &dai_obj);
			capture = true;
			err = tplg_string_config_set_and_add(dai_obj, "direction", "capture");
			if (err < 0) {
				fprintf(stderr, "error creating direction for capture DAi widget %s\n", dai_widget);
				free(dai_widget);
				return err;
			}
			free(dai_widget);
		}

		if (err < 0)
			return err;

		err = tplg_copy_all_configs(n, dai_obj);
		if (err < 0) {
			fprintf(stderr, "error copying configs for DAI widget %s\n", id);
			return err;
		}

		err = tplg_add_all_tuples(tplg_pp, n, dai_obj);
		if (err < 0)
			return err;
	}

	/* add direction */
	err = tplg_config_make_add(&type_cfg, "direction", SND_CONFIG_TYPE_STRING, dest);
	if (err < 0)
		return err;

	if (playback && capture)
		err = snd_config_set_string(type_cfg, "duplex");
	else if (!playback)
		err = snd_config_set_string(type_cfg, "capture");
	else
		err = snd_config_set_string(type_cfg, "playback");

	return err;
}

static snd_config_t *tplg_find_scheduler_widget(struct tplg_pre_processor *tplg_pp, char *pipe_id)
{
	snd_config_iterator_t i, next;
	snd_config_t *section_widget, *n, *index_cfg;
	int err;

	err = snd_config_search(tplg_pp->input_cfg, "SectionWidget", &section_widget);
	if (err < 0) {
		fprintf(stderr, "no section widget found\n");
		return NULL;
	}

	snd_config_for_each(i, next, section_widget) {
		const char *id, *index;

		n = snd_config_iterator_entry(i);

		if (snd_config_get_id(n, &id) < 0)
			continue;

		if (!strstr(id, "PIPELINE"))
			continue;

		err = snd_config_search(n, "index", &index_cfg);
		if (err < 0)
			continue;

		if (snd_config_get_string(index_cfg, &index) < 0)
			continue;

		if (!strcmp(index, pipe_id))
			return n;
	}

	return NULL;
}

static int parse_control_data(struct tplg_pre_processor *tplg_pp, const char *name, snd_config_t *dest)
{
	snd_config_iterator_t i, next, i2, next2;
	snd_config_t *section, *n, *n2, *data;
	int err;

	err = snd_config_search(tplg_pp->input_cfg, "SectionControlBytes", &section);
	if (err < 0) {
		fprintf(stderr, "no section controlbytes found\n");
		return -EINVAL;
	}

	snd_config_for_each(i, next, section) {
		const char *id;

		n = snd_config_iterator_entry(i);

		if (snd_config_get_id(n, &id) < 0)
			continue;

		if (strcmp(id, name))
			continue;

		err = snd_config_search(n, "data", &data);
		if (err < 0)
			continue;

		snd_config_for_each(i2, next2, data) {
			snd_config_t *new;
			const char *data_name;
			char *data_obj, *new_name;

			n2 = snd_config_iterator_entry(i2);

			if (snd_config_get_string(n2, &data_name) < 0)
				continue;

			/* extract coeff name */
			if (strstr(data_name, "eq_iir_coef_")) {
				char *str = strchr(data_name, '.');

				if (str) {
					new_name = calloc(1, strlen("eqiir_") + strlen(data_name) - strlen("eq_iir_coef_") - strlen(str));
					if (!new_name)
						return -ENOMEM;
					snprintf(new_name, strlen("eqiir_") + strlen(data_name) - strlen("eqiir_coef_") - strlen(str), "eqiir_%s", data_name + strlen("eq_iir_coef_"));
				}
			} else {
				new_name = strdup(data_name);
				if (!new_name)
					return -ENOMEM;
			}

			data_obj = calloc(1, strlen("Object.Base.data.") + strlen(new_name) + 1);
			if (!data_obj)
				return -ENOMEM;

			snprintf(data_obj, strlen("Object.Base.data.") + strlen(new_name) + 1,
			 "%s%s", "Object.Base.data.", new_name);

			/* add data object for control object */
			err = snd_config_search(dest, data_obj, &new);
			if (err < 0) {
				err = tplg_create_and_add_config(dest, data_obj, true);
				if (err < 0) {
					fprintf(stderr, "cannot create data object %s\n", data_obj);
					free(data_obj);
					return err;
				}
			}

			free(data_obj);
		}
	}

	return 0;
}

static int parse_controls(struct tplg_pre_processor *tplg_pp, snd_config_t *config,
			   char *widget_obj, snd_config_t *dest, const char *control_type)
{
	snd_config_iterator_t i, next;
	snd_config_t *n, *control, *widget;
	int err;

	snd_config_for_each(i, next, config) {
		const char *name, *id;
		char *control_obj;
		int index;

		n = snd_config_iterator_entry(i);

		if (snd_config_get_id(n, &id) < 0)
			continue;

		if (snd_config_get_string(n, &name) < 0)
			continue;

		index = atoi(id) + 1;
		
		/* add widget object if needed */
		err = snd_config_search(dest, widget_obj, &widget);
		if (err < 0) {
			err = tplg_create_and_add_config(dest, widget_obj, true);
			if (err < 0) {
				fprintf(stderr, "cannot create widget object %s\n", widget_obj);
				return err;
			}

			err = snd_config_search(dest, widget_obj, &widget);
			if (err < 0)
				return err;
		}

		control_obj = tplg_snprintf("%s%s.%d", "Object.Control.", control_type, index);

		/* add control for control object */
		err = snd_config_search(widget, control_obj, &control);
		if (err < 0) {
			err = tplg_create_and_add_config(widget, control_obj, true);
			if (err < 0) {
				fprintf(stderr, "cannot create control object %s\n", widget_obj);
				free(control_obj);
				return err;
			}

			err = snd_config_search(widget, control_obj, &control);
			if (err < 0) {
				free(control_obj);
				return err;
			}

			/* add name for control */
			err = tplg_string_config_set_and_add(control, "name", (char *)name);
			if (err < 0) {
				free(control_obj);
				return err;
			}

			if (!strcmp(control_type, "bytes")) {
				err = parse_control_data(tplg_pp, name, control);
				if (err < 0) {
					fprintf(stderr, "error parsing data for bytes %s\n", control_obj);
					free(control_obj);
					return err;
				}
			}
		}

		free(control_obj);
	}

	return 0;
}

static int parse_host_stream_name(struct tplg_pre_processor *tplg_pp, snd_config_t *config, snd_config_t *dest)
{
	snd_config_t *stream_name, *host, *section_caps, *caps, *child;
	const char *name, *sname, *value;
	char *host_obj;
	int err;

	if (snd_config_get_id(config, &name) < 0)
		return 0;

	err = snd_config_search(config, "stream_name", &stream_name);
	if (err < 0) {
		fprintf(stderr, "no stream name for %s\n", name);
		return err;
	}

	if (snd_config_get_string(stream_name, &sname) < 0)
		return -EINVAL;

	if (strstr(sname, "Playback"))
		host_obj = "Object.Widget.host.playback";
	else
		host_obj = "Object.Widget.host.capture";

	err = tplg_create_and_add_config(dest, host_obj, true);
	if (err < 0) {
		fprintf(stderr, "cannot create host widget object %s\n", name);
		return err;
	}

	err = snd_config_search(dest, host_obj, &host);
	if (err < 0)
		return err;

	/* add stream name for host */
	err = tplg_string_config_set_and_add(host, "stream_name", (char *)sname);
	if (err < 0) {
		fprintf(stderr, "failed to add stream name for host %s\n", name);
		return err;
	}
	
	err = snd_config_search(tplg_pp->input_cfg, "SectionPCMCapabilities", &section_caps);
	if (err < 0) {
		fprintf(stderr, "no PCM caps section config\n");
		return err;
	}

	err = snd_config_search(section_caps, sname, &caps);
	if (err < 0) {
		fprintf(stderr, "no PCM caps %s\n", sname);
		return err;
	}

	/* find pcm max rate */
	err = snd_config_search(caps, "rate_max", &child);
	if (err < 0) {
		fprintf(stderr, "no PCM max rate for %s\n", sname);
		return err;
	}

	if (snd_config_get_string(child, &value) < 0) {
		fprintf(stderr, "invalid value for rate max for caps %s\n", sname);
		return -EINVAL;
	}

	/* add rate for pipeline */
	err = tplg_integer_config_set_and_add(dest, "rate", (char *)value);
	if (err < 0) {
		fprintf(stderr, "failed to add stream name for host %s\n", name);
		return err;
	}

	/* find pcm channels */
	err = snd_config_search(caps, "channels_max", &child);
	if (err < 0) {
		fprintf(stderr, "no PCM max rate for %s\n", sname);
		return err;
	}

	if (snd_config_get_string(child, &value) < 0) {
		fprintf(stderr, "invalid value for rate max for caps %s\n", sname);
		return -EINVAL;
	}

	/* add rate for pipeline */
	err = tplg_integer_config_set_and_add(dest, "channels", (char *)value);
	if (err < 0)
		fprintf(stderr, "failed to add stream name for host %s\n", name);

	return err;
}

static int parse_pipe_stream_name(struct tplg_pre_processor *tplg_pp, snd_config_t *config, snd_config_t *dest)
{
	snd_config_t *stream_name, *host;
	const char *name, *sname;
	char *new_sname;
	int err;

	if (snd_config_get_id(config, &name) < 0)
		return 0;

	err = snd_config_search(config, "stream_name", &stream_name);
	if (err < 0) {
		fprintf(stderr, "no stream name for %s\n", name);
		return err;
	}

	if (snd_config_get_string(stream_name, &sname) < 0)
		return -EINVAL;

	err = tplg_create_and_add_config(dest, "Object.Widget.pipeline.1", true);
	if (err < 0) {
		fprintf(stderr, "cannot create host widget object %s\n", name);
		return err;
	}

	err = snd_config_search(dest, "Object.Widget.pipeline.1", &host);
	if (err < 0)
		return err;

	new_sname = tplg_get_tplg2_widget_name((char *)sname);

	/* add name for control */
	err = tplg_string_config_set_and_add(host, "stream_name", new_sname);
	free(new_sname);

	return err;
}

static int parse_virtual_widgets(struct tplg_pre_processor *tplg_pp, snd_config_t *widget)
{
	snd_config_t *type_cfg, *index_cfg, *top, *new;
	const char *widget_name, *index, *type;
	int err;

	if (snd_config_get_id(widget, &widget_name) < 0)
		return 0;

	err = snd_config_search(widget, "type", &type_cfg);
	if (err < 0) {
		fprintf(stderr, "no type for widget %s\n", widget_name);
		return err;
	}

	err = snd_config_search(widget, "index", &index_cfg);
	if (err < 0) {
		fprintf(stderr, "no type for widget %s\n", widget_name);
		return err;
	}

	if (snd_config_get_string(index_cfg, &index) < 0)
		return 0;

	if (snd_config_get_string(type_cfg, &type) < 0)
		return 0;

	if (strcmp(type, "input") && strcmp(type, "output") && strcmp(type, "out_drv"))
		return 0;

	/* add virtual widget to top-level config */
	err = snd_config_search(tplg_pp->output_cfg, "Object.Widget.virtual", &top);
	if (err < 0) {
		err = tplg_create_and_add_config(tplg_pp->output_cfg, "Object.Widget.virtual", true);
		if (err < 0) {
			fprintf(stderr, "Error creating virtual widget config\n");
			return err;
		}

		err = snd_config_search(tplg_pp->output_cfg, "Object.Widget.virtual", &top);

	}

	err = tplg_config_make_add(&new, widget_name, SND_CONFIG_TYPE_COMPOUND, top);
	if (err < 0)
		return err;

	err = tplg_string_config_set_and_add(new, "type", (char *)type);
	if (err < 0)
		return err;

	err = tplg_string_config_set_and_add(new, "index", (char *)index);

	return 0;

}

static int parse_all_virtual_widgets(struct tplg_pre_processor *tplg_pp)
{
	snd_config_iterator_t i, next;
	snd_config_t *section_widget, *n;
	int err;

	err = snd_config_search(tplg_pp->input_cfg, "SectionWidget", &section_widget);
	if (err < 0) {
		fprintf(stderr, "no section widget found\n");
		return -EINVAL;
	}

	snd_config_for_each(i, next, section_widget) {
		const char *name;

		n = snd_config_iterator_entry(i);

		if (snd_config_get_id(n, &name) < 0)
			continue;

		/* add virtual widget */
		err = parse_virtual_widgets(tplg_pp, n);
		if (err < 0) {
			fprintf(stderr, "error parsing virtual widget %s\n", name);
			return err;
		}
	}

	return 0;
}

static int parse_widget_controls(struct tplg_pre_processor *tplg_pp, char *widget_name, snd_config_t *dest)
{
	snd_config_iterator_t i, next;
	snd_config_t *section_widget, *n, *control, *format_cfg, *dest_format;
	int err;

	err = snd_config_search(tplg_pp->input_cfg, "SectionWidget", &section_widget);
	if (err < 0) {
		fprintf(stderr, "no section widget found\n");
		return -EINVAL;
	}

	snd_config_for_each(i, next, section_widget) {
		const char *name, *format;
		char *instance, *widget_obj, *class_name;
		int instance_id;

		n = snd_config_iterator_entry(i);

		if (snd_config_get_id(n, &name) < 0)
			continue;

		if (strcmp(name, widget_name))
			continue;

		class_name = tplg_get_tplg2_widget_classname(name);
		if (!class_name)
			return -EINVAL;

		/* check if it has format value set */
		format_cfg = tplg_find_tuple_config(tplg_pp, n, "SOF_TKN_COMP_FORMAT");
		if (!format_cfg)
			goto instance;

		if (snd_config_get_string(format_cfg, &format) < 0)
			goto instance;

		err = snd_config_search(dest, "format", &dest_format);
		if (err < 0) {
			err = tplg_string_config_set_and_add(dest, "format", (char *)format);
			if (err < 0){
				fprintf(stderr, "failed to add format cfg for %s\n", name);
				return err;
			}
		}

instance:
		instance = strrchr(name, '.');
		if (!instance) {

			if (!strcmp(name, "Dmic0") || !strcmp(name, "Dmic1")) {
				instance = "0";
				widget_obj = tplg_snprintf("%s%s%s", "Object.Widget.", class_name, instance);
				goto widget_obj;
			}

			fprintf(stderr, "No index for widget %s, skip adding controls\n", name);
			if (strstr(name, "PCM")) {
				err = parse_host_stream_name(tplg_pp, n, dest);
				if (err < 0) {
					fprintf(stderr, " failed to add host stream name \n");
					return err;
				}
			}
			continue;
		}

		instance += 1;
		instance_id = atoi(instance) + 1;
		widget_obj = tplg_snprintf("%s%s%d", "Object.Widget.", class_name, instance_id);

widget_obj:
		err = snd_config_search(n, "mixer", &control);
		if (err < 0)
			goto bytes;

		err = parse_controls(tplg_pp, control, widget_obj, dest, "mixer");
		if (err < 0) {
			fprintf(stderr, "failed to parse mixer controls for %s\n", widget_obj);
			goto err;
		}

bytes:
		err = snd_config_search(n, "bytes", &control);
		if (err < 0)
			goto enums;

		err = parse_controls(tplg_pp, control, widget_obj, dest, "bytes");
		if (err < 0) {
			fprintf(stderr, "failed to parse enum controls for %s\n", widget_obj);
			goto err;
		}

enums:
		err = snd_config_search(n, "enum", &control);
		if (err < 0) {
			/* not really an error, no controls found */
			err = 0;
			goto err;
		}

		err = parse_controls(tplg_pp, control, widget_obj, dest, "enum");
		if (err < 0)
			fprintf(stderr, "failed to parse enum controls for %s\n", widget_obj);
err:
		free(widget_obj);
		if (err < 0)
			return err;
	}

	return 0;
}

static int parse_graph_widgets(struct tplg_pre_processor *tplg_pp, snd_config_t *config, snd_config_t *dest)
{
	snd_config_iterator_t i, next;
	snd_config_t *lines, *n2;
	int err;

	err = snd_config_search(config, "lines", &lines);
	if (err < 0)
		return 0;

	snd_config_for_each(i, next, lines) {
		const char *str;
		char *temp1, *temp2, *src, *sink;

		n2 = snd_config_iterator_entry(i);

		if (snd_config_get_string(n2, &str) < 0)
			continue;

		temp1 = strchr(str, ',');
		temp2 = strrchr(str, ' ');

		if (!temp1 || !temp2)
			return -EINVAL;

		src = calloc(1, strlen(str) - strlen(temp1) + 1);
		if (!src)
			return -ENOMEM;

		snprintf(src, strlen(str) - strlen(temp1) + 1, "%s", str);
		sink = temp2 + 1;

		err = parse_widget_controls(tplg_pp, src, dest);
		if (err < 0) {
			fprintf(stderr, "cannot parse source widget %s\n", src);
			goto err;
		}

		err = parse_widget_controls(tplg_pp, sink, dest);
		if (err < 0) {
			fprintf(stderr, "cannot parse sink widget %s\n", sink);
			goto err;
		}
err:
		free(src);
		if (err < 0)
			return err;
	}

	return 0;
}

static int parse_top_level_graph(struct tplg_pre_processor *tplg_pp, snd_config_t *n)
{
	snd_config_iterator_t i, next;
	snd_config_t *lines, *n2, *route_cfg, *new_route;
	const char *id;
	static int top_level_routes = 1;
	int err;

	if (snd_config_get_id(n, &id) < 0)
		return 0;

	err = snd_config_search(n, "lines", &lines);
	if (err < 0)
		return 0;

	snd_config_for_each(i, next, lines) {
		const char *str;
		char *temp1, *temp2, *src, *sink;
		char *new_src, *new_sink, route_id[100];

		n2 = snd_config_iterator_entry(i);

		if (snd_config_get_string(n2, &str) < 0)
			continue;

		temp1 = strchr(str, ',');
		temp2 = strrchr(str, ' ');

		if (!temp1 || !temp2)
			return -EINVAL;

		sink = calloc(1, strlen(str) - strlen(temp1) + 1);
		if (!sink)
			return -ENOMEM;

		snprintf(sink, strlen(str) - strlen(temp1) + 1, "%s", str);
		src = temp2 + 1;

		/* create route object */
		err = snd_config_search(tplg_pp->output_cfg, "Object.Base.route", &route_cfg);
		if (err < 0)
			goto err;

		snprintf(route_id, 100, "%d", top_level_routes++);
		err = tplg_config_make_add_join(&new_route, route_id, SND_CONFIG_TYPE_COMPOUND, route_cfg, false);
		if (err < 0) {
			goto err;
		}

		new_src = tplg_get_tplg2_widget_name(src);
		if (!new_src)
			return -EINVAL;

		err = tplg_string_config_set_and_add(new_route, "source", new_src);
		free(new_src);
		if (err < 0)
			goto err;

		new_sink = tplg_get_tplg2_widget_name(sink);
		if (!new_sink)
			return -EINVAL;

		err = tplg_string_config_set_and_add(new_route, "sink", new_sink);
		free(new_sink);
err:
		free(sink);
		if (err < 0)
			return err;
	}

	return 0;
}

static int parse_graph(struct tplg_pre_processor *tplg_pp, snd_config_t *n)
{
	snd_config_iterator_t i, next;
	snd_config_t *n2, *new, *pipe_cfg, *pipe_class, *pipe_widget;
	const char *id;
	int err;

	snd_config_for_each(i, next, n) {
		char *str, *pipe, *name;

		n2 = snd_config_iterator_entry(i);

		if (snd_config_get_id(n2, &id) < 0)
			continue;

		if (snd_config_get_type(n2) != SND_CONFIG_TYPE_COMPOUND) {
			fprintf(stderr, "compound type expected for %s", id);
			return -EINVAL;
		}

		/* find pipeline graphs */
		str = strstr(id, "pipe");
		if (!str) {
			/* top-level connections */
			err = parse_top_level_graph(tplg_pp, n2);
			if (err < 0)
				return err;
		}

		/* get pipe name */
		pipe = strchr(id, '-');
		if (!pipe)
			continue;

		/* drop pipeline ID from pipe name */
		str = strchr(id, ' ');
		if (!str)
			continue;

		name = calloc(1, strlen(pipe + 1) - strlen(str) + 1);
		if (!name)
			return -ENOMEM;

		snprintf(name, strlen(pipe + 1) - strlen(str) + 1, "%s", pipe + 1);

		/* rename all variants of volume capture */
		if (!strcmp(name, "volume-switch-capture")) {
			free(name);
			name = calloc(1, strlen("volume-capture") + 1);
			if (!name)
				return -ENOMEM;
			snprintf(name, strlen("volume-capture") + 1, "%s", "volume-capture");
		}

		/* rename all variants of highpass-capture */
		if (!strcmp(name, "highpass-capture") || !strcmp(name, "eq-iir-volume-capture-16khz")
		    || !strcmp(name, "highpass-switch-capture")) {
			free(name);
			name = calloc(1, strlen("eq-iir-volume-capture") + 1);
			if (!name)
				return -ENOMEM;
			snprintf(name, strlen("eq-iir-volume-capture") + 1, "%s", "eq-iir-volume-capture");
		}

		err = snd_config_search(tplg_pp->output_cfg, "Object.Pipeline", &pipe_cfg);
		if (err < 0) {
			fprintf(stderr, "Error creating Object config\n");
			free(name);
			return err;
		}

		/* find pipeline class */
		err = snd_config_search(pipe_cfg, name, &pipe_class);
		if (err < 0) {
			err = tplg_config_make_add_join(&pipe_class, name, SND_CONFIG_TYPE_COMPOUND, pipe_cfg, true);
			if (err < 0) {
				fprintf(stderr, "Error creating pipe class config %s\n", name);
				free(name);
				return err;
			}
		}

		err = tplg_config_make_add(&new, str + 1, SND_CONFIG_TYPE_COMPOUND, pipe_class);
		if (err < 0) {
			fprintf(stderr, "Error creating new pipeline class %s instance %s config\n", name, str + 1);
			free(name);
			return err;
		}

		/* find scheduler widget and all tuples */
		pipe_widget = tplg_find_scheduler_widget(tplg_pp, str + 1);
		if (!pipe_widget) {
			fprintf(stderr, "no scheduler widget for pipeline %s\n", name);
			free(name);
			return -EINVAL;
		}

		err = tplg_add_all_tuples(tplg_pp, pipe_widget, new);
		if (err < 0) {
			fprintf(stderr, "error copying tuples for pipeline %s\n", name);
			free(name);
			return err;
		}

		/* add pipeline widget stream name */
		err = parse_pipe_stream_name(tplg_pp, pipe_widget, new);
		if (err < 0) {
			fprintf(stderr, "failed to add pipeline widget stream name for %s\n", name);
			free(name);
			return err;
		}

		err = parse_graph_widgets(tplg_pp, n2, new);
		if (err < 0) {
			fprintf(stderr, "error parsing graph widgets for %s\n", name);
			free(name);
			return err;
		}
		free(name);
	}

	return 0;
}

static int parse_config(struct tplg_pre_processor *tplg_pp, snd_config_t *cfg)
{
	snd_config_iterator_t i, next;
	snd_config_t *n;
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

		if (!strcmp(id, "SectionPCM")) {
			err = parse_pcm(tplg_pp, n);
			if (err < 0)
				return err;
		}

		if (!strcmp(id, "SectionBE")) {
			err = parse_be(tplg_pp, n);
			if (err < 0)
				return err;
		}

		if (!strcmp(id, "SectionGraph")) {
			err = parse_graph(tplg_pp, n);
			if (err < 0)
				return err;
		}
	}

	err = parse_all_virtual_widgets(tplg_pp);
	if (err < 0) {
		fprintf(stderr, "error parsing virtual widget");
		return err;
	}

	return 0;
}

int reverse_config(struct tplg_pre_processor *tplg_pp, char *config, size_t config_size)
{
	snd_input_t *in;
	snd_config_t *top, *obj, *child, *base;
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

	tplg_pp->input_cfg = top;

	err = tplg_config_make_add(&obj, "Object", SND_CONFIG_TYPE_COMPOUND, tplg_pp->output_cfg);
	if (err < 0) {
		fprintf(stderr, "Error creating Object config\n");
		return err;
	}

	err = tplg_config_make_add(&child, "Pipeline", SND_CONFIG_TYPE_COMPOUND, obj);
	if (err < 0) {
		fprintf(stderr, "Error creating Pipeline config\n");
		return err;
	}

	err = tplg_config_make_add(&child, "PCM", SND_CONFIG_TYPE_COMPOUND, obj);
	if (err < 0) {
		fprintf(stderr, "Error creating PCM config\n");
		return err;
	}

	err = tplg_config_make_add(&child, "Dai", SND_CONFIG_TYPE_COMPOUND, obj);
	if (err < 0) {
		fprintf(stderr, "Error creating DAI config\n");
		return err;
	}

	err = tplg_config_make_add(&base, "Base", SND_CONFIG_TYPE_COMPOUND, obj);
	if (err < 0) {
		fprintf(stderr, "Error creating Base config\n");
		return err;
	}
	err = tplg_config_make_add_join(&child, "route", SND_CONFIG_TYPE_COMPOUND, base, true);
	if (err < 0) {
		fprintf(stderr, "Error creating Base.route config\n");
		return err;
	}

	err = parse_config(tplg_pp, top);
	if (err < 0)
		fprintf(stderr, "Unable to pre-process configuration\n");

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

static int tplg_is_inherited_attribute(snd_config_t *parent, snd_config_t *attr, bool *inherited)
{
	snd_config_iterator_t i, next;
	snd_config_t *n;
	const char *attr_id;

	*inherited = false;

	if (snd_config_get_id(attr, &attr_id) < 0)
		return -EINVAL;

	snd_config_for_each(i, next, parent) {
		const char *id;

		n = snd_config_iterator_entry(i);
		if (snd_config_get_id(n, &id) < 0)
			return -EINVAL;;

		/* skip compound attributes */
		if (snd_config_get_type(n) == SND_CONFIG_TYPE_COMPOUND)
			continue;

		/* skip is attribute names dont match */
		if (strcmp(id, attr_id))
			continue;

		if (snd_config_get_type(n) == SND_CONFIG_TYPE_STRING) {
			const char *parent_value, *child_value;

			if (snd_config_get_string(n, &parent_value) < 0)
				return -EINVAL;

			if (snd_config_get_string(attr, &child_value) < 0)
				return -EINVAL;;

			if (!strcmp(parent_value, child_value)) {
				*inherited = true;
				return 0;
			}
		}

		if (snd_config_get_type(n) == SND_CONFIG_TYPE_INTEGER) {
			long parent_value, child_value;

			if (snd_config_get_integer(n, &parent_value) < 0)
				return -EINVAL;

			if (snd_config_get_integer(attr, &child_value) < 0)
				return -EINVAL;;

			if (parent_value == child_value) {
				*inherited = true;
				return 0;
			}
		}
	}

	return 0;

}

/* create top-level topology objects */
int tplg_condense_objects(struct tplg_pre_processor *tplg_pp, snd_config_t *cfg,
			     snd_config_t *parent)
{
	snd_config_iterator_t i, next, i2, next2, i3, next3;
	snd_config_t *n, *n2, *n3, *class_cfg;
	const char *id, *class_type, *class_name;
	int ret;

	if (snd_config_get_id(cfg, &class_type) < 0)
		return 0;

	/* create all objects of the same type and class */
	snd_config_for_each(i, next, cfg) {
		n = snd_config_iterator_entry(i);
		if (snd_config_get_id(n, &class_name) < 0)
			continue;

		if (!strcmp(class_name, "VendorToken")) {
			snd_config_delete(n);
			continue;
		}
		snd_config_for_each(i2, next2, n) {
			snd_config_t *_obj_type, *_obj_class, *_obj;

			n2 = snd_config_iterator_entry(i2);
			if (snd_config_get_id(n2, &id) < 0) {
				fprintf(stderr, "Invalid id for object\n");
				return -EINVAL;
			}

			/* include data elems with no parent */
			if (!strcmp(class_name, "data") && strstr(id, "eqiir"))
				snd_output_printf(tplg_pp->output, "<include/components/%s.conf>\n", id);

			/* create a temp config for object with class type as the root node */
			ret = snd_config_make(&_obj_type, class_type, SND_CONFIG_TYPE_COMPOUND);
			if (ret < 0)
				return ret;

			ret = snd_config_make(&_obj_class, class_name, SND_CONFIG_TYPE_COMPOUND);
			if (ret < 0)
				goto err;

			ret = snd_config_add(_obj_type, _obj_class);
			if (ret < 0) {
				snd_config_delete(_obj_class);
				goto err;
			}

			ret = snd_config_copy(&_obj, n2);
			if (ret < 0)
				goto err;

			ret = snd_config_add(_obj_class, _obj);
			if (ret < 0) {
				snd_config_delete(_obj);
				goto err;
			}

			class_cfg = tplg_class_lookup(tplg_pp, _obj_type);
			if (!class_cfg)
				continue;

			/* iterate through all configs */
			snd_config_for_each(i3, next3, n2) {
				snd_config_t *class_attr_cfg;
				const char *unique_attr_name;
				bool inherited;
				int err;

				n3 = snd_config_iterator_entry(i3);
				if (snd_config_get_id(n3, &id) < 0) {
					fprintf(stderr, "Invalid id for attribute\n");
					ret = -EINVAL;
					break;
				}

				unique_attr_name = tplg_class_get_unique_attribute_name(tplg_pp, class_cfg);
				if (!unique_attr_name) {
					fprintf(stderr, "no unqiue attribute in class\n");
					ret = -EINVAL;
					break;
				}

				/* remove inherited attributes */
				if (parent) {
					ret = tplg_is_inherited_attribute(parent, n3, &inherited);
					if (ret < 0) {
						fprintf(stderr, "error checking if attribute is inherited %s\n", id);
						return ret;
					}

					/* remove inherited attribute */
					if (inherited) {
						snd_config_delete(n3);
						continue;
					}
				}

				/* remove unique attribute */
				if (!strcmp(unique_attr_name, id)) {
					snd_config_delete(n3);
					continue;
				}

				/* iterate through all child objects */
				if (snd_config_get_type(n3) == SND_CONFIG_TYPE_COMPOUND) {
					snd_config_iterator_t i4, next4;
					snd_config_t *child;

					if (strcmp(id, "Object"))
						continue;

					snd_config_for_each(i4, next4, n3) {
						child = snd_config_iterator_entry(i4);

						ret = tplg_condense_objects(tplg_pp, child, _obj);
						if (ret < 0) {
							fprintf(stderr, "cannot condense child objects\n");
							break;
						}
					}

					if (ret < 0)
						break;
					continue;
				}

				err = snd_config_search(class_cfg, id, &class_attr_cfg);
				if (err < 0)
					continue;

				if (snd_config_get_type(n3) == SND_CONFIG_TYPE_STRING) {
					const char *obj_value, *class_value;

					if (snd_config_get_string(n3, &obj_value) < 0) {
						fprintf(stderr, "Invalid value for object attr\n");
						ret = -EINVAL;
						break;
					}

					if (snd_config_get_string(class_attr_cfg, &class_value) < 0) {
						fprintf(stderr, "Invalid value for class string attr %s\n", id);
						ret = -EINVAL;
						break;
					}

					if (!strcmp(obj_value, class_value))
						snd_config_delete(n3);
				}

				if (snd_config_get_type(n3) == SND_CONFIG_TYPE_INTEGER) {
					long obj_value, class_value;

					if (snd_config_get_integer(n3, &obj_value) < 0) {
						fprintf(stderr, "Invalid value for object attr\n");
						ret = -EINVAL;
						break;
					}

					if (snd_config_get_integer(class_attr_cfg, &class_value) < 0) {
						fprintf(stderr, "Invalid value for class integer attr %s\n", id);
						ret = -EINVAL;
						break;
					}

					if (obj_value == class_value) {
						snd_config_delete(n3);
						continue;
					}

					if (obj_value == 0)
						snd_config_delete(n3);
				}
			}
err:
			snd_config_delete(_obj_type);
			if (ret < 0)
				return ret;
		}
	}

	return 0;
}

static int condense_object_configs(struct tplg_pre_processor *tplg_pp, snd_config_t *cfg)
{
	snd_config_iterator_t i, next, i2, next2;
	snd_config_t *n, *n2, *new;
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

			if (snd_config_get_id(n, &id) < 0)
				continue;

			if (snd_config_get_type(n2) != SND_CONFIG_TYPE_COMPOUND) {
				fprintf(stderr, "compound type expected for %s", id);
				return -EINVAL;
			}

			/* pre-process Object instance. Top-level object have no parent */
			err = tplg_condense_objects(tplg_pp, n2, NULL);
			if (err < 0)
				return err;
		}

		err = snd_config_copy(&new, n);
		if (err < 0) {
			fprintf(stderr, "can't copy object config\n");
			return err;
		}

		err = snd_config_add(tplg_pp->output_cfg, new);
		if (err < 0) {
			fprintf(stderr, "can't add object config\n");
			return err;
		}
	}

	return 0;
}

int condense_config(struct tplg_pre_processor *tplg_pp, char *config, size_t config_size)
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

	tplg_pp->input_cfg = top;

	/* condense config */
	err = condense_object_configs(tplg_pp, top);
	if (err < 0) {
		fprintf(stderr, "failed to condense object config: %d\n", err);
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
