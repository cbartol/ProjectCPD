#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BLACK_TURN 0
#define RED_TURN 1
#define MAX 100
#define isBlackTurn(t) ((t) == BLACK_TURN)
#define isRedTurn(t) ((t) == RED_TURN)
#define max(a,b) ((a) > (b) ? (a) : (b))
#define min(a,b) ((a) < (b) ? (a) : (b))

enum Move {
	TOP = 0,
	RIGHT = 1,
	BOTTOM = 2,
	LEFT = 3,
	NONE = 4
};

enum Type {
	EMPTY = 0,
	WOLF = 1,
	SQUIRREL = 2,
	TREE = 3,
	ICE = 4,
	SQUIRREL_ON_TREE = 5
};

typedef struct {
	enum Type type;
	int breeding_period;
	int starvation_period;
} World[MAX][MAX];

typedef struct {
	int row;
	int column;
} Position;

int NUM_ARGUMENTS = 6;
int WOLF_BREEDING_LEVEL;
int SQUIRREL_BREEDING_LEVEL;
int WOLF_STARVING_LEVEL;
int WORLD_SIZE;
World old_world;
World new_world;


#ifdef PROJ_DEBUG
/* DEBUG declaration */
	void printWord(World, int);
	char reverseConvertType(enum Type);
#endif



/* returns 1 if animal is breeding, 0 otherwise */
int isBreeding(Position pos) {
	if (new_world[pos.row][pos.column].type == WOLF) {
		return new_world[pos.row][pos.column].breeding_period == WOLF_BREEDING_LEVEL;
	}
	else if (new_world[pos.row][pos.column].type == SQUIRREL) {
		return new_world[pos.row][pos.column].breeding_period == SQUIRREL_BREEDING_LEVEL;
	}
	return 0;
}

/* returns 1 if wolf is starving, 0 otherwise */
int isStarving(Position pos) {
	return new_world[pos.row][pos.column].type == WOLF ? 
		new_world[pos.row][pos.column].starvation_period == WOLF_STARVING_LEVEL :
		0;
}

/* transforms a Position into the corresponding number in the matrix */
int numberOfPosition(Position pos) {
	return pos.row * WORLD_SIZE + pos.column;
}

/* returns 1 if type can move to the given Position, 0 otherwise */
int canMoveTo(Position from, Position to){
	int fromCell = old_world[from.row][from.column].type;
	int toCell = old_world[to.row][to.column].type;
	
	// can't move a tree or ice cell nor it can move to a ice cell
	if ((fromCell == SQUIRREL || fromCell == SQUIRREL_ON_TREE || fromCell == WOLF) && (toCell != ICE)) {
		// wolf's can't go to trees
		if ((fromCell == WOLF) && (toCell == TREE || toCell == SQUIRREL_ON_TREE)) { 
			return 0;
		}
		return 1;
	}
	return 0;
}

/* returns 1 if pos is a squirrel */
int isSquirrel(Position pos ){
    return new_world[pos.row][pos.column].type == SQUIRREL;
}

/* returns 1 if pos is a wolf */
int isWolf(Position pos ){
    return new_world[pos.row][pos.column].type == WOLF;
}

/* returns a valid move */
Position getDestination(Position pos) {
	Position possible[4];
	int available[4] = {0,0,0,0};
    
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
    int nSquirrels = 0;
    
	for (i = 0; i < 4; i++) {
		if (possible[i].row >= 0 && possible[i].row < WORLD_SIZE &&
            possible[i].column >= 0 && possible[i].column < WORLD_SIZE){
			if (canMoveTo(pos, possible[i])) {
                if(isWolf(pos) && isSquirrel(possible[i])){
                    available[i] = 2;
                    nSquirrels++;
                }
				else {
                    available[i] = 1;
                    nAvailable++;
                }
            }
		}
	}
    
    if(nAvailable == 0 && nSquirrels == 0)
        return pos;
    
    int n = 1;
    if(nSquirrels != 0){
        nAvailable = nSquirrels;
        n = 2;
    }
    
    int selected = numberOfPosition(pos) % nAvailable;
	for (i = 0; i < 4; i++) {
		if (available[i] == n) {
			if (selected == 0) {
				return possible[i];
			}
			selected--;
		}
	}
	return pos;
}


/* resets the periods in the position */
void breed(Position pos) {
    new_world[pos.row][pos.column].starvation_period = 0;
    new_world[pos.row][pos.column].breeding_period = 0;
}

/* the cell becomes empty */
void clean(Position pos) {
    enum Type type = new_world[pos.row][pos.column].type; 
    if (type == WOLF || type == SQUIRREL) {
    	new_world[pos.row][pos.column].type = EMPTY;
    }
    else if (type == SQUIRREL_ON_TREE) {
    	new_world[pos.row][pos.column].type = TREE;
    }

    new_world[pos.row][pos.column].starvation_period = 0;
    new_world[pos.row][pos.column].breeding_period = 0;
}

/* compare two positions and return 1 if they are the same position */
int isEqualPos(Position pos1, Position pos2){
	return (pos1.row == pos2.row) && (pos1.column == pos2.column);
}


/* moves animal from the current position to its destination */
void moveTo(Position from, Position to) { 
	enum Type from_type = old_world[from.row][from.column].type;  
    enum Type to_type = new_world[to.row][to.column].type;
    int breeded = 0;

    if (isStarving(from)) {
    	clean(from);
    	return;
    }

    if (isBreeding(from)) { 
    	breed(from);
    	breeded = 1;
    }

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
    			new_world[to.row][to.column].type = to_type == TREE ? SQUIRREL_ON_TREE : SQUIRREL;
    			new_world[to.row][to.column].breeding_period = 
    					old_world[from.row][from.column].breeding_period + 1;
    			new_world[to.row][to.column].starvation_period = 0;
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
    		if (old_world[from.row][from.column].starvation_period + 1 == new_world[to.row][to.column].starvation_period) {
    			new_world[to.row][to.column].breeding_period = max(old_world[from.row][from.column].breeding_period + 1,
    			                                                   new_world[to.row][to.column].breeding_period);
    		}
    		else if (old_world[from.row][from.column].starvation_period + 1 < new_world[to.row][to.column].starvation_period) {
    			new_world[to.row][to.column].breeding_period = old_world[from.row][from.column].breeding_period + 1;
    		}
    	}
    	else {
    		new_world[to.row][to.column].type = WOLF;
    		new_world[to.row][to.column].starvation_period = old_world[from.row][from.column].starvation_period + 1;
    		new_world[to.row][to.column].breeding_period = old_world[from.row][from.column].breeding_period + 1;
    	}
    }

    if (!breeded) {
    	clean(from);
    }



#ifdef PROJ_DEBUG
    printf("from: %c %d,%d  to: %c %d,%d\n", reverseConvertType(from_type),from.row,from.column, reverseConvertType(to_type),to.row,to.column);
#endif
}

void updateCell(Position pos) {
    enum Type type = old_world[pos.row][pos.column].type;
    if (type == ICE || type == TREE || type == EMPTY) {
    	return;
    }

	Position to = getDestination(pos);
	if (isEqualPos(pos, to)) {
		moveTo(pos, to);
	}
	else {
		if (new_world[pos.row][pos.column].type == WOLF) {
			max(new_world[pos.row][pos.column].starvation_period - 1, 0);
		}

		if (isStarving(pos)) {
			clean(pos);
		}
	}
}

enum Type convertType(char type) {
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

void createWorld(World aWorld, FILE *input) {
	// initialize World with zeros
	memset(aWorld, EMPTY, sizeof(World));

	// read World size
	fscanf(input, "%d", &WORLD_SIZE);

	int row;
	int column;
	char type;
	while (fscanf(input, "%d %d %c", &row, &column, &type) == 3) {
		old_world[row][column].type = convertType(type);
	}
}

void play(int turn) {
	int i, j;
	Position pos;
	for (i = 0; i < WORLD_SIZE; i++) {
		for (j = 0; j < WORLD_SIZE; j++) {
			pos.row = i;
			pos.column = j;
			if ((isRedTurn(turn) && (i % 2 == j % 2)) || (isBlackTurn(turn) && (i % 2 != j % 2))) {
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
	memcpy(new_world, old_world, sizeof(World));
	fclose(input);

	WOLF_BREEDING_LEVEL = atoi(argv[2]);
	SQUIRREL_BREEDING_LEVEL = atoi(argv[3]);
	WOLF_STARVING_LEVEL = atoi(argv[4]);
	int number_generations = atoi(argv[5]);
	#ifdef PROJ_DEBUG
		fprintf(stdout, "initial variables:\n");
		fprintf(stdout,"\t- WOLF_BREEDING_LEVEL[%d]\n", WOLF_BREEDING_LEVEL);
		fprintf(stdout,"\t- SQUIRREL_BREEDING_LEVEL[%d]\n", SQUIRREL_BREEDING_LEVEL);
		fprintf(stdout,"\t- WOLF_STARVING_LEVEL[%d]\n", WOLF_STARVING_LEVEL);
		fprintf(stdout,"\t- number_generations[%d]\n\n", number_generations);
	#endif


	int gen;
	#ifdef PROJ_DEBUG
		fprintf(stdout, "init\n");
		printWord(new_world, WORLD_SIZE);
		fprintf(stdout,"----------------------------------\n");
	#endif
	for (gen = 0; gen < number_generations; gen++) {
		play(RED_TURN);
		memcpy(old_world, new_world, sizeof(World));
		#ifdef PROJ_DEBUG
			fprintf(stdout, "RED_TURN\n");
			printWord(new_world, WORLD_SIZE);
			fprintf(stdout, "\n\n");
		#endif
		play(BLACK_TURN);
		memcpy(old_world, new_world, sizeof(World));
		#ifdef PROJ_DEBUG
			fprintf(stdout, "BLACK_TURN\n");
			printWord(new_world, WORLD_SIZE);
			fprintf(stdout, "\n\n");
		#endif
	}
	#ifdef PROJ_DEBUG
		fprintf(stdout,"\n\n----------------------------------\n");
		fprintf(stdout, "END WORLD\n");
		printWord(new_world, WORLD_SIZE);
	#endif
	return 0;
}


/*
 **********************************
 **                              **
 **        DEBUG FUNCTIONS       **
 **                              ** 
 **********************************
 */


#ifdef PROJ_DEBUG
	void printWord(World w, int w_size){
		int i, j;
		for (i = 0; i < WORLD_SIZE; i++) {
			for (j = 0; j < WORLD_SIZE; j++) {
				fprintf(stdout, "%c ", reverseConvertType(w[i][j].type));
			}
			fprintf(stdout, "\n");
		}
	}

	char reverseConvertType(enum Type type) {
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
#endif

