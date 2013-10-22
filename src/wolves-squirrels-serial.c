#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX 100

enum move {
	TOP = 0,
	RIGHT = 1,
	BOTTOM = 2,
	LEFT = 3,
	NONE = 4
};

enum type {
	WOLF = 1,
	SQUIRREL = 2,
	TREE = 3,
	ICE = 4,
	SQUIRREL_ON_TREE = 5,
	EMPTY = 6
};

typedef struct {
	enum type type;
	int breeding_period;
	int starvation_period;
} world[MAX][MAX];

typedef struct {
	int row;
	int column;
} position;

int NUM_ARGUMENTS = 6;
int WOLF_BREEDING_LEVEL;
int SQUIRREL_BREEDING_LEVEL;
int WOLF_STARVING_LEVEL;
int WORLD_SIZE;
world old_world;
world new_world;

/* returns 1 if animal is breeding, 0 otherwise */
int isBreeding(position pos) {
	if (old_world[pos.row][pos.column].type == WOLF) {
		return old_world[pos.row][pos.column].breeding_period == WOLF_BREEDING_LEVEL;
	}
	else if (old_world[pos.row][pos.column].type == SQUIRREL) {
		return old_world[pos.row][pos.column].breeding_period == SQUIRREL_BREEDING_LEVEL;
	}
	else {
		return 0;
	}
}

/* returns 1 if animal is starving, 0 otherwise */
int isStarving(position pos) {
	if (old_world[pos.row][pos.column].type == WOLF) {
		return old_world[pos.row][pos.column].starvation_period == WOLF_STARVING_LEVEL;
	}
	else {
		return 0;
	}
}

int numberOfPosition(position pos) {
	return pos.row * WORLD_SIZE + pos.column;
}

/* returns 1 if can move to theb given position*/
int canMoveTo(position currentPos, position possiblePos){
	
	int currentCell = old_world[currentPos.row][currentPos.column].type;
	int possibleCell = old_world[possiblePos.row][possiblePos.column].type;
	
	if(currentCell != ICE || currentCell != TREE){
		if(possibleCell != ICE)
			if(currentCell == WOLF ) 
				if(possibleCell != TREE || possibleCell != SQUIRREL_ON_TREE)
					return 1;
	}
	else return 0;
	
	return 0;
}

/* returns a valid move */
position getDestination(position pos) {
	position possible[4];
	int available[4];

	possible[TOP].row = pos.row - 1;
	possible[TOP].column = pos.column;
	possible[RIGHT].row = pos.row;
	possible[RIGHT].column = pos.column + 1;
	possible[BOTTOM].row = pos.row + 1;
	possible[BOTTOM].column = pos.column;
	possible[LEFT].row = pos.row;
	possible[LEFT].column = pos.column - 1;

	int i;
	int nAvailable = 0;
	for (i = 0; i < 4; i++) {
		if (possible[i].row >= 0 && possible[i].row < WORLD_SIZE && 
		    	possible[i].column >= 0 && possible[i].column < WORLD_SIZE)
		if (canMoveTo(pos, possible[i])) {
			available[i] = 1;
			nAvailable++;
		}
	}

	int selected = numberOfPosition(pos) % nAvailable;
	for (i = 0; i < 4; i++) {
		if (available[i] == 1) {
			if (selected == 0) {
				return possible[i];
			}

			selected--;
		}
	}
	return pos;
}

// why returns something??
int updateCell(position pos) {
	position destinationPosition = getDestination(pos); // posicao para onde se vai mover
	
	if((pos.row == destinationPosition.row) && (pos.column == destinationPosition.column))
		;
	// se posicao retornada igual a' actual bubai
	
	// se nao vai alterar world aux na posicao destination 
	// inc breeding period e testar se e' igual a BREEDING_LEVEL
	// se sim deixar baby pa trás se nao empty  e reset a breeding period do esquilo que vai pa destination

	return 0;
}

enum type convertType(char type) {
	switch (type) {
	case 'w': return WOLF;
	case 's': return SQUIRREL;
	case 'i': return ICE;
	case 't': return TREE;
	case '$': return SQUIRREL_ON_TREE;
	}

	fprintf(stderr, "Unknown type: %c\n", type);
	exit(EXIT_FAILURE);
}

void createWorld(world aWorld, FILE *input) {
	// initialize world with zeros
	memset(aWorld, 0, sizeof(world));

	// read world size
	fscanf(input, "%d", &WORLD_SIZE);

	int row;
	int column;
	char type;
	while (fscanf(input, "%d %d %c", &row, &column, &type)) {
		old_world[row-1][column-1].type = convertType(type);
	}
}

// blackTurn is 0 for red and 1 for blacks
void play(int blackTurn) {
	int i, j;
	for (i = 0; i < WORLD_SIZE; i++) {
		for (j = 0; j < WORLD_SIZE; j++) {
			if (!blackTurn && (i >> 1 == j >> 1)) {
				// red turn
				;
			}
			else if (blackTurn && (i >> 1 != j >> 1)) {
				// black turn
				;
			}
		}
	}
}

/*
	arg[1] = filename
	arg[2] = wolf_breeding_period
	arg[3] = squirtle_breeding_period
	arg[4] = wolf_starvation_period
	arg[5] = # generations
*/
int main(int argc, char **argv) {
	if (argc < NUM_ARGUMENTS) {
		fprintf(stderr, "Not enough arguments...");
		exit(EXIT_FAILURE);
	}

	// read file
	FILE *input = fopen(argv[1], "r");
	if (input == NULL) {
		fprintf(stderr, "File %s not found...", argv[1]);
		exit(EXIT_FAILURE);
	}
	createWorld(old_world, input);
	memcpy(new_world, old_world, sizeof(world));
	fclose(input);

	WOLF_BREEDING_LEVEL = atoi(argv[2]);
	SQUIRREL_BREEDING_LEVEL = atoi(argv[3]);
	WOLF_STARVING_LEVEL = atoi(argv[4]);
	int number_generations = atoi(argv[5]);

	int gen;
	for (gen = 0; gen < number_generations; gen++) {
		play(0);
		memcpy(new_world, old_world, sizeof(world));
		play(1);
		memcpy(new_world, old_world, sizeof(world));
	}

	return 0;
}
