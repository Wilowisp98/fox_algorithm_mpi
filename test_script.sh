#!/bin/bash
#!/bin/bash

MPI_SCRIPT="read_matrix_speedup.c"
MPI_EXECUTABLE="./a.out"

# Input file
INPUT_DIR="./inputs"

# Output file
OUTPUT_DIR="./outputs"

start_time=$(date +"%s")

# Compile the C program
mpicc "$MPI_SCRIPT" -o "$MPI_EXECUTABLE" -lm


# Number of process to iterate through
numprocs=("1" "4" "9" "16" "25" "36")

# Iterate through the array
for i in "${numprocs[@]}"
do
    echo "############################################################"
    echo ""
    echo "                 TRYING WITH '$i' PROCESSES"
    echo ""
    echo "############################################################"
    for file in "$INPUT_DIR"/*
    do
        echo "mpirun --hostfile hostfile -n $i \"$MPI_EXECUTABLE\" < \"$file\" > \"$OUTPUT_DIR/output_$(basename $file)-np_$i.txt\" 2>&1"
        start_time=$(date +"%s")
        mpirun --hostfile hostfile -n $i "$MPI_EXECUTABLE" < "$file" > "$OUTPUT_DIR/output_$(basename $file)-np_$i.txt" 2>&1
        echo "Elapsed Time: $(($(date +"%s") - start_time)) seconds"
    done
done


# # Run the MPI program using mpirun
# mpirun --hostfile hostfile -n 1 "$MPI_EXECUTABLE" < "$INPUT_DIR/input6" > "$OUTPUT_DIR/output6.txt" 2>&1
# echo "Elapsed Time: $(($(date +"%s") - start_time)) seconds"

# start_time=$(date +"%s")
# echo "mpirun -n 1 \"$MPI_EXECUTABLE\" < \"$INPUT_DIR/input300\" > \"$OUTPUT_DIR/output300.txt\" 2>&1"
# mpirun --hostfile hostfile -n 1 "$MPI_EXECUTABLE" < "$INPUT_DIR/input300" > "$OUTPUT_DIR/output300.txt" 2>&1
# echo "Elapsed Time: $(($(date +"%s") - start_time)) seconds"

# start_time=$(date +"%s")
# echo "mpirun -n 1 \"$MPI_EXECUTABLE\" < \"$INPUT_DIR/input600\" > \"$OUTPUT_DIR/output600.txt\" 2>&1"
# mpirun --hostfile hostfile -n 1 "$MPI_EXECUTABLE" < "$INPUT_DIR/input600" > "$OUTPUT_DIR/output600.txt" 2>&1
# echo "Elapsed Time: $(($(date +"%s") - start_time)) seconds"

# start_time=$(date +"%s")
# echo "mpirun -n 1 \"$MPI_EXECUTABLE\" < \"$INPUT_DIR/input900\" > \"$OUTPUT_DIR/output900.txt\" 2>&1"
# mpirun --hostfile hostfile -n 1 "$MPI_EXECUTABLE" < "$INPUT_DIR/input900" > "$OUTPUT_DIR/output900.txt" 2>&1
# echo "Elapsed Time: $(($(date +"%s") - start_time)) seconds"

# start_time=$(date +"%s")
# echo "mpirun -n 1 \"$MPI_EXECUTABLE\" < \"$INPUT_DIR/input1200\" > \"$OUTPUT_DIR/output1200.txt\" 2>&1"
# mpirun --hostfile hostfile -n 1 "$MPI_EXECUTABLE" < "$INPUT_DIR/input1200" > "$OUTPUT_DIR/output1200.txt" 2>&1
# echo "Elapsed Time: $(($(date +"%s") - start_time)) seconds"

# # Print a message indicating the completion of the run
# echo "Completed run. Output saved to $OUTPUT_DIR"
