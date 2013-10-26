#!/bin/sh

# Generate random tests for the parallel versions
# assuming the serial version is correct.
# Each test corresponds to a file .header and a .in.
# The .header contains the variables given to the program when it's executed.
# The .in contains the map.

# Environment variables
SERIAL_PROGRAM="bin/wolves-squirrels-serial"
OUTPUT_DIR="test/generated"

# Simulator variables
NUMBER_TESTS=1
MAX_WORLD_SIZE=10
MAX_WOLF_BREEDING_PERIOD=50
MAX_SQUIRREL_BREEDING_PERIOD=50
MAX_WOLF_STARVATION_PERIOD=50
MAX_NUMBER_GENERATIONS=1000

if [ -x $SERIAL_PROGRAM ]
then
	rm -rf $OUTPUT_DIR
	mkdir -p $OUTPUT_DIR

	for (( count = 0; count < $NUMBER_TESTS; count++ ));
	do
		file="${OUTPUT_DIR}/${count}"

		# generating .in
		pieces=('$' 'w' 's' 'i' 't' 'e')
		input="${file}.in"

		world_size=$RANDOM
		let "world_size %= $MAX_WORLD_SIZE"
		echo "$world_size" >> $input

		for (( i = 0; i < $world_size; i++ )); do
			for (( j = 0; j < $world_size; j++ )); do
				pos=$RANDOM
				let "pos %= ${#pieces[@]}"
				if [ ${pieces[${pos}]} != 'e' ]
				then
					echo "$i $j ${pieces[${pos}]}" >> $input
				fi			
			done
		done

		# generating .header
		header="${file}.header"

		wolf_breeding_period=$RANDOM
		squirrel_breeding_period=$RANDOM
		wolf_starvation_period=$RANDOM
		number_generations=$RANDOM
		let "wolf_breeding_period %= $MAX_WOLF_BREEDING_PERIOD"
		let "squirrel_breeding_period %= $MAX_SQUIRREL_BREEDING_PERIOD"
		let "wolf_starvation_period %= $MAX_WOLF_STARVATION_PERIOD"
		let "number_generations %= $MAX_NUMBER_GENERATIONS"

		printf "%d %d %d %d\n" $wolf_breeding_period $squirrel_breeding_period \
				$wolf_starvation_period $number_generations >> $header

		# generating .out
		./$SERIAL_PROGRAM $input $wolf_breeding_period $squirrel_breeding_period \
				$wolf_starvation_period $number_generations > "${file}.out"
	done
else
	echo "$SERIAL_PROGRAM doesn't exist."
fi	
