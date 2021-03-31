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
#include "pre-processor.h"

/* find config by id */
snd_config_t *tplg_find_config(snd_config_t *config, char *name)
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

/* set the config node in parent whose ID and type match with the attribute's name and type */
int tplg_attribute_config_update(snd_config_t *parent, struct tplg_attribute *attr)
{
	snd_config_type_t type;
	snd_config_t *cfg;

	/* match config ID and attribute name */
	cfg = tplg_find_config(parent, attr->name);
	if (!cfg)
		return 0;

	/* types must match */
	type = snd_config_get_type(cfg);
	if (type != attr->type)
		return 0;

	/* set config */
	switch (attr->type) {
	case SND_CONFIG_TYPE_INTEGER:
		return snd_config_set_integer(cfg, attr->value.integer);
	case SND_CONFIG_TYPE_INTEGER64:
		return snd_config_set_integer64(cfg, attr->value.integer64);
	case SND_CONFIG_TYPE_STRING:
		return snd_config_set_string(cfg, attr->value.string);
	case SND_CONFIG_TYPE_COMPOUND:
	{
		snd_config_iterator_t i, next;
		snd_config_t *n;
		const char *id;
		int ret;

		snd_config_for_each(i, next, attr->cfg) {
			snd_config_t *dst;
			n = snd_config_iterator_entry(i);

			if (snd_config_get_id(n, &id) < 0)
				continue;

			/* copy the attribute cfg */
			ret = snd_config_copy(&dst, n);
			if (ret < 0) {
				SNDERR("Error copying config node for '%s'\n",
				       attr->name);
				return ret;
			}

			ret = snd_config_add(cfg, dst);
			if (ret < 0) {
				SNDERR("Error adding config node for %s\n",
				       attr->name);
				return ret;
			}
		}
		return 0;
	}
	default:
		return 0;
	}

	return 0;
}
