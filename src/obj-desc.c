/**
 * \file obj-desc.c
 * \brief Create object name descriptions
 *
 * Copyright (c) 1997 - 2007 Angband contributors
 *
 * This work is free software; you can redistribute it and/or modify it
 * under the terms of either:
 *
 * a) the GNU General Public License as published by the Free Software
 *    Foundation, version 2, or
 *
 * b) the "Angband licence":
 *    This software may be copied and distributed for educational, research,
 *    and not for profit purposes provided that this copyright and statement
 *    are included in all such copies.  Other copyrights may also apply.
 */

#include "angband.h"
#include "obj-chest.h"
#include "obj-desc.h"
#include "obj-gear.h"
#include "obj-ignore.h"
#include "obj-knowledge.h"
#include "obj-tval.h"
#include "obj-util.h"
#include "player-abilities.h"
#include "player-calcs.h"

const char *inscrip_text[OBJ_PSEUDO_MAX] = {
	NULL,
	"average",
	"artefact, cursed",
	"special, cursed",
	"cursed",
	"special",
	"artefact",
	"uncursed",
};

/**
 * Puts the object base kind's name into buf.
 */
void object_base_name(char *buf, size_t max, int tval, bool plural)
{
	struct object_base *kb = &kb_info[tval];
	size_t end = 0;

	if (kb->name && kb->name[0]) 
		(void) obj_desc_name_format(buf, max, end, kb->name, NULL, plural);
}


/**
 * Puts a very stripped-down version of an object's name into buf.
 * If easy_know is true, then the IDed names are used, otherwise
 * flavours, scroll names, etc will be used.
 *
 * Just truncates if the buffer isn't big enough.
 */
void object_kind_name(char *buf, size_t max, const struct object_kind *kind,
					  bool easy_know)
{
	/* If not aware, the plain flavour (e.g. Copper) will do. */
	if (!easy_know && !kind->aware && kind->flavor)
		my_strcpy(buf, kind->flavor->text, max);

	/* Use proper name (Healing, or whatever) */
	else
		(void) obj_desc_name_format(buf, max, 0, kind->name, NULL, false);
}


/**
 * A modifier string, put where '#' goes in the basename below.  The weird
 * games played with book names are to allow the non-essential part of the
 * name to be abbreviated when there is not much room to display.
 */
static const char *obj_desc_get_modstr(const struct object_kind *kind)
{
	if (tval_can_have_flavor_k(kind))
		return kind->flavor ? kind->flavor->text : "";

	return "";
}

/**
 * An object's basic name - a generic name for flavored objects (with the
 * actual name added later depending on awareness, the name from object.txt
 * for almost everything else, and a bit extra for books. 
 */
static const char *obj_desc_get_basename(const struct object *obj, bool aware,
		bool terse, uint32_t mode, const struct player *p)
{
	bool show_flavor = !terse && obj->kind->flavor;

	if (aware && p && !OPT(p, show_flavors)) show_flavor = false;

	/* Artifacts are special */
	if (obj->artifact && (aware || object_is_known(obj) || terse ||
						  !obj->kind->flavor))
		return obj->kind->name;

	/* Analyze the object */
	switch (obj->tval)
	{
		case TV_NOTE:
		case TV_USELESS:
		case TV_METAL:
		case TV_FLASK:
		case TV_CHEST:
		case TV_ARROW:
		case TV_BOW:
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_SWORD:
		case TV_DIGGING:
		case TV_BOOTS:
		case TV_GLOVES:
		case TV_CLOAK:
		case TV_HELM:
		case TV_SHIELD:
		case TV_SOFT_ARMOR:
		case TV_MAIL:
		case TV_LIGHT:
		case TV_FOOD:
			return obj->kind->name;

		case TV_RING:
			return (show_flavor ? "& # Ring~" : "& Ring~");

		case TV_STAFF:
			return (show_flavor ? "& # Sta|ff|ves|" : "& Sta|ff|ves|");

		case TV_POTION:
			return (show_flavor ? "& # Potion~" : "& Potion~");

	}

	return "(nothing)";
}


/**
 * Start to description, indicating number/uniqueness (a, the, no more, 7, etc)
 */
static size_t obj_desc_name_prefix(char *buf, size_t max, size_t end,
		const struct object *obj, const char *basename,
		const char *modstr, bool terse, uint16_t number)
{
	if (number == 0) {
		strnfcat(buf, max, &end, "no more ");
	} else if (number > 1) {
		strnfcat(buf, max, &end, "%u ", number);
	} else if (obj->artifact && object_is_known(obj)) {
		strnfcat(buf, max, &end, "the ");
	} else if (*basename == '&') {
		bool an = false;
		const char *lookahead = basename + 1;

		while (*lookahead == ' ') lookahead++;

		if (*lookahead == '#') {
			if (modstr && is_a_vowel(*modstr))
				an = true;
		} else if (is_a_vowel(*lookahead)) {
			an = true;
		}

		if (!terse) {
			if (an)
				strnfcat(buf, max, &end, "an ");
			else
				strnfcat(buf, max, &end, "a ");			
		}
	}

	return end;
}



/**
 * Formats 'fmt' into 'buf', with the following formatting characters:
 *
 * '~' at the end of a word (e.g. "fridge~") will pluralise
 *
 * '|x|y|' will be output as 'x' if singular or 'y' if plural
 *    (e.g. "kni|fe|ves|")
 *
 * '#' will be replaced with 'modstr' (which may contain the pluralising
 * formats given above).
 */
size_t obj_desc_name_format(char *buf, size_t max, size_t end,
		const char *fmt, const char *modstr, bool pluralise)
{
	/* Copy the string */
	while (*fmt) {
		/* Skip */
		if (*fmt == '&') {
			while (*fmt == ' ' || *fmt == '&')
				fmt++;
			continue;
		} else if (*fmt == '~') {
			/* Pluralizer (regular English plurals) */
			char prev = *(fmt - 1);

			if (!pluralise)	{
				fmt++;
				continue;
			}

			/* e.g. cutlass-e-s, torch-e-s, box-e-s */
			if (prev == 's' || prev == 'h' || prev == 'x')
				strnfcat(buf, max, &end, "es");
			else
				strnfcat(buf, max, &end, "s");
		} else if (*fmt == '|') {
			/* Special plurals 
			* e.g. kni|fe|ves|
			*          ^  ^  ^ */
			const char *singular = fmt + 1;
			const char *plural   = strchr(singular, '|');
			const char *endmark  = NULL;

			if (plural) {
				plural++;
				endmark = strchr(plural, '|');
			}

			if (!singular || !plural || !endmark) return end;

			if (!pluralise)
				strnfcat(buf, max, &end, "%.*s",
					(int) (plural - singular) - 1,
					singular);
			else
				strnfcat(buf, max, &end, "%.*s",
					(int) (endmark - plural), plural);

			fmt = endmark;
		} else if (*fmt == '#') {
			/* Add modstr, with pluralisation if relevant */
			end = obj_desc_name_format(buf, max, end, modstr, NULL,	pluralise);
		}

		else
			buf[end++] = *fmt;

		fmt++;
	}

	buf[end] = 0;

	return end;
}


/**
 * Format object obj's name into 'buf'.
 */
static size_t obj_desc_name(char *buf, size_t max, size_t end,
		const struct object *obj, bool prefix, uint32_t mode,
		bool terse, const struct player *p)
{
	bool spoil = mode & ODESC_SPOIL ? true : false;
	uint16_t number = (mode & ODESC_ALTNUM) ?
		(mode & 0xFFFF0000) >> 16 : obj->number;
	
	/* Actual name for flavoured objects if aware, or spoiled */
	bool aware = object_flavor_is_aware(obj) || spoil;
	/* Pluralize if (not forced singular) and
	 * (not a known/visible artifact) and
	 * (not one in stack or forced plural) */
	bool plural = !(mode & ODESC_SINGULAR) &&
		!obj->artifact &&
		(number != 1 || (mode & ODESC_PLURAL));
	const char *basename = obj_desc_get_basename(obj, aware, terse,
		mode, p);
	const char *modstr = obj_desc_get_modstr(obj->kind);

	/* Quantity prefix */
	if (prefix)
		end = obj_desc_name_prefix(buf, max, end, obj, basename,
			modstr, terse, number);

	/* Base name */
	end = obj_desc_name_format(buf, max, end, basename, modstr, plural);

	/* Append extra names of various kinds */
	if (obj->artifact && object_is_known(obj))
		strnfcat(buf, max, &end, " %s", obj->artifact->name);
	else if (obj->ego && object_is_known(obj) && !(mode & ODESC_NOEGO))
		strnfcat(buf, max, &end, " %s", obj->ego->name);
	else if (aware && !obj->artifact && obj->kind->flavor) {
		if (terse)
			strnfcat(buf, max, &end, " '%s'", obj->kind->name);
		else
			strnfcat(buf, max, &end, " of %s", obj->kind->name);
	}

	return end;
}

/**
 * Special descriptions for types of chest traps
 */
static size_t obj_desc_chest(const struct object *obj, char *buf, size_t max,
							 size_t end)
{
	if (!tval_is_chest(obj)) return end;

	/* The chest is unopened, but we know nothing about its trap/lock */
	if (obj->pval && !object_is_known(obj)) return end;

	/* Describe the traps */
	strnfcat(buf, max, &end, " (%s)", chest_trap_name(obj));

	return end;
}

/**
 * Describe combat properties of an item - attack, damage dice, evasion,
 * protection dice
 */
static size_t obj_desc_combat(const struct object *obj, char *buf, size_t max, 
		size_t end, uint32_t mode, const struct player *p)
{
	/* Display damage dice for weapons */
	if (kf_has(obj->kind->kind_flags, KF_SHOW_DICE)) {
		int ds = obj->ds + hand_and_a_half_bonus((struct player *) p, obj);
		strnfcat(buf, max, &end, " (%+d,%dd%d)", obj->att, obj->dd, ds);
	} else if (tval_is_ammo(obj) && obj->att) {
		/* Display attack for arrows if non-zero */
		strnfcat(buf, max, &end, " (%+d)", obj->att);
	} else if (obj->att && !(tval_is_ring(obj) && !object_is_known(obj))) {
		/* Display attack if non-zero */
		strnfcat(buf, max, &end, " (%+d)", obj->att);
	}

	/* Show evasion/protection info */
	if (tval_is_ring(obj) && !object_is_known(obj)) {
		/* Do nothing */
	} else if (obj->pd && obj->ps) {
		strnfcat(buf, max, &end, " [%+d,%dd%d]", obj->evn, obj->pd, obj->ps);
	} else if (obj->evn) {
		strnfcat(buf, max, &end, " [%+d]", obj->evn);
	}

	return end;
}

/**
 * Describe remaining light for refuellable lights
 */
static size_t obj_desc_light(const struct object *obj, char *buf, size_t max,
							 size_t end)
{
	/* Fuelled light sources get number of remaining turns appended */
	if (tval_is_light(obj) && !of_has(obj->flags, OF_NO_FUEL))
		strnfcat(buf, max, &end, " (%d turns)", obj->timeout);

	return end;
}

/**
 * Describe numerical modifiers to stats and other player qualities which
 * allow numerical bonuses - speed, stealth, etc
 */
static size_t obj_desc_mods(const struct object *obj, char *buf, size_t max,
							size_t end)
{
	int i, j, num_mods = 0;
	int mods[OBJ_MOD_MAX] = { 0 };

	if (!object_is_known(obj)) return end;

	/* Run through possible modifiers and store distinct ones */
	for (i = 0; i < OBJ_MOD_MAX; i++) {
		/* Check for known non-zero mods */
		if (obj->modifiers[i] != 0) {
			/* If no mods stored yet, store and move on */
			if (!num_mods) {
				mods[num_mods++] = obj->modifiers[i];
				continue;
			}

			/* Run through the existing mods, quit on duplicates */
			for (j = 0; j < num_mods; j++)
				if (mods[j] == obj->modifiers[i]) break;

			/* Add another mod if needed */
			if (j == num_mods)
				mods[num_mods++] = obj->modifiers[i];
		}
	}

	if (!num_mods) return end;

	/* Print the modifiers */
	strnfcat(buf, max, &end, " <");
	for (j = 0; j < num_mods; j++) {
		if (j) strnfcat(buf, max, &end, ", ");
		strnfcat(buf, max, &end, "%+d", mods[j]);
	}
	strnfcat(buf, max, &end, ">");

	return end;
}

/**
 * Describe charges or charging status for re-usable items with magic effects
 */
static size_t obj_desc_charges(const struct object *obj, char *buf, size_t max,
		size_t end, uint32_t mode)
{
	bool aware = object_flavor_is_aware(obj);

	/* Wands and staffs have charges, others may be charging */
/*	if (aware && tval_can_have_charges(obj)) {
		strnfcat(buf, max, &end, " (%d charge%s)", obj->pval,
				 PLURAL(obj->pval));
	}*/

	return end;
}

/**
 * Describe charges or charging status for re-usable items with magic effects
 */
static size_t obj_desc_staminapercent(const struct object *obj, char *buf, size_t max,
		size_t end, uint32_t mode)
{
	bool aware = object_flavor_is_aware(obj);
    int staminacost = player->msp * obj->kind->staminapercent / 100;
	/* Wands and staffs have stamina percent, but display as actual stamina cost*/
	if (aware && tval_can_have_staminapercent(obj)) {
		strnfcat(buf, max, &end, " (%d stamina)", staminacost);
	}

	return end;
}

/**
 * Add player-defined inscriptions or game-defined descriptions
 */
static size_t obj_desc_inscrip(const struct object *obj, char *buf,
		size_t max, size_t end, const struct player *p)
{
	const char *u[6] = { 0, 0, 0, 0, 0, 0 };
	int n = 0;

	/* Get inscription */
	if (obj->note)
		u[n++] = quark_str(obj->note);

	/* Use special inscription, if any */
	if (!object_flavor_is_aware(obj)) {
		if (object_flavor_was_tried(obj))
			u[n++] = "tried";
	}

	/* Note curses, use special inscription, if any */
	if (object_is_cursed(obj) &&
		(object_is_known(obj) || player_active_ability(player, "Lore-Keeper"))){
		u[n++] = "cursed";
	} else if (obj->pseudo != OBJ_PSEUDO_NONE) {
		assert(obj->pseudo < OBJ_PSEUDO_MAX);
		u[n++] = inscrip_text[obj->pseudo];
	}

	/* Note ignore */
	if (p && ignore_item_ok(p, obj))
		u[n++] = "ignore";

	if (n) {
		int i;
		for (i = 0; i < n; i++) {
			if (i == 0)
				strnfcat(buf, max, &end, " {");
			strnfcat(buf, max, &end, "%s", u[i]);
			if (i < n - 1)
				strnfcat(buf, max, &end, ", ");
		}

		strnfcat(buf, max, &end, "}");
	}

	return end;
}


/**
 * Describes item `obj` into buffer `buf` of size `max`.
 *
 * \param buf is the buffer for the description.  Must have space for at least
 * max bytes.
 * \param max is the size of the buffer, in bytes.
 * \param obj is the object to describe.
 * \param mode must be a bitwise-or of zero or one more of the following:
 * ODESC_PREFIX prepends a 'the', 'a' or number
 * ODESC_BASE results in a base description.
 * ODESC_COMBAT will add to-hit, to-dam and AC info.
 * ODESC_EXTRA will add pval/charge/inscription/ignore info.
 * ODESC_PLURAL will pluralise regardless of the number in the stack.
 * ODESC_SPOIL treats the object as fully identified.
 * ODESC_CAPITAL capitalises the object name.
 * ODESC_TERSE causes a terse name to be used.
 * ODESC_NOEGO omits ego names.
 * ODESC_ALTNUM causes the high 16 bits of mode to be used as the number
 * of objects instead of using obj->number.  Note that using ODESC_ALTNUM
 * is not fully compatible with ODESC_EXTRA:  the display of number of rods
 * charging does not account for the alternate number.
 * \param p is the player whose knowledge is factored into the description.
 * If p is NULL, the description is for an omniscient observer.
 *
 * \returns The number of bytes used of the buffer.
 */
size_t object_desc(char *buf, size_t max, const struct object *obj,
		uint32_t mode, const struct player *p)
{
	bool prefix = mode & ODESC_PREFIX ? true : false;
	bool spoil = mode & ODESC_SPOIL ? true : false;
	bool terse = mode & ODESC_TERSE ? true : false;

	size_t end = 0;

	/* Simple description for null item */
	if (!obj)
		return strnfmt(buf, max, "(nothing)");

	/* Egos and kinds whose name we know are seen */
	if (obj->ego && object_is_known(obj) && !spoil)
		obj->ego->everseen = true;

	if (object_flavor_is_aware(obj) && !spoil)
		obj->kind->everseen = true;

	/** Construct the name **/

	/* Copy the base name to the buffer */
	end = obj_desc_name(buf, max, end, obj, prefix, mode, terse, p);

	/* Combat properties */
	if (mode & ODESC_COMBAT) {
		if (tval_is_chest(obj))
			end = obj_desc_chest(obj, buf, max, end);
		else if (tval_is_light(obj))
			end = obj_desc_light(obj, buf, max, end);

		end = obj_desc_combat(obj, buf, max, end, mode, p);
	}

	/* Modifiers, charges, stamina cost, flavour details, inscriptions */
	if (mode & ODESC_EXTRA) {
		end = obj_desc_mods(obj, buf, max, end);
		end = obj_desc_charges(obj, buf, max, end, mode);
		end = obj_desc_staminapercent(obj, buf, max, end, mode);
		end = obj_desc_inscrip(obj, buf, max, end, p);
	}

	return end;
}
