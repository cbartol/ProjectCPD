#define MAX 100

enum MOVE {
	TOP = 1,
	RIGHT = 2,
	BOTTOM = 3,
	LEFT = 4,
	NONE = 5
}

struct world {
	int type; /* Wolf, Squirrel, etc. */
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

/* returns 1 if animal is breeding, 0 otherwise */
int isBreeding(position pos);

/* returns 1 if animal is starving, 0 otherwise */
int isStarving(position pos);

/* returns a valid move */
int getDestination(position pos);
int updateCell(position pos);
int createWorld(char **input);

int main(int argc, char **argv) {
	return 0;
}
