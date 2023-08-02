gcc -Werror -Wall project.c -lm -g
python3 clear_output.py  
valgrind --leak-check=full ./a.out 3 1 1024 0.001 3000 4 0.75 256 > log.txt
python3 verify_output.py 0 

python3 clear_output.py  
valgrind --leak-check=full ./a.out 8 6 512 0.001 3000 6 0.9 128 > log1.txt
python3 verify_output.py 1

python3 clear_output.py 
valgrind --leak-check=full ./a.out 16 2 256 0.001 3000 4 0.5 32 > log2.txt
python3 verify_output.py 2
