#pragma once

// Using some STL for a simpler implementation.
// Engines have their own data types and algorithms, or their own STL implementations that are more friendly for game dev, so would use those when working in an engine.
// For example, this life program is very slow in debug, but very fast in release due to bounds checking and similar. In a game, that would make debug builds unusable, which is bad news.
#include <vector>

// These parameters could be tuned for memory vs speed based on profiling usage cases.
static const int64_t c_cellPageSize = 256; // this many cells squared per page
static const int64_t c_bytesPerPage = c_cellPageSize * c_cellPageSize / 8;

class CellPages
{
public:
	bool Load(const char* filename);

	void SetCellAlive(int64_t cellX, int64_t cellY, bool alive);
	bool GetCellAlive(int64_t cellX, int64_t cellY) const;

	void Print() const;

	void Simulate();

private:
	// A page contains c_cellPageSize x c_cellPageSize cells.
	// It starts at cell index (pageX * c_cellPageSize, pageY * c_cellPageSize)
	struct CellPage
	{
		// the page coordinates of this page
		int64_t pageX = 0;
		int64_t pageY = 0;

		// 1 bit per cell.
		unsigned char cells[c_bytesPerPage];
	};

	const CellPage* GetPageForCell(int64_t cellX, int64_t cellY) const;
	CellPage* GetPageForCell(int64_t cellX, int64_t cellY);

	int GetNeighborCount(int64_t cellX, int64_t cellY) const;

	// All pages which have ever had alive cells. Sorted on x axis for faster operations, using dimensional reduction.
	// If we found that there was a lot of overlap on the y axis, and not much on the x axis, we could sort by y instead.
	// We could also do spatial hashing, but we would have to watch out for hash collisions - so may have a linked list per hash bucket to account for that.
	// Another way to go could be to find clusters of alive cells and store them as rectangles, but I'm pretty sure that is an "NP hard" problem. A greedy algorithm could work well enough maybe, but would be complex.
	// A vector sorted on x axis is simple, so should be good to start with to see if it fit the usage case, before going for more complexity in the name of better perf.
	// If we found that there was a lot of churn in sorting and adding pages, we could make this store CellPage* instead to make those lighter weight operations.
	std::vector<CellPage> m_cellPages;
};