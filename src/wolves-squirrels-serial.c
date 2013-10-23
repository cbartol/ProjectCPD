#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX 100
#define max(a,b) ((a) > (b) ? (a) : (b))
#define min(a,b) ((a) < (b) ? (a) : (b))

enum move {
	TOP = 0,
	RIGHT = 1,
	BOTTOM = 2,
	LEFT = 3,
	NONE = 4
};

enum type {
	EMPTY = 0,
	WOLF = 1,
	SQUIRREL = 2,
	TREE = 3,
	ICE = 4,
	SQUIRREL_ON_TREE = 5
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
	if (new_world[pos.row][pos.column].type == WOLF) {
		return new_world[pos.row][pos.column].breeding_period == WOLF_BREEDING_LEVEL;
	}
	else if (new_world[pos.row][pos.column].type == SQUIRREL) {
		return new_world[pos.row][pos.column].breeding_period == SQUIRREL_BREEDING_LEVEL;
	}
	return 0;
}

/* returns 1 if wolf is starving, 0 otherwise */
int isStarving(position pos) {
	return new_world[pos.row][pos.column].type == WOLF ? 
		new_world[pos.row][pos.column].starvation_period == WOLF_STARVING_LEVEL :
		0;
}

/* transforms a position into the corresponding number in the matrix */
int numberOfPosition(position pos) {
	return pos.row * WORLD_SIZE + pos.column;
}

/* returns 1 if type can move to the given position, 0 otherwise */
int canMoveTo(position from, position to){
	int fromCell = old_world[from.row][from.column].type;
	int toCell = old_world[to.row][to.column].type;
	
	// can't move a tree or ice cell nor it can move to a ice cell
	if ((fromCell == SQUIRREL || fromCell == WOLF) && (toCell != ICE)) {
		// wolf's can't go to trees
		if ((fromCell == WOLF) && (toCell == TREE || toCell == SQUIRREL_ON_TREE)) { 
			return 0;
		}
		return 1;
	}
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

/* leaves a child in the previous position */
void breed(position prev, position cur) {
    enum type type = new_world[cur.row][cur.column].type;
    if (type == SQUIRREL_ON_TREE || type == SQUIRREL) {
    	new_world[prev.row][prev.column].type = SQUIRREL;
    }
    else {
   	    new_world[prev.row][prev.column].type = WOLF;
    }
    new_world[prev.row][prev.column].starvation_period = 0;
    new_world[prev.row][prev.column].breeding_period = 0;
}

/* the cell becomes empty */
void clean(position pos) {
    enum type type = new_world[pos.row][pos.column].type;
    if (type == WOLF || type == SQUIRREL) {
    	new_world[pos.row][pos.column].type = EMPTY;
    }
    else if (type == SQUIRREL_ON_TREE) {
    	new_world[pos.row][pos.column].type = TREE;
    }

    new_world[pos.row][pos.column].starvation_period = 0;
    new_world[pos.row][pos.column].breeding_period = 0;
}

/* moves animal from the current position to its destination */
/* 	Duvida: aumenta-se o breeding period antes de mudar a posição ou depois?
	O problema verifica-se se o lobo como o esquilo antes ou depois de aumentar?
	Outro problema é se o lobo como o esquilo antes ou depois de ter o filho?

	Actualmente aumenta o breeding period depois
 	e só tem o filho se continuar vivo depois de andar */
void moveTo(position from, position to) { 
	enum type from_type = old_world[from.row][from.column].type;  
    enum type to_type = new_world[to.row][to.column].type;

    // only animals are moved
    if (from_type == SQUIRREL || from_type == SQUIRREL_ON_TREE) {
    	if (to_type == WOLF) {
    		new_world[to.row][to.column].starvation_period = 
  					max(0, old_world[from.row][from.column].starvation_period - 1);
    	}
    	else {
    		if (to_type == SQUIRREL || to_type == SQUIRREL_ON_TREE) {
    			new_world[to.row][to.column].breeding_period = 
    					max(old_world[from.row][from.column].breeding_period + 1,
    						new_world[to.row][to.column].breeding_period);
    		}
    		else {
    			new_world[to.row][to.column].breeding_period = 
    					old_world[from.row][from.column].breeding_period + 1;
    		}
    	}
    }
    else {
    	if (to_type == SQUIRREL) {
    		new_world[to.row][to.column].type = WOLF;
    		new_world[to.row][to.column].starvation_period = 
  					max(0, old_world[from.row][from.column].starvation_period - 1);
    		new_world[to.row][to.column].breeding_period = old_world[to.row][to.column].breeding_period + 1;
    	}
    	else if (to_type == WOLF) {
    		new_world[to.row][to.column].starvation_period = 
    				min(old_world[from.row][from.column].starvation_period,
    				    new_world[to.row][to.column].starvation_period);
    	}
    }
    clean(from);
}

void updateCell(position pos) {
    enum type type = old_world[pos.row][pos.column].type;
    if (type == ICE || type == TREE || type == NONE) {
    	return;
    }

    position to = getDestination(pos);
    moveTo(pos,to);
    if (isStarving(to)) clean(pos);
    if (isBreeding(to)) breed(pos,to);
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
	position pos;
	for (i = 0; i < WORLD_SIZE; i++) {
		for (j = 0; j < WORLD_SIZE; j++) {
			pos.row = i;
			pos.column = j;
			if ((!blackTurn && (i >> 1 == j >> 1)) || (blackTurn && (i >> 1 != j >> 1))) {
				updateCell(pos);
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
