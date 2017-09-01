#pragma once

#include "stlastar.h" // See header for copyright and usage information
#include <math.h>



// Definitions

class AStarMapSearchNode : public AStarState<AStarMapSearchNode>
{
public:
	int x;	 // the (x,y) positions of the node
	int y;

	explicit AStarMapSearchNode() { x = y = 0; }
	AStarMapSearchNode(int px, int py) { x=px; y=py; }

	float GoalDistanceEstimate( AStarMapSearchNode &nodeGoal );
	bool IsGoal( AStarMapSearchNode &nodeGoal );
	bool GetSuccessors( AStarSearch<AStarMapSearchNode> *astarsearch, AStarMapSearchNode *parent_node );
//	float GetCost( AStarMapSearchNode &successor );
	bool IsSameState( AStarMapSearchNode &rhs );
};

bool AStarMapSearchNode::IsSameState( AStarMapSearchNode &rhs )
{

	// same state in a maze search is simply when (x,y) are the same
	if( (x == rhs.x) &&
		(y == rhs.y) )
	{
		return true;
	}
	else
	{
		return false;
	}

}

// Here's the heuristic function that estimates the distance from a Node
// to the Goal. 

float AStarMapSearchNode::GoalDistanceEstimate( AStarMapSearchNode &nodeGoal )
{
	return fabsf(x - nodeGoal.x) + fabsf(y - nodeGoal.y);
}

bool AStarMapSearchNode::IsGoal( AStarMapSearchNode &nodeGoal )
{

	if( (x == nodeGoal.x) &&
		(y == nodeGoal.y) )
	{
		return true;
	}

	return false;
}

// This generates the successors to the given Node. It uses a helper function called
// AddSuccessor to give the successors to the AStar class. The A* specific initialisation
// is done for each node internally, so here you just set the state information that
// is specific to the application
bool AStarMapSearchNode::GetSuccessors( AStarSearch<AStarMapSearchNode> *astarsearch, AStarMapSearchNode *parent_node )
{

	int parent_x = -1;
	int parent_y = -1;

	if( parent_node )
	{
		parent_x = parent_node->x;
		parent_y = parent_node->y;
	}



	// push each possible move except allowing the search to go backwards

	if( (astarsearch->GetMap( x-1, y ) < 9)
		&& !((parent_x == x-1) && (parent_y == y))
			)
	{
		AStarMapSearchNode NewNode = AStarMapSearchNode( x-1, y );
		astarsearch->AddSuccessor( NewNode );
	}

	if( (astarsearch->GetMap( x, y-1 ) < 9)
		&& !((parent_x == x) && (parent_y == y-1))
			)
	{
		AStarMapSearchNode NewNode = AStarMapSearchNode( x, y-1 );
		astarsearch->AddSuccessor( NewNode );
	}

	if( (astarsearch->GetMap( x+1, y ) < 9)
		&& !((parent_x == x+1) && (parent_y == y))
			)
	{
		AStarMapSearchNode NewNode = AStarMapSearchNode( x+1, y );
		astarsearch->AddSuccessor( NewNode );
	}


	if( (astarsearch->GetMap( x, y+1 ) < 9)
		&& !((parent_x == x) && (parent_y == y+1))
			)
	{
		AStarMapSearchNode NewNode = AStarMapSearchNode( x, y+1 );
		astarsearch->AddSuccessor( NewNode );
	}

	return true;
}

// given this node, what does it cost to move to successor. In the case
// of our map the answer is the map terrain value at this node since that is 
// conceptually where we're moving
/*
float AStarMapSearchNode::GetCost( AStarMapSearchNode &successor )
{
	return (float) GetMap( x, y );

}

*/
