#include "CellPages.h"

#include <algorithm>
#include <fstream>
#include <string>

// Some static non members, so they can be defined here in the CPP and be inlined
static inline void CellIndexToPageIndex(int64_t cellX, int64_t cellY, int64_t& pageX, int64_t& pageY)
{
	// Handle negative values
	pageX = (cellX >= 0) ? cellX / c_cellPageSize : (cellX - c_cellPageSize + 1) / c_cellPageSize;
	pageY = (cellY >= 0) ? cellY / c_cellPageSize : (cellY - c_cellPageSize + 1) / c_cellPageSize;
}

static inline void CellIndexToByteAndBitIndex(int64_t cellX, int64_t cellY, int64_t& byteIndex, int64_t& bitIndex)
{
	// For negative coordinates, C++ modulo returns negative, so we need to normalize to [0, c_cellPageSize)
	int64_t cellXInPage = ((cellX % c_cellPageSize) + c_cellPageSize) % c_cellPageSize;
	int64_t cellYInPage = ((cellY % c_cellPageSize) + c_cellPageSize) % c_cellPageSize;
	int64_t cellIndex = cellYInPage * c_cellPageSize + cellXInPage;
	byteIndex = cellIndex / 8;
	bitIndex = cellIndex % 8;
}

CellPages::~CellPages()
{
	// Free allocated memory
	for (CellPage* page : m_cellPages)
		delete page;
	m_cellPages.clear(); // Not strictly necessary, but leaving invalid pointers laying around tends to cause problems as code changes.
}

bool CellPages::Load(const char* filename)
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

// Given a cell coordinate, returns the page that contains that cell, or nullptr if no page for that cell exists
CellPages::CellPage* CellPages::GetPageForCell(int64_t cellX, int64_t cellY) const
{
	// calculate what page the cell would be on
	int64_t pageX, pageY;
	CellIndexToPageIndex(cellX, cellY, pageX, pageY);

	// binary search on pageX to see where to start looking for pageY
	auto it = std::lower_bound(m_cellPages.begin(), m_cellPages.end(), pageX,
		[](const CellPage* page, int64_t x)
		{
			return page->pageX < x;
		}
	);

	// While we have pages with the same pageX, look for our pageY
	while (it != m_cellPages.end() && (*it)->pageX == pageX)
	{
		// If we found it, we are done
		if ((*it)->pageY == pageY)
			return *it;

		++it;
	}

	// We could not find the page
	return nullptr;
}

bool CellPages::GetCellAlive(int64_t cellX, int64_t cellY) const
{
	// Get the page that our cell is on
	CellPage* page = GetPageForCell(cellX, cellY);

	// if there isn't a page for it, that means it isn't alive
	if (!page)
		return false;

	// Return whether the cell is alive or not by returning the value of the bit for that cell in the page.
	int64_t byteIndex = 0;
	int64_t bitIndex = 0;
	CellIndexToByteAndBitIndex(cellX, cellY, byteIndex, bitIndex);
	return (page->cells[byteIndex] & (1 << bitIndex)) != 0;
}

void CellPages::SetCellAlive(int64_t cellX, int64_t cellY, bool alive)
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
			CellIndexToPageIndex(cellX, cellY, newPage->pageX, newPage->pageY);
			memset(newPage->cells, 0, c_bytesPerPage);

			// Add the new page to the list, in the proper location
			auto it = std::lower_bound(m_cellPages.begin(), m_cellPages.end(), newPage,
				[](const CellPage* a, const CellPage* b)
				{
					if (a->pageX != b->pageX)
						return a->pageX < b->pageX;
					return a->pageY < b->pageY;
				}
			);

			m_cellPages.insert(it, newPage);

			page = newPage;
		}

		// Set the cell to alive
		int64_t byteIndex = 0;
		int64_t bitIndex = 0;
		CellIndexToByteAndBitIndex(cellX, cellY, byteIndex, bitIndex);
		page->cells[byteIndex] |= (1 << bitIndex);
	}
	// Else we are trying to make a cell be dead
	else
	{
		// if we are trying to unalive a cell that doesn't have a page, it already is dead. Nothing to do
		if (!page)
			return;

		// Set the bit for that cell in the page to 0
		int64_t byteIndex = 0;
		int64_t bitIndex = 0;
		CellIndexToByteAndBitIndex(cellX, cellY, byteIndex, bitIndex);
		page->cells[byteIndex] &= ~(1 << bitIndex);

		// Note: we could scan cells to see if the page now has all dead cells, and if so, remove it from the list.
		// Or we could have a function called "PruneDeadPages" that we call every frame or every N frames.
		// Or perhaps, we check M pages every N frames to keep from having a performance spike every N frames.
		// For simplicity, we will let dead pages stick around and not worry about it for now.
	}
}

void CellPages::Print() const
{
	printf("#Life 1.06\n");

	for (const CellPage* page : m_cellPages)
	{
		for (int64_t cellYInPage = 0; cellYInPage < c_cellPageSize; ++cellYInPage)
		{
			for (int64_t cellXInPage = 0; cellXInPage < c_cellPageSize; ++cellXInPage)
			{
				int64_t cellX = page->pageX * c_cellPageSize + cellXInPage;
				int64_t cellY = page->pageY * c_cellPageSize + cellYInPage;
				if (GetCellAlive(cellX, cellY))
					printf("%zi %zi\n", cellX, cellY);

				// Note: we could do this more performantly by looping through the cell bytes in the page, and only scanning the bits if it was non zero.
				// If we expected to have a lot of empty space, instead of using unsigned char for the cells, we could use uint64_t so that when we saw a zero
				// we skipped more bits at a time.
			}
		}
	}
}
