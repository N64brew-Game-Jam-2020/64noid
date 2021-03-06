#include <nusys.h>
#include "save.h"
#include "bool.h"

#define SAVE_MAGIC 0x5341

static bool save_used;
static SaveBuffer save_buf;
SaveData *save_data;

static void ClearSaveMap(int map)
{
	int i;
	for(i=0; i<MAP_WIDTH*MAP_HEIGHT; i++) {
		save_data->edited_maps[map][i] = '.';
	}
}

static u16 GetSaveChecksum()
{
	u16 *buf = (u16 *)save_data;
	u32 len = (&save_data->checksum)-buf;
	u16 hash = 0;
	u32 i;
	for(i=0; i<len; i++) {
		hash += *buf++;
	}
	return hash;
}

void SaveReset()
{
	int i;
	save_buf.magic = SAVE_MAGIC;
	save_data->high_score = 10000;
	for(i=0; i<MAX_EDITOR_MAPS; i++) {
		ClearSaveMap(i);
	}
	save_data->checksum = GetSaveChecksum();
	if(save_used) {
		nuEepromWrite(0, (u8 *)&save_buf, SAVE_SIZE);
	}
}

void SaveInit()
{
	save_data = &save_buf.data;
	nuEepromMgrInit();
	save_used = false;
	SaveReset();
	if(nuEepromCheck() == EEPROM_TYPE_4K || nuEepromCheck() == EEPROM_TYPE_16K) {
		save_used = true;
		nuEepromRead(0, (u8 *)&save_buf, SAVE_SIZE);
		if(save_buf.magic != SAVE_MAGIC || save_data->checksum != GetSaveChecksum()) {
			SaveReset();
		}
	}
}

void SaveWrite()
{
	save_data->checksum = GetSaveChecksum();
	if(save_used) {
		nuEepromWrite(0, (u8 *)&save_buf, SAVE_SIZE);
	}
}