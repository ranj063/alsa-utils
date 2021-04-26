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
#include <alsa/input.h>
#include <alsa/output.h>
#include <alsa/conf.h>
#include <alsa/error.h>
#include "topology.h"
#include "pre-processor.h"

/* function to create the widget config node */
int tplg_create_widget_config_template(snd_config_t **wtemplate)
{
	snd_config_t *wtop, *child;
	int ret;

	ret = snd_config_make(&wtop, "template", SND_CONFIG_TYPE_COMPOUND);
	if (ret < 0)
		return ret;

	ret = tplg_config_make_add(&child, "index", SND_CONFIG_TYPE_INTEGER, wtop);

	if (ret >= 0)
	ret = tplg_config_make_add(&child, "type", SND_CONFIG_TYPE_STRING, wtop);

	if (ret >= 0)
	ret = tplg_config_make_add(&child, "stream_name", SND_CONFIG_TYPE_STRING, wtop);

	if (ret >= 0)
	ret = tplg_config_make_add(&child, "no_pm", SND_CONFIG_TYPE_INTEGER, wtop);

	if (ret >= 0)
	ret = tplg_config_make_add(&child, "shift", SND_CONFIG_TYPE_INTEGER, wtop);

	if (ret >= 0)
	ret = tplg_config_make_add(&child, "invert", SND_CONFIG_TYPE_INTEGER, wtop);

	if (ret >= 0)
	ret = tplg_config_make_add(&child, "subseq", SND_CONFIG_TYPE_INTEGER, wtop);

	if (ret >= 0)
	ret = tplg_config_make_add(&child, "event_type", SND_CONFIG_TYPE_INTEGER, wtop);

	if (ret >= 0)
	ret = tplg_config_make_add(&child, "event_flags", SND_CONFIG_TYPE_INTEGER, wtop);

	if (ret >= 0)
		*wtemplate = wtop;
	else
		snd_config_delete(wtop);

	return ret;
}

/* create new mixer config template */
int tplg_create_mixer_template(snd_config_t **mixer_template)
{
	snd_config_t *top, *child;
	int ret;

	ret = snd_config_make(&top, "template", SND_CONFIG_TYPE_COMPOUND);
	if (ret < 0)
		return ret;

	ret = tplg_config_make_add(&child, "index", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
	ret = tplg_config_make_add(&child, "max", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
	ret = tplg_config_make_add(&child, "invert", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
	ret = tplg_config_make_add(&child, "access", SND_CONFIG_TYPE_COMPOUND, top);

	if (ret < 0)
		snd_config_delete(top);

	*mixer_template = top;
	return ret;
}

/* create new mixer config template */
int tplg_create_bytes_template(snd_config_t **bytes_template)
{
	snd_config_t *top, *child;
	int ret;

	ret = snd_config_make(&top, "template", SND_CONFIG_TYPE_COMPOUND);
	if (ret < 0)
		return ret;

	if (ret >= 0)
	ret = tplg_config_make_add(&child, "index", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
	ret = tplg_config_make_add(&child, "base", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
	ret = tplg_config_make_add(&child, "num_regs", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
	ret = tplg_config_make_add(&child, "max", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
	ret = tplg_config_make_add(&child, "mask", SND_CONFIG_TYPE_COMPOUND, top);

	if (ret < 0)
		snd_config_delete(top);

	*bytes_template = top;

	return ret;
}

/* create scale config template */
int tplg_create_scale_template(snd_config_t **scale_template)
{
	snd_config_t *top, *child;
	int ret;

	ret = snd_config_make(&top, "template", SND_CONFIG_TYPE_COMPOUND);
	if (ret < 0)
		return ret;

	if (ret >= 0)
	ret = tplg_config_make_add(&child, "mute", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
	ret = tplg_config_make_add(&child, "min", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
	ret = tplg_config_make_add(&child, "step", SND_CONFIG_TYPE_INTEGER, top);

	if (ret < 0)
		snd_config_delete(top);

	*scale_template = top;

	return ret;
}

/* create new ops config template */
int tplg_create_ops_template(snd_config_t **ops_template)
{
	snd_config_t *top, *child;
	int ret;

	ret = snd_config_make(&top, "template", SND_CONFIG_TYPE_COMPOUND);
	if (ret < 0)
		return ret;

	if (ret >= 0)
	ret = tplg_config_make_add(&child, "info", SND_CONFIG_TYPE_STRING, top);

	if (ret >= 0)
	ret = tplg_config_make_add(&child, "get", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
	ret = tplg_config_make_add(&child, "put", SND_CONFIG_TYPE_INTEGER, top);

	if (ret < 0)
		snd_config_delete(top);

	*ops_template = top;

	return ret;
}

/* create new channel config template */
int tplg_create_channel_template(snd_config_t **ctemplate)
{
	snd_config_t *top, *child;
	int ret;

	ret = snd_config_make(&top, "template", SND_CONFIG_TYPE_COMPOUND);
	if (ret < 0)
		return ret;

	if (ret >= 0)
	ret = tplg_config_make_add(&child, "reg", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
	ret = tplg_config_make_add(&child, "shift", SND_CONFIG_TYPE_INTEGER, top);

	if (ret < 0)
		snd_config_delete(top);

	*ctemplate = top;

	return ret;
}

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

	tplg_pp_config_debug(tplg_pp, tplg_pp->output_cfg);

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
