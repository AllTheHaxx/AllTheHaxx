#pragma once

typedef unsigned long AStarNodeUID;


class IAStarWorldMap
{
public:
	virtual ~IAStarWorldMap() {}

	virtual const int GetWidth() const = 0;
	virtual const int GetHeight() const = 0;
	virtual const int GetSize() const = 0;

	virtual int GetField(int x, int y) const = 0;
};


class IAStarState
{
public:
	virtual ~IAStarState() {}
	virtual float GoalDistanceEstimate( const IAStarState &nodeGoal ) const = 0; // Heuristic function which computes the estimated cost to the goal node
	virtual bool IsGoal( IAStarState &nodeGoal ) const = 0; // Returns true if this node is the goal node
//	virtual bool GetSuccessors( CAStarSearch<UserState> *astarsearch, IAStarState *parent_node ) = 0; // Retrieves all successors to this node and adds them via astarsearch.addSuccessor()
	virtual float GetCost( const IAStarWorldMap *pMap, const IAStarState &successor ) const = 0; // Computes the cost of travelling from this node to the successor node
	virtual float GetOwnCost( const IAStarWorldMap *pMap ) const = 0; // Computes the cost of travelling from this node to the successor node
	virtual bool IsSameState( const IAStarState &rhs ) const = 0; // Returns true if this node is the same as the rhs node
	virtual AStarNodeUID GetUID() const = 0;
};

