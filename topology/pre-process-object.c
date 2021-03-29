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

static int tplg_pp_add_object_tuple_sections(struct tplg_pre_processor *tplg_pp,
					      struct tplg_object *object)
{
	struct tplg_attribute_set *set;
	snd_config_t *top;
	int ret;

	tplg_pp_debug("Building vendor tuples sections for object: '%s' ...", object->name);

	ret = snd_config_search(tplg_pp->cfg, "SectionVendorTuples", &top);
	if (ret < 0) {
		ret = snd_config_make_add(&top, "SectionVendorTuples",
					  SND_CONFIG_TYPE_COMPOUND, tplg_pp->cfg);
		if (ret < 0) {
			SNDERR("Error creating SectionVendorTuples config\n");
			return ret;
		}
	}

	list_for_each_entry(set, &object->attribute_set_list, list) {
		struct tplg_attribute *attr;
		snd_config_t *vcfg, *child, *tuples;
		char *data_name, *token_name, *type;
		size_t len;

		/* create the new VendorToken config */
		len = strlen(object->name) + strlen(set->token_ref) + 2;
		data_name = calloc(1, len + 1);
		snprintf(data_name, len + 1, "%s.%s", object->name, set->token_ref);

		type = strchr(set->token_ref, '.');

		ret = snd_config_make_add(&vcfg, data_name, SND_CONFIG_TYPE_COMPOUND,
					  top);
		if (ret < 0) {
			SNDERR("Error creating vendor tuples config for '%s'\n", object->name);
			return ret;
		}

		/* add token config */
		len = strlen(set->token_ref) - strlen(type);
		token_name = calloc(1, len + 1);
		snprintf(token_name, len + 1, "%s", set->token_ref);
		ret = snd_config_make_add(&child, "tokens", SND_CONFIG_TYPE_STRING,
					  vcfg);
		if (ret < 0) {
			SNDERR("Error creating tokens config for '%s'\n", object->name);
			return ret;
		}

		ret = snd_config_set_string(child, token_name);
		if (ret < 0) {
			SNDERR("Error setting tokens config for '%s'\n", object->name);
			return ret;
		}

		/* add tuples section */
		ret = snd_config_make_add(&tuples, "tuples", SND_CONFIG_TYPE_COMPOUND,
					  vcfg);
		if (ret < 0) {
			SNDERR("Error creating tuples config for '%s'\n", object->name);
			return ret;
		}

		ret = snd_config_make_add(&child, type + 1, SND_CONFIG_TYPE_COMPOUND,
					  tuples);
		if (ret < 0) {
			SNDERR("Error creating tuples config for '%s'\n", object->name);
			return ret;
		}

		/* add each token-tuple pair */
		list_for_each_entry(attr, &set->attribute_list, list) {
			snd_config_t *dst;
			const char *id;

			if (!attr->cfg) {
				SNDERR("no config for %s\n", attr->name);
				continue;
			}

			if (snd_config_get_id(attr->cfg, &id) < 0)
				continue;

			/* copy the attribute cfg */
			ret = snd_config_copy(&dst, attr->cfg);
			if (ret < 0) {
				SNDERR("Error copying config node %s for '%s'\n", id, object->name);
				return ret;
			}

			ret = snd_config_add(child, dst);
			if (ret < 0) {
				SNDERR("Error adding vendor token %s for %s\n", id, object->name);
				return ret;
			}
		}
	}

	return 0;
}

static int tplg_pp_add_object_data_sections(struct tplg_pre_processor *tplg_pp,
					    struct tplg_object *object)
{
	struct tplg_attribute_set *set;
	snd_config_t *top;
	int ret;

	tplg_pp_debug("Building data sections for object: '%s' ...", object->name);

	ret = snd_config_search(tplg_pp->cfg, "SectionData", &top);
	if (ret < 0) {
		ret = snd_config_make_add(&top, "SectionData", SND_CONFIG_TYPE_COMPOUND,
					  tplg_pp->cfg);
		if (ret < 0) {
			SNDERR("Failed to add SectionData\n");
			return ret;
		}
	}

	/* first add the data section to the object */
	list_for_each_entry(set, &object->attribute_set_list, list) {
		snd_config_t *child, *data_cfg;
		char *data_name;
		size_t len;

		len = strlen(object->name) + strlen(set->token_ref) + 2;
		data_name = calloc(1, len + 1);
		snprintf(data_name, len + 1, "%s.%s", object->name, set->token_ref);

		ret = snd_config_make_add(&data_cfg, data_name, SND_CONFIG_TYPE_COMPOUND, top);
		if (ret < 0)
			return ret;

		ret = snd_config_make_add(&child, "tuples", SND_CONFIG_TYPE_STRING, data_cfg);
		if (ret < 0) {
			SNDERR("error adding data ref for %s\n", object->name);
			return ret;
		}

		ret = snd_config_set_string(child, data_name);
		if (ret < 0) {
			SNDERR("error setting tuples ref for %s\n", object->name);
			return ret;
		}
	}

	ret = tplg_pp_add_object_tuple_sections(tplg_pp, object);
	if (ret < 0)
		SNDERR("Failed to add SectionVendorTuples configs for widget %s\n",
		       object->name);

	return 0;
}

int tplg_pp_add_object_data(struct tplg_pre_processor *tplg_pp, struct tplg_object *object,
			    snd_config_t *top)
{
	struct tplg_attribute_set *set;
	snd_config_t *data_cfg;
	int ret, i = 0;

	/* add data config to top */
	ret = snd_config_make_add(&data_cfg, "data", SND_CONFIG_TYPE_COMPOUND, top);
	if (ret < 0) {
		SNDERR("error creating data config for %s\n", object->name);
	}

	/* add data reference for each attribute set in the object */
	list_for_each_entry(set, &object->attribute_set_list, list) {
		snd_config_t *child;
		char id[SNDRV_CTL_ELEM_ID_NAME_MAXLEN];
		char *data_name;
		size_t len;

		snprintf(id, SNDRV_CTL_ELEM_ID_NAME_MAXLEN, "%d", i++);
		ret = snd_config_make_add(&child, id, SND_CONFIG_TYPE_STRING, data_cfg);
		if (ret < 0) {
			SNDERR("error adding data ref for %s\n", object->name);
			return ret;
		}

		len = strlen(object->name) + strlen(set->token_ref) + 1;
		data_name = calloc(1, len + 1);
		snprintf(data_name, len + 1, "%s.%s", object->name, set->token_ref);
		ret = snd_config_set_string(child, data_name);
		if (ret < 0) {
			SNDERR("error setting data ref for %s\n", object->name);
			return ret;
		}
	}

	/* save the SectionData configs for each attribute set */
	ret = tplg_pp_add_object_data_sections(tplg_pp, object);
	if (ret < 0)
		SNDERR("Failed to save SectionData for widget %s\n", object->name);

	return 0;
}

static int tplg_copy_attribute(struct tplg_attribute *attr, struct tplg_attribute *ref_attr);

static int tplg_get_object_attribute_set(struct tplg_object *object,
					 struct tplg_attribute_set **out,
					 const char *token_ref)
{
	struct tplg_attribute_set *set;

	/* return set if found */
	list_for_each_entry(set, &object->attribute_set_list, list)
		if (!strcmp(set->token_ref, token_ref)) {
			*out = set;
			return 0;
		}

	/* else create a new set and add it to the object's attribute_set_list */
	set = calloc(1, sizeof(struct tplg_attribute_set));
	if (!set)
		return -ENOMEM;

	snd_strlcpy(set->token_ref, token_ref, SNDRV_CTL_ELEM_ID_NAME_MAXLEN);
	INIT_LIST_HEAD(&set->attribute_list);
	list_add_tail(&set->list, &object->attribute_set_list);
	*out = set;
	return 0;
}


/* Build attribute sets to add SectionVendorTuples */
int tplg_build_object_attribute_sets(struct tplg_object *object)
{
	struct tplg_attribute *attr;
	int ret;

	list_for_each_entry(attr, &object->attribute_list, list) {
		struct tplg_attribute *new_attr;
		struct tplg_attribute_set *set;

		/* skip attributes that dont have a token_ref or value */
		if (!strcmp(attr->token_ref, "") || !attr->found)
			continue;

		/* get attribute set if it exists already or create one */
		ret = tplg_get_object_attribute_set(object, &set, attr->token_ref);
		if (ret < 0) {
			SNDERR("Can't create attribute set for '%s'\n", object->name);
			return ret;
		}

		new_attr = calloc(1, sizeof(struct tplg_attribute));
		if (!new_attr)
			return -ENOMEM;

		ret = tplg_copy_attribute(new_attr, attr);
		if (ret < 0) {
			SNDERR("Cannot add attribute to set for object %s\n", object->name);
			return ret;
		}

		list_add(&new_attr->list, &set->attribute_list);
	}

	return 0;
}

static const struct build_function_map object_build_map[] = {
	{SND_TPLG_CLASS_TYPE_BASE, "data", &tplg_build_data_object},
	{SND_TPLG_CLASS_TYPE_BASE, "manifest", &tplg_build_manifest_object},
	{SND_TPLG_CLASS_TYPE_BASE, "VendorToken", &tplg_build_vendor_token_object},
	{SND_TPLG_CLASS_TYPE_BASE, "tlv", &tplg_pp_build_tlv_object},
	{SND_TPLG_CLASS_TYPE_BASE, "hw_config", &tplg_pp_build_hw_cfg_object},
	{SND_TPLG_CLASS_TYPE_BASE, "route", &tplg_build_dapm_route_object},
	{SND_TPLG_CLASS_TYPE_WIDGET, "", &tplg_build_widget_object},
	{SND_TPLG_CLASS_TYPE_CONTROL, "mixer", &tplg_build_mixer_control},
	{SND_TPLG_CLASS_TYPE_CONTROL, "bytes", &tplg_build_bytes_control},
	{SND_TPLG_CLASS_TYPE_DAI, "", &tplg_build_dai_object},
	{SND_TPLG_CLASS_TYPE_PCM, "pcm", &tplg_build_pcm_object},
};

static build_func tplg_pp_lookup_object_build_func(struct tplg_object *object)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(object_build_map); i++) {
		if (object->type == SND_TPLG_CLASS_TYPE_WIDGET &&
		    object_build_map[i].class_type == SND_TPLG_CLASS_TYPE_WIDGET)
			return object_build_map[i].builder;

		if (object->type == SND_TPLG_CLASS_TYPE_DAI &&
		    object_build_map[i].class_type == SND_TPLG_CLASS_TYPE_DAI)
			return object_build_map[i].builder;

		/* for SND_TPLG_CLASS_TYPE_BASE type objects, also match the object->class_name */
		if (object->type == object_build_map[i].class_type &&
		    !strcmp(object_build_map[i].class_name, object->class_name))
			return object_build_map[i].builder;
	}

	return NULL;
}

/* build the object and its child objects recursively */
static int tplg_build_object(struct tplg_pre_processor *tplg_pp, struct tplg_object *object)
{
	struct list_head *pos;
	build_func builder;
	int ret;

	/* sort attributes with token references into separate sets */
	ret = tplg_build_object_attribute_sets(object);
	if (ret < 0)
		SNDERR("Failed to build attribute sets for object %s\n", object->name);

	builder = tplg_pp_lookup_object_build_func(object);
	if (!builder) {
		tplg_pp_debug("skipping build for %s\n", object->name);
		goto child;
	}

	/* build the object and save the sections to the output buffer */
	ret = builder(tplg_pp, object);
	if (ret < 0)
		return ret;

child:
	/* build child objects */
	list_for_each(pos, &object->object_list) {
		struct tplg_object *child = list_entry(pos, struct tplg_object, list);

		ret = tplg_build_object(tplg_pp, child);
		if (ret < 0) {
			SNDERR("Failed to build object %s\n", child->name);
			return ret;
		}
	}

	return ret;
}

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

/* translate string values to integer if needed to be added to private data */
static int tplg_pp_update_valid_tuples(struct tplg_object *object)
{
	struct tplg_attribute *attr;

	list_for_each_entry(attr, &object->attribute_list, list) {
		switch (attr->type) {
		case SND_CONFIG_TYPE_STRING:
		{
			struct tplg_attribute_ref *v;

			/* skip attributes with no pre-defined valid values */
			if (list_empty(&attr->constraint.value_list))
				continue;

			/* Translate the string value to integer if needed */
			list_for_each_entry(v, &attr->constraint.value_list, list) {
				if (strcmp(attr->value.string, v->string))
					continue;
				if (v->value == -EINVAL)
					break;

				attr->value.integer = v->value;
				attr->type = SND_CONFIG_TYPE_INTEGER;
				break;
			}
			break;
		}
		default:
			break;
		}
	}

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

	/* set new object name */
	ret = tplg_update_object_name_from_args(object);
	if (ret < 0)
		return ret;

	/* sanity check for object attributes */
	list_for_each(pos, &object->attribute_list) {
		struct tplg_attribute *attr = list_entry(pos, struct tplg_attribute, list);

		/* check if mandatory and value provided */
		if (attr->constraint.mask & TPLG_CLASS_ATTRIBUTE_MASK_MANDATORY &&
		    !attr->found) {
			SNDERR("Mandatory attribute %s not found for object %s\n", attr->name,
			       object->name);
			return -EINVAL;
		}

		if (attr->constraint.mask & TPLG_CLASS_ATTRIBUTE_MASK_DEPRECATED &&
		    attr->found) {
			SNDERR("Attibrute %s decprecated\n", attr->name);
			return -EINVAL;
		}
	}

	/* update attribute string values to valid tuple values */
	ret = tplg_pp_update_valid_tuples(object);
	if (ret < 0) {
		SNDERR("Error updating valid tuples for object attributes\n");
		return ret;
	}

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
				     struct tplg_object *base, struct tplg_object *current,
				     const char *class_name, struct list_head *list)
{
	snd_config_iterator_t i, next;
	snd_config_t *n;
	int ret;

	snd_config_for_each(i, next, cfg) {
		struct tplg_class *class;
		snd_config_type_t type;
		const char *id;

		n = snd_config_iterator_entry(i);
		if (snd_config_get_id(n, &id) < 0) {
			return -EINVAL;
		}

		type = snd_config_get_type(n);

		/* set the attribute for the current object */
		if (current) {
			if (type != SND_CONFIG_TYPE_COMPOUND) {
				ret = tplg_parse_attribute_value(n, &current->attribute_list,
								 false);
				if (ret < 0)
					SNDERR("Failed to set attribute for '%s'\n", current->name);
				return ret;
			} else {
				/* move to the next node and pass the current object list */
				ret = tplg_set_child_attributes(tplg_pp, cfg, base, NULL, NULL,
							&current->object_list);
				if (ret < 0)
					return ret;

				continue;
			}
		}

		/* look for the object in the list and pass it to the next node */
		if (class_name) {
			struct tplg_object *child;

			/* get object of type class_name and unique attribute value */
			child = tplg_object_lookup_in_list(list, class_name, (char *)id);
			if (!child) {
				SNDERR("No child %s.%s found for object %s\n",
					class_name, id, base->name);
				return -EINVAL;
			}

			/* move to the next node and pass the child object */
			ret = tplg_set_child_attributes(tplg_pp, n, base, child, NULL, list);
			if (ret < 0)
				return ret;

			continue;
		}

		/* look up class name and pass it to the next node*/
		class = tplg_class_lookup(tplg_pp, id);
		if (!class) {
			/* skip all nodes that arent for setting child attributes */
			if (!current) {
				continue;
			} else {
				SNDERR("No class found with name '%s'\n", id);
				return -ENOENT;
			}
		}

		ret = tplg_set_child_attributes(tplg_pp, n, base, NULL, id, list);
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
	INIT_LIST_HEAD(&dest->attribute_set_list);
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
					attr->cfg = ref_attr->cfg;
					break;
				case SND_CONFIG_TYPE_INTEGER64:
					attr->value.integer64 = ref_attr->value.integer64;
					attr->type = ref_attr->type;
					attr->cfg = ref_attr->cfg;
					break;
				case SND_CONFIG_TYPE_STRING:
					snd_strlcpy(attr->value.string,
						    ref_attr->value.string,
						    SNDRV_CTL_ELEM_ID_NAME_MAXLEN);
					attr->type = ref_attr->type;
					attr->cfg = ref_attr->cfg;
					break;
				case SND_CONFIG_TYPE_REAL:
					attr->value.d = ref_attr->value.d;
					attr->type = ref_attr->type;
					attr->cfg = ref_attr->cfg;
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
	INIT_LIST_HEAD(&object->attribute_set_list);
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
	ret = tplg_set_child_attributes(tplg_pp, cfg, object, NULL, NULL, &object->object_list);
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

		/* Build the object now */
		ret = tplg_build_object(tplg_pp, object);
		if (ret < 0) {
			SNDERR("Error building object %s\n", object->name);
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
