#!/bin/sh

# Generate random tests for the parallel versions
# assuming the serial version is correct.
# Each test corresponds to a file .header and a .in.
# The .header contains the variables given to the program when it's executed.
# The .in contains the map.

# Environment variables
SERIAL_PROGRAM = "bin/executable"
OUTPUT_DIR = "test/generated"

# Simulator variables
NUMBER_TESTS 					= 1000
MAX_WORLD_SIZE					= 1000
MAX_WOLF_BREEDING_PERIOD 		= 50
MAX_SQUIRREL_BREEDING_PERIOD 	= 50
MAX_WOLF_STARVATION_PERIOD 		= 50
MAX_NUMBER_GENERATIONS			= 1000

if [ -x $SERIAL_PROGRAM ];
	mkdir -p OUTPUT_DIR

	count = 1
	for $count in {1..$NUMBER_TESTS}
	do
		file = "${OUTPUT_DIR}/${count}"
		rm "${file}.*" 2> /dev/null

		# generating .in
		input = "${file}.in"
		declare -a pieces=('$' 'w' 's' 'i' 't')
		world_size = $RANDOM
		let "$world_size %= $MAX_WORLD_SIZE"

		for (( i = 0; i < $world_size; i++ )); do
			for (( j = 0; j < $world_size; j++ )); do
				pos = $RANDOM
				let "$pos %= ${#pieces[@]}"
				echo "$pieces[$pos]" >> $input				
			done
			echo '\n'
		done

		# generating .header
		header = "${file}.header"

		wolf_breeding_period = $RANDOM
		let "$wolf_breeding_period %= $MAX_WOLF_BREEDING_PERIOD"

		squirrel_breeding_period = $RANDOM
		let "$squirrel_breeding_period %= $MAX_SQUIRREL_BREEDING_PERIOD"

		wolf_starvation_period = $RANDOM
		let "$wolf_starvation_period %= $MAX_WOLF_STARVATION_PERIOD"

		number_generations = $RANDOM
		let "$number_generations %= $MAX_NUMBER_GENERATIONS"

		$wolf_breeding_period >> $header
		$squirrel_breeding_period >> $header
		$wolf_starvation_period >> $header
		$number_generations >> $header

		# generating .out
		./$SERIAL_PROGRAM $wolf_breeding_period $squirrel_breeding_period \
				$wolf_starvation_period $number_generations > "${count}.out"
	done
else;
	echo "$SERIAL_PROGRAM doesn't exist."
fi	
