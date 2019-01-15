/*
A* Algorithm Implementation using STL is
Copyright (C)2001-2005 Justin Heyes-Jones

Permission is given by the author to freely redistribute and 
include this code in any program as long as this credit is 
given where due.
 
  COVERED CODE IS PROVIDED UNDER THIS LICENSE ON AN "AS IS" BASIS, 
  WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, 
  INCLUDING, WITHOUT LIMITATION, WARRANTIES THAT THE COVERED CODE 
  IS FREE OF DEFECTS, MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE
  OR NON-INFRINGING. THE ENTIRE RISK AS TO THE QUALITY AND 
  PERFORMANCE OF THE COVERED CODE IS WITH YOU. SHOULD ANY COVERED 
  CODE PROVE DEFECTIVE IN ANY RESPECT, YOU (NOT THE INITIAL 
  DEVELOPER OR ANY OTHER CONTRIBUTOR) ASSUME THE COST OF ANY 
  NECESSARY SERVICING, REPAIR OR CORRECTION. THIS DISCLAIMER OF 
  WARRANTY CONSTITUTES AN ESSENTIAL PART OF THIS LICENSE. NO USE 
  OF ANY COVERED CODE IS AUTHORIZED HEREUNDER EXCEPT UNDER
  THIS DISCLAIMER.
 
  Use at your own risk!

*/

#ifndef STLASTAR_H
#define STLASTAR_H
#include <base/system++/system++.h>

// stl includes
#include <algorithm>
#include <set>
#include <vector>
#include <map>
#include <cfloat>

//using namespace std;
using std::make_heap;
using std::push_heap;
using std::pop_heap;
using std::vector;
using std::map;

// fast fixed size memory allocator, used for fast node memory management
#include "fsa.h"
#include "interfaces.h"
#include "mapsearchnode.h"

// Fixed size memory allocator can be disabled to compare performance
// Uses std new and delete instead if you turn it off
#define USE_FSA_MEMORY 1

// disable warning that debugging information has lines that are truncated
// occurs in stl headers
#if defined(WIN32) && defined(_WINDOWS)
#pragma warning( disable : 4786 )
#endif



// The AStar search class. UserState is the users state space type
template <class UserState>
class CAStarSearch
{
public: // data

	enum
	{
		SEARCH_STATE_NOT_INITIALISED,
		SEARCH_STATE_SEARCHING,
		SEARCH_STATE_SUCCEEDED,
		SEARCH_STATE_FAILED,
		SEARCH_STATE_OUT_OF_MEMORY,
		SEARCH_STATE_INVALID
	};


	// A node represents a possible state in the search
	// The user provided state type is included inside this type

	public:

	class Node
	{
		public:

			Node *parent; // used during the search to record the parent of successor nodes
			Node *child; // used after the search for the application to view the search in reverse
			
			float g; // cost of this node + it's predecessors
			float h; // heuristic estimate of distance to goal
			float f; // sum of cumulative cost of predecessors and self and heuristic

			Node() :
				parent( 0 ),
				child( 0 ),
				g( 0.0f ),
				h( 0.0f ),
				f( 0.0f )
			{			
			}

			UserState m_UserState;
	};


	// For sorting the heap the STL needs compare function that lets us compare
	// the f value of two nodes

	class HeapCompare_f 
	{
		public:

			bool operator() ( const Node *x, const Node *y ) const
			{
				return x->f > y->f;
			}
	};


public: // methods


	// constructor just initialises private data
	CAStarSearch(int SolidTileCost = 9) :
		m_SolidTileCost(SolidTileCost),
		m_State( SEARCH_STATE_NOT_INITIALISED ),
		m_CurrentSolutionNode( NULL ),
#if USE_FSA_MEMORY
		m_FixedSizeAllocator( 1000 ),
#endif
		m_AllocateNodeCount(0),
		m_CancelRequest( false )
	{
	}

	CAStarSearch( unsigned int MaxNodes, int SolidTileCost = 9) :
		m_SolidTileCost(SolidTileCost),
		m_State( SEARCH_STATE_NOT_INITIALISED ),
		m_CurrentSolutionNode( NULL ),
#if USE_FSA_MEMORY
		m_FixedSizeAllocator( MaxNodes ),
#endif
		m_AllocateNodeCount(0),
		m_CancelRequest( false )
	{
	}

	// call at any time to cancel the search and free up all the memory
	void CancelSearch()
	{
		m_CancelRequest = true;
	}

	// Set Start and goal states
	void SetStartAndGoalStates( const IAStarWorldMap *pMap, const UserState &Start, const UserState &Goal )
	{
		// check if start and goal are valid
		if(Start.GetOwnCost(pMap) >= m_SolidTileCost || Goal.GetOwnCost(pMap) >= m_SolidTileCost)
		{
			m_State = SEARCH_STATE_FAILED;
			return;
		}

		m_CancelRequest = false;

		m_Start = AllocateNode();
		m_Goal = AllocateNode();

		m_pMap = pMap;

		dbg_assert((m_Start != NULL && m_Goal != NULL), "failed to allocate start or goal");
		
		m_Start->m_UserState = static_cast<UserState>(Start);
		m_Goal->m_UserState = static_cast<UserState>(Goal);

		m_State = SEARCH_STATE_SEARCHING;
		
		// Initialise the AStar specific parts of the Start Node
		// The user only needs fill out the state information

		m_Start->g = 0; 
		m_Start->h = m_Start->m_UserState.GoalDistanceEstimate( m_Goal->m_UserState );
		m_Start->f = m_Start->g + m_Start->h;
		m_Start->parent = 0;

		// Push the start node on the Open list
		m_FixedSizeAllocator.MakeWeakReference(m_Start, &m_Start);
		m_OpenList.push_back( m_Start ); // heap now unsorted
		m_OpenListMap[m_Start->m_UserState.GetUID()] = m_Start;

		// Sort back element into heap
		push_heap( m_OpenList.begin(), m_OpenList.end(), HeapCompare_f() );

		// Initialise counter for search steps
		m_Steps = 0;
	}

	// Advances search one step 
	unsigned int SearchStep()
	{
		// Firstly break if the user has not initialised the search
		if(dbg_assert_strict( (m_State > SEARCH_STATE_NOT_INITIALISED) && (m_State < SEARCH_STATE_INVALID), "SearchStep called before initializing" ))
		{
			m_State = SEARCH_STATE_FAILED;
		}

		// Next I want it to be safe to do a searchstep once the search has succeeded...
		if( (m_State == SEARCH_STATE_SUCCEEDED) ||
			(m_State == SEARCH_STATE_FAILED) 
		  )
		{
			return m_State; 
		}

		// Failure is defined as emptying the open list as there is nothing left to 
		// search...
		// New: Allow user abort
		if( m_OpenList.empty() || m_CancelRequest )
		{
			FreeAllNodes();
			m_State = SEARCH_STATE_FAILED;
			return m_State;
		}
		
		// Incremement step count
		m_Steps ++;

		// Pop the best node (the one with the lowest f) 
		Node *n = m_OpenList.front(); // get pointer to the node
		pop_heap( m_OpenList.begin(), m_OpenList.end(), HeapCompare_f() );
		m_OpenList.pop_back();
		m_OpenListMap.erase(n->m_UserState.GetUID());

		// Check for the goal; once we pop that, we're done
		if( n->m_UserState.IsGoal( m_Goal->m_UserState ) )
		{
			// The user is going to use the Goal Node he passed in 
			// so copy the parent pointer of n 
			m_Goal->parent = n->parent;
			m_Goal->g = n->g;

			// A special case is that the goal was passed in as the start state
			// so handle that here
			if( ! n->m_UserState.IsSameState( m_Start->m_UserState ) )
			{
				FreeNode( &n );

				// set the child pointers in each node (except Goal which has no child)
				Node *nodeChild = m_Goal;
				Node *nodeParent = m_Goal->parent;

				do 
				{
					nodeParent->child = nodeChild;

					nodeChild = nodeParent;
					nodeParent = nodeParent->parent;
				
				} 
				while( nodeChild != m_Start ); // Start is always the first node by definition

			}

			// delete nodes that aren't needed for the solution
			FreeUnusedNodes();

			m_State = SEARCH_STATE_SUCCEEDED;

			return m_State;
		}
		else // not goal
		{

			// We now need to generate the successors of this node
			// The user helps us to do this, and we keep the new nodes in
			// m_Successors ...

			m_Successors.clear(); // empty vector of successor nodes to n

			// User provides this functions and uses AddSuccessor to add each successor of
			// node 'n' to m_Successors
			bool ret = n->m_UserState.GetSuccessors( this, n->parent ? &n->parent->m_UserState : NULL ); 

			// if it failed, we must have gone oom
			if( !ret )
			{
				dbg_msg("astar/warning", "out of memory, aborting search!");
				#if USE_FSA_MEMORY
				dbg_msg("astar/memory", "[FSA] Used Elements: %u, Max Elements: %u, Free Elements: %u", m_FixedSizeAllocator.GetElementCount(), m_FixedSizeAllocator.GetMaxElements(), m_FixedSizeAllocator.GetMaxElements()-m_FixedSizeAllocator.GetElementCount());
				#endif
			    typename vector< Node * >::iterator successor;

				// free the nodes that may previously have been added 
				for( successor = m_Successors.begin(); successor != m_Successors.end(); successor ++ )
				{
					FreeNode( &(*successor) );
				}

				m_Successors.clear(); // empty vector of successor nodes to n

				// free up everything else we allocated
				FreeAllNodes();

				m_State = SEARCH_STATE_OUT_OF_MEMORY;
				return m_State;
			}
			
			// Now handle each successor to the current node ...
			for( typename vector< Node * >::iterator successor = m_Successors.begin(); successor != m_Successors.end(); successor ++ )
			{

				// 	The g value for this successor ...
				float newg = n->g + n->m_UserState.GetCost(m_pMap, (*successor)->m_UserState);

				// Now we need to find whether the node is on the open or closed lists
				// If it is but the node that is already on them is better (lower g)
				// then we can forget about this successor

				// search this node on the open list
				Node *openlist_node = NULL;
				try
				{
					openlist_node = m_OpenListMap.at((*successor)->m_UserState.GetUID());

					// we found this state on open
					if( openlist_node->g <= newg )
					{
						// the one on Open is cheaper than this one
						FreeNode( &(*successor) );
						continue;
					}
				} catch(std::out_of_range&) {};

				// search this node on the closed list
				Node *closedlist_node = NULL;
				try
				{
					closedlist_node = m_ClosedListMap.at((*successor)->m_UserState.GetUID());

					// we found this state on closed
					if( closedlist_node->g <= newg )
					{
						// the one on Closed is cheaper than this one
						FreeNode( &(*successor) );
						continue;
					}
				} catch(std::out_of_range&) {};

				// This node is the best node so far with this particular state
				// so lets keep it and set up its AStar specific data ...

				(*successor)->parent = n;
				(*successor)->g = newg;
				(*successor)->h = (*successor)->m_UserState.GoalDistanceEstimate( m_Goal->m_UserState );
				(*successor)->f = (*successor)->g + (*successor)->h;

				// Remove successor from closed if it was on it
				if( closedlist_node )
				{
					// remove it from Closed
					m_ClosedListMap.erase(closedlist_node->m_UserState.GetUID());
					FreeNode( &closedlist_node );
				}

				// Update old version of this node
				if( openlist_node )
				{
					typename vector< Node * >::iterator openlist_result;
					openlist_result = std::find(m_OpenList.begin(), m_OpenList.end(), openlist_node);
					if(!dbg_assert_strict(openlist_result != m_OpenList.end(), "m_OpenListMap contains element that's not in m_OpenList"))
					{
						m_OpenListMap.erase((*openlist_result)->m_UserState.GetUID());
						FreeNode(&(*openlist_result));
						m_OpenList.erase(openlist_result);
					}
					// re-make the heap 
					// make_heap rather than sort_heap is an essential bug fix
					// thanks to Mike Ryynanen for pointing this out and then explaining
					// it in detail. sort_heap called on an invalid heap does not work
					make_heap( m_OpenList.begin(), m_OpenList.end(), HeapCompare_f() );
				}

				// heap now unsorted
				m_OpenList.push_back( (*successor) );
				m_OpenListMap[(*successor)->m_UserState.GetUID()] = *successor;

				// sort back element into heap
				push_heap( m_OpenList.begin(), m_OpenList.end(), HeapCompare_f() );

			}

			// push n onto Closed, as we have expanded it now

			m_ClosedListMap[n->m_UserState.GetUID()] = n;

		} // end else (not goal so expand)

 		return m_State; // Succeeded bool is false at this point. 

	}

	// User calls this to add a successor to a list of successors
	// when expanding the search frontier
	bool AddSuccessor( const UserState &State )
	{
		Node *node = AllocateNode();

		if( node )
		{
			node->m_UserState = State;

			m_Successors.push_back( node );

			return true;
		}

		return false;
	}

	// Free the solution nodes
	// This is done to clean up all used Node memory when you are done with the
	// search
	void FreeSolutionNodes()
	{
		Node *n = m_Start;

		if( m_Start->child )
		{
			do
			{
				Node *del = n;
				n = n->child;
				FreeNode( &del );
			} while( n != m_Goal );

			FreeNode( &n ); // Delete the goal

		}
		else
		{
			// if the start node is the solution we need to just delete the start and goal
			// nodes
			FreeNode( &m_Start );
			FreeNode( &m_Goal );
		}

	}

	const int GetSolidTileCost() const { return m_SolidTileCost; }

	const IAStarWorldMap *GetMap() const { return m_pMap; }


	// Functions for traversing the solution

	// Get start node
	IAStarState *GetSolutionStart()
	{
		m_CurrentSolutionNode = m_Start;
		if( m_Start )
		{
			return &m_Start->m_UserState;
		}
		else
		{
			return NULL;
		}
	}
	
	// Get next node
	IAStarState *GetSolutionNext()
	{
		if( m_CurrentSolutionNode )
		{
			if( m_CurrentSolutionNode->child )
			{

				Node *child = m_CurrentSolutionNode->child;

				m_CurrentSolutionNode = m_CurrentSolutionNode->child;

				return &child->m_UserState;
			}
		}

		return NULL;
	}
	
	// Get end node
	IAStarState *GetSolutionEnd()
	{
		m_CurrentSolutionNode = m_Goal;
		if( m_Goal )
		{
			return &m_Goal->m_UserState;
		}
		else
		{
			return NULL;
		}
	}
	
	// Step solution iterator backwards
	IAStarState *GetSolutionPrev()
	{
		if( m_CurrentSolutionNode )
		{
			if( m_CurrentSolutionNode->parent )
			{

				Node *parent = m_CurrentSolutionNode->parent;

				m_CurrentSolutionNode = m_CurrentSolutionNode->parent;

				return &parent->m_UserState;
			}
		}

		return NULL;
	}

	// Get final cost of solution
	// Returns FLT_MAX if goal is not defined or there is no solution
	float GetSolutionCost()
	{
		if( m_Goal && m_State == SEARCH_STATE_SUCCEEDED )
		{
			return m_Goal->g;
		}
		else
		{
			return FLT_MAX;
		}
	}

	// Get the number of steps
	int GetStepCount() { return m_Steps; }

	void EnsureMemoryFreed()
	{
#if USE_FSA_MEMORY
		if(m_AllocateNodeCount != 0)
		{
			dbg_msg("astar/assertinfo", "-----------------------------------------");
			dbg_msg("astar/assertinfo", "StartNode: %s, EndNode: %s, AllocationCount: %i", m_Start ? "alloc" : "free", m_Goal ? "alloc" : "free", m_AllocateNodeCount);
			dbg_msg("astar/assertinfo", "[FSA] Used Elements: %u, Max Elements: %u, Free Elements: %u", m_FixedSizeAllocator.GetElementCount(), m_FixedSizeAllocator.GetMaxElements(), m_FixedSizeAllocator.GetMaxElements()-m_FixedSizeAllocator.GetElementCount());
		}
		dbg_assert_strict(m_AllocateNodeCount == 0, "CAStarSearch::EnsureMemoryFreed() - memory was not freed entirely");
#endif
	}

private: // methods

	// This is called when a search fails or is cancelled to free all used
	// memory 
	void FreeAllNodes()
	{
		// iterate open list and delete all nodes
		for(typename vector< Node * >::iterator it = m_OpenList.begin(); it != m_OpenList.end(); ++it)
		{
			Node *n = (*it);
			FreeNode( &n );
		}

		m_OpenList.clear();
		m_OpenListMap.clear();

		// iterate closed list and delete unused nodes
		for(typename map<AStarNodeUID, Node *>::iterator it = m_ClosedListMap.begin(); it != m_ClosedListMap.end(); ++it)
		{
			Node *n = it->second;
			FreeNode( &n );
		}

		m_ClosedListMap.clear();

		// delete the start and goal nodes
		FreeNode(&m_Start);
		FreeNode(&m_Goal);
	}


	// This call is made by the search class when the search ends. A lot of nodes may be
	// created that are still present when the search ends. They will be deleted by this 
	// routine once the search ends
	void FreeUnusedNodes()
	{
		// iterate open list and delete unused nodes
		{
			for(typename vector< Node * >::iterator it = m_OpenList.begin(); it != m_OpenList.end();)
			{
				Node *n = (*it);

				if( !n->child )
				{
					// delete all occurences in the map
					for(typename std::map<AStarNodeUID, Node *>::iterator mapIt = m_OpenListMap.begin(); mapIt != m_OpenListMap.end(); /* noop */)
					{
						if(mapIt->second == n)
							mapIt = m_OpenListMap.erase(mapIt);
						else
							++mapIt;
					}

					FreeNode( &n );

					// remove it from our list
					it = m_OpenList.erase(it);
				}
				else
					++it;
			}

		}

		// iterate closed list and delete unused nodes
		typename map<AStarNodeUID, Node *>::iterator iterClosed;
		for( iterClosed = m_ClosedListMap.begin(); iterClosed != m_ClosedListMap.end(); /* noop */ )
		{
			Node *n = iterClosed->second;

			if( !n->child )
			{
				FreeNode( &n );
				iterClosed = m_ClosedListMap.erase(iterClosed);
			}
			else
				++iterClosed;
		}
	}

	// Node memory management
	Node *AllocateNode()
	{

#if !USE_FSA_MEMORY
		Node *p = new Node;
		return p;
#else
		Node *address = m_FixedSizeAllocator.alloc();

		if( !address )
		{
			return NULL;
		}
		m_AllocateNodeCount ++;
		Node *p = new (address) Node;
		return p;
#endif
	}

	void FreeNode( Node **node_ptr )
	{
		//if(dbg_assert_strict(node_ptr != NULL && *node_ptr != NULL, "FreeNode got nullptr"))
		if(node_ptr == NULL || *node_ptr == NULL)
			return;

		dbg_assert(m_AllocateNodeCount > 0, "going to free something we didn't allocate...?!");
		Node *node = *node_ptr;
		*node_ptr = NULL;
		m_AllocateNodeCount --;

#if !USE_FSA_MEMORY
		delete node;
#else
		node->~Node();
		m_FixedSizeAllocator.free( node );
#endif
	}

private: // data

	// Heap (simple vector but used as a heap, cf. Steve Rabin's game gems article)
	vector< Node *> m_OpenList;

	// provides fast access to specific nodes (these maps are keeping iterators into m_OpenList bzw. m_ClosedList)
	map<AStarNodeUID, Node *> m_OpenListMap;
	map<AStarNodeUID, Node *> m_ClosedListMap;

	// Successors is a vector filled out by the user each type successors to a node
	// are generated
	vector< Node * > m_Successors;

	const IAStarWorldMap *m_pMap;

	const int m_SolidTileCost;

	// State
	unsigned int m_State;

	// Counts steps
	int m_Steps;

	// Start and goal state pointers
	Node *m_Start;
	Node *m_Goal;

	Node *m_CurrentSolutionNode;

#if USE_FSA_MEMORY
	// Memory
 	FixedSizeAllocator<Node> m_FixedSizeAllocator;
#endif

	// debugging : count memory allocation and free's
	int m_AllocateNodeCount;
	
	bool m_CancelRequest;

};

#endif

   
