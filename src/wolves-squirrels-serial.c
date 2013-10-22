#include <stdio.h>
#include <string.h>

#define MAX 100

enum move {
	TOP = 0,
	RIGHT = 1,
	BOTTOM = 2,
	LEFT = 3,
	NONE = 4
};

typedef struct world {
	int type;
	int breeding_period;
	int starvation_period;
} world[MAX][MAX];

typedef struct {
	int row;
	int column;
} position;

int NUM_ARGUMENTS = 5;
int BREEDING_LEVEL;
int STARVING_LEVEL;
int WORLD_SIZE;
struct world old_world;
struct world new_world;

/* returns 1 if animal is breeding, 0 otherwise */
int isBreeding(position pos) {
	return wold[pos.row][pos.column].breeding_period == BREEDING_LEVEL;
}

/* returns 1 if animal is starving, 0 otherwise */
int isStarving(position pos) {
	return wold[pos.row][pos.column].starvation_period == STARVING_LEVEL;
}

int numberOfPosition(position pos) {
	return pos.row * WORLD_SIZE + pow.column;
}

/* returns a valid move */
int getDestination(position pos) {
	position possible[4];
	int available[4];

	possible[move.TOP].row = pos.row - 1;
	possible[move.TOP].column = pow.column;
	possible[move.RIGHT].row = pos.row;
	possible[move.RIGHT].column = pow.column + 1;
	possible[move.BOTTOM].row = pos.row + 1;
	possible[move.BOTTOM].column = pow.column;
	possible[move.LEFT].row = pos.row;
	possible[move.LEFT].column = pow.column - 1;

	int i;
	int nAvailable = 0;
	for (i = 0; i < 4; i++) {
		if (possible[i].row >= 0 && possible[i].row < WORLD_SIZE && 
		    	possible[i].column >= 0 && possible[i].column < WORLD_SIZE)
		if (canMove(pos, possible[i])) {
			available[i] = 1;
			nAvailable++;
		}
	}

	int selected = numberOfPosition(pos) % nAvailable;
	for (i = 0; i < 4; i++) {
		if (available[i] == 1) {
			if (selected == 0) {
				return available[i];
			}

			selected--;
		}
	}
}

int updateCell(position pos);
void createWorld(world aWorld, FILE *input);
void playReds();
void playBlacks();

int main(int argc, char **argv) {
	if (argc < NUM_ARGUMENTS) {
		fprintf(stderr, "Not enough arguments...");
		exit(EXIT_FAILURE);
	}

	FILE *input = fopen(argv[1], "r");
	if (input == null) {
		fprintf(stderr, "File %s not found...", argv[1]);
		exit(EXIT_FAILURE);
	}
	createWorld(old_world, input);
	memset(new_world, old_world, sizeof(world));
	fclose(input);

	playReds();
	playBlacks();
	return 0;
}
