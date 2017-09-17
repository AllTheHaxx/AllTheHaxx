#pragma once

#include "interfaces.h"
#include "stlastar.h" // See header for copyright and usage information



// Definitions

class CAStarMapSearchNode : public IAStarState
{
public:
	int m_X;	 // the (x,y) positions of the node
	int m_Y;

	CAStarMapSearchNode() { m_X = m_Y = 0; }
	CAStarMapSearchNode(int px, int py) { m_X=px; m_Y=py; }

	float GoalDistanceEstimate( const IAStarState &nodeGoal ) const;
	bool IsGoal( IAStarState &nodeGoal ) const;
	bool GetSuccessors( CAStarSearch<CAStarMapSearchNode> *astarsearch, IAStarState *parent );
	float GetCost( const IAStarWorldMap *pMap, const IAStarState &successor ) const;
	float GetOwnCost( const IAStarWorldMap *pMap ) const;
	bool IsSameState( const IAStarState &rhs ) const;
	AStarNodeUID GetUID() const { return (unsigned)((AStarNodeUID)m_X << 0xF) | (unsigned)m_Y; };
};

bool CAStarMapSearchNode::IsSameState( const IAStarState &rhs ) const
{
	const CAStarMapSearchNode &node = dynamic_cast<const CAStarMapSearchNode &>(rhs);

	// same state in a maze search is simply when (x,y) are the same
	return (m_X == node.m_X) &&
		   (m_Y == node.m_Y);

}

// Here's the heuristic function that estimates the distance from a Node
// to the Goal. 

float CAStarMapSearchNode::GoalDistanceEstimate( const IAStarState &nodeGoal ) const
{
	return 0.0f;//(fabsf(x - nodeGoal.x) + fabsf(y - nodeGoal.y));
}

bool CAStarMapSearchNode::IsGoal( IAStarState &nodeGoal ) const
{
	return IsSameState(nodeGoal);
}

// This generates the successors to the given Node. It uses a helper function called
// AddSuccessor to give the successors to the AStar class. The A* specific initialisation
// is done for each node internally, so here you just set the state information that
// is specific to the application
bool CAStarMapSearchNode::GetSuccessors( CAStarSearch<CAStarMapSearchNode> *astarsearch, IAStarState *parent )
{
	CAStarMapSearchNode *parent_node = dynamic_cast<CAStarMapSearchNode *>(parent);

	int parent_x = -1;
	int parent_y = -1;

	if( parent_node )
	{
		parent_x = parent_node->m_X;
		parent_y = parent_node->m_Y;
	}



	// push each possible move except allowing the search to go backwards

	#define TEST_NODE(xval, yval) \
	if( (astarsearch->GetMap()->GetField( xval, yval ) < astarsearch->GetSolidTileCost()) \
		&& !((parent_x == (xval)) && (parent_y == (yval))) \
			) \
	{ \
		CAStarMapSearchNode NewNode = CAStarMapSearchNode( xval, yval ); \
		if(!astarsearch->AddSuccessor( NewNode )) \
			return false; \
	}

	// NORTH
	TEST_NODE(m_X, m_Y-1)

	// NORTH-EAST
	TEST_NODE(m_X+1, m_Y-1)

	// EAST
	TEST_NODE(m_X+1, m_Y)

	// SOUTH-EAST
	TEST_NODE(m_X+1, m_Y+1)

	// SOUTH
	TEST_NODE(m_X, m_Y+1)

	// SOUTH-WEST
	TEST_NODE(m_X-1, m_Y+1)

	// WEST
	TEST_NODE(m_X-1, m_Y)

	// NORTH-WEST
	TEST_NODE(m_X-1, m_Y-1)

	#undef TEST_NODE

	return true;
}

// given this node, what does it cost to move to successor. In the case
// of our map the answer is the map terrain value at this node since that is 
// conceptually where we're moving

float CAStarMapSearchNode::GetCost( const IAStarWorldMap *pMap, const IAStarState &successor ) const
{
	const CAStarMapSearchNode &next = dynamic_cast<const CAStarMapSearchNode &>(successor);

	float ExtraCost = 0.0f;

	// when going diagonal, we have to take a couple more things into account
	if(m_X != next.m_X && m_Y != next.m_Y)
	{
		// make sure we are not being blocked
		int dx = next.m_X - m_X;
		int dy = next.m_Y - m_Y;

		float Average = ((float)pMap->GetField(m_X+dx, m_Y) + (float)pMap->GetField(m_X, m_Y+dy)) / 2.0f;
		ExtraCost = Average / 2.0f;
	}

	return pMap->GetField(next.m_X, next.m_Y) + ExtraCost;
}

float CAStarMapSearchNode::GetOwnCost( const IAStarWorldMap *pMap ) const
{
	return pMap->GetField(m_X, m_Y);
}


