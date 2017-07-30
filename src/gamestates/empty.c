/*! \file empty.c
 *  \brief Empty gamestate.
 */
/*
 * Copyright (c) Sebastian Krzyszkowiak <dos@dosowisko.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "../common.h"
#include <math.h>
#include <libsuperderpy.h>

struct GamestateResources {
		// This struct is for every resource allocated and used by your gamestate.
		// It gets created on load and then gets passed around to all other function calls.
		ALLEGRO_FONT *font;
		unsigned int blink_counter;
		ALLEGRO_BITMAP *canvas, *maskbmp;
		ALLEGRO_LOCKED_REGION *mask;


		ALLEGRO_BITMAP *light1, *light2, *light3, *light4, *tmp, *bg;
		struct Character *warthog, *table, *fire;
		bool button;
		int x, y;
		float rand;

		int stage;
		bool drawing;
};

int Gamestate_ProgressCount = 9; // number of loading steps as reported by Gamestate_Load

void SwitchSpritesheet(struct Game *game, struct Character *character, char *name) {
	struct Spritesheet *tmp = character->spritesheets;
	if (!tmp) {
		PrintConsole(game, "ERROR: No spritesheets registered for %s!", character->name);
		return;
	}
	while (tmp) {
		if (!strcmp(tmp->name, name)) {
			character->spritesheet = tmp;
			if (character->bitmap) {
				if ((al_get_bitmap_width(character->bitmap) != tmp->width / tmp->cols) || (al_get_bitmap_height(character->bitmap) != tmp->height / tmp->rows)) {
					al_destroy_bitmap(character->bitmap);
					character->bitmap = al_create_bitmap(tmp->width / tmp->cols, tmp->height / tmp->rows);
				}
			} else {
				character->bitmap = al_create_bitmap(tmp->width / tmp->cols, tmp->height / tmp->rows);
			}
			return;
		}
		tmp = tmp->next;
	}
	PrintConsole(game, "ERROR: No spritesheets registered for %s with given name: %s", character->name, name);
	return;
}

void Gamestate_Logic(struct Game *game, struct GamestateResources* data) {
	// Called 60 times per second. Here you should do all your game logic.
	data->blink_counter++;
	if (data->blink_counter % 4 == 0) {
		data->rand = rand() / (float)RAND_MAX / 4.0 + 0.75;
	}

	int width = al_get_bitmap_width(data->canvas);
	int height = al_get_bitmap_height(data->canvas);
	ALLEGRO_LOCKED_REGION *region = al_lock_bitmap(data->canvas, ALLEGRO_PIXEL_FORMAT_RGBA_8888, ALLEGRO_LOCK_READONLY);

	char *d = region->data;
	char *mask = data->mask->data;
	int white = 0; int black = 0; int white2 = 0; int black2 = 0;
	for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
			//for (int z = 0; z < region->pixel_size; z++) {
			//	d[x * region->pixel_size + region->pitch * y + z]--;
			//}
			if (d[x * region->pixel_size + region->pitch * y]) {
				if (mask[x * region->pixel_size + region->pitch * y]) {
					white++;
				} else {
					black++;
				}
			}
			if (mask[x * region->pixel_size + region->pitch * y]) {
				if (d[x * region->pixel_size + region->pitch * y]) {
					white2++;
				} else {
					black2++;
				}
			}
			//al_get_pixel(data->canvas, x, y);
		}
		//printf("%d\n", x);
	}
	//printf("END\n");
	al_unlock_bitmap(data->canvas);
	PrintConsole(game, "%f%% %f%% = %f%%", (white/(float)(white+black)) * 100, (white2/(float)(white2+black2)) * 100,
	             ((white/(float)(white+black)) * 100 + (white2/(float)(white2+black2)) * 100) - 100);

	AnimateCharacter(game, data->fire, 1);
	AnimateCharacter(game, data->warthog, 1);
}

void Gamestate_Draw(struct Game *game, struct GamestateResources* data) {
	// Called as soon as possible, but no sooner than next Gamestate_Logic call.
	// Draw everything to the screen here.

	al_draw_scaled_bitmap(data->bg, 0, 0, al_get_bitmap_width(data->bg), al_get_bitmap_height(data->bg), 0, 0, game->viewport.width, game->viewport.height, 0);

	SwitchSpritesheet(game, data->table, "1");
	if (data->stage < 5) {
		SwitchSpritesheet(game, data->warthog, "1");
		DrawScaledCharacter(game, data->warthog, al_map_rgb(255,255,255), game->viewport.width / (float)3840, game->viewport.height / (float)2160, 0);
	}
	DrawScaledCharacter(game, data->table, al_map_rgb(255,255,255), game->viewport.width / (float)3840, game->viewport.height / (float)2160, 0);

	if (data->stage >= 1) {
		al_set_target_bitmap(data->tmp);
		al_clear_to_color(al_map_rgba(0,0,0,0));

		SwitchSpritesheet(game, data->table, "2");
//		if (data->stage < 5) {
		  SwitchSpritesheet(game, data->warthog, "2");
			DrawScaledCharacter(game, data->warthog, al_map_rgb(255,255,255), game->viewport.width / (float)3840, game->viewport.height / (float)2160, 0);
//		}
		DrawScaledCharacter(game, data->table, al_map_rgb(255,255,255), game->viewport.width / (float)3840, game->viewport.height / (float)2160, 0);
		al_set_blender(ALLEGRO_ADD, ALLEGRO_ZERO, ALLEGRO_ALPHA); // now as a mask

		ALLEGRO_BITMAP *bmp = data->light3;
		if (data->stage == 1) {
			bmp = data->light1;
		}
		if (data->stage == 2) {
			bmp = data->light2;
		}
		if (data->stage > 3) {
			bmp = data->light4;
		}

		al_draw_scaled_bitmap(bmp, 0, 0, al_get_bitmap_width(data->light1), al_get_bitmap_height(data->light1), 0, 0, game->viewport.width, game->viewport.height, 0);
		al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);

		al_set_target_backbuffer(game->display);
		al_draw_tinted_bitmap(data->tmp, al_map_rgba_f(data->rand, data->rand, data->rand, data->rand), 0, 0,  0);
	}

	if (data->stage >= 2) {
		al_set_target_bitmap(data->tmp);
		al_clear_to_color(al_map_rgba(0,0,0,0));

		SwitchSpritesheet(game, data->table, "3");
		if (data->stage < 5) {
			SwitchSpritesheet(game, data->warthog, "3");
		} else {
			SwitchSpritesheet(game, data->warthog, "happy");
		}
		DrawScaledCharacter(game, data->warthog, al_map_rgb(255,255,255), game->viewport.width / (float)3840, game->viewport.height / (float)2160, 0);
		DrawScaledCharacter(game, data->table, al_map_rgb(255,255,255), game->viewport.width / (float)3840, game->viewport.height / (float)2160, 0);
		al_set_blender(ALLEGRO_ADD, ALLEGRO_ZERO, ALLEGRO_ALPHA); // now as a mask

		ALLEGRO_BITMAP *bmp = data->light3;
		if (data->stage == 2) {
			bmp = data->light1;
		}
		if (data->stage == 3) {
			bmp = data->light2;
		}
		if (data->stage > 4) {
			bmp = data->light4;
		}

		al_draw_scaled_bitmap(bmp, 0, 0, al_get_bitmap_width(data->light1), al_get_bitmap_height(data->light1), 0, 0, game->viewport.width, game->viewport.height, 0);
		al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);

		al_set_target_backbuffer(game->display);
		al_draw_tinted_bitmap(data->tmp, al_map_rgba_f(data->rand, data->rand, data->rand, data->rand), 0, 0,  0);
	}

	SwitchSpritesheet(game, data->table, "1");
	if (data->stage < 5) {
		SwitchSpritesheet(game, data->warthog, "1");
	}

	if (data->stage) {
		DrawScaledCharacter(game, data->fire, al_map_rgb(255,255,255), game->viewport.width / (float)3840, game->viewport.height / (float)2160, 0);
	}

	if (!data->drawing) {
	return;
	}
	al_draw_filled_rectangle(0, 0, al_get_display_width(game->display), al_get_display_height(game->display), al_map_rgba(0,0,0,127));
	al_draw_tinted_scaled_bitmap(data->maskbmp, al_map_rgba(127,127,127,127), 0, 0, al_get_bitmap_width(data->maskbmp), al_get_bitmap_height(data->maskbmp), 0, 0, game->viewport.width, game->viewport.height, 0);
	al_draw_scaled_bitmap(data->canvas, 0, 0, al_get_bitmap_width(data->canvas), al_get_bitmap_height(data->canvas), 0, 0, game->viewport.width, game->viewport.height, 0);

}

void Gamestate_ProcessEvent(struct Game *game, struct GamestateResources* data, ALLEGRO_EVENT *ev) {
	// Called for each event in Allegro event queue.
	// Here you can handle user input, expiring timers etc.
	if ((ev->type==ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_ESCAPE)) {
		UnloadCurrentGamestate(game); // mark this gamestate to be stopped and unloaded
		// When there are no active gamestates, the engine will quit.
	}

	if (ev->type==ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
		if (ev->mouse.button==1) {
			data->button = !data->button;
			int x = ev->mouse.x, y = ev->mouse.y;
			WindowCoordsToViewport(game, &x, &y);
			x *= al_get_bitmap_width(data->canvas) / (float)game->viewport.width;
			y *= al_get_bitmap_height(data->canvas) / (float)game->viewport.height;
			data->x = x;
			data->y = y;
		}
	}
	if (ev->type==ALLEGRO_EVENT_MOUSE_BUTTON_UP) {
		if (ev->mouse.button==1) {
			//data->button = false;
		}
	}

	if (ev->type==ALLEGRO_EVENT_MOUSE_AXES) {
		if (data->button) {
			int x = ev->mouse.x, y = ev->mouse.y;
			WindowCoordsToViewport(game, &x, &y);
			x *= al_get_bitmap_width(data->canvas) / (float)game->viewport.width;
			y *= al_get_bitmap_height(data->canvas) / (float)game->viewport.height;
			al_set_target_bitmap(data->canvas);
			al_draw_line(data->x, data->y, x, y, al_map_rgb(255,255,255), 13);
			al_draw_filled_rounded_rectangle(x-5, y-5, x+5, y+5, 2, 2, al_map_rgb(255,255,255));

			data->x = x;
			data->y = y;

			al_set_target_backbuffer(game->display);
		}
	}

	if ((ev->type==ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_C)) {
		al_set_target_bitmap(data->canvas);
		al_clear_to_color(al_map_rgba(0, 0, 0, 0));
		al_set_target_backbuffer(game->display);
	}

	if ((ev->type==ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_D)) {
		data->drawing = !data->drawing;
	}

	if ((ev->type==ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_SPACE)) {
		data->stage++;
		if (data->stage == 5) {
			SelectSpritesheet(game, data->warthog, "happy");
		}
		if (data->stage == 6) {
			data->stage = 0;
		}
	}

	if (ev->type == ALLEGRO_EVENT_DISPLAY_RESIZE) {
		al_destroy_bitmap(data->tmp);
		data->tmp = CreateNotPreservedBitmap(game->viewport.width, game->viewport.height);
	}
}

void GenerateLight(ALLEGRO_BITMAP *bitmap, int maxr) {
	int width = al_get_bitmap_width(bitmap);
	int height = al_get_bitmap_height(bitmap);
	ALLEGRO_LOCKED_REGION *region = al_lock_bitmap(bitmap, ALLEGRO_PIXEL_FORMAT_RGBA_8888, ALLEGRO_LOCK_WRITEONLY);
	char *d = region->data;
	for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
			float r = sqrt(pow(x-325, 2) + pow(y-256, 2));
			for (int z = 0; z < region->pixel_size; z++) {
				d[x * region->pixel_size + region->pitch * y + z] = fmin(255, fmax(0, maxr - r) * (256 / (float)maxr) * 4);
				if (!r) {
					d[x * region->pixel_size + region->pitch * y + z] = 255;
				}
			}
		}
	}
	al_unlock_bitmap(bitmap);
}

void* Gamestate_Load(struct Game *game, void (*progress)(struct Game*)) {
	// Called once, when the gamestate library is being loaded.
	// Good place for allocating memory, loading bitmaps etc.

	al_set_new_bitmap_flags(ALLEGRO_MIN_LINEAR | ALLEGRO_MAG_LINEAR);

	struct GamestateResources *data = malloc(sizeof(struct GamestateResources));
	data->font = al_create_builtin_font();

	data->bg = al_load_bitmap(GetDataFilePath(game, "bg.png"));
	progress(game); // report that we progressed with the loading, so the engine can draw a progress bar
	data->canvas = al_create_bitmap(320*2, 180*2);
	progress(game); // report that we progressed with the loading, so the engine can draw a progress bar

	al_set_target_bitmap(data->canvas);
	al_clear_to_color(al_map_rgba(0, 0, 0, 0));
	al_set_target_backbuffer(game->display);

	data->maskbmp = al_load_bitmap(GetDataFilePath(game, "mask.png"));
	data->mask = al_lock_bitmap(data->maskbmp, ALLEGRO_PIXEL_FORMAT_RGBA_8888, ALLEGRO_LOCK_READONLY);
	progress(game); // report that we progressed with the loading, so the engine can draw a progress bar

	data->tmp = CreateNotPreservedBitmap(game->viewport.width, game->viewport.height);

	data->light1 = al_create_bitmap(320*2, 180*2);
	data->light2 = al_create_bitmap(320*2, 180*2);
	data->light3 = al_create_bitmap(320*2, 180*2);
	data->light4 = al_create_bitmap(320*2, 180*2);

	GenerateLight(data->light1, 32);
	progress(game); // report that we progressed with the loading, so the engine can draw a progress bar
	GenerateLight(data->light2, 64);
	progress(game); // report that we progressed with the loading, so the engine can draw a progress bar
	GenerateLight(data->light3, 128);
	progress(game); // report that we progressed with the loading, so the engine can draw a progress bar

	al_set_target_bitmap(data->light4);
	al_clear_to_color(al_map_rgb(255,255,255));
	al_set_target_backbuffer(game->display);

	data->warthog = CreateCharacter(game, "warthog");
	data->table = CreateCharacter(game, "table");
	data->fire = CreateCharacter(game, "fire");

	RegisterSpritesheet(game, data->warthog, "1");
	RegisterSpritesheet(game, data->warthog, "2");
	RegisterSpritesheet(game, data->warthog, "3");
	RegisterSpritesheet(game, data->warthog, "happy");
	RegisterSpritesheet(game, data->table, "1");
	RegisterSpritesheet(game, data->table, "2");
	RegisterSpritesheet(game, data->table, "3");
	RegisterSpritesheet(game, data->fire, "fire");
	LoadSpritesheets(game, data->warthog);
	progress(game); // report that we progressed with the loading, so the engine can draw a progress bar
	LoadSpritesheets(game, data->table);
	progress(game); // report that we progressed with the loading, so the engine can draw a progress bar
	LoadSpritesheets(game, data->fire);

	progress(game); // report that we progressed with the loading, so the engine can draw a progress bar
	return data;
}

void Gamestate_Unload(struct Game *game, struct GamestateResources* data) {
	// Called when the gamestate library is being unloaded.
	// Good place for freeing all allocated memory and resources.
	al_destroy_font(data->font);

	al_unlock_bitmap(data->maskbmp);
	al_destroy_bitmap(data->maskbmp);
	al_destroy_bitmap(data->canvas);
	al_destroy_bitmap(data->light1);
	al_destroy_bitmap(data->light2);
	al_destroy_bitmap(data->light3);
	al_destroy_bitmap(data->light4);
	al_destroy_bitmap(data->bg);
	DestroyCharacter(game, data->fire);
	DestroyCharacter(game, data->warthog);
	DestroyCharacter(game, data->table);
	free(data);
}

void Gamestate_Start(struct Game *game, struct GamestateResources* data) {
	// Called when this gamestate gets control. Good place for initializing state,
	// playing music etc.
	data->blink_counter = 0;
	al_show_mouse_cursor(game->display);
	data->button = false;

	ALLEGRO_MOUSE_STATE state;
	al_get_mouse_state(&state);
	data->x = state.x * al_get_bitmap_width(data->canvas) / (float)game->viewport.width;
	data->y = state.y * al_get_bitmap_height(data->canvas) / (float)game->viewport.height;

	SelectSpritesheet(game, data->warthog, "1");
	SelectSpritesheet(game, data->table, "1");
	SelectSpritesheet(game, data->fire, "fire");
	SetCharacterPositionF(game, data->warthog, 0.3776, 0.04166, 0);
	SetCharacterPositionF(game, data->table, 0.24505, 0.714814, 0);
	SetCharacterPositionF(game, data->fire, 0.502, 0.6775, 0);

	data->stage = 0;
	data->drawing = false;

}

void Gamestate_Stop(struct Game *game, struct GamestateResources* data) {
	// Called when gamestate gets stopped. Stop timers, music etc. here.
}

void Gamestate_Pause(struct Game *game, struct GamestateResources* data) {
	// Called when gamestate gets paused (so only Draw is being called, no Logic not ProcessEvent)
	// Pause your timers here.
}

void Gamestate_Resume(struct Game *game, struct GamestateResources* data) {
	// Called when gamestate gets resumed. Resume your timers here.
}

// Ignore this for now.
// TODO: Check, comment, refine and/or remove:
void Gamestate_Reload(struct Game *game, struct GamestateResources* data) {}
