/**
 * \file player-quest.c
 * \brief All throne room-related code
 *
 * Copyright (c) 2013 Angband developers
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
#include "combat.h"
#include "datafile.h"
#include "game-input.h"
#include "game-world.h"
#include "generate.h"
#include "init.h"
#include "mon-calcs.h"
#include "mon-desc.h"
#include "mon-move.h"
#include "mon-util.h"
#include "monster.h"
#include "obj-desc.h"
#include "obj-gear.h"
#include "obj-make.h"
#include "obj-pile.h"
#include "obj-tval.h"
#include "obj-util.h"
#include "player-abilities.h"
#include "player-attack.h"
#include "player-calcs.h"
#include "player-history.h"
#include "player-quest.h"
#include "project.h"

/**
 * Makes Saruman drop the Keys of Orthanc with an appropriate message.
 */
void drop_the_keys(struct monster *mon, const char *message)
{
	int i;
	struct loc grid;
	struct monster_race *race = mon->race;
	const struct artifact *keys = lookup_artifact_name("of Orthanc");
	bool note;

	if (!is_artifact_created(keys)) {
		struct object *obj;
		struct object_kind *kind;
		msg(message);

		/* Choose a nearby location, but not his own square */
		find_nearby_grid(cave, &grid, mon->grid, 1, 1);
		for (i = 0; i < 1000; i++) {
			find_nearby_grid(cave, &grid, mon->grid, 1, 1);
			if (!loc_eq(grid, mon->grid) && square_isfloor(cave, grid)) break;
		}

		/* Allocate by hand, prep, apply magic */
		obj = mem_zalloc(sizeof(*obj));
		kind = lookup_kind(keys->tval, keys->sval);
		object_prep(obj, kind, z_info->dun_depth, RANDOMISE);
		obj->artifact = keys;
		copy_artifact_data(obj, obj->artifact);
		mark_artifact_created(keys, true);

		/* Set origin details */
		obj->origin = ORIGIN_DROP;
		obj->origin_depth = convert_depth_to_origin(cave->depth);
		obj->origin_race = race;
		obj->number = 1;

		/* Drop it there */
		floor_carry(cave, grid, obj, &note);

        /* Increase Saruman's will and perception when he loses the keys*/
		race->wil += 2;
		race->per += 4;
	}
}


/**
 * Checks if the player has the Keys
 */
int keys_possessed(struct player *p)
{
	int keys = 0;
	struct object *obj;

	for (obj = p->gear; obj; obj = obj->next) {
		if (obj->artifact && streq(obj->artifact->name, "of Orthanc")) {
			keys += obj->pval;
		}
	}

	return keys;
}
