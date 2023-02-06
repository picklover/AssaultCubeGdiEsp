#pragma once

static int base_address = 0x400000;
static int player_base = base_address + 0x10F4F4;
static int entity_base = player_base + 0x4;
static int players_in_map = player_base + 0xc;

static int view_matrix = 0x501AE8;
static int team = 0x32C;

#pragma pack(1)
struct Player {
	unsigned char pad1[4];

	float head_x;
	float head_y;
	float head_z;

	unsigned char pad2[0x34 - 0xC - sizeof(head_z)];

	float foot_x;
	float foot_y;
	float foot_z;

	unsigned char pad3[0xF8 - 0x3C - sizeof(foot_z)];

	int health;

	unsigned char pad4[0x225 - 0xF8 - sizeof(health)];

	char name[12];
};
#pragma pack ()

