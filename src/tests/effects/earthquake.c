/*
 * effects/earthquake
 * Test aspects of the earthquake effect.
 */

#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "effects.h"
#include "game-world.h"
#include "init.h"
#include "obj-knowledge.h"
#include "obj-make.h"
#include "obj-pile.h"
#include "obj-util.h"
#include "player-birth.h"
#include "player-util.h"

int setup_tests(void **state) {
	set_file_paths();
	if (!init_angband()) {
		return 1;
	}
#ifdef UNIX
	create_needed_dirs();
#endif

	if (!player_make_simple(NULL, NULL, "Tester")) {
		cleanup_angband();
		return 1;
	}

	return 0;
}

int teardown_tests(void *state) {
	cleanup_angband();
	return 0;
}

static struct chunk *create_empty_cave(int height, int width) {
	struct chunk *c = cave_new(height, width);
	struct loc grid;

	grid.y = 0;
	for (grid.x = 0; grid.x < width; ++grid.x) {
		square_set_feat(c, grid, FEAT_PERM);
	}
	for (grid.y = 1; grid.y < height - 1; ++grid.y) {
		grid.x = 0;
		square_set_feat(c, grid, FEAT_PERM);
		for (grid.x = 1; grid.x < width - 1; ++grid.x) {
			square_set_feat(c, grid, FEAT_FLOOR);
		}
		grid.x = width - 1;
		square_set_feat(c, grid, FEAT_PERM);
	}
	grid.y = height - 1;
	for (grid.x = 0; grid.x < width; ++grid.x) {
		square_set_feat(c, grid, FEAT_PERM);
	}
	return c;
}

static struct object *setup_object(int tval, int sval, int num) {
	struct object_kind *kind = lookup_kind(tval, sval);
	struct object *obj = NULL;

	if (kind) {
		obj = object_new();
		object_prep(obj, kind, 0, RANDOMISE);
		obj->number = num;
	}
	return obj;
}

static void generate_piles(struct chunk *c, struct player *p) {
	const int allowed_tvals[] = { TV_BOOTS, TV_LIGHT, TV_POTION };
	struct loc grid;

	for (grid.y = 1; grid.y < c->height - 1; ++grid.y) {
		for (grid.x = 1; grid.x < c->width - 1; ++grid.x) {
			int n = randint1((int) N_ELEMENTS(allowed_tvals)), i;

			for (i = 0; i < n; ++i) {
				struct object *obj =
					setup_object(allowed_tvals[i], 1, 1);
				bool note = false;

				if (obj && !floor_carry(c, grid, obj, &note)) {
					object_delete(c, &obj);
				}
			}
		}
	}
}

static void orphan_piles(struct chunk *c) {
	struct loc grid;

	for (grid.y = 1; grid.y < c->height - 1; ++grid.y) {
		for (grid.x = 1; grid.x < c->width - 1; ++grid.x) {
			struct object *obj = square_object(c, grid);

			while (obj) {
				struct object *next_obj = obj->next;

				square_delete_object(c, grid, obj, false, false);
				obj = next_obj;
			}
		}
	}
}

static int test_obj_pile_simple(void *state) {
	character_dungeon = false;
	player->depth = 1;
	cave = create_empty_cave(11, 15);
	cave->depth = player->depth;
	player_place(cave, player, loc(cave->width / 2, cave->height / 2));
	character_dungeon = true;
	on_new_level();
	generate_piles(cave, player);
	effect_simple(EF_EARTHQUAKE, source_player(), "0", 0,
		MAX(cave->width / 2, cave->height / 2), 0, NULL);
	wiz_light(cave, player);
	cave_free(cave);
	cave = NULL;
	ok;
}

static int test_obj_pile_orphan(void *state) {
	character_dungeon = false;
	player->depth = 1;
	cave = create_empty_cave(11, 15);
	cave->depth = player->depth;
	player_place(cave, player, loc(cave->width / 2, cave->height / 2));
	character_dungeon = true;
	on_new_level();
	generate_piles(cave, player);
	orphan_piles(cave);
	effect_simple(EF_EARTHQUAKE, source_player(), "0", 0,
		MAX(cave->width / 2, cave->height / 2), 0, NULL);
	wiz_light(cave, player);
	cave_free(cave);
	cave = NULL;
	ok;
}

const char *suite_name = "effects/earthquake";
struct test tests[] = {
	{ "quake:  object pile simple", test_obj_pile_simple },
	{ "quake:  object pile orphan", test_obj_pile_orphan },
	{ NULL, NULL }
};
