/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (C) 1995 Ronny Wester
    Copyright (C) 2003 Jeremy Chin
    Copyright (C) 2003-2007 Lucas Martin-King

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    This file incorporates work covered by the following copyright and
    permission notice:

    Copyright (c) 2013-2014, Cong Xu
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
    Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.
*/
#include "map_classic.h"

#include <assert.h>

static int MapTryBuildSquare(Map *map);
static int MapTryBuildRoom(
	Map *map, Vec2i mapSize,
	int hasDoors, int doorMin, int doorMax, int hasKeys,
	int roomMinP, int roomMaxP, int edgeRooms);
static int MapTryBuildPillar(
	Map *map, Vec2i mapSize, int pillarMin, int pillarMax);
static int MapTryBuildWall(Map *map, Vec2i mapSize, int wallLength);
void MapClassicLoad(Map *map, Mission *mission)
{
	// place squares
	int count = 0;
	int i = 0;
	while (i < 1000 && count < mission->u.Classic.Squares)
	{
		if (MapTryBuildSquare(map))
		{
			count++;
		}
		i++;
	}

	// place rooms
	map->keyAccessCount = 0;
	count = 0;
	i = 0;
	while (i < 1000 && count < mission->u.Classic.Rooms.Count)
	{
		int doorMin = CLAMP(mission->u.Classic.Doors.Min, 1, 6);
		int doorMax = CLAMP(mission->u.Classic.Doors.Max, doorMin, 6);
		if (MapTryBuildRoom(
			map, mission->Size,
			mission->u.Classic.Doors.Enabled,
			doorMin, doorMax, AreKeysAllowed(gCampaign.Entry.mode),
			mission->u.Classic.Rooms.Min,
			mission->u.Classic.Rooms.Max,
			mission->u.Classic.Rooms.Edge))
		{
			count++;
		}
		i++;
	}

	// place pillars
	count = 0;
	i = 0;
	while (i < 1000 && count < mission->u.Classic.Pillars.Count)
	{
		if (MapTryBuildPillar(
			map, mission->Size,
			mission->u.Classic.Pillars.Min,
			mission->u.Classic.Pillars.Max))
		{
			count++;
		}
		i++;
	}

	// place walls
	count = 0;
	i = 0;
	while (i < 1000 && count < mission->u.Classic.Walls)
	{
		if (MapTryBuildWall(
			map, mission->Size, mission->u.Classic.WallLength))
		{
			count++;
		}
		i++;
	}
}

static Vec2i GuessCoords(Vec2i mapSize);

static int MapTryBuildSquare(Map *map)
{
	Vec2i v = GuessCoords(gMission.missionData->Size);
	Vec2i size;
	size.x = rand() % 9 + 7;
	size.y = rand() % 9 + 7;

	if (MapIsAreaClear(
		map, Vec2iNew(v.x - 1, v.y - 1), Vec2iNew(size.x + 2, size.y + 2)))
	{
		MapMakeSquare(map, v, size);
		return 1;
	}
	return 0;
}
static unsigned short GenerateAccessMask(int *accessLevel);
static int MapTryBuildRoom(
	Map *map, Vec2i mapSize,
	int hasDoors, int doorMin, int doorMax, int hasKeys,
	int roomMinP, int roomMaxP, int edgeRooms)
{
	// make sure rooms are large enough to accomodate doors
	int roomMin = MAX(roomMinP, doorMin + 4);
	int roomMax = MAX(roomMaxP, doorMin + 4);
	int w = rand() % (roomMax - roomMin + 1) + roomMin;
	int h = rand() % (roomMax - roomMin + 1) + roomMin;
	Vec2i pos = GuessCoords(mapSize);
	Vec2i clearPos = Vec2iNew(pos.x - 1, pos.y - 1);
	Vec2i clearSize = Vec2iNew(w + 2, h + 2);
	int doors[4];

	// left, right, top, bottom
	doors[0] = doors[1] = doors[2] = doors[3] = 1;
	if (edgeRooms)
	{
		// Check if room is at edge; if so only check if clear inside edge
		if (pos.x == (XMAX - mapSize.x) / 2 ||
			pos.x == (XMAX - mapSize.x) / 2 + 1)
		{
			clearPos.x = (XMAX - mapSize.x) / 2 + 1;
			doors[0] = 0;
		}
		else if (pos.x + w == (XMAX + mapSize.x) / 2 - 2 ||
			pos.x + w == (XMAX + mapSize.x) / 2 - 1)
		{
			clearSize.x = (XMAX + mapSize.x) / 2 - 2 - pos.x;
			doors[1] = 0;
		}
		if (pos.y == (YMAX - mapSize.y) / 2 ||
			pos.y == (YMAX - mapSize.y) / 2 + 1)
		{
			clearPos.y = (YMAX - mapSize.y) / 2 + 1;
			doors[2] = 0;
		}
		else if (pos.y + h == (YMAX + mapSize.y) / 2 - 2 ||
			pos.y + h == (YMAX + mapSize.y) / 2 - 1)
		{
			clearSize.y = (YMAX + mapSize.y) / 2 - 2 - pos.y;
			doors[3] = 0;
		}
	}

	if (MapIsAreaClear(map, clearPos, clearSize))
	{
		int doormask = rand() % 15 + 1;
		int doorsUnplaced = 0;
		int i;
		unsigned short accessMask = 0;
		if (hasKeys && hasDoors)
		{
			accessMask = GenerateAccessMask(&map->keyAccessCount);
		}

		// Try to place doors according to the random mask
		// If we cannot place a door, remember this and try to place it
		// on the next door
		for (i = 0; i < 4; i++)
		{
			if ((doormask & (1 << i)) && !doors[i])
			{
				doorsUnplaced++;
			}
		}
		for (i = 0; i < 4; i++)
		{
			if (!(doormask & (1 << i)))
			{
				if (doorsUnplaced == 0)
				{
					doors[i] = 0;
				}
				else
				{
					doorsUnplaced--;
				}
			}
		}
		MapMakeRoom(
			map, pos.x, pos.y, w, h,
			hasDoors, doors, doorMin, doorMax, accessMask);
		if (hasKeys)
		{
			if (map->keyAccessCount < 1)
			{
				map->keyAccessCount = 1;
			}
		}
		return 1;
	}
	return 0;
}
static int MapTryBuildPillar(
	Map *map, Vec2i mapSize, int pillarMin, int pillarMax)
{
	Vec2i size = Vec2iNew(
		rand() % (pillarMax - pillarMin + 1) + pillarMin,
		rand() % (pillarMax - pillarMin + 1) + pillarMin);
	Vec2i pos = GuessCoords(mapSize);
	Vec2i clearPos = Vec2iNew(pos.x - 1, pos.y - 1);
	Vec2i clearSize = Vec2iNew(size.x + 2, size.y + 2);

	// Check if pillar is at edge; if so only check if clear inside edge
	if (pos.x == (XMAX - mapSize.x) / 2 ||
		pos.x == (XMAX - mapSize.x) / 2 + 1)
	{
		clearPos.x = (XMAX - mapSize.x) / 2 + 1;
	}
	else if (pos.x + size.x == (XMAX + mapSize.x) / 2 - 2 ||
		pos.x + size.x == (XMAX + mapSize.x) / 2 - 1)
	{
		clearSize.x = (XMAX + mapSize.x) / 2 - 2 - pos.x;
	}
	if (pos.y == (YMAX - mapSize.y) / 2 ||
		pos.y == (YMAX - mapSize.y) / 2 + 1)
	{
		clearPos.y = (YMAX - mapSize.y) / 2 + 1;
	}
	else if (pos.y + size.y == (YMAX + mapSize.y) / 2 - 2 ||
		pos.y + size.y == (YMAX + mapSize.y) / 2 - 1)
	{
		clearSize.y = (YMAX + mapSize.y) / 2 - 2 - pos.y;
	}

	if (MapIsAreaClear(map, clearPos, clearSize))
	{
		MapMakePillar(map, pos, size);
		return 1;
	}
	return 0;
}
static void MapGrowWall(Map *map, int x, int y, int d, int length);
static int MapTryBuildWall(Map *map, Vec2i mapSize, int wallLength)
{
	Vec2i v = GuessCoords(mapSize);
	if (MapIsValidStartForWall(map, v.x, v.y))
	{
		MapMakeWall(map, v);
		MapGrowWall(map, v.x, v.y, rand() & 3, wallLength);
		return 1;
	}
	return 0;
}
static void MapGrowWall(Map *map, int x, int y, int d, int length)
{
	int l;

	if (length <= 0)
		return;

	switch (d) {
	case 0:
		if (y < 3 ||
			IMapGet(map, Vec2iNew(x - 1, y - 1)) ||
			IMapGet(map, Vec2iNew(x + 1, y - 1)) ||
			IMapGet(map, Vec2iNew(x - 1, y - 2)) ||
			IMapGet(map, Vec2iNew(x, y - 2)) ||
			IMapGet(map, Vec2iNew(x + 1, y - 2)))
		{
			return;
		}
		y--;
		break;
	case 1:
		if (x > XMAX - 3 ||
			IMapGet(map, Vec2iNew(x + 1, y - 1)) ||
			IMapGet(map, Vec2iNew(x + 1, y + 1)) ||
			IMapGet(map, Vec2iNew(x + 2, y - 1)) ||
			IMapGet(map, Vec2iNew(x + 2, y)) ||
			IMapGet(map, Vec2iNew(x + 2, y + 1)))
		{
			return;
		}
		x++;
		break;
	case 2:
		if (y > YMAX - 3 ||
			IMapGet(map, Vec2iNew(x - 1, y + 1)) ||
			IMapGet(map, Vec2iNew(x + 1, y + 1)) ||
			IMapGet(map, Vec2iNew(x - 1, y + 2)) ||
			IMapGet(map, Vec2iNew(x, y + 2)) ||
			IMapGet(map, Vec2iNew(x + 1, y + 2)))
		{
			return;
		}
		y++;
		break;
	case 4:
		if (x < 3 ||
			IMapGet(map, Vec2iNew(x - 1, y - 1)) ||
			IMapGet(map, Vec2iNew(x - 1, y + 1)) ||
			IMapGet(map, Vec2iNew(x - 2, y - 1)) ||
			IMapGet(map, Vec2iNew(x - 2, y)) ||
			IMapGet(map, Vec2iNew(x - 2, y + 1)))
		{
			return;
		}
		x--;
		break;
	}
	MapMakeWall(map, Vec2iNew(x, y));
	length--;
	if (length > 0 && (rand() & 3) == 0)
	{
		l = rand() % length;
		MapGrowWall(map, x, y, rand() & 3, l);
		length -= l;
	}
	MapGrowWall(map, x, y, d, length);
}

static Vec2i GuessCoords(Vec2i mapSize)
{
	Vec2i v;
	if (mapSize.x)
	{
		v.x = (rand() % mapSize.x) + (XMAX - mapSize.x) / 2;
	}
	else
	{
		v.x = rand() % XMAX;
	}

	if (mapSize.y)
	{
		v.y = (rand() % mapSize.y) + (YMAX - mapSize.y) / 2;
	}
	else
	{
		v.y = rand() % YMAX;
	}
	return v;
}

static unsigned short GenerateAccessMask(int *accessLevel)
{
	unsigned short accessMask = 0;
	switch (rand() % 20)
	{
	case 0:
		if (*accessLevel >= 4)
		{
			accessMask = MAP_ACCESS_RED;
			*accessLevel = 5;
		}
		break;
	case 1:
	case 2:
		if (*accessLevel >= 3)
		{
			accessMask = MAP_ACCESS_BLUE;
			if (*accessLevel < 4)
			{
				*accessLevel = 4;
			}
		}
		break;
	case 3:
	case 4:
	case 5:
		if (*accessLevel >= 2)
		{
			accessMask = MAP_ACCESS_GREEN;
			if (*accessLevel < 3)
			{
				*accessLevel = 3;
			}
		}
		break;
	case 6:
	case 7:
	case 8:
	case 9:
		if (*accessLevel >= 1)
		{
			accessMask = MAP_ACCESS_YELLOW;
			if (*accessLevel < 2)
			{
				*accessLevel = 2;
			}
		}
		break;
	}
	return accessMask;
}
