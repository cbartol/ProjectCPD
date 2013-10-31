#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

#define FALSE 0
#define TRUE 1
#define max(a,b) ((a) > (b) ? (a) : (b))
#define min(a,b) ((a) < (b) ? (a) : (b))

// Empty must always be 0
typedef enum {
	EMPTY = 0,
	WOLF = 1,
	SQUIRREL = 2,
	TREE = 3,
	ICE = 4,
	SQUIRREL_ON_TREE = 5
} type_e;

// None must always be the last one
typedef enum {
	TOP = 0,
	RIGHT = 1,
	BOTTOM = 2,
	LEFT = 3,
	NONE = 4
} move_e;

typedef struct {
	type_e type;
	int breeding_period;
	int starvation_period;
	int has_moved;
} world_pos;

typedef world_pos **world_t;
typedef world_pos *world_pos_t;

int numberOfPosition(int row, int col);
type_e atot(char c);
char ttoa(type_e type);
void init(FILE *file, char **argv);
void printWorld();
int isRedGen(int row, int col);
int isBlackGen(int row, int col);
void playGen();
void cleanWorld();
int isWolfToSquirrel(world_pos_t from, world_pos_t to);
move_e getMove(int row, int col);
world_pos_t getDestination(int row, int col, move_e move, omp_lock_t **to_lock);
void chooseBestSquirrel(world_pos_t from, world_pos_t to);
void chooseBestWolf(world_pos_t from, world_pos_t to);
void movePos(world_pos_t from, world_pos_t to);
void updatePos(int row, int col);
void copyPos(world_pos_t from, world_pos_t to);
void copyWorld();
int isBreeding(world_pos_t pos);
int isStarving(world_pos_t pos);
void breed(world_pos_t pos);
void clean(world_pos_t pos);

const int NUM_ARGUMENTS = 6;
int WORLD_SIZE = 0;
int WOLF_BREEDING_LEVEL = 0;
int SQUIRREL_BREEDING_LEVEL = 0;
int WOLF_STARVING_LEVEL = 0;
int NUM_GENERATIONS = 0;
world_t old_world = NULL;
world_t new_world = NULL;
omp_lock_t **lock_world = NULL;

int numberOfPosition(int row, int col) {
	return row*WORLD_SIZE + col;
}

type_e atot(char c) {
	switch (c) {
		case 'w': return WOLF;
		case 's': return SQUIRREL;
		case 'i': return ICE;
		case 't': return TREE;
		case '$': return SQUIRREL_ON_TREE;
	}

	fprintf(stderr, "Unknown type: %c\n", c);
	exit(EXIT_FAILURE);
}

char ttoa(type_e type) {
	switch (type) {
		case EMPTY:            return ' ';
		case WOLF:             return 'w';
		case SQUIRREL:         return 's';
		case ICE:              return 'i';
		case TREE:             return 't';
		case SQUIRREL_ON_TREE: return '$';
	}

	fprintf(stderr, "Unknown type: %d\n", type);
	exit(EXIT_FAILURE);
}

void init(FILE *file, char **argv) {
	// read WORLD_SIZE
	fscanf(file, "%d", &WORLD_SIZE);

	world_pos_t newWorld = malloc(sizeof(world_pos) * WORLD_SIZE * WORLD_SIZE);
	world_pos_t oldWorld = malloc(sizeof(world_pos) * WORLD_SIZE * WORLD_SIZE);
	omp_lock_t *lockWorld = malloc(sizeof(omp_lock_t) * WORLD_SIZE * WORLD_SIZE);
	new_world = malloc(sizeof(world_pos_t) * WORLD_SIZE);
	old_world = malloc(sizeof(world_pos_t) * WORLD_SIZE);
	lock_world = malloc(sizeof(omp_lock_t *) * WORLD_SIZE);

	int i;
	for (i = 0; i < WORLD_SIZE; i++) {
		new_world[i] = newWorld + i*WORLD_SIZE;
		old_world[i] = oldWorld + i*WORLD_SIZE;
		lock_world[i] = lockWorld + i*WORLD_SIZE;
	}

	// initialize both worlds with zeros
	memset(oldWorld, 0, sizeof(world_pos) * WORLD_SIZE * WORLD_SIZE);
	memset(newWorld, 0, sizeof(world_pos) * WORLD_SIZE * WORLD_SIZE);
	for (i = 0; i < WORLD_SIZE*WORLD_SIZE; ++i) {
		omp_init_lock(lockWorld + i);
	}
	

	// initialize both worlds with the map
	int row;
	int col;
	char type;
	while (fscanf(file, "%d %d %c", &row, &col, &type) == 3) {
		old_world[row][col].type = atot(type);
		new_world[row][col].type = atot(type);
	}

	WOLF_BREEDING_LEVEL = atoi(argv[2]);
	SQUIRREL_BREEDING_LEVEL = atoi(argv[3]);
	WOLF_STARVING_LEVEL = atoi(argv[4]);
	NUM_GENERATIONS = atoi(argv[5]);
}

void printWorld() {
	int i, j;
	for (i = 0; i < WORLD_SIZE; i++) {
		for (j = 0; j < WORLD_SIZE; j++) {
			if (new_world[i][j].type != EMPTY){
				fprintf(stdout, "%d %d %c\n", i,j, ttoa(new_world[i][j].type));
			}
		}
	}
}

int isRedGen(int row, int col) {
	return (row % 2) == (col % 2);
}

int isBlackGen(int row, int col) {
	return !isRedGen(row, col);
}


void clean(world_pos_t pos) {
	switch (pos->type) {
		case TREE:
		case SQUIRREL_ON_TREE:
			pos->type = TREE;
			break;

		case ICE:
			pos->type = ICE;
			break;

		default:
			pos->type = EMPTY;
	}

	pos->breeding_period = 0;
	pos->starvation_period = 0;
	pos->has_moved = 0;
}

void cleanWorld() {
	int i, j;
	#pragma omp parallel for private(i,j)
	for (i = 0; i < WORLD_SIZE; i++) {
		for (j = 0; j < WORLD_SIZE; j++) {
			clean(&new_world[i][j]);
		}
	}
}

int canMoveTo(world_pos_t from, world_pos_t to) {
	// Can't move to ice
	if (to->type == ICE) {
		return FALSE;
	}

	switch (from->type) {
		case SQUIRREL:
		case SQUIRREL_ON_TREE:
			return (to->type == EMPTY) || (to->type == TREE);

		case WOLF:
			return (to->type == EMPTY) || (to->type == SQUIRREL);

		default:
			fprintf(stderr, "Can't move this type: %d\n", from->type);
			exit(EXIT_FAILURE);
	}

	return FALSE;
}

int isWolfToSquirrel(world_pos_t from, world_pos_t to) {
	return (from->type == WOLF) && (to->type == SQUIRREL);
}

move_e getMove(int row, int col) {
	const int NUM_OPTION = 4;
	int available[NUM_OPTION];
	memset( available, 0, NUM_OPTION*sizeof(int) );
	int nAvailable = 0;
    int nSquirrels = 0;

    world_pos_t cur = &old_world[row][col];

    // TOP
    if ((row-1 >= 0) && canMoveTo(cur, &old_world[row-1][col])) {
    	if (isWolfToSquirrel(cur, &old_world[row-1][col])) {
			available[TOP] = 2;
            nSquirrels++;
		} else {
            available[TOP] = 1;
            nAvailable++;
        }
    }
    
    // RIGHT
    if ((col+1 < WORLD_SIZE) && canMoveTo(cur, &old_world[row][col+1])) {
    	if (isWolfToSquirrel(cur, &old_world[row][col+1])) {
			available[RIGHT] = 2;
            nSquirrels++;
		} else {
            available[RIGHT] = 1;
            nAvailable++;
        }
    }

    // BOTTOM
    if ((row+1 < WORLD_SIZE) && canMoveTo(cur, &old_world[row+1][col])) {
    	if (isWolfToSquirrel(cur, &old_world[row+1][col])) {
			available[BOTTOM] = 2;
            nSquirrels++;
		} else {
            available[BOTTOM] = 1;
            nAvailable++;
        }
    } 

    // LEFT
    if ((col-1 >= 0) && canMoveTo(cur, &old_world[row][col-1])) {
    	if (isWolfToSquirrel(cur, &old_world[row][col-1])) {
			available[LEFT] = 2;
            nSquirrels++;
		} else {
            available[LEFT] = 1;
            nAvailable++;
        }
    }
    
    if (nAvailable == 0 && nSquirrels == 0)
        return NONE;
    
    int n = 1;
    if(nSquirrels != 0){
        nAvailable = nSquirrels;
        n = 2;
    }
    
    int selected = numberOfPosition(row, col) % nAvailable;
    int i;
	for (i = 0; i < NUM_OPTION; i++) {
		if (available[i] == n) {
			if (selected == 0){
				return i;
			}
			selected--;
		}
	}

	return NONE;
}

world_pos_t getDestination(int row, int col, move_e move, omp_lock_t **to_lock) {
	switch (move) {
		case TOP:
			*to_lock = &lock_world[row-1][col]; 
			return &new_world[row-1][col];

		case RIGHT:
			*to_lock = &lock_world[row][col+1]; 
			return &new_world[row][col+1];

		case BOTTOM:
			*to_lock = &lock_world[row+1][col];
			return &new_world[row+1][col];

		case LEFT:
			*to_lock = &lock_world[row][col-1]; 
			return &new_world[row][col-1];

		case NONE:
			*to_lock = &lock_world[row][col]; 
			return &new_world[row][col];

		default:
			fprintf(stderr, "Unknown move: %d\n", move);
			exit(EXIT_FAILURE);
			return NULL;
	}
}

void chooseBestSquirrel(world_pos_t from, world_pos_t to) {
	to->breeding_period = max(from->breeding_period, to->breeding_period);
}

void chooseBestWolf(world_pos_t from, world_pos_t to) {
	if (from->starvation_period == to->starvation_period) {
		to->breeding_period = max(from->breeding_period, to->breeding_period);
	} else if (from->starvation_period < to->starvation_period) {
		to->breeding_period = from->breeding_period;
		to->starvation_period = from->starvation_period;
	}
}


/* 	Since animals avoid collisions by default,
	if they encounter another animal, both have moved.

	If an animal goes to an 'empty' position, it moves.

	Since squirrels avoid wolfs, if a squirrel goes to
	a position with a wolf, both have moved.

	Since wolfs run after squirrels, he has moved.

	Conclusion, in all cases, the animal in the end
	always moves.
*/
void movePos(world_pos_t from, world_pos_t to) {
	switch (from->type) {
	   	case SQUIRREL:
	   	case SQUIRREL_ON_TREE: {
	   		switch (to->type) {
	   		case WOLF:
	   			to->starvation_period = 0;
	   			break;

	   		case SQUIRREL:
	   		case SQUIRREL_ON_TREE:
	   			chooseBestSquirrel(from, to);
				break;

			default:
				copyPos(from, to);
	   		}

	   		break;
	   	}

		case WOLF: {
			switch (to->type) {
			case SQUIRREL:
				copyPos(from, to);
	    		to->starvation_period = 0;
	    		break;

			case WOLF:
				chooseBestWolf(from, to);
	    	 	break;

	    	default:
	    		copyPos(from, to);
	    	}

	    	break;
	    }

	    default:
	    	fprintf(stderr, "Can't move %d!", from->type);
	    	exit(EXIT_FAILURE);
	}

	to->has_moved = TRUE;
}

void updatePos(int row, int col) {
	if ((old_world[row][col].type == EMPTY) || (old_world[row][col].type == TREE) || (old_world[row][col].type == ICE)) {
		return;
	}

	move_e move = getMove(row, col);
	world_pos_t from = &new_world[row][col];
	omp_lock_t *to_lock = NULL;
	world_pos_t to = getDestination(row, col, move, &to_lock);

	if (from == to) {
		return;
	}

	if (isBreeding(from)) {
		from->breeding_period = 0;

		omp_set_lock(to_lock);
		movePos(from, to);
		omp_unset_lock(to_lock);

		breed(from); // This is the inverse of what we had
	} else {
		omp_set_lock(to_lock);
		movePos(from, to);
		omp_unset_lock(to_lock);
		
		clean(from);
	}
}

void copyPos(world_pos_t from, world_pos_t to) {
	switch (from->type) {
		case SQUIRREL:
		case SQUIRREL_ON_TREE: {
			switch (to->type) {
				case SQUIRREL_ON_TREE:
				case TREE:
					to->type = SQUIRREL_ON_TREE;
					break;

				default:
					to->type = SQUIRREL;
			}
			break;
		}

		default:
			to->type = from->type;
	}

	to->breeding_period = from->breeding_period;
	to->starvation_period = from->starvation_period;
	to->has_moved = from->has_moved;
}

// Copies the new world to the old world
// It's independent of init
void copyWorld() {
	int i;
	#pragma omp parallel for private(i)
	for (i = 0; i < WORLD_SIZE; i++) {
		memcpy(old_world[i], new_world[i], sizeof(world_pos)*WORLD_SIZE);
	}
}

int isBreeding(world_pos_t pos) {
	switch (pos->type) {
		case WOLF:
			return pos->breeding_period == WOLF_BREEDING_LEVEL;

		case SQUIRREL:
		case SQUIRREL_ON_TREE:
			return pos->breeding_period == SQUIRREL_BREEDING_LEVEL;

		default:
			return FALSE;
	}
}

int isStarving(world_pos_t pos) {
	switch (pos->type) {
		case WOLF:
			return pos->starvation_period == WOLF_STARVING_LEVEL;

		default:
			return FALSE;
	}
}

// Same as clean but the type remains
void breed(world_pos_t pos) {
	pos->breeding_period = 0;
	pos->starvation_period = 0;
	pos->has_moved = 0;
}

// Can be improved
void playGen() {
	// Before generation, cleans starving animals
	int i, j;
	#pragma omp parallel for private(i,j)
	for (i = 0; i < WORLD_SIZE; i++) {
		for (j = 0; j < WORLD_SIZE; j++) {
			if (isStarving(&new_world[i][j])) {
				clean(&new_world[i][j]);
			}
		}
	}

	// Must keep consistency between worlds
	copyWorld();

	// Red sub-generation
	#pragma omp parallel for private(i,j)
	for (i = 0; i < WORLD_SIZE; i++) {
		for (j = 0; j < WORLD_SIZE; j++) {
			if (isRedGen(i, j)) {
				updatePos(i, j);
			}
		}
	}

	// Must keep consistency between worlds
	copyWorld();

	// Black sub-generation
	#pragma omp parallel for private(i,j)
	for (i = 0; i < WORLD_SIZE; i++) {
		for (j = 0; j < WORLD_SIZE; j++) {
			if (isBlackGen(i, j)) {
				updatePos(i, j);
			}
		}
	}

	// After generation, increase breeding_period to the animals
	// that moved
	#pragma omp parallel for private(i,j)
	for (i = 0; i < WORLD_SIZE; i++) {
		for (j = 0; j < WORLD_SIZE; j++) {
			if (new_world[i][j].has_moved) {
				new_world[i][j].breeding_period++;
				new_world[i][j].has_moved = FALSE;
			}
		}
	}
}

int main(int argc, char **argv) {
	if (argc < NUM_ARGUMENTS) {
		fprintf(stderr, "Not enough arguments...\n");
		exit(EXIT_FAILURE);
	}

	FILE *input = fopen(argv[1], "r");
	if (input == NULL) {
		fprintf(stderr, "File %s not found...\n", argv[1]);
		exit(EXIT_FAILURE);
	}

	init(input, argv);
	fclose(input);

	double start = omp_get_wtime();
	int gen;
	for (gen = 0; gen < NUM_GENERATIONS; gen++) {
		playGen();
	}

	double end = omp_get_wtime();
	printf("Took %f\n", end - start);

	printWorld();
	return 0;
}
