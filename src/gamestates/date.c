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
		ALLEGRO_FONT *font, *smallfont;
		unsigned int blink_counter;
		ALLEGRO_BITMAP *canvas;
		ALLEGRO_BITMAP *drawbmp;
		ALLEGRO_BITMAP *pointer, *pencil;

		ALLEGRO_BITMAP *sn, *sheart, *sberry, *swarthog;

		ALLEGRO_AUDIO_STREAM *bgnoise, *careless;

		ALLEGRO_BITMAP *light1, *light2, *light3, *light4, *tmp, *bg;
		struct Character *warthog, *table, *fire;
		bool button;
		int x, y;
		float rand;

		int stage;
		bool drawing;

		int timeleft, time;
		bool cheat;

		struct {
				bool name;
				bool weakness;
				bool bitten;
				bool crocodile;
		} facts;

		bool skip;
		char* text;
		bool player;

		struct Timeline *timeline;

		float score1, score2;

		bool end;

		float heart1, heart2, hearts;
		ALLEGRO_BITMAP *heart;

		bool touch;
};

bool Speak(struct Game *game, struct TM_Action *action, enum TM_ActionState state) {
	struct GamestateResources *data = TM_GetArg(action->arguments, 0);
	ALLEGRO_AUDIO_STREAM *stream = TM_GetArg(action->arguments, 1);
	char *text = TM_GetArg(action->arguments, 2);
	bool player = TM_GetArg(action->arguments, 3);

	if (state == TM_ACTIONSTATE_INIT) {
		al_set_audio_stream_playing(stream, false);
		al_set_audio_stream_playmode(stream, ALLEGRO_PLAYMODE_ONCE);
	}

	if (state == TM_ACTIONSTATE_START) {
		data->skip = false;
		data->text = text;
		data->player = player;
		//al_rewind_audio_stream(stream);
		al_attach_audio_stream_to_mixer(stream, game->audio.voice);
		al_set_audio_stream_playing(stream, true);
	}

	if (state == TM_ACTIONSTATE_RUNNING) {
		return !al_get_audio_stream_playing(stream) || data->skip;
	}

	if (state == TM_ACTIONSTATE_DESTROY) {
		al_destroy_audio_stream(stream);
		data->text = NULL;
	}
	return false;
}

bool NextStage(struct Game *game, struct TM_Action *action, enum TM_ActionState state) {
	struct GamestateResources *data = TM_GetArg(action->arguments, 0);


	if (state == TM_ACTIONSTATE_START) {
		data->stage++;
	}

	if (state == TM_ACTIONSTATE_RUNNING) {
		return true;
	}

	return false;
}

bool End(struct Game *game, struct TM_Action *action, enum TM_ActionState state) {
	struct GamestateResources *data = TM_GetArg(action->arguments, 0);

	if (state == TM_ACTIONSTATE_START) {
		al_set_audio_stream_playing(data->careless, true);
		data->end = true;
	}

	if (state == TM_ACTIONSTATE_RUNNING) {
		return true;
	}

	return false;
}

bool DecideWhatToDo(struct Game *game, struct TM_Action *action, enum TM_ActionState state);

void CalculateScore(struct Game *game, struct GamestateResources* data) {
	int width = al_get_bitmap_width(data->canvas);
	int height = al_get_bitmap_height(data->canvas);
	ALLEGRO_LOCKED_REGION *region = al_lock_bitmap(data->canvas, ALLEGRO_PIXEL_FORMAT_RGBA_8888, ALLEGRO_LOCK_READONLY);

	ALLEGRO_LOCKED_REGION *region2 = al_lock_bitmap(data->drawbmp, ALLEGRO_PIXEL_FORMAT_RGBA_8888, ALLEGRO_LOCK_READONLY);

	char *d = region->data;
	char *mask = region2->data;
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
	al_unlock_bitmap(data->drawbmp);

	data->score1 = (white/(float)(white+black)); // percentage of drawing inside
	data->score2 = (white2/(float)(white2+black2)); // percentage of inside drawed on

	PrintConsole(game, "%f%% %f%% = %f%%", (white/(float)(white+black)) * 100, (white2/(float)(white2+black2)) * 100,
	             ((white/(float)(white+black)) * 100 + (white2/(float)(white2+black2)) * 100) - 100);

}

bool Draw(struct Game *game, struct TM_Action *action, enum TM_ActionState state) {
	struct GamestateResources *data = TM_GetArg(action->arguments, 0);
	ALLEGRO_BITMAP *bmp = TM_GetArg(action->arguments, 1);

	if (state == TM_ACTIONSTATE_START) {
		data->drawing = true;
		data->time = 60*6;
		data->timeleft = data->time;
		data->button = false;
		data->drawbmp = bmp;
		data->score1 = 0;
		data->score2 = 0;

		al_set_target_bitmap(data->canvas);
		al_clear_to_color(al_map_rgba(0, 0, 0, 0));
		al_set_target_backbuffer(game->display);

		al_hide_mouse_cursor(game->display);
	}

	if (state == TM_ACTIONSTATE_RUNNING) {
		if (!data->drawing) {

			// points counting

			if (!game->config.fullscreen) {
				al_show_mouse_cursor(game->display);
			}

			bool won = true;

			CalculateScore(game, data);
			PrintConsole(game, "score1: %f%%, score2: %f%%", data->score1 * 100, data->score2 * 100);

			if (data->stage == 1) {

				won = (data->score1 > 0.75) && (data->score2 > 0.45);

				if (data->cheat) {
					won = true;
					data->cheat = false;
				}

				if (won) {
					TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 4, data,
					             al_load_audio_stream(GetDataFilePath(game, "voices/player-04.flac"), 4, 1024),
					             "Umm, Nolan.", true), "speak");
					data->stage++;
					TM_AddAction(data->timeline, &DecideWhatToDo, TM_AddToArgs(NULL, 1, data), "decidewhattodo");
				} else {

					    TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 4, data,
							             al_load_audio_stream(GetDataFilePath(game, "voices/player-03.flac"), 4, 1024),
							             "HRMPF!!!", true), "speak");

							TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 4, data,
							             al_load_audio_stream(GetDataFilePath(game, "voices/greg-04.flac"), 4, 1024),
							             "Oh, sorry, did I take it too aggressively? I'm so bad at this... Let's try again.", false), "speak");

							// TODO: *sigh* Okay. Once again.

							TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 4, data,
							             al_load_audio_stream(GetDataFilePath(game, "voices/greg-02.flac"), 4, 1024),
							             "So... uhmm... My name is Greg. What's yours?", false), "speak");

							TM_AddAction(data->timeline, &Draw, TM_AddToArgs(NULL, 2, data, data->sn), "draw");
				}
			}



			else if (data->stage == 2) {

				won = (data->score1 > 0.8) && (data->score2 > 0.35);
				if (data->cheat) {
					won = true;
					data->cheat = false;
				}

				if (won) {
					TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 4, data,
					             al_load_audio_stream(GetDataFilePath(game, "voices/player-12.flac"), 4, 1024),
					             "Strawberries!!!", true), "speak");
					if (data->facts.crocodile) {
						TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 4, data,
						             al_load_audio_stream(GetDataFilePath(game, "voices/greg-12.flac"), 4, 1024),
						             "You sure do have a thing for strawberries, huh?", false), "speak");
					} else {
						TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 4, data,
						             al_load_audio_stream(GetDataFilePath(game, "voices/greg-11.flac"), 4, 1024),
						             "Oh, that's cool! I like them too.", false), "speak");

					}
					data->stage++;
					TM_AddAction(data->timeline, &DecideWhatToDo, TM_AddToArgs(NULL, 1, data), "decidewhattodo");
				} else {

					    TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 4, data,
							             al_load_audio_stream(GetDataFilePath(game, "voices/player-11.flac"), 4, 1024),
							             "Um, hrmpf, no.", true), "speak");

							TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 4, data,
							             al_load_audio_stream(GetDataFilePath(game, "voices/greg-04.flac"), 4, 1024),
							             "Oh, sorry, did I take it too aggressively? I'm so bad at this... Let's try again.", false), "speak");

							// TODO: *sigh* Okay. Once again.
	data->stage--;
	            TM_AddAction(data->timeline, &DecideWhatToDo, TM_AddToArgs(NULL, 1, data), "decidewhattodo");
				}
			}



			else if (data->stage == 3) {

				won = (data->score1 > 0.85) && (data->score2 > 0.45);
				if (data->cheat) {
					won = true;
					data->cheat = false;
				}

				if (won) {

					if (!data->facts.bitten) {
					TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 4, data,
					             al_load_audio_stream(GetDataFilePath(game, "voices/player-14.flac"), 4, 1024),
					             "Well, okay... When I was little, I got bitten by a warthog. I've been afraid of them ever since.", true), "speak");

					TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 4, data,
					             al_load_audio_stream(GetDataFilePath(game, "voices/player-15.flac"), 4, 1024),
					             "Uff. There it is. My biggest secret.", true), "speak");
					data->facts.bitten = true;
					} else {
						TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 4, data,
						             al_load_audio_stream(GetDataFilePath(game, "voices/player-16.flac"), 4, 1024),
						             "I think the thing with... you know, warthogs. That was it.", true), "speak");

					}

					TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 4, data,
					             al_load_audio_stream(GetDataFilePath(game, "voices/greg-19.flac"), 4, 1024),
					             "That's okay. I'm proud of you, that was really brave.", false), "speak");


					TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 4, data,
					             al_load_audio_stream(GetDataFilePath(game, "voices/greg-21.flac"), 4, 1024),
					             "You know, at first I was a bit afraid that we wouldn’t click, but I think I warmed up to you.", false), "speak");
					TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 4, data,
					             al_load_audio_stream(GetDataFilePath(game, "voices/greg-22.flac"), 4, 1024),
					             "There is just something you should know about me...", false), "speak");
					TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 4, data,
					             al_load_audio_stream(GetDataFilePath(game, "voices/greg-23.flac"), 4, 1024),
					             "But I worry that it will make you hate me.", false), "speak");

					TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 4, data,
					             al_load_audio_stream(GetDataFilePath(game, "voices/player-17.flac"), 4, 1024),
					             "You don’t really like strawberries, do you?", true), "speak");

					TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 4, data,
					             al_load_audio_stream(GetDataFilePath(game, "voices/greg-24.flac"), 4, 1024),
					             "No, that's not it.", false), "speak");

					TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 4, data,
					             al_load_audio_stream(GetDataFilePath(game, "voices/player-18.flac"), 4, 1024),
					             "Is it connected to the fact that you’re all furry?", true), "speak");


					TM_AddAction(data->timeline, &NextStage, TM_AddToArgs(NULL, 1, data), "nextstage");

					TM_AddAction(data->timeline, &DecideWhatToDo, TM_AddToArgs(NULL, 1, data), "decidewhattodo");
				} else {

					    TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 4, data,
							             al_load_audio_stream(GetDataFilePath(game, "voices/player-13.flac"), 4, 1024),
							             "I... was. Born. ... Too. Eh. ", true), "speak");

							TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 4, data,
							             al_load_audio_stream(GetDataFilePath(game, "voices/greg-20.flac"), 4, 1024),
							             "Hmm, are you joking? I'm really trying my best here to open up and you are just mocking me.", false), "speak");

							TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 4, data,
							             al_load_audio_stream(GetDataFilePath(game, "voices/greg-05.flac"), 4, 1024),
							             "*sigh* Okay. Once again.", false), "speak");
	data->stage--;
	            TM_AddAction(data->timeline, &DecideWhatToDo, TM_AddToArgs(NULL, 1, data), "decidewhattodo");
				}
			}


			else if (data->stage == 4) {

				won = (data->score1 > 0.9) && (data->score2 > 0.5);
				if (data->cheat) {
					won = true;
					data->cheat = false;
				}

				if (won) {
					TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 4, data,
					             al_load_audio_stream(GetDataFilePath(game, "voices/player-19.flac"), 4, 1024),
					             "Oh... oh! Um... I guess that's okay!", true), "speak");
					TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 4, data,
					             al_load_audio_stream(GetDataFilePath(game, "voices/player-20.flac"), 4, 1024),
					             "You're... nice! I'm sorry, I've been wrong about warthogs all this time.", true), "speak");
					TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 4, data,
					             al_load_audio_stream(GetDataFilePath(game, "voices/player-21.flac"), 4, 1024),
					             "Would you maybe... want to spend more time... with me?", true), "speak");
					TM_AddAction(data->timeline, &NextStage, TM_AddToArgs(NULL, 1, data), "nextstage");

					TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 4, data,
					             al_load_audio_stream(GetDataFilePath(game, "voices/greg-26.flac"), 4, 1024),
					             "Oh, really? Absolutely!", false), "speak");

					TM_AddAction(data->timeline, &End, TM_AddToArgs(NULL, 1, data), "end");
					TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 4, data,
					             al_load_audio_stream(GetDataFilePath(game, "voices/love.flac"), 4, 1024),
					             NULL, false), "speak");

				} else {
					TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 4, data,
					             al_load_audio_stream(GetDataFilePath(game, "voices/player-01.flac"), 4, 1024),
					             "Uhm...", true), "speak");

					TM_AddAction(data->timeline, &Draw, TM_AddToArgs(NULL, 2, data, data->sheart), "draw");

				}
			}
			return true;
		}
		return false;



	}


	return false;
}

int WrappedTextWithShadow(struct Game *game, ALLEGRO_FONT *font, ALLEGRO_COLOR color, float x, float y, int width, int flags, char const *text) {
	     DrawWrappedText(font, al_map_rgba(0,0,0,255), x+(4/1920.0)*game->viewport.width, y+(4/1080.0)*game->viewport.height, width, flags, text);
			 return DrawWrappedText(font, color, x, y, width, flags, text);
}

bool DecideWhatToDo(struct Game *game, struct TM_Action *action, enum TM_ActionState state) {
	struct GamestateResources *data = TM_GetArg(action->arguments, 0);

	if (state == TM_ACTIONSTATE_START) {
		if (data->stage == 1) {

			if (data->facts.name) {
				TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 4, data,
				             al_load_audio_stream(GetDataFilePath(game, "voices/greg-03.flac"), 4, 1024),
				             "Uhm, sorry, but I forgot your name. Could you tell me again?", false), "speak");
			} else {
				TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 4, data,
				             al_load_audio_stream(GetDataFilePath(game, "voices/greg-01.flac"), 4, 1024),
				             "Uhm... hello... Nice to meet you...", false), "speak");
				TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 4, data,
				             al_load_audio_stream(GetDataFilePath(game, "voices/player-02.flac"), 4, 1024),
				             "Hrmpf", true), "speak");
				TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 4, data,
				             al_load_audio_stream(GetDataFilePath(game, "voices/greg-02.flac"), 4, 1024),
				             "So... uhmm... My name is Greg. What's yours?", false), "speak");
			}
			data->facts.name = true;
			TM_AddAction(data->timeline, &Draw, TM_AddToArgs(NULL, 2, data, data->sn), "draw");

//			TM_AddAction(data->timeline, &DecideWhatToDo, TM_AddToArgs(NULL, 1, data), "decidewhattodo");

		}

		if (data->stage == 2) {
			if (!data->facts.weakness) {
				TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 4, data,
				             al_load_audio_stream(GetDataFilePath(game, "voices/greg-06.flac"), 4, 1024),
				             "Hi Nolan! ... Ok. You don't seem to use a lot of words...", false), "speak");

				TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 4, data,
				             al_load_audio_stream(GetDataFilePath(game, "voices/player-05.flac"), 4, 1024),
				             "Hrmpf.", true), "speak");

				TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 4, data,
				             al_load_audio_stream(GetDataFilePath(game, "voices/greg-07.flac"), 4, 1024),
				             "I must admit that I have a weakness for people who just know what they want.", false), "speak");

				TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 4, data,
				             al_load_audio_stream(GetDataFilePath(game, "voices/greg-08.flac"), 4, 1024),
				             "So... maybe let's get to know each other!", false), "speak");

				TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 4, data,
				             al_load_audio_stream(GetDataFilePath(game, "voices/player-10.flac"), 4, 1024),
				             "Hrmpf, ok...", true), "speak");

				TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 4, data,
				             al_load_audio_stream(GetDataFilePath(game, "voices/greg-09.flac"), 4, 1024),
				             "I told you about one of my weaknesses. Do *you* have any?", false), "speak");
			} else {
				TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 4, data,
				             al_load_audio_stream(GetDataFilePath(game, "voices/greg-10.flac"), 4, 1024),
				             "So, what was it about your weakness again?", false), "speak");

			}
			data->facts.weakness = true;
			TM_AddAction(data->timeline, &Draw, TM_AddToArgs(NULL, 2, data, data->sberry), "draw");


		}



		if (data->stage == 3) {
			if (!data->facts.crocodile) {
				TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 4, data,
				             al_load_audio_stream(GetDataFilePath(game, "voices/greg-13.flac"), 4, 1024),
				             "You know, you seem like a very nice... um, person. I'd really like to share something with you.", false), "speak");


				TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 4, data,
				             al_load_audio_stream(GetDataFilePath(game, "voices/greg-14.flac"), 4, 1024),
				             "Okay... There it goes... I'm... I'm afraid of crocodiles.", false), "speak");

				TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 4, data,
				             al_load_audio_stream(GetDataFilePath(game, "voices/greg-15.flac"), 4, 1024),
				             "They just have this really weird look. Like they are making fun of me.", false), "speak");

				TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 4, data,
				             al_load_audio_stream(GetDataFilePath(game, "voices/greg-16.flac"), 4, 1024),
				             "And I never know if they just want to eat me or if they are laughing.", false), "speak");

				TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 4, data,
				             al_load_audio_stream(GetDataFilePath(game, "voices/greg-17.flac"), 4, 1024),
				             "Please don't tell anyone.", false), "speak");
			}
			  TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 4, data,
				             al_load_audio_stream(GetDataFilePath(game, "voices/greg-18.flac"), 4, 1024),
				             "Would you like to share something with me?", false), "speak");

			data->facts.crocodile = true;
			TM_AddAction(data->timeline, &Draw, TM_AddToArgs(NULL, 2, data, data->swarthog), "draw");

		}

		if (data->stage == 4) {


			TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 4, data,
			             al_load_audio_stream(GetDataFilePath(game, "voices/greg-25.flac"), 4, 1024),
			             "Yes, kind of... so... it’s just that I'm a warthog.", false), "speak");

			TM_AddAction(data->timeline, &Draw, TM_AddToArgs(NULL, 2, data, data->sheart), "draw");


		}



	}

	return true;
}

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
	TM_Process(data->timeline);

if (data->end) {
	data->hearts += 0.01;
	data->heart1 -= 0.01;
	data->heart2 -= 0.01;

	if (data->heart2 < -0.2) {
		data->heart1 += 1.7;
		data->heart2 += 1.7;
	}
}
  data->blink_counter++;
	if (data->blink_counter % 4 == 0) {
		data->rand = rand() / (float)RAND_MAX / 4.0 + 0.75;
	}
	if (data->blink_counter == 60) {
		data->blink_counter = 0;
	}
	if (data->drawing) {
		data->timeleft--;
	}
	if (data->timeleft==0) {
		data->drawing = false;
		data->button = false;
	}

	AnimateCharacter(game, data->fire, 1);
	AnimateCharacter(game, data->warthog, 1);
}

void Gamestate_Draw(struct Game *game, struct GamestateResources* data) {
	// Called as soon as possible, but no sooner than next Gamestate_Logic call.
	// Draw everything to the screen here.

	al_draw_scaled_bitmap(data->bg, 0, 0, al_get_bitmap_width(data->bg), al_get_bitmap_height(data->bg), 0, 0, game->viewport.width, game->viewport.height, 0);

	SwitchSpritesheet(game, data->table, "1");

	int scale = 1;
#ifdef ALLEGRO_ANDROID
	scale = 2;
#endif
	if ((data->stage < 5) && (data->stage)) {
		SwitchSpritesheet(game, data->warthog, "1");
		DrawScaledCharacter(game, data->warthog, al_map_rgb(255,255,255), scale * game->viewport.width / (float)3840, scale * game->viewport.height / (float)2160, 0);
	}
	DrawScaledCharacter(game, data->table, al_map_rgb(255,255,255), scale * game->viewport.width / (float)3840, scale * game->viewport.height / (float)2160, 0);

	if (data->stage >= 1) {
		al_set_target_bitmap(data->tmp);
		al_clear_to_color(al_map_rgba(0,0,0,0));

		SwitchSpritesheet(game, data->table, "2");
//		if (data->stage < 5) {
		  SwitchSpritesheet(game, data->warthog, "2");
			DrawScaledCharacter(game, data->warthog, al_map_rgb(255,255,255), scale * game->viewport.width / (float)3840, scale * game->viewport.height / (float)2160, 0);
//		}
		DrawScaledCharacter(game, data->table, al_map_rgb(255,255,255), scale * game->viewport.width / (float)3840, scale * game->viewport.height / (float)2160, 0);
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
		DrawScaledCharacter(game, data->warthog, al_map_rgb(255,255,255), scale * game->viewport.width / (float)3840, scale * game->viewport.height / (float)2160, 0);
		DrawScaledCharacter(game, data->table, al_map_rgb(255,255,255), scale * game->viewport.width / (float)3840, scale * game->viewport.height / (float)2160, 0);
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
	} else {
		al_draw_text(data->font, al_map_rgba_f(data->rand, data->rand, data->rand, data->rand), game->viewport.width / 2, game->viewport.height * 0.3, ALLEGRO_ALIGN_CENTER, "The Blind Date");
		if (data->blink_counter < 40) {
			char *text = "Press SPACE...";
#ifdef ALLEGRO_ANDROID
			text = "Touch to start...";
#endif
			al_draw_text(data->smallfont, al_map_rgba_f(data->rand, data->rand, data->rand, data->rand), game->viewport.width / 2, game->viewport.height * 0.55, ALLEGRO_ALIGN_CENTER, text);
		}
	}


	if (data->text) {
		if (data->player) {
			float pos = 0.85;
			if (strlen(data->text) > 70) {
				pos = 0.75;
			}
			if (data->stage >= 4) {
				al_draw_filled_rectangle(0, game->viewport.height * (pos - 0.02), game->viewport.width, game->viewport.height, al_map_rgba(0,0,0,128));
			}

			WrappedTextWithShadow(game, data->smallfont, al_map_rgba_f(1,1,1,1), game->viewport.width * 0.05, game->viewport.height * pos, game->viewport.width * 0.9, ALLEGRO_ALIGN_CENTER, data->text);

		} else {
			if (data->stage >= 4) {
				al_draw_filled_rectangle(0, 0, game->viewport.width, game->viewport.height * 0.15,  al_map_rgba(0,0,0,128));
			}
			WrappedTextWithShadow(game, data->smallfont, al_map_rgba_f(1,1,1,1), game->viewport.width * 0.05, game->viewport.height * 0.05, game->viewport.width * 0.9, ALLEGRO_ALIGN_CENTER, data->text);
		}
	}

	if (data->drawing) {
		al_draw_filled_rectangle(0, 0, al_get_display_width(game->display), al_get_display_height(game->display), al_map_rgba(0,0,0,127));
		al_draw_tinted_scaled_bitmap(data->drawbmp, al_map_rgba(127,127,127,127), 0, 0, al_get_bitmap_width(data->drawbmp), al_get_bitmap_height(data->drawbmp), 0, 0, game->viewport.width, game->viewport.height, 0);
		al_draw_scaled_bitmap(data->canvas, 0, 0, al_get_bitmap_width(data->canvas), al_get_bitmap_height(data->canvas), 0, 0, game->viewport.width, game->viewport.height, 0);

		al_draw_filled_rectangle(0, game->viewport.height*0.98, game->viewport.width * data->timeleft / (float)data->time, game->viewport.height, al_map_rgb(255,255,255));

		float x = data->x; float y = data->y;
		x /= (float)al_get_bitmap_width(data->canvas);// * (float)game->viewport.width;
		y /= (float)al_get_bitmap_height(data->canvas);// * (float)game->viewport.height;
		if (!data->touch) {
			ALLEGRO_BITMAP *pointer = data->button ? data->pencil : data->pointer;
			al_draw_scaled_bitmap(pointer, 0, 0, al_get_bitmap_width(pointer), al_get_bitmap_height(pointer),
			                      x * game->viewport.width, y * game->viewport.height,
			                      game->viewport.width * 0.05, game->viewport.height * 0.06, 0);
		}

	}

	if (data->end) {

		al_draw_text(data->font, al_map_rgba_f(data->rand, data->rand, data->rand, data->rand), game->viewport.width * 0.2, game->viewport.height * 0.3, ALLEGRO_ALIGN_CENTER, "LOVE");
		al_draw_text(data->font, al_map_rgba_f(data->rand, data->rand, data->rand, data->rand), game->viewport.width * 0.83, game->viewport.height * 0.6, ALLEGRO_ALIGN_CENTER, "LOVE");

		al_draw_scaled_bitmap(data->heart, 0, 0, al_get_bitmap_width(data->heart), al_get_bitmap_height(data->heart),
		                      (0.1 + cos(data->hearts) * 0.15) * game->viewport.width, data->heart1 * game->viewport.height,
		                      game->viewport.width * 0.06, game->viewport.height * 0.12, 0);

		al_draw_scaled_bitmap(data->heart, 0, 0, al_get_bitmap_width(data->heart), al_get_bitmap_height(data->heart),
		                      (0.85 - sin(data->hearts) * 0.15) * game->viewport.width, data->heart2 * game->viewport.height,
		                      game->viewport.width * 0.06, game->viewport.height * 0.12, 0);

	}

}

void Gamestate_ProcessEvent(struct Game *game, struct GamestateResources* data, ALLEGRO_EVENT *ev) {
	// Called for each event in Allegro event queue.
	// Here you can handle user input, expiring timers etc.
	TM_HandleEvent(data->timeline, ev);

	if ((ev->type==ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_ESCAPE)) {
		UnloadCurrentGamestate(game); // mark this gamestate to be stopped and unloaded
		// When there are no active gamestates, the engine will quit.
	}

	if ((ev->type==ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) || (ev->type==ALLEGRO_EVENT_TOUCH_BEGIN)) {
		if ((ev->mouse.button==1) || (ev->type==ALLEGRO_EVENT_TOUCH_BEGIN)) {
			data->button = !data->button;
			int x = ev->mouse.x, y = ev->mouse.y;
			if (ev->type==ALLEGRO_EVENT_TOUCH_BEGIN) {
				x = ev->touch.x; y = ev->touch.y;
			}
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

	if ((ev->type==ALLEGRO_EVENT_MOUSE_AXES) || (ev->type==ALLEGRO_EVENT_TOUCH_MOVE)) {
		int x = ev->mouse.x, y = ev->mouse.y;
		if (ev->type==ALLEGRO_EVENT_TOUCH_MOVE) {
			data->touch = true;
			x = ev->touch.x; y = ev->touch.y;
		}
		WindowCoordsToViewport(game, &x, &y);
		x *= al_get_bitmap_width(data->canvas) / (float)game->viewport.width;
		y *= al_get_bitmap_height(data->canvas) / (float)game->viewport.height;
		if ((data->button) || ((ev->type==ALLEGRO_EVENT_TOUCH_MOVE) && (ev->touch.primary))) {
			al_set_target_bitmap(data->canvas);
			al_draw_line(data->x, data->y, x, y, al_map_rgb(255,255,255), 13);
			al_draw_filled_rounded_rectangle(x-5, y-5, x+5, y+5, 2, 2, al_map_rgb(255,255,255));

			al_set_target_backbuffer(game->display);
		}
		data->x = x;
		data->y = y;
	}

	if ((ev->type==ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_C)) {
		data->cheat = true;
	}

	if ((ev->type==ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_FULLSTOP)) {
		data->skip = true;
	}

	if ((ev->type==ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_S)) {
		data->timeleft = 1;
	}

/*	if ((ev->type==ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_D)) {
		data->drawing = !data->drawing;
		data->time = 60*6;
		data->timeleft = data->time;
		data->button = false;

		al_set_target_bitmap(data->canvas);
		al_clear_to_color(al_map_rgba(0, 0, 0, 0));
		al_set_target_backbuffer(game->display);
	}*/

	if (ev->type==ALLEGRO_EVENT_KEY_DOWN) {
		data->touch = false;
	}

	if (((ev->type==ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_SPACE)) ||
	  (ev->type==ALLEGRO_EVENT_TOUCH_BEGIN)) {
		if (data->stage == 0){
			TM_AddAction(data->timeline, &DecideWhatToDo, TM_AddToArgs(NULL, 1, data), "start");
			data->stage++;
			return;
		}
		return;
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
		al_destroy_font(data->font);
		data->font = al_load_font(GetDataFilePath(game, "fonts/VINCHAND.ttf"), game->viewport.height * 0.2, 0);
		al_destroy_font(data->smallfont);
		data->smallfont = al_load_font(GetDataFilePath(game, "fonts/VINCHAND.ttf"), game->viewport.height * 0.072, 0);
	}
}

void GenerateLight(ALLEGRO_BITMAP *bitmap, int maxr) {
	int width = al_get_bitmap_width(bitmap);
	int height = al_get_bitmap_height(bitmap);
	ALLEGRO_LOCKED_REGION *region = al_lock_bitmap(bitmap, ALLEGRO_PIXEL_FORMAT_RGBA_8888, ALLEGRO_LOCK_WRITEONLY);
	unsigned char *d = region->data;
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
	data->font = al_load_font(GetDataFilePath(game, "fonts/VINCHAND.ttf"), game->viewport.height * 0.2, 0);
	data->smallfont = al_load_font(GetDataFilePath(game, "fonts/VINCHAND.ttf"), game->viewport.height * 0.072, 0);

	data->bg = al_load_bitmap(GetDataFilePath(game, "bg.png"));
	progress(game); // report that we progressed with the loading, so the engine can draw a progress bar
	data->canvas = al_create_bitmap(320*2, 180*2);
	progress(game); // report that we progressed with the loading, so the engine can draw a progress bar

	data->bgnoise = al_load_audio_stream(GetDataFilePath(game, "bg.ogg"), 4, 1024);
	al_attach_audio_stream_to_mixer(data->bgnoise, game->audio.fx);
	al_set_audio_stream_playmode(data->bgnoise, ALLEGRO_PLAYMODE_LOOP);
	al_set_audio_stream_gain(data->bgnoise, 0.5);
	al_set_audio_stream_playing(data->bgnoise, false);

	data->careless = al_load_audio_stream(GetDataFilePath(game, "careless.ogg"), 4, 1024);
	al_attach_audio_stream_to_mixer(data->careless, game->audio.music);
	al_set_audio_stream_playmode(data->careless, ALLEGRO_PLAYMODE_LOOP);
	al_set_audio_stream_playing(data->careless, false);


	al_set_target_bitmap(data->canvas);
	al_clear_to_color(al_map_rgba(0, 0, 0, 0));
	al_set_target_backbuffer(game->display);

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

//	ALLEGRO_BITMAP *sn, *sheart, *sberry, *swarthog;
	data->sn = al_load_bitmap(GetDataFilePath(game, "symbols/n.png"));
	data->sheart = al_load_bitmap(GetDataFilePath(game, "symbols/heart.png"));
	data->sberry = al_load_bitmap(GetDataFilePath(game, "symbols/berry.png"));
	data->swarthog = al_load_bitmap(GetDataFilePath(game, "symbols/warthog.png"));

	data->timeline = TM_Init(game, "timeline");

	data->pointer =  al_load_bitmap(GetDataFilePath(game, "point.png"));
	data->pencil =  al_load_bitmap(GetDataFilePath(game, "draw.png"));

	data->heart = al_load_bitmap(GetDataFilePath(game, "heart.png"));

	progress(game); // report that we progressed with the loading, so the engine can draw a progress bar
	return data;
}

void Gamestate_Unload(struct Game *game, struct GamestateResources* data) {
	// Called when the gamestate library is being unloaded.
	// Good place for freeing all allocated memory and resources.
	TM_Destroy(data->timeline);

	al_destroy_font(data->font);
	al_destroy_font(data->smallfont);

	al_destroy_bitmap(data->canvas);
	al_destroy_bitmap(data->pointer);
	al_destroy_bitmap(data->pencil);
	al_destroy_bitmap(data->sn);
	al_destroy_bitmap(data->sheart);
	al_destroy_bitmap(data->sberry);
	al_destroy_bitmap(data->swarthog);

	al_destroy_bitmap(data->tmp);
	al_destroy_bitmap(data->heart);

	al_destroy_bitmap(data->light1);
	al_destroy_bitmap(data->light2);
	al_destroy_bitmap(data->light3);
	al_destroy_bitmap(data->light4);
	al_destroy_bitmap(data->bg);
	DestroyCharacter(game, data->fire);
	DestroyCharacter(game, data->warthog);
	DestroyCharacter(game, data->table);

	al_destroy_audio_stream(data->bgnoise);
	al_destroy_audio_stream(data->careless);

	free(data);
}

void Gamestate_Start(struct Game *game, struct GamestateResources* data) {
	// Called when this gamestate gets control. Good place for initializing state,
	// playing music etc.
	data->blink_counter = 0;
	//al_show_mouse_cursor(game->display);
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
data->end = false;
data->timeleft = -1;
  data->facts.bitten = false;
	data->facts.name = false;
	data->facts.weakness = false;
	data->facts.crocodile = false;
	al_set_audio_stream_playing(data->bgnoise, true);
data->text = NULL;
data->drawbmp = NULL;
data->cheat = false;

data->hearts = 0;
data->heart1 = 1;
data->heart2 = 1.5;

data->touch = false;
#ifdef ALLEGRO_ANDROID
data->touch = true;
#endif
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
