#define MAX 100

enum move {
	TOP = 0,
	RIGHT = 1,
	BOTTOM = 2,
	LEFT = 3,
	NONE = 4
};

typedef struct {
	int type;
	int breeding_period;
	int starvation_period;
} world;

typedef struct {
	int row;
	int column;
} position;

int BREEDING_LEVEL;
int STARVING_LEVEL;
int WORLD_SIZE;
world WORLD[MAX][MAX];

/* returns 1 if animal is breeding, 0 otherwise */
int isBreeding(position pos);

/* returns 1 if animal is starving, 0 otherwise */
int isStarving(position pos);

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
int createWorld(char **input);

int main(int argc, char **argv) {
	return 0;
}
