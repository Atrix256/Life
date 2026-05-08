// Game of life, using Life 1.06 format as input (https://conwaylife.com/wiki/Life_1.06)

#include "CellPages.h"
#include <stdio.h>

// How many simulation steps to do before showing output
static const int c_numSimulationSteps = 10;

void Simulate()
{
	// TODO: this
}

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
	{
		Simulate();
	}

	// report the results
	board.Print();

	return 0;
}
/*
TODO:
* need to read from stdin, not a file. is there a way to pipe a file to this as stdin?
* scan this code before you call it g2g
*/
