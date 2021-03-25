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
#include "gettext.h"
#include "topology.h"
#include "pre-processor.h"

/* Process the attribute values provided during object instantiation */
static int tplg_process_attributes(snd_config_t *cfg, struct tplg_object *object)
{
	snd_config_iterator_t i, next;
	struct list_head *pos;
	snd_config_t *n;
	const char *id;
	int ret;

	snd_config_for_each(i, next, cfg) {
		n = snd_config_iterator_entry(i);
		if (snd_config_get_id(n, &id) < 0)
			continue;

		list_for_each(pos, &object->attribute_list) {
			struct tplg_attribute *attr = list_entry(pos, struct tplg_attribute, list);

			/* copy new value set in the object instance */
			if (!strcmp(id, attr->name)) {
				/* cannot update immutable attributes */
				if (attr->constraint.mask & TPLG_CLASS_ATTRIBUTE_MASK_IMMUTABLE) {
					SNDERR("Can't update immutable attribute '%s' in object '%s'\n",
					       attr->name, object->name);
					return -EINVAL;
				}

				ret = tplg_parse_attribute_value(n, &object->attribute_list, true);
				if (ret < 0) {
					SNDERR("Error: %d parsing attribute '%s' for object '%s'\n",
					       ret, attr->name, object->name);
					return ret;
				}
				break;
			}
		}
	}

	return 0;
}

/*
 * Find the attribute with the mask TPLG_CLASS_ATTRIBUTE_MASK_UNIQUE and set its value and type.
 * Only string or integer types are allowed for unique values.
 */
static int tplg_object_set_unique_attribute(struct tplg_object *object, snd_config_t *cfg)
{
	struct tplg_attribute *attr;
	struct list_head *pos;
	const char *id;
	bool found = false;
	int ret;

	list_for_each(pos, &object->attribute_list) {
		attr = list_entry(pos, struct tplg_attribute, list);

		if (attr->constraint.mask & TPLG_CLASS_ATTRIBUTE_MASK_UNIQUE) {
			found = true;
			break;
		}
	}

	/* no unique attribute for object */
	if (!found) {
		SNDERR("No unique attribute set for object %s\n", object->name);
		return -EINVAL;
	}

	/* no need to check return value as it is already checked in tplg_create_object() */
	snd_config_get_id(cfg, &id);

	if (id[0] >= '0' && id[0] <= '9') {
		attr->value.integer = atoi(id);
		attr->type = SND_CONFIG_TYPE_INTEGER;

		/* add attr to config */
		ret = snd_config_make(&attr->cfg, attr->name, SND_CONFIG_TYPE_INTEGER);
		if (ret < 0)
			return ret;

		ret = snd_config_set_integer(attr->cfg, atoi(id));
		if (ret < 0)
			return ret;
	} else {
		snd_strlcpy(attr->value.string, id, SNDRV_CTL_ELEM_ID_NAME_MAXLEN);
		attr->type = SND_CONFIG_TYPE_STRING;

		/* add attr to config */
		ret = snd_config_make(&attr->cfg, attr->name, SND_CONFIG_TYPE_STRING);
		if (ret < 0)
			return ret;

		ret = snd_config_set_string(attr->cfg, id);
		if (ret < 0)
			return ret;
	}

	attr->found = true;

	return 0;
}

/* copy the class attribute values and constraints */
static int tplg_copy_attribute(struct tplg_attribute *attr, struct tplg_attribute *ref_attr)
{
	struct tplg_attribute_ref *ref;

	snd_strlcpy(attr->name, ref_attr->name, SNDRV_CTL_ELEM_ID_NAME_MAXLEN);
	snd_strlcpy(attr->token_ref, ref_attr->token_ref, SNDRV_CTL_ELEM_ID_NAME_MAXLEN);
	attr->found = ref_attr->found;
	attr->param_type = ref_attr->param_type;
	attr->type = ref_attr->type;
	attr->cfg = ref_attr->cfg;

	/* copy value */
	if (ref_attr->found) {
		switch (ref_attr->type) {
		case SND_CONFIG_TYPE_INTEGER:
			attr->value.integer = ref_attr->value.integer;
			break;
		case SND_CONFIG_TYPE_INTEGER64:
			attr->value.integer64 = ref_attr->value.integer64;
			break;
		case SND_CONFIG_TYPE_STRING:
			snd_strlcpy(attr->value.string, ref_attr->value.string,
				    SNDRV_CTL_ELEM_ID_NAME_MAXLEN);
			break;
		case SND_CONFIG_TYPE_REAL:
		{
			attr->value.d = ref_attr->value.d;
			break;
		}
		case SND_CONFIG_TYPE_COMPOUND:
			break;
		default:
			SNDERR("Unsupported type %d for attribute %s\n", attr->type, attr->name);
			return -EINVAL;
		}
	}

	/* copy attribute constraints */
	INIT_LIST_HEAD(&attr->constraint.value_list);
	list_for_each_entry(ref, &ref_attr->constraint.value_list, list) {
		struct tplg_attribute_ref *new_ref = calloc(1, sizeof(*new_ref));

		memcpy(new_ref, ref, sizeof(*ref));
		list_add(&new_ref->list, &attr->constraint.value_list);
	}
	attr->constraint.mask = ref_attr->constraint.mask;
	attr->constraint.min = ref_attr->constraint.min;
	attr->constraint.max = ref_attr->constraint.max;

	return 0;
}

/* create the child object */
static int tplg_create_child_object(struct tplg_pre_processor *tplg_pp, snd_config_t *cfg,
				    struct tplg_object *parent, struct list_head *list,
				    struct tplg_class *class)
{
	snd_config_iterator_t i, next;
	snd_config_t *n;
	const char *id;

	snd_config_for_each(i, next, cfg) {
		struct tplg_object *object;

		n = snd_config_iterator_entry(i);
		if (snd_config_get_id(n, &id) < 0)
			continue;

		object = tplg_create_object(tplg_pp, n, class, parent, list);
		if (!object) {
			SNDERR("Error creating child %s for parent %s\n", id, parent->name);
			return -EINVAL;
		}
	}

	return 0;
}

/* create all child objects of the same class */
int tplg_create_child_objects_type(struct tplg_pre_processor *tplg_pp, snd_config_t *cfg,
				   struct tplg_object *parent, struct list_head *list)
{
	snd_config_iterator_t i, next;
	struct tplg_class *class;
	snd_config_t *n;
	const char *id;
	int ret;

	snd_config_for_each(i, next, cfg) {
		n = snd_config_iterator_entry(i);
		if (snd_config_get_id(n, &id) < 0)
			continue;

		/* check if it is a valid object */
		class = tplg_class_lookup(tplg_pp, id);
		if (!class)
			continue;

		ret = tplg_create_child_object(tplg_pp, n, parent, list, class);
		if (ret < 0) {
			SNDERR("Error creating %s type child object for parent %s\n",
			       class->name, parent->name);
			return ret;
		}
	}

	return 0;
}

/* create child objects that are part of the parent object instance */
static int tplg_create_child_objects(struct tplg_pre_processor *tplg_pp, snd_config_t *cfg,
				     struct tplg_object *parent)
{
	snd_config_iterator_t i, next;
	snd_config_t *n;
	const char *id;
	int ret;

	snd_config_for_each(i, next, cfg) {
		n = snd_config_iterator_entry(i);
		if (snd_config_get_id(n, &id) < 0)
			continue;

		if (strcmp(id, "Object"))
			continue;

		/* create object */
		ret = tplg_create_child_objects_type(tplg_pp, n, parent, &parent->object_list);
		if (ret < 0) {
			SNDERR("Error creating child objects for %s\n", parent->name);
			return ret;
		}
	}

	return 0;
}

/*
 * Create an object  by copying the attribute list, number of arguments, constraints and
 * default attribute values from the class definition.
 */
struct tplg_object *
tplg_create_object(struct tplg_pre_processor *tplg_pp, snd_config_t *cfg, struct tplg_class *class,
		   struct tplg_object *parent, struct list_head *list)
{
	struct tplg_object *object;
	struct list_head *pos;
	const char *id;
	char object_name[SNDRV_CTL_ELEM_ID_NAME_MAXLEN];
	int ret;

	if (!class ||  !list) {
		SNDERR("Invalid class elem or parent list for object\n");
		return NULL;
	}

	/* get object unique attrbute value */
	if (snd_config_get_id(cfg, &id) < 0)
		return NULL;

	ret = snprintf(object_name, SNDRV_CTL_ELEM_ID_NAME_MAXLEN, "%s.%s", class->name, id);
	if (ret > SNDRV_CTL_ELEM_ID_NAME_MAXLEN)
		SNDERR("Warning: object name %s truncated to %d characters\n", object_name,
		       SNDRV_CTL_ELEM_ID_NAME_MAXLEN);

	object = calloc(1, sizeof(*object));
	if (!object) {
		return NULL;
	}

	object->parent = parent;
	object->cfg = cfg;
	object->num_args = class->num_args;
	snd_strlcpy(object->name, object_name, SNDRV_CTL_ELEM_ID_NAME_MAXLEN);
	snd_strlcpy(object->class_name, class->name, SNDRV_CTL_ELEM_ID_NAME_MAXLEN);
	object->type = class->type;
	INIT_LIST_HEAD(&object->attribute_list);
	INIT_LIST_HEAD(&object->object_list);
	list_add_tail(&object->list, list);

	/* copy attributes from class */
	list_for_each(pos, &class->attribute_list) {
		struct tplg_attribute *attr = list_entry(pos, struct tplg_attribute, list);
		struct tplg_attribute *new_attr = calloc(1, sizeof(*attr));

		if (!new_attr)
			return NULL;

		ret = tplg_copy_attribute(new_attr, attr);
		if (ret < 0) {
			SNDERR("Error copying attribute %s\n", attr->name);
			free(new_attr);
			return NULL;
		}
		list_add_tail(&new_attr->list, &object->attribute_list);
	}

	/* set unique attribute for object */
	ret = tplg_object_set_unique_attribute(object, cfg);
	if (ret < 0)
		return NULL;

	/* process object attribute values */
	ret = tplg_process_attributes(cfg, object);
	if (ret < 0)
		return NULL;

	/* create child objects that are part of the object instance */
	ret = tplg_create_child_objects(tplg_pp, cfg, object);
	if (ret < 0) {
		SNDERR("failed to create child objects for %s\n", object->name);
		return NULL;
	}

	return object;
}

int tplg_create_new_object(struct tplg_pre_processor *tplg_pp, snd_config_t *cfg,
			   struct tplg_class *class)
{
	snd_config_iterator_t i, next;
	struct tplg_object *object;
	snd_config_t *n;
	const char *id;

	/* create all objects of the same class type */
	snd_config_for_each(i, next, cfg) {

		n = snd_config_iterator_entry(i);
		if (snd_config_get_id(n, &id) < 0)
			continue;

		/*
		 * Create object by duplicating the attributes and child objects from the class
		 * definiion
		 */
		object = tplg_create_object(tplg_pp, n, class, NULL, &tplg_pp->object_list);
		if (!object) {
			SNDERR("Error creating object %s for class %s\n", id, class->name);
			return -EINVAL;
		}
	}

	return 0;
}

/* create top-level topology objects */
int tplg_create_objects(struct tplg_pre_processor *tplg_pp, snd_config_t *cfg)
{
	struct tplg_class *class;
	const char *id;

	if (snd_config_get_id(cfg, &id) < 0)
		return -EINVAL;

	/* check if the class exists already */
	class = tplg_class_lookup(tplg_pp, id);
	if (!class) {
		SNDERR("No class definition found for %s\n", id);
		return -EINVAL;
	}

	return tplg_create_new_object(tplg_pp, cfg, class);
}
