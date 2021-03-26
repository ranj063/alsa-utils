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

/*
 * Child objects could have arguments inherited from the parent. Update the name now that the
 * parent has been instantiated and values updated.
 */
static int tplg_update_object_name_from_args(struct tplg_object *object)
{
	struct list_head *pos;
	char string[SNDRV_CTL_ELEM_ID_NAME_MAXLEN];
	int ret;

	snprintf(string, SNDRV_CTL_ELEM_ID_NAME_MAXLEN, "%s", object->class_name);

	list_for_each(pos, &object->attribute_list) {
		struct tplg_attribute *attr = list_entry(pos, struct tplg_attribute, list);
		char new_str[SNDRV_CTL_ELEM_ID_NAME_MAXLEN];

		if (attr->param_type != TPLG_CLASS_PARAM_TYPE_ARGUMENT)
			continue;

		switch (attr->type) {
		case SND_CONFIG_TYPE_INTEGER:
			ret = snprintf(new_str, SNDRV_CTL_ELEM_ID_NAME_MAXLEN, "%s.%ld",
				 string, attr->value.integer);
			if (ret > SNDRV_CTL_ELEM_ID_NAME_MAXLEN) {
				SNDERR("Object name too long for %s\n", object->name);
				return -EINVAL;
			}
			snd_strlcpy(string, new_str, SNDRV_CTL_ELEM_ID_NAME_MAXLEN);
			break;
		case SND_CONFIG_TYPE_STRING:
			ret = snprintf(new_str, SNDRV_CTL_ELEM_ID_NAME_MAXLEN, "%s.%s",
				 string, attr->value.string);
			if (ret > SNDRV_CTL_ELEM_ID_NAME_MAXLEN) {
				SNDERR("Object name too long for %s\n", object->name);
				return -EINVAL;
			}
			snd_strlcpy(string, new_str, SNDRV_CTL_ELEM_ID_NAME_MAXLEN);
			break;
		default:
			break;
		}
	}

	snd_strlcpy(object->name, string, SNDRV_CTL_ELEM_ID_NAME_MAXLEN);

	return 0;
}

/*
 * Check if all mandatory attributes have been provided with a value and
 * translate valid values to tuple values if needed.
 */
static int tplg_object_attributes_sanity_check(struct tplg_object *object)
{
	struct list_head *pos;
	int ret;

	/* sanity check for object attributes */
	list_for_each(pos, &object->attribute_list) {
		struct tplg_attribute *attr = list_entry(pos, struct tplg_attribute, list);

		/* check if mandatory and value provided */
		if (attr->constraint.mask & TPLG_CLASS_ATTRIBUTE_MASK_MANDATORY && !attr->found) {
			SNDERR("Mandatory attribute %s not found for object %s\n", attr->name,
			       object->name);
			return -EINVAL;
		}

		if (attr->constraint.mask & TPLG_CLASS_ATTRIBUTE_MASK_DEPRECATED && attr->found) {
			SNDERR("Attibrute %s decprecated\n", attr->name);
			return -EINVAL;
		}

		/* translate string values to integer if needed to be added to private data */
		switch (attr->type) {
		case SND_CONFIG_TYPE_STRING:
		{
			struct list_head *pos1;

			/* skip attributes with no pre-defined valid values */
			if (list_empty(&attr->constraint.value_list))
				continue;

			/* Translate the string value to integer if needed */
			list_for_each(pos1, &attr->constraint.value_list) {
				struct tplg_attribute_ref *v;

				v = list_entry(pos1, struct tplg_attribute_ref, list);

				if (!strcmp(attr->value.string, v->string)) {
					if (v->value != -EINVAL) {
						attr->value.integer = v->value;
						attr->type = SND_CONFIG_TYPE_INTEGER;
					}
					break;
				}
			}
			break;
		}
		default:
			break;
		}
	}

	/* set new object name */
	ret = tplg_update_object_name_from_args(object);
	if (ret < 0)
		return ret;

	/* recursively check all child objects */
	list_for_each(pos, &object->object_list) {
		struct tplg_object *child = list_entry(pos, struct tplg_object, list);
		int ret;

		ret = tplg_object_attributes_sanity_check(child);
		if (ret < 0)
			return ret;
	}

	return 0;
}

/*
 * Set child object attribute by passing the class_name and unique attribute value.
 * For example to set the mixer.0 name from a pga object,
 * Object.pga {
 * 	mixer.0.name	"Master Volume Control"
 * }
 * 
 * or to set the channel name in the mixer:
 * Object.pga {
 * 	mixer.0.channel.0.name	"flw"
 * }
 */
static int tplg_set_child_attributes(struct tplg_pre_processor *tplg_pp, snd_config_t *cfg,
				     struct tplg_object *object)
{
	snd_config_iterator_t i, next;
	snd_config_t *n;
	int ret;

	snd_config_for_each(i, next, cfg) {
		snd_config_iterator_t first, second;
		snd_config_t *first_cfg, *second_cfg;
		struct tplg_class *class;
		struct tplg_object *child;
		const char *class_name, *index_str, *attribute_name;

		n = snd_config_iterator_entry(i);
		if (snd_config_get_id(n, &class_name) < 0)
			continue;

		if (snd_config_get_type(n) != SND_CONFIG_TYPE_COMPOUND)
	                continue;

		/* check if it is a valid class name */
		class = tplg_class_lookup(tplg_pp, class_name);
		if (!class)
			continue;

		/* get index */
		first = snd_config_iterator_first(n);
		first_cfg = snd_config_iterator_entry(first);
		if (snd_config_get_id(first_cfg, &index_str) < 0)
			continue;

		if (snd_config_get_type(first_cfg) != SND_CONFIG_TYPE_COMPOUND) {
			SNDERR("No attribute name for child %s.%s\n", class_name, index_str);
			return -EINVAL;
		}

		/* the next node can either be an attribute name or a child class */
		second = snd_config_iterator_first(first_cfg);
		second_cfg = snd_config_iterator_entry(second);
		if (snd_config_get_id(second_cfg, &attribute_name) < 0)
			continue;

		/* get object of type class_name and unique attribute value */
		child = tplg_object_lookup_in_list(&object->object_list, class_name,
						   (char *)index_str);
		if (!child) {
			SNDERR("No child %s.%s found for object %s\n",
				class_name, index_str, object->name);
			return -EINVAL;
		}

		/*
		 * if the second conf node is an attribute name, set the value but do not
		 * override the object value if already set.
		 */
		if (snd_config_get_type(second_cfg) != SND_CONFIG_TYPE_COMPOUND) {
			ret = tplg_parse_attribute_value(second_cfg, &child->attribute_list, false);

			if (ret < 0) {
				SNDERR("Failed to set attribute for object %s\n", object->name);
				return ret;
			}
			continue;
		}

		/* otherwise pass it down to the child object */
		ret = tplg_set_child_attributes(tplg_pp, first_cfg, child);
		if (ret < 0)
			return ret;
	}

	return 0;
}

/* look up object based on class type and unique attribute value in a list */
struct tplg_object *tplg_object_lookup_in_list(struct list_head *list, const char *class_name,
					       char *input)
{
        struct tplg_object *object;

        if (!class_name)
                return NULL;

        list_for_each_entry(object, list, list) {
		struct tplg_attribute *attr;
		bool found = false;

		/* check if class_name natches */
                if (strcmp(object->class_name, class_name))
			continue;

		/* find attribute with TPLG_CLASS_ATTRIBUTE_MASK_UNIQUE mask */
		list_for_each_entry(attr, &object->attribute_list, list) {
			if (attr->constraint.mask & TPLG_CLASS_ATTRIBUTE_MASK_UNIQUE) {
				found = true;
				break;
			}
		}

		if (!found)
			continue;

		/* check if the value matches based on type */
		switch (attr->type) {
		case SND_CONFIG_TYPE_INTEGER:
			if (attr->value.integer == atoi(input))
				return object;
			break;
		case SND_CONFIG_TYPE_STRING:
			if (!strcmp(attr->value.string, input))
				return object;
			break;
		default:
			break;
		}
        }

        return NULL;
}

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
	} else {
		snd_strlcpy(attr->value.string, id, SNDRV_CTL_ELEM_ID_NAME_MAXLEN);
		attr->type = SND_CONFIG_TYPE_STRING;
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
	attr->cfg = ref_attr->cfg;
	attr->type = ref_attr->type;

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

/* Copy object from class definition */
static int tplg_copy_object(struct tplg_object *src, struct tplg_object *parent)
{
	struct tplg_object *dest;
	struct list_head *pos;
	int ret;

	if (!src) {
		SNDERR("Invalid src object\n");
		return -EINVAL;
	}

	/* init new object */
	dest = calloc(1, sizeof(*dest));
	if (!dest)
		return -ENOMEM;

	dest->num_args = src->num_args;
	snd_strlcpy(dest->name, src->name, SNDRV_CTL_ELEM_ID_NAME_MAXLEN);
	snd_strlcpy(dest->class_name, src->class_name, SNDRV_CTL_ELEM_ID_NAME_MAXLEN);
	dest->type = src->type;
	dest->cfg = src->cfg;
	dest->parent = parent;
	INIT_LIST_HEAD(&dest->attribute_list);
	INIT_LIST_HEAD(&dest->object_list);
	list_add_tail(&dest->list, &parent->object_list);

	/* copy attributes from class child object */
	list_for_each(pos, &src->attribute_list) {
		struct tplg_attribute *attr = list_entry(pos, struct tplg_attribute, list);
		struct tplg_attribute *new_attr = calloc(1, sizeof(*attr));

		if (!new_attr)
			return -ENOMEM;

		ret = tplg_copy_attribute(new_attr, attr);
		if (ret < 0) {
			SNDERR("Error copying attribute %s from object %s\n", attr->name,
				src->name);
			free(new_attr);
			return -ENOMEM;
		}
		list_add_tail(&new_attr->list, &dest->attribute_list);
	}

	/* copy the child objects recursively */
	list_for_each(pos, &src->object_list) {
		struct tplg_object *child = list_entry(pos, struct tplg_object, list);

		ret = tplg_copy_object(child, dest);
		if (ret < 0) {
			SNDERR("error copying child object %s\n", child->name);
			return ret;
		}
		child->parent = dest;
	}

	return 0;
}

/* class definitions may have pre-defined objects. Copy these into the object */
static int tplg_copy_child_objects(struct tplg_class *class, struct tplg_object *object)
{
	struct tplg_object *obj;
	int ret;

	/* copy class child objects */
	list_for_each_entry(obj, &class->object_list, list) {
		ret = tplg_copy_object(obj, object);
		if (ret < 0)
			return ret;
	}

	return 0;
}

/* update attributes inherited from parent */
static int tplg_update_attributes_from_parent(struct tplg_object *object,
					    struct tplg_object *ref_object)
{
	struct list_head *pos, *pos1;

	/* update object attribute values from reference object */
	list_for_each(pos, &object->attribute_list) {
		struct tplg_attribute *attr =  list_entry(pos, struct tplg_attribute, list);

		/* parent cannot override child object's attribute values */
		if (attr->found)
			continue;

		list_for_each(pos1, &ref_object->attribute_list) {
			struct tplg_attribute *ref_attr;

			ref_attr = list_entry(pos1, struct tplg_attribute, list);
			if (!ref_attr->found)
				continue;

			if (!strcmp(attr->name, ref_attr->name)) {
				switch (ref_attr->type) {
				case SND_CONFIG_TYPE_INTEGER:
					attr->value.integer = ref_attr->value.integer;
					attr->type = ref_attr->type;
					break;
				case SND_CONFIG_TYPE_INTEGER64:
					attr->value.integer64 = ref_attr->value.integer64;
					attr->type = ref_attr->type;
					break;
				case SND_CONFIG_TYPE_STRING:
					snd_strlcpy(attr->value.string,
						    ref_attr->value.string,
						    SNDRV_CTL_ELEM_ID_NAME_MAXLEN);
					attr->type = ref_attr->type;
					break;
				case SND_CONFIG_TYPE_REAL:
					attr->value.d = ref_attr->value.d;
					attr->type = ref_attr->type;
					break;
				default:
					SNDERR("Unsupported type %d for attribute %s\n",
						attr->type, attr->name);
					return -EINVAL;
				}
				attr->found = true;
			}
		}
	}

	return 0;
}

/* Propdagate the updated attribute values to child o bjects */
static int tplg_process_child_objects(struct tplg_object *parent)
{
	struct list_head *pos;
	int ret;

	list_for_each(pos, &parent->object_list) {
		struct tplg_object *object = list_entry(pos, struct tplg_object, list);

		/* update attribute values inherited from parent */
		ret = tplg_update_attributes_from_parent(object, parent);
		if (ret < 0) {
			SNDERR("failed to update arguments for %s\n", object->name);
			return ret;
		}

		/* now process its child objects */
		ret = tplg_process_child_objects(object);
		if (ret < 0) {
			SNDERR("Cannot update child object for %s\n", object->name);
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

	/* now copy child objects from the class definition */
	ret = tplg_copy_child_objects(class, object);
	if (ret < 0) {
		SNDERR("Failed to create DAI object for %s\n", object->name);
		return NULL;
	}

	/* create child objects that are part of the object instance */
	ret = tplg_create_child_objects(tplg_pp, cfg, object);
	if (ret < 0) {
		SNDERR("failed to create child objects for %s\n", object->name);
		return NULL;
	}

	/* pass the object attributes to its child objects */
	ret = tplg_process_child_objects(object);
	if (ret < 0) {
		SNDERR("failed to create child objects for %s\n", object->name);
		return NULL;
	}

	/* set child object attributes from parent object */
	ret = tplg_set_child_attributes(tplg_pp, cfg, object);
	if (ret < 0) {
		SNDERR("failed to set child attributes for %s\n", object->name);
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
	int ret;

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

		/*
		 * Check if all mandatory values are present and translate valid values to
		 * tuple values.
		 */
		ret = tplg_object_attributes_sanity_check(object);
		if (ret < 0) {
			SNDERR("Object %s failed sanity check\n", object->name);
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
