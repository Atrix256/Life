// Game of life, using Life 1.06 format as input (https://conwaylife.com/wiki/Life_1.06)
// If you compare debug vs release you may wonder why debug is so slow. It's because I'm using STL and there are a lot of bounds checks and similar slowing it down.
// In a real game engine, there'd be containers that were more suitable for debug builds, or a way to turn off the extra checks, or similar.

#include "CellPages.h"
#include <stdio.h>

// How many simulation steps to do before showing output
static const int c_numSimulationSteps = 10;

int main(int argc, char** arg)
{
	CellPages board;

	// Load the data
	if (!board.Load("input.life"))
	{
		printf("Could not load file\n");
		return 1;
	}

	// Simulate
	for (int i = 0; i < c_numSimulationSteps; ++i)
		board.Simulate();

	// report the results
	board.Print();

	return 0;
}
/*
TODO:
* need to read from stdin, not a file. is there a way to pipe a file to this as stdin?
* scan this code before you call it g2g
*/
