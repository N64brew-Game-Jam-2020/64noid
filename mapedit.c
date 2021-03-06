#include <ultra64.h>
#include "mapsel.h"
#include "render.h"
#include "main.h"
#include "pad.h"
#include "game.h"
#include "save.h"
#include "map.h"
#include "text.h"

#define SCREEN_W 320
#define SCREEN_H 240
#define UI_BASE_POS_X (MAP_X_OFS+(MAP_WIDTH*MAP_BRICK_W)+12)
#define UI_CENTER_POS_X (UI_BASE_POS_X+((SCREEN_W-UI_BASE_POS_X)/2)-12)
#define UI_RIGHT_POS_X (SCREEN_W-24)
#define CURSOR_DEADZONE 32
#define INPUT_REPEAT_DELAY 8

static SpriteData *mapedit_sprite_data;
static SpriteInfo *sprite_cursor;
static SpriteInfo *sprite_brick;
static SpriteInfo *sprite_l_button;
static SpriteInfo *sprite_r_button;
static SpriteInfo *sprite_border;
static SpriteInfo *sprite_c_left;
static SpriteInfo *sprite_c_right;
static int cursor_x, cursor_y;
static int cursor_repeat_timer;
static int brick_repeat_timer;
static int brick_type = 0;

struct control {
	char *image_name;
	char *text;
	SpriteInfo *sprite;
};

static struct control controls_list[] = {
	{ "button_a", "Place Brick", NULL },
	{ "button_b", "Save and Exit", NULL },
	{ "button_z", "Clear Level", NULL },
	{ "button_start", "Play Level", NULL },
};

static int num_controls = sizeof(controls_list)/sizeof(struct control);
static int controls_y = (SCREEN_H-24-((sizeof(controls_list)/sizeof(struct control))*9));

static char *brick_images[] = {
	"brick_empty",
	"brick_white",
	"brick_orange",
	"brick_cyan",
	"brick_green",
	"brick_red",
	"brick_blue",
	"brick_magenta",
	"brick_yellow",
	"brick_rock3",
	"brick_gold"
};

static char brick_types[] = { '.', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'l' };

static void UpdateCursorSpritePos()
{
	SpriteSetPos(sprite_cursor, (cursor_x*MAP_BRICK_W)+MAP_X_OFS+(MAP_BRICK_W/2), (cursor_y*MAP_BRICK_H)+MAP_Y_OFS+(MAP_BRICK_H/2));
}

static void CursorInit()
{
	sprite_cursor = SpriteCreate(mapedit_sprite_data);
	SpriteSetAnim(sprite_cursor, "cursor_pulsate");
	cursor_x = MAP_WIDTH/2;
	cursor_y = MAP_HEIGHT/2;
	UpdateCursorSpritePos();
}

static void UpdateBrickSprite()
{
	SpriteSetImage(sprite_brick, brick_images[brick_type]);
}

static void BrickInit()
{
	sprite_brick = SpriteCreate(mapedit_sprite_data);
	sprite_l_button = SpriteCreate(mapedit_sprite_data);
	sprite_r_button = SpriteCreate(mapedit_sprite_data);
	SpriteSetPos(sprite_brick, UI_CENTER_POS_X, 64);
	UpdateBrickSprite();
	SpriteSetPos(sprite_l_button, UI_BASE_POS_X+8, 64);
	SpriteSetImage(sprite_l_button, "button_l");
	SpriteSetPos(sprite_r_button, UI_RIGHT_POS_X-8, 64);
	SpriteSetImage(sprite_r_button, "button_r");
}

static void LevelButtonInit()
{
	sprite_c_left = SpriteCreate(mapedit_sprite_data);
	sprite_c_right = SpriteCreate(mapedit_sprite_data);
	SpriteSetPos(sprite_c_left, UI_BASE_POS_X+4, 28);
	SpriteSetImage(sprite_c_left, "button_c_left");
	SpriteSetPos(sprite_c_right, UI_RIGHT_POS_X-4, 28);
	SpriteSetImage(sprite_c_right, "button_c_right");
}

static void BorderInit()
{
	sprite_border = SpriteCreate(mapedit_sprite_data);
	SpriteSetPos(sprite_border, MAP_X_OFS-8, MAP_Y_OFS-8);
	SpriteSetImage(sprite_border, "border");
}

static void ControlsInit()
{
	int i;
	for(i=0; i<num_controls; i++) {
		controls_list[i].sprite = SpriteCreate(mapedit_sprite_data);
		SpriteSetPos(controls_list[i].sprite, UI_BASE_POS_X, controls_y+(i*9));
		SpriteSetImage(controls_list[i].sprite, controls_list[i].image_name);
	}
}

void MapEditorInit()
{
	RenderSetSize(SCREEN_W, SCREEN_H);
	MapLoadSave(game_globals.map_num);
	mapedit_sprite_data = SpriteLoadFile("mapeditsprites.spr");
	BrickInit();
	CursorInit();
	LevelButtonInit();
	BorderInit();
	ControlsInit();
	cursor_repeat_timer = brick_repeat_timer = 0;
}

static void ClearMap()
{
	int i;
	for(i=0; i<MAP_WIDTH*MAP_HEIGHT; i++) {
		save_data->edited_maps[game_globals.map_num][i] = BRICK_EMPTY;
		MapSetBrick(MapGetBrick(i%MAP_WIDTH, i/MAP_WIDTH), BRICK_EMPTY);
	}
}

static bool IsMapEmpty()
{
	int i;
	for(i=0; i<MAP_WIDTH*MAP_HEIGHT; i++) {
		if(save_data->edited_maps[game_globals.map_num][i] != BRICK_EMPTY) {
			return false;
		}
	}
	return true;
}

static void UpdateBrickSelect()
{
	if(brick_repeat_timer == 0) {
		if(pad_data[0].trigger & L_TRIG) {
			if(brick_type == 0) {
				brick_type = (sizeof(brick_types)/sizeof(char))-1;
			} else {
				brick_type--;
			}
			brick_repeat_timer = INPUT_REPEAT_DELAY;
		}
		if(pad_data[0].trigger & R_TRIG) {
			if(brick_type >= (sizeof(brick_types)/sizeof(char))-1) {
				brick_type = 0;
			} else {
				brick_type++;
			}
			brick_repeat_timer = INPUT_REPEAT_DELAY;
		}
		UpdateBrickSprite();
	} else {
		brick_repeat_timer--;
	}
	if(pad_data[0].trigger & Z_TRIG) {
		if(!IsMapEmpty()) {
			ClearMap();
			game_globals.save_map = true;
		}
	}
}

static void UpdateMapNumber()
{
	if(pad_data[0].trigger & L_CBUTTONS) {
		if(game_globals.map_num == 0) {
			game_globals.map_num = MAX_EDITOR_MAPS-1;
		} else {
			game_globals.map_num--;
		}
		MapUnload();
		MapLoadSave(game_globals.map_num);
	}
	if(pad_data[0].trigger & R_CBUTTONS) {
		if(game_globals.map_num == MAX_EDITOR_MAPS-1) {
			game_globals.map_num = 0;
		} else {
			game_globals.map_num++;
		}
		MapUnload();
		MapLoadSave(game_globals.map_num);
	}
}

static void UpdateCursor()
{
	if(cursor_repeat_timer == 0) {
		if(pad_data[0].stick_x <= -CURSOR_DEADZONE && cursor_x != 0) {
			cursor_x--;
			cursor_repeat_timer = INPUT_REPEAT_DELAY;
		}
		if(pad_data[0].stick_x >= CURSOR_DEADZONE && cursor_x < MAP_WIDTH-1) {
			cursor_x++;
			cursor_repeat_timer = INPUT_REPEAT_DELAY;
		}
		if(pad_data[0].stick_y <= -CURSOR_DEADZONE && cursor_y < MAP_HEIGHT-1) {
			cursor_y++;
			cursor_repeat_timer = INPUT_REPEAT_DELAY;
		}
		if(pad_data[0].stick_y >= CURSOR_DEADZONE && cursor_y != 0) {
			cursor_y--;
			cursor_repeat_timer = INPUT_REPEAT_DELAY;
		}
		UpdateCursorSpritePos();
	} else {
		cursor_repeat_timer--;
	}
	if(pad_data[0].trigger & A_BUTTON) {
		save_data->edited_maps[game_globals.map_num][(cursor_y*MAP_WIDTH)+cursor_x] = brick_types[brick_type];
		MapSetBrick(MapGetBrick(cursor_x, cursor_y), brick_types[brick_type]);
		game_globals.save_map = true;
	}
}

void MapEditorUpdate()
{
	UpdateBrickSelect();
	UpdateMapNumber();
	UpdateCursor();
	if(pad_data[0].trigger & B_BUTTON) {
		if(game_globals.save_map) {
			SaveWrite();
		}
		SetNextStage(STAGE_TITLE);
	}
	if(pad_data[0].trigger & START_BUTTON && !IsMapEmpty()) {
		game_globals.score = 0;
		game_globals.num_lives = 5;
		SetNextStage(STAGE_GAME);
	}
}

static void DrawMapNumber()
{
	char text_buf[64];
	sprintf(text_buf, "Level %d", game_globals.map_num+1);
	TextDraw(UI_CENTER_POS_X, 24, TEXT_ALIGNMENT_CENTER, text_buf);
}

static void DrawControls()
{
	int i;
	for(i=0; i<num_controls; i++) {
		SpriteDraw(controls_list[i].sprite);
		TextDraw(UI_BASE_POS_X+9, controls_y+(i*9), TEXT_ALIGNMENT_LEFT, controls_list[i].text);
	}
}

void MapEditorDraw()
{
	RenderClear(0, 0, 0);
	MapDraw();
	SpriteDraw(sprite_border);
	SpriteDraw(sprite_cursor);
	SpriteDraw(sprite_brick);
	SpriteDraw(sprite_l_button);
	SpriteDraw(sprite_r_button);
	DrawMapNumber();
	SpriteDraw(sprite_c_left);
	SpriteDraw(sprite_c_right);
	DrawControls();
}

static void DestroyControls()
{
	int i;
	for(i=0; i<num_controls; i++) {
		SpriteDelete(controls_list[i].sprite);
	}
}

void MapEditorDestroy()
{
	MapUnload();
	SpriteFreeData(mapedit_sprite_data);
	DestroyControls();
	SpriteDelete(sprite_border);
	SpriteDelete(sprite_cursor);
	SpriteDelete(sprite_brick);
	SpriteDelete(sprite_l_button);
	SpriteDelete(sprite_r_button);
	SpriteDelete(sprite_c_left);
	SpriteDelete(sprite_c_right);
}