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

int BREEDING_LEVEL;
int STARVING_LEVEL;
int WORLD_SIZE;
world WORLD;

/* returns 1 if animal is breeding, 0 otherwise */
int isBreeding(position pos);

/* returns 1 if animal is starving, 0 otherwise */
int isStarving(position pos);

int numberOfPosition(position pos) {
	return pos.row * WORLD_SIZE + pos.column;
}

/* returns 1 if can move to theb given position*/
int canMoveTo(position currentPos, position possiblePos){
	
	int currentCell = WORLD[currentPos.row][currentPos.column].type;
	int possibleCell = WORLD[possiblePos.row][possiblePos.column].type;
	
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

/* */
int updateCell(position pos){

	position destinationPosition = getDestination(pos); // posicao para onde se vai mover
	
	if((pos.row == destinationPosition.row) && (pos.column == destinationPosition.column))
	// se posicao retornada igual a' actual bubai
	
	// se nao vai alterar world aux na posicao destination 
	// inc breeding period e testar se e' igual a BREEDING_LEVEL
	// se sim deixar baby pa trás se nao empty  e reset a breeding period do esquilo que vai pa destination
	
	
	

}

int createWorld(char **input);

int main(int argc, char **argv) {
	return 0;
}
