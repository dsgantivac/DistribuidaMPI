clear
mpic++ -o mpi_test mpiSecuencial.cpp
#mpirun -np 1 -mca btl ^openib mpi_test
time mpirun -np 1 --host localhost -mca btl ^openib mpi_test
