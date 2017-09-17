// (c) 2017 The AllTheHaxx Team
// Please note that this file is not part of the astar-algorithm-cpp project,
// but of AllTheHaxx itself. Thus, copyright goes to The AllTheHaxx Team.

#pragma once

#include <base/system.h>
#include "interfaces.h"


// The world map
class CAStarWorldMap : public IAStarWorldMap
{
	const int m_MapWidth;
	const int m_MapHeight;

	int *m_pWorldMap;
	int m_CurrentIndex;

public:
	CAStarWorldMap(const int Width, const int Height)
			: m_MapWidth(Width), m_MapHeight(Height)
	{
		m_pWorldMap = mem_allocb(int, Width*Height);
		m_CurrentIndex = 0;
	}

	virtual ~CAStarWorldMap()
	{
		mem_free(m_pWorldMap);
	}

	int AddNext(int Cost /* in range of 0-9 */)
	{
		if(m_CurrentIndex < m_MapWidth*m_MapHeight)
		{
			m_pWorldMap[m_CurrentIndex++] = Cost;
			return m_CurrentIndex; // number of elements
		}
		return -1; // error
	}

	int GetPrevious() const
	{
		if(m_CurrentIndex == 0)
			return -1; // error
		return m_pWorldMap[m_CurrentIndex-1 - 1]; // m_CurrentIndex is actually the NEXT thing that is yet to be written, thus -2
	}

	const int GetWidth() const { return m_MapWidth; }
	const int GetHeight() const { return m_MapHeight; }
	const int GetSize() const { return m_MapWidth*m_MapHeight; }


// map helper function

	int GetField( int x, int y ) const
	{
		dbg_assert_strict(m_CurrentIndex == m_MapWidth*m_MapHeight, "map not entirely initialized!");

		if( x < 0 ||
			x >= m_MapWidth ||
			y < 0 ||
			y >= m_MapHeight
				)
		{
			return 9;
		}

		return m_pWorldMap[(y*m_MapWidth)+x];
	}

};
