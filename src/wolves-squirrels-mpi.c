#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

#define MASTER 0
#define FALSE 0
#define TRUE 1
#define max(a,b) ((a) > (b) ? (a) : (b))
#define min(a,b) ((a) < (b) ? (a) : (b))

// Empty must always be 0
#define EMPTY 0
#define WOLF 1
#define SQUIRREL 2
#define TREE 3
#define ICE 4
#define SQUIRREL_ON_TREE 5

// None must always be the last one
typedef enum {
	TOP = 0,
	RIGHT = 1,
	BOTTOM = 2,
	LEFT = 3,
	NONE = 4
} move_e;

#define type_e unsigned char

typedef struct {
	type_e type;
	unsigned char breeding_period;
	unsigned char starvation_period;
	unsigned char has_moved;
} world_pos;

typedef world_pos **world_t;
typedef world_pos *world_pos_t;

const int NUM_ARGUMENTS = 6;
int WORLD_SIZE;
int WOLF_BREEDING_LEVEL;
int SQUIRREL_BREEDING_LEVEL;
int WOLF_STARVING_LEVEL;
int NUM_GENERATIONS;

int processor_id;
int num_processors;

unsigned char *top_send_line = NULL;
unsigned char *bottom_send_line = NULL;

unsigned char *top_line = NULL;
world_pos_t top_changed_line = NULL;

world_t old_world_section = NULL;
world_t new_world_section = NULL;

world_pos_t bottom_changed_line = NULL;
unsigned char *bottom_line = NULL;
int section_lines = 0;
int start_with_black = 0;
int real_row_start = 0;

/* Function that returns the number of a position, given a row and a column. */ 
int numberOfPosition(int row, int col) {
	return row*WORLD_SIZE + col;
}

/* Function that returns the number of world section lines of a given process. */
int numberLinesForProcess(int process_id) {
	return ((WORLD_SIZE / num_processors) + ((WORLD_SIZE + (WORLD_SIZE%num_processors))/(WORLD_SIZE + process_id + 1)));
}

/* Function that converts a given char into unsigned char, it's used when reading */
/* the input file to convert it into each process world matrix. */
unsigned char atot(char c) {
	switch (c) {
		case 'w': return WOLF;
		case 's': return SQUIRREL;
		case 'i': return ICE;
		case 't': return TREE;
		case '$': return SQUIRREL_ON_TREE;
	}

	fprintf(stderr, "Unknown type: %c\n", c);
	MPI_Finalize();
	exit(EXIT_FAILURE);
}

/* Function that converts a given unsigned char into char, it's used when printing */
/* the world matrix to convert each position to a char. */
char ttoa(unsigned char type) {
	switch (type) {
		case EMPTY:            return ' ';
		case WOLF:             return 'w';
		case SQUIRREL:         return 's';
		case ICE:              return 'i';
		case TREE:             return 't';
		case SQUIRREL_ON_TREE: return '$';
	}

	fprintf(stderr, "Unknown type: %d\n", type);
	MPI_Finalize();
	exit(EXIT_FAILURE);
}

/* Function to initialize each process world section and respective lines to send to */
/* other processes. */
void init_proc_section(FILE *file, char **argv) {

	if (fscanf(file, "%d", &WORLD_SIZE) == 0) {
		MPI_Finalize();
		exit(EXIT_FAILURE);
	}

	WOLF_BREEDING_LEVEL = atoi(argv[2]);
	SQUIRREL_BREEDING_LEVEL = atoi(argv[3]);
	WOLF_STARVING_LEVEL = atoi(argv[4]);
	NUM_GENERATIONS = atoi(argv[5]);

	int i;
	section_lines = numberLinesForProcess(processor_id);

	top_send_line = malloc(WORLD_SIZE*sizeof(unsigned char));
	bottom_send_line = malloc(WORLD_SIZE*sizeof(unsigned char));
	if (processor_id != num_processors-1) {
		bottom_changed_line = malloc(sizeof(world_pos) * WORLD_SIZE);
		bottom_line = malloc(sizeof(unsigned char) * WORLD_SIZE); 
		memset(bottom_changed_line, 0, sizeof(world_pos) * WORLD_SIZE);	
	}

	if (processor_id != MASTER) {
		top_changed_line = malloc(sizeof(world_pos) * WORLD_SIZE);
		top_line = malloc(sizeof(unsigned char) * WORLD_SIZE);
		memset(top_changed_line, 0, sizeof(world_pos) * WORLD_SIZE);
	}

	world_pos_t oldWorldSection = malloc(sizeof(world_pos) * WORLD_SIZE * section_lines);
	world_pos_t newWorldSection = malloc(sizeof(world_pos) * WORLD_SIZE * section_lines);
	memset(oldWorldSection, 0, sizeof(world_pos) * section_lines * WORLD_SIZE);
	memset(newWorldSection, 0, sizeof(world_pos) * section_lines * WORLD_SIZE);
	old_world_section = malloc(sizeof(world_pos_t) * section_lines);
	new_world_section = malloc(sizeof(world_pos_t) * section_lines);


	for (i = 0; i < section_lines; i++) {
		new_world_section[i] = newWorldSection + i*WORLD_SIZE;
		old_world_section[i] = oldWorldSection + i*WORLD_SIZE;
	}
	// initialize both worlds sections with the map
	int row;
	int col;
	char type;
	while (fscanf(file, "%d %d %c", &row, &col, &type) == 3) {
		if ((row >= real_row_start) && (row < (real_row_start + section_lines))) {
			new_world_section[row-real_row_start][col].type = atot(type);
			old_world_section[row-real_row_start][col].type = atot(type);
		}
	}
}

/* Function that initializes master process world section, and sends to other processes */
/* their world section first line index and a variable that indicates whether to start */ 
/* with black or red. */
void master_init(FILE *file, char **argv) {
	
	init_proc_section(file, argv);

	int start_black = 0 ^ (numberLinesForProcess(MASTER)%2);
	int real_line_num = 0;

	int i;
	for (i = 1; i < num_processors; ++i) {
		real_line_num += numberLinesForProcess(i-1);
		MPI_Send(&real_line_num, 1, MPI_INT, i, i, MPI_COMM_WORLD);
		MPI_Send(&start_black, 1, MPI_INT, i, i, MPI_COMM_WORLD);
		start_black ^= (numberLinesForProcess(i)%2);
	}
}

/* Function that initializes the process world section, and receives its world section first */
/* line index and a variable that indicates whether to start with black or red. */
void proc_init(FILE *file, char **argv) {

	MPI_Recv(&real_row_start, 1, MPI_INT, MASTER, processor_id, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	init_proc_section(file, argv);	
	MPI_Recv(&start_with_black, 1, MPI_INT, MASTER, processor_id, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}

/* Function that sends the process world section to the Master process. */ 
void proc_final() {
	int section_size = sizeof(world_pos)*WORLD_SIZE*section_lines;
	MPI_Send(*new_world_section, section_size, MPI_BYTE, MASTER, processor_id, MPI_COMM_WORLD);
}

/* Function that prints a given world section. */
void printSection(world_t section, int sectionLines, int row) {
	int i,j;
	for (i = 0; i < sectionLines; i++) {
		for (j = 0; j < WORLD_SIZE; j++) {
			if (section[i][j].type != EMPTY){
				fprintf(stdout, "%d %d %c\n", row+i,j, ttoa(section[i][j].type));
			}
		}
	}
}

/* Function that receives each process' world section and prints it ordered by processor id. */
void printWorld() {
	int n;

	printSection(new_world_section, section_lines, real_row_start);
	int i, maxLines = 0 , procLines;
	for (i = 0 ; i < num_processors ; i++) {
		procLines = numberLinesForProcess(i);
		if (procLines > maxLines) {
			maxLines = procLines;
		}
	}
	void *buff = malloc(sizeof(world_pos) * maxLines * WORLD_SIZE);
	world_t matrix_buff = malloc(sizeof(world_t) * maxLines);
	for (i = 0; i < maxLines ;i++) {
		matrix_buff[i] = buff + i*WORLD_SIZE*sizeof(world_pos);
	}
	int row = section_lines;
	for (n = 1; n < num_processors ; n++) { 
		int sectionLines = numberLinesForProcess(n);
		int section_size = sectionLines*WORLD_SIZE * sizeof(world_pos);
		MPI_Recv(buff, section_size, MPI_BYTE, n, n, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		printSection(matrix_buff, sectionLines, row);
		row += sectionLines;
		memset(buff, 0, sizeof(world_pos) * WORLD_SIZE * maxLines);
	}
}


/* Function that receives a position and sets all its values to zero. */
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

/* Function that returns 1 if animal in 'from' position can move to position 'to' , and */
/* returns 0 otherwise. */
int canMoveTo(type_e from, type_e to) {
	// Can't move to ice
	if (to == ICE) {
		return FALSE;
	}

	switch (from) {
		case SQUIRREL:
		case SQUIRREL_ON_TREE:
			return (to == EMPTY) || (to == TREE);

		case WOLF:
			return (to == EMPTY) || (to == SQUIRREL);

		default:
			fprintf(stderr, "Can't move this type: %d\n", from);
			MPI_Finalize();
			exit(EXIT_FAILURE);
	}

	return FALSE;
}

/* Function that returns 1 if animal moving from position 'from' is a wolf and animal in */
/* position 'to' is a squirrel , returns 0 otherwise. */
int isWolfToSquirrel(type_e from, type_e to) {
	return (from == WOLF) && (to == SQUIRREL);
}

/* Function that given a position, tests adjacent positions (in all directions) starting on the top position and */
/* continuing to test in clockwise . Returns a move if it can move , or none otherwise. */
move_e getMove(int row, int col) {
	const int NUM_OPTION = 4;
	int available[NUM_OPTION];
	memset( available, 0, NUM_OPTION*sizeof(int) );
	int nAvailable = 0;
    int nSquirrels = 0;

    type_e cur = old_world_section[row][col].type;
    type_e top_element = EMPTY;
    int map_inbounds = 1;
    if (processor_id != MASTER && (row-1 < 0)) {
    	top_element = top_line[col]; 
    } else if (row > 0) {
    	top_element = old_world_section[row-1][col].type;
    } else {
    	map_inbounds = 0;
    }

    // TOP
    if (map_inbounds && canMoveTo(cur, top_element))   {
    	if (isWolfToSquirrel(cur, top_element)) {
			available[TOP] = 2;
            nSquirrels++;
		} else {
            available[TOP] = 1;
            nAvailable++;
        }
    }
    
    // RIGHT
    if ((col+1 < WORLD_SIZE) && canMoveTo(cur, old_world_section[row][col+1].type)) {
    	if (isWolfToSquirrel(cur, old_world_section[row][col+1].type)) {
			available[RIGHT] = 2;
            nSquirrels++;
		} else {
            available[RIGHT] = 1;
            nAvailable++;
        }
    }

    map_inbounds = 1;
    type_e bottom_element = EMPTY;
    if (processor_id != num_processors-1 && (row+1 >= section_lines)) {
    	bottom_element = bottom_line[col];
    } else if (row+1 < section_lines) {
    	bottom_element = old_world_section[row+1][col].type;
    } else {
    	map_inbounds = 0;
    }

    // BOTTOM
    if (map_inbounds && canMoveTo(cur, bottom_element)) {
    	if (isWolfToSquirrel(cur, bottom_element)) {
			available[BOTTOM] = 2;
            nSquirrels++;
		} else {
            available[BOTTOM] = 1;
            nAvailable++;
        }
    } 

    // LEFT
    if ((col-1 >= 0) && canMoveTo(cur, old_world_section[row][col-1].type)) {
    	if (isWolfToSquirrel(cur, old_world_section[row][col-1].type)) {
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
    
    int selected = numberOfPosition((real_row_start+row), col) % nAvailable;
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

/* Function that given an original position and a move, returns the destination cell by applying */
/* the move to the original position. */ 
world_pos * getDestination(int row, int col, move_e move) {
	switch (move) {
		case TOP:
			return &new_world_section[row-1][col];

		case RIGHT:
			return &new_world_section[row][col+1];

		case BOTTOM:
			return &new_world_section[row+1][col];

		case LEFT:
			return &new_world_section[row][col-1];

		case NONE:
			return &new_world_section[row][col];

		default:
			fprintf(stderr, "Unknown move: %d\n", move);
			MPI_Finalize();
			exit(EXIT_FAILURE);

			// doesn't do anything
			return &new_world_section[row][col];
	}
}

/* Function used to deal with conflicts, applies the squirrel rules to make the decision. */
void chooseBestSquirrel(world_pos *from, world_pos *to) {
	to->breeding_period = max(from->breeding_period, to->breeding_period);
}

/* Function used to deal with conflicts, applies the wolf rules to make the decision. */
void chooseBestWolf(world_pos *from, world_pos *to) {
	if (from->starvation_period == to->starvation_period) {
		to->breeding_period = max(from->breeding_period, to->breeding_period);
	} else if (from->starvation_period < to->starvation_period) {
		to->breeding_period = from->breeding_period;
		to->starvation_period = from->starvation_period;
	}
}

/* Function that given to cells, copies all the information from the cell 'from' into the cell 'to'. */
void copyPos(world_pos *from, world_pos *to) {
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

/* 	Since animals avoid collisions by default,
	if they encounter another animal, both have moved.

	If an animal goes to an 'empty' position, it moves.

	Since squirrels avoid wolfs, if a squirrel goes to
	a position with a wolf, both have moved.

	Since wolfs run after squirrels, he has moved.

	Conclusion, in all cases, the animal in the end
	always moves.
*/
void movePos(world_pos  *from, world_pos *to) {
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
	    	return;
	}

	to->has_moved = TRUE;
}

/* Function that given a cell reset all its values except its type. */
void breed(world_pos *pos) {
	pos->breeding_period = 0;
	pos->starvation_period = 0;
	pos->has_moved = 0;
}

/* Function that copies the new_world_section into the old_world_section. */
void copyWorld() {
	int i;
	for (i = 0; i < section_lines; i++) {
		memcpy(old_world_section[i], new_world_section[i], sizeof(world_pos)*WORLD_SIZE);
	}
}

/* Function that given a cell tests if there's an animal there, and if it is tests if the animal is about to breed. */
int isBreeding(world_pos *pos) {
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

/* Function that given a cell tests if there's a wolf there, and if it is tests if the animal is about to starve. */
int isStarving(world_pos *pos) {
	switch (pos->type) {
		case WOLF:
			return pos->starvation_period == WOLF_STARVING_LEVEL;

		default:
			return FALSE;
	}
}

/* Function that given a position makes all the necessary tests to process it, and updates all its values. */
void updatePos(int row, int col) {
	if ((old_world_section[row][col].type == EMPTY) || (old_world_section[row][col].type == TREE) || (old_world_section[row][col].type == ICE)) {
		return;
	}

	move_e move = getMove(row, col);

	world_pos *from = &(new_world_section[row][col]);
	world_pos *to = NULL;
	if (move == TOP && row == 0) {
		to = &(top_changed_line[col]);
	} else if (move == BOTTOM && row == section_lines-1) { 
		to = &(bottom_changed_line[col]);
	} else {
		to = getDestination(row, col, move);
	}

	if (from == to) {
		return;
	}

	if (isBreeding(from)) {
		from->breeding_period = 0;
		movePos(from, to);
		breed(from); 
	} else {
		movePos(from, to);
		clean(from);
	}
}

/* Function that given two lines sent by other processes, merge them with the process world section. */
void merge(world_pos_t topLine , world_pos_t bottomLine) {
	int j;
	for (j = 0; j < WORLD_SIZE; j++) {
		movePos(&topLine[j], &new_world_section[0][j]);
		movePos(&bottomLine[j], &new_world_section[section_lines-1][j]);
	}
}

/* Function that  sends the border lines that are inside of the process's world's section. */
/* Note: only sends cell types to the other process */
void sendInsideBorders(MPI_Request *request1, MPI_Request *request2) {
	int section_size = WORLD_SIZE*sizeof(unsigned char);
	int j;
	for (j = 0; j < WORLD_SIZE; ++j){
		top_send_line[j] = new_world_section[0][j].type;
		bottom_send_line[j] = new_world_section[section_lines-1][j].type;
	}
	if(processor_id == MASTER){
		MPI_Isend(bottom_send_line, section_size, MPI_BYTE, processor_id+1, 1, MPI_COMM_WORLD, request2);
	} else if(processor_id == num_processors-1){
		MPI_Isend(top_send_line, section_size, MPI_BYTE, processor_id-1, 0, MPI_COMM_WORLD, request1);
	} else {
		MPI_Isend(top_send_line, section_size, MPI_BYTE, processor_id-1, 0, MPI_COMM_WORLD, request1);
		MPI_Isend(bottom_send_line, section_size, MPI_BYTE, processor_id+1, 1, MPI_COMM_WORLD, request2);
	}
}

/* Function that receives the border lines that are outside of the process's world's section. */
/* Note: only receives cell types from the other process */
void receiveOutsideBorders() {
	int section_size = WORLD_SIZE*sizeof(unsigned char);
	if(processor_id == MASTER){
		MPI_Recv(bottom_line, section_size, MPI_BYTE, processor_id+1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	} else if(processor_id == num_processors-1){
		MPI_Recv(top_line, section_size, MPI_BYTE, processor_id-1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	} else {
		MPI_Recv(top_line, section_size, MPI_BYTE, processor_id-1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		MPI_Recv(bottom_line, section_size, MPI_BYTE, processor_id+1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	}
}

/* Function that sends the border lines that affect other processes' world's section. */
/* Note: sends a complete line (with all atributes) to the other process */
void sendOutsideBorders(MPI_Request *request1, MPI_Request *request2) {
	if(processor_id == MASTER){
		MPI_Isend(bottom_changed_line, WORLD_SIZE*sizeof(world_pos), MPI_BYTE, processor_id+1, 1, MPI_COMM_WORLD, request2);
	} else if(processor_id == num_processors-1){
		MPI_Isend(top_changed_line, WORLD_SIZE*sizeof(world_pos), MPI_BYTE, processor_id-1, 0, MPI_COMM_WORLD, request1);
	} else {
		MPI_Isend(top_changed_line, WORLD_SIZE*sizeof(world_pos), MPI_BYTE, processor_id-1, 0, MPI_COMM_WORLD, request1);
		MPI_Isend(bottom_changed_line, WORLD_SIZE*sizeof(world_pos), MPI_BYTE, processor_id+1, 1, MPI_COMM_WORLD, request2);
	}
}

/* Function that receives the border lines that affect the process's world's section.
/* These lines will be merged and conflicts are resolved. */
/* Note: receives a complete line (with all atributes) from the other process. */
void receiveInsideBorders() {
	int section_size = WORLD_SIZE*sizeof(world_pos);
	world_pos_t top_received_line = malloc(section_size);
	world_pos_t bottom_received_line = malloc(section_size);
	if(processor_id == MASTER){
		memset(top_received_line, 0, section_size);
		MPI_Recv(bottom_received_line, section_size, MPI_BYTE, processor_id+1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	} else if(processor_id == num_processors-1){
		memset(bottom_received_line, 0, section_size);
		MPI_Recv(top_received_line, section_size, MPI_BYTE, processor_id-1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	} else {
		MPI_Recv(top_received_line, section_size, MPI_BYTE, processor_id-1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		MPI_Recv(bottom_received_line, section_size, MPI_BYTE, processor_id+1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	}
	
	merge(top_received_line, bottom_received_line);
	
	free(top_received_line);
	free(bottom_received_line);
}

/* Function that resets the two border lines. */
void resetOutsideBorders() {
	int section_size = WORLD_SIZE*sizeof(world_pos);
	if(processor_id != MASTER){
		memset(top_changed_line, 0, section_size);
	}
	if(processor_id != num_processors-1){
		memset(bottom_changed_line, 0, section_size);
	}
}

/* Function that processes the two sub-generations of each generation, dealing with the communication among processes. */
void playGen() {
	MPI_Request request1;
	MPI_Request request2;
	// Before generation, cleans starving animals
	int i, j;
	for (i = 0; i < section_lines; i++) {
		for (j = 0; j < WORLD_SIZE; j++) {
			if (isStarving(&new_world_section[i][j])) {
				clean(&new_world_section[i][j]);
			}
		}
	}
	// Must keep consistency between worlds
	sendInsideBorders(&request1,&request2);
	receiveOutsideBorders();
	if(processor_id != MASTER){
		MPI_Wait(&request1, MPI_STATUS_IGNORE);
	}
	if(processor_id != num_processors-1){
		MPI_Wait(&request2, MPI_STATUS_IGNORE);
	}
	copyWorld();

	// Red sub-generation
	for (i = 0; i < section_lines; i++) {
		for (j = (i % 2)^start_with_black; j < WORLD_SIZE; j+=2) {
			updatePos(i, j);
		}
	}

	// Must keep consistency between worlds
	sendOutsideBorders(&request1,&request2);
	receiveInsideBorders();
	if(processor_id != MASTER){
		MPI_Wait(&request1, MPI_STATUS_IGNORE);
	}
	if(processor_id != num_processors-1){
		MPI_Wait(&request2, MPI_STATUS_IGNORE);
	}
	resetOutsideBorders();

	sendInsideBorders(&request1,&request2);
	receiveOutsideBorders();
	if(processor_id != MASTER){
		MPI_Wait(&request1, MPI_STATUS_IGNORE);
	}
	if(processor_id != num_processors-1){
		MPI_Wait(&request2, MPI_STATUS_IGNORE);
	}
	copyWorld();
	// Black sub-generation
	for (i = 0; i < section_lines; i++) {
		for (j = (!(i % 2))^start_with_black; j < WORLD_SIZE; j+=2) {
			updatePos(i, j);
		}
	}

	sendOutsideBorders(&request1,&request2);
	receiveInsideBorders();
	if(processor_id != MASTER){
		MPI_Wait(&request1, MPI_STATUS_IGNORE);
	}
	if(processor_id != num_processors-1){
		MPI_Wait(&request2, MPI_STATUS_IGNORE);
	}
	resetOutsideBorders();
	// After generation, increase breeding_period to the animals
	// that moved
	for (i = 0; i < section_lines; i++) {
		for (j = 0; j < WORLD_SIZE; j++) {
			if (new_world_section[i][j].has_moved) {
				new_world_section[i][j].breeding_period++;
				new_world_section[i][j].has_moved = FALSE;
			}
		}
	}
}

/* Function main, runs all the generations and prints the whole world. */
int main(int argc, char **argv) {
	if (argc < NUM_ARGUMENTS) {
		fprintf(stderr, "Not enough arguments...\n");
		exit(EXIT_FAILURE);
	}

	MPI_Init (&argc, &argv);

	MPI_Comm_rank (MPI_COMM_WORLD, &processor_id);
	MPI_Comm_size (MPI_COMM_WORLD, &num_processors);

	FILE *input = fopen(argv[1], "r");
	if (input == NULL) {
		fprintf(stderr, "File %s not found...\n", argv[1]);
		MPI_Finalize();
		exit(EXIT_FAILURE);
	}

		
	if (processor_id == MASTER){
		master_init(input, argv);
	} else {
		proc_init(input, argv);
	}
	fclose(input);

	MPI_Barrier (MPI_COMM_WORLD);

	double start = MPI_Wtime();
	int gen;

	for (gen = 0; gen < NUM_GENERATIONS; gen++) {
		playGen();
	}

	double end = MPI_Wtime();
	printf("process %2d took %f\n", processor_id, end - start);

	MPI_Barrier (MPI_COMM_WORLD);
	if (processor_id != MASTER){
		proc_final();
	}

	//MPI_Barrier (MPI_COMM_WORLD);
	if (processor_id == MASTER)	{
		printWorld();
	}
//	freeAll();
	MPI_Finalize();
	return 0;
}
