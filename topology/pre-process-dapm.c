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
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <alsa/input.h>
#include <alsa/output.h>
#include <alsa/conf.h>
#include <alsa/error.h>
#include <alsa/global.h>
#include <alsa/topology.h>
#include <alsa/soc-topology.h>
#include "gettext.h"
#include "topology.h"
#include "pre-processor.h"
#include "../alsactl/list.h"

static int tplg_add_control_config(struct tplg_object *object, snd_config_t *widget,
				   char *control_type)
{
	struct tplg_object *child;
	snd_config_t *control_top;
	int ret, i = 0;

	/* add control config */
	ret = snd_config_make_add(&control_top, control_type,
				  SND_CONFIG_TYPE_COMPOUND, widget);
	if (ret < 0) {
		SNDERR("Cant add %s config for widget %s\n", control_type, object->name);
		return ret;
	}

	/* add control names */
	list_for_each_entry(child, &object->object_list, list) {
		struct tplg_attribute *name;
		snd_config_t *control;
		char id[SNDRV_CTL_ELEM_ID_NAME_MAXLEN];

		if (strcmp(child->class_name, control_type))
			continue;

		/* skip if no name is provided */
		name = tplg_get_attribute_by_name(&child->attribute_list, "name");
		if (!strcmp(name->value.string, ""))
			continue;

		snprintf(id, SNDRV_CTL_ELEM_ID_NAME_MAXLEN, "%d", i++);

		/* add control name to config */
		ret = snd_config_make_add(&control, id, SND_CONFIG_TYPE_STRING, control_top);
		if (ret < 0)
			return ret;

		ret = snd_config_set_string(control, name->value.string);
		if (ret < 0)
			return ret;
	}

	return 0;
}

/* function to create the widget config node */
static int tplg_create_widget_config(snd_config_t *parent, char *name, int pipeline_id)
{
	snd_config_t *wtop, *child;
	int ret;

	ret = snd_config_make_add(&wtop, name, SND_CONFIG_TYPE_COMPOUND, parent);

	if (ret >= 0)
	ret = snd_config_make_add(&child, "index", SND_CONFIG_TYPE_INTEGER, wtop);

	if (ret >= 0)
	ret = snd_config_set_integer(child, pipeline_id);

	if (ret >= 0)
	ret = snd_config_make_add(&child, "type", SND_CONFIG_TYPE_STRING, wtop);

	if (ret >= 0)
	ret = snd_config_make_add(&child, "stream_name", SND_CONFIG_TYPE_STRING, wtop);

	if (ret >= 0)
	ret = snd_config_make_add(&child, "no_pm", SND_CONFIG_TYPE_INTEGER, wtop);

	if (ret >= 0)
	ret = snd_config_make_add(&child, "shift", SND_CONFIG_TYPE_INTEGER, wtop);

	if (ret >= 0)
	ret = snd_config_make_add(&child, "invert", SND_CONFIG_TYPE_INTEGER, wtop);

	if (ret >= 0)
	ret = snd_config_make_add(&child, "subseq", SND_CONFIG_TYPE_INTEGER, wtop);

	if (ret >= 0)
	ret = snd_config_make_add(&child, "event_type", SND_CONFIG_TYPE_INTEGER, wtop);

	if (ret >= 0)
	ret = snd_config_make_add(&child, "event_flags", SND_CONFIG_TYPE_INTEGER, wtop);

	return ret;
}

int tplg_build_widget_object(struct tplg_pre_processor *tplg_pp, struct tplg_object *object)
{
	struct tplg_attribute *pipeline_id, *attr, *name;
	snd_config_t *top, *wtop;
	char object_name[SNDRV_CTL_ELEM_ID_NAME_MAXLEN];
	int ret;

	tplg_pp_debug("Building DAPM widget object: '%s' ...", object->name);

	ret = snd_config_search(tplg_pp->cfg, "SectionWidget", &top);
	if (ret < 0) {
		ret = snd_config_make_add(&top, "SectionWidget", SND_CONFIG_TYPE_COMPOUND, tplg_pp->cfg);
		if (ret < 0) {
			SNDERR("Error creating 'SectionWidget' config\n");
			return ret;
		}
	}

	name = tplg_get_attribute_by_name(&object->attribute_list, "name");
	pipeline_id = tplg_get_attribute_by_name(&object->attribute_list, "pipeline_id");

	if (name)
		snd_strlcpy(object_name, name->value.string, SNDRV_CTL_ELEM_ID_NAME_MAXLEN);
	else
		snd_strlcpy(object_name, object->name, SNDRV_CTL_ELEM_ID_NAME_MAXLEN);

	/* create widget config */
	ret = tplg_create_widget_config(top, object_name, pipeline_id->value.integer);
	if (ret < 0) {
		SNDERR("Failed to create widget config for %s\n", object->name);
		return ret;
	}

	wtop = tplg_find_config(top, object_name);
	if (!wtop) {
		SNDERR("Can't find widget config for %s\n", object->name);
		return -ENOENT;
	}

	/* update widget params from attributes */
	list_for_each_entry(attr, &object->attribute_list, list) {
		if (!attr->found || !strcmp(attr->name, "index"))
			continue;

		ret = tplg_attribute_config_update(wtop, attr);
		if (ret < 0) {
			SNDERR("failed to add config for attribute %s in widget %s\n",
			       attr->name, object->name);
			return ret;
		}
	}

	/* Add widget control config */
	ret = tplg_add_control_config(object, wtop, "mixer");
	if (ret < 0)
		return ret;

	ret = tplg_add_control_config(object, wtop, "bytes");
	if (ret < 0)
		return ret;

	ret = tplg_add_control_config(object, wtop, "enum");
	if (ret < 0)
		return ret;

	tplg_pp_config_debug(tplg_pp, top);

	return 0;
}
