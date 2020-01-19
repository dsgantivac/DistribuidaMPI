clear
mpic++ -o mpi_test mpiSecuencial.cpp
#mpirun -np 1 -mca btl ^openib mpi_test
time mpirun -np 1 --host localhost -mca btl ^openib mpi_test
time mpirun -np 4 --host localhost,10.0.0.5,10.0.0.6,10.0.0.7 -mca btl ^openib mpi_test

