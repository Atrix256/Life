// Game of life, using Life 1.06 format as input (https://conwaylife.com/wiki/Life_1.06)
// Using some STL for a simple implementation.
// Engines have their own data types and algorithms, or their own STL implementations that are more friendly for game dev, so would use those when working in an engine.
// Same for file I/O, using std::ifstream.

#include <stdio.h>
#include <vector>
#include <algorithm>
#include <fstream>
#include <string>

// How many simulation steps to do before showing output
static const int c_numSimulationSteps = 10;

// These parameters could be tuned for memory vs speed based on profiling usage cases.
static const int64_t c_cellPageSize = 256; // this many cells squared per page
static const int64_t c_bytesPerPage = c_cellPageSize * c_cellPageSize / 8;

// A page contains c_cellPageSize x c_cellPageSize cells.
// It starts at cell index (pageX * c_cellPageSize, pageY * c_cellPageSize)
struct CellPage
{
	// the page coordinates of this page
	int64_t pageX = 0;
	int64_t pageY = 0;

	// 1 bit per cell
	unsigned char cells[c_bytesPerPage];
};

// All pages which have alive cells. Sorted on x axis for faster operations, using dimensional reduction.
// If we found that there was a lot of overlap on the y axis, and not much on the x axis, we could sort by y instead.
// We could also do spatial hashing, but we would have to watch out for hash collisions - so may have a linked list per hash bucket to account for that.
// Another way to go could be to find clusters of alive cells and store them as rectangles, but I'm pretty sure that is an "NP hard" problem. A greedy algorithm could work well enough maybe, but would be complex.
// A vector sorted on x axis is simple, so should be good to start with to see if it fit the usage case, before going for more complexity in the name of better perf.
// Using pointers to cell pages so we can sort them and resize the vector without doing large copies.
std::vector<CellPage*> g_cellPages;

// Given a cell coordinate, returns the page that contains that cell, or nullptr if no page for that cell exists
CellPage* GetPageForCell(int64_t cellX, int64_t cellY)
{
	// calculate what page the cell would be on
	int64_t pageX = cellX / c_cellPageSize;
	int64_t pageY = cellY / c_cellPageSize;

	// binary search on pageX to see where to start looking for pageY
	auto it = std::lower_bound(g_cellPages.begin(), g_cellPages.end(), pageX,
		[](const CellPage* page, int64_t x)
		{
			return page->pageX < x;
		}
	);

	// While we have pages with the same pageX, look for our pageY
	while (it != g_cellPages.end() && (*it)->pageX == pageX)
	{
		if ((*it)->pageY != pageY)
			continue;

		// We found the page!
		return *it;
	}

	// We could not find the page
	return nullptr;
}

bool IsCellAlive(int64_t cellX, int64_t cellY)
{
	// Get the page that our cell is on
	CellPage* page = GetPageForCell(cellX, cellY);

	// if there isn't a page for it, that means it isn't alive
	if (!page)
		return false;

	// Return whether the cell is alive or not by returning the value of the bit for that cell in the page.
	int64_t cellIndex = (cellY % c_cellPageSize) * c_cellPageSize + (cellX % c_cellPageSize);
	int64_t byteIndex = cellIndex / 8;
	int64_t bitIndex = cellIndex % 8;
	return (page->cells[byteIndex] & (1 << bitIndex)) != 0;
}

void SetCellAlive(int64_t cellX, int64_t cellY, bool alive)
{
	// Get the page that our cell is on
	CellPage* page = GetPageForCell(cellX, cellY);

	// If we are trying to make a cell be alive
	if (alive)
	{
		// if there is no page, we need to make one
		if (!page)
		{
			// Make a new page, setting the pageX and pageY and setting all cells inside to dead
			CellPage* newPage = new CellPage();
			newPage->pageX = cellX / c_cellPageSize;
			newPage->pageY = cellY / c_cellPageSize;
			memset(newPage->cells, 0, c_bytesPerPage);

			// Add the new page to the list, in the proper location
			auto it = std::lower_bound(g_cellPages.begin(), g_cellPages.end(), newPage,
				[](const CellPage* a, const CellPage* b)
				{
					if (a->pageX != b->pageX)
						return a->pageX < b->pageX;
					return a->pageY < b->pageY;
				}
			);

			g_cellPages.insert(it, newPage);
		}
	}
	// Else we are trying to make a cell be dead
	else
	{
		// if we are trying to unalive a cell that doesn't have a page, it already is dead. Nothing to do
		if (!page)
			return;

		// Set the bit for that cell in the page to 0
		int64_t cellIndex = (cellY % c_cellPageSize) * c_cellPageSize + (cellX % c_cellPageSize);
		int64_t byteIndex = cellIndex / 8;
		int64_t bitIndex = cellIndex % 8;
		page->cells[byteIndex] &= ~(1 << bitIndex);

		// Note: we could scan cells to see if the page now has all dead cells, and if so, remove it from the list.
		// Or we could have a function called "PruneDeadPages" that we call every frame or every N frames.
		// Or perhaps, we check M pages every N frames to keep from having a performance spike every N frames.
		// For simplicity, we will let dead pages stick around and not worry about it for now.
	}
}

bool ParseFile(const char* filename)
{
	// try and open the file
	std::ifstream file(filename);
	if (!file.is_open())
		return false;

	// Read the file line by line
	bool ret = true;
	std::string line;
	bool gotHeader = false;
	while (std::getline(file, line))
	{
		if (!gotHeader)
		{
			if (line != "#Life 1.06")
			{
				printf("Invalid file format: Bad header\n");
				ret = false;
				break;
			}
			gotHeader = true;
		}
		// NOTE: unsure if empty lines are allowed per the spec, but we filter them out. Maybe should also trim trailing and following whitespace before this check, which would make white space only lines into empty lines.
		else if (!line.empty())
		{
			int64_t cellX, cellY;
			if (sscanf_s(line.c_str(), "%zi %zi", &cellX, &cellY) != 2)
			{
				printf("Invalid file format: Bad cell coordinates\n");
				ret = false;
				break;
			}

			SetCellAlive(cellX, cellY, true);
		}
	}

	file.close();
	return ret;
}

void Simulate()
{
	// TODO: this
}

void PrintBoard()
{
	// TODO: this
}

int main(int argc, char** arg)
{
	// Load the data
	if (!ParseFile("input.txt"))
	{
		printf("Could not load file\n");
		return 1;
	}

	// Simulate
	for (int i = 0; i < c_numSimulationSteps; ++i)
		Simulate();

	// report the results
	PrintBoard();

	return 0;
}
/*
TODO: need to read from stdin, not a file. is there a way to pipe a file to this as stdin?
*/
