//
// Please note that this file is not part of the astar-algorithm-cpp project,
// but of AllTheHaxx itself. Thus, copyright goes to The AllTheHaxx Team.
//

#pragma once

#include <base/system.h>


// The world map
class AStarWorldMap
{
	const int m_MapWidth;
	const int m_MapHeight;

	int *m_pWorldMap;
	int m_CurrentIndex;

public:
	AStarWorldMap(const int Width, const int Height)
			: m_MapWidth(Width), m_MapHeight(Height)
	{
		m_pWorldMap = mem_allocb(int, Width*Height);
		m_CurrentIndex = 0;
	}

	~AStarWorldMap()
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

	const int GetWidth() const { return m_MapWidth; }
	const int GetHeight() const { return m_MapHeight; }


// map helper function

	int GetMap( int x, int y ) const
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
