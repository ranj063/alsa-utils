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
#include "topology.h"
#include "pre-processor.h"

/* Parse VendorToken object, create the "SectionVendorToken" and save it */
int tplg_build_vendor_token_object(struct tplg_pre_processor *tplg_pp,
			       snd_config_t *obj_cfg, snd_config_t *parent)
{
	snd_config_iterator_t i, next;
	snd_config_t *vtop, *n, *obj;
	const char *name;
	int ret;

	ret = tplg_build_object_from_template(tplg_pp, obj_cfg, &vtop, NULL, false);
	if (ret < 0)
		return ret;

	ret = snd_config_get_id(vtop, &name);
	if (ret < 0)
		return ret;

	/* add the tuples */
	obj = tplg_object_get_instance_config(tplg_pp, obj_cfg);
	snd_config_for_each(i, next, obj) {
		snd_config_t *dst;
		const char *id;

		n = snd_config_iterator_entry(i);

		if (snd_config_get_id(n, &id) < 0)
			continue;

		if (!strcmp(id, "name"))
			continue;

		ret = snd_config_copy(&dst, n);
		if (ret < 0) {
			SNDERR("Error copying config node %s for '%s'\n", id, name);
			return ret;
		}

		ret = snd_config_add(vtop, dst);
		if (ret < 0) {
			SNDERR("Error adding vendortoken %s for %s\n", id, name);
			return ret;
		}
	}

	tplg_pp_config_debug(tplg_pp, vtop);

	return ret;
}

int tplg_parent_update(struct tplg_pre_processor *tplg_pp, snd_config_t *parent,
			  const char *section_name, const char *item_name)
{
	snd_config_iterator_t i, next;
	snd_config_t *child, *cfg, *top, *item_config, *n;
	const char *parent_name;
	char *item_id;
	int ret, id = 0;

	child = tplg_object_get_instance_config(tplg_pp, parent);
	ret = snd_config_search(child, "name", &cfg);
	if (ret < 0) {
		ret = snd_config_get_id(child, &parent_name);
		if (ret < 0) {
			SNDERR("No name config for parent\n");
			return ret;
		}
	} else {
		ret = snd_config_get_string(cfg, &parent_name);
		if (ret < 0) {
			SNDERR("Invalid name for parent\n");
			return ret;
		}
	}

	top = tplg_object_get_section(tplg_pp, parent);
	if (!top)
		return -EINVAL;

	/* get config with name */
	cfg = tplg_find_config(top, parent_name);
	if (!cfg)
		return ret;

	/* get section config */
	if (!strcmp(section_name, "tlv")) {
		ret = tplg_config_make_add(&item_config, section_name,
					  SND_CONFIG_TYPE_STRING, cfg);
		if (ret < 0) {
			SNDERR("Error creating section config widget %s for %s\n",
			       section_name, parent_name);
			return ret;
		}

		return snd_config_set_string(item_config, item_name);
	}

	ret = snd_config_search(cfg, section_name, &item_config);
	if (ret < 0) {
		ret = tplg_config_make_add(&item_config, section_name,
					  SND_CONFIG_TYPE_COMPOUND, cfg);
		if (ret < 0) {
			SNDERR("Error creating section config widget %s for %s\n",
			       section_name, parent_name);
			return ret;
		}
	}

	snd_config_for_each(i, next, item_config) {
		const char *name;

		n = snd_config_iterator_entry(i);
		if (snd_config_get_string(n, &name) < 0)
			continue;

		/* item already exists */
		if (!strcmp(name, item_name))
			return 0;
		id++;
	}

	/* add new item */
	item_id = tplg_snprintf("%d", id);
	if (!item_id)
		return -ENOMEM;

	ret = snd_config_make(&cfg, item_id, SND_CONFIG_TYPE_STRING);
	free(item_id);
	if (ret < 0)
		return ret;

	ret = snd_config_set_string(cfg, item_name);
	if (ret < 0)
		return ret;

	ret = snd_config_add(item_config, cfg);
	if (ret < 0)
		snd_config_delete(cfg);
	else
		tplg_pp_config_debug(tplg_pp, tplg_pp->output_cfg);

	return ret;
}

/* function to create the data config template */
int tplg_create_data_config_template(snd_config_t **dtemplate)
{
	snd_config_t *top, *child;
	int ret;

	ret = snd_config_make(&top, "template", SND_CONFIG_TYPE_COMPOUND);
	if (ret < 0)
		return ret;

	if (ret >= 0)
	ret = tplg_config_make_add(&child, "bytes", SND_CONFIG_TYPE_STRING, top);

	if (ret >= 0)
		*dtemplate = top;
	else
		snd_config_delete(top);

	return ret;
}


/* Parse data object, create the "SectionData" and save it. Only "bytes" data supported for now */
int tplg_build_data_object(struct tplg_pre_processor *tplg_pp, snd_config_t *obj_cfg,
			    snd_config_t *parent)
{
	snd_config_t *dtop;
	const char *name;
	int ret;

	ret = tplg_build_object_from_template(tplg_pp, obj_cfg, &dtop, NULL, false);
	if (ret < 0)
		return ret;

	ret = snd_config_get_id(dtop, &name);
	if (ret < 0)
		return ret;

	return tplg_parent_update(tplg_pp, parent, "data", name);
}
