/* 
 Juan Camilo Lozano Mejia 1233898705
 David Santiago Gantiva Castro 1032494203

VERSION SECUENCIAL YA FUNCIONAL

*/
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <time.h>
#include <bits/stdc++.h> 
#include <stdio.h>
#include <sstream> 
#include <mpi.h>

#define KCLUSTERS 4//360
#define ITERATIONS 1
int limitLoop = 1;//6;
int limit = 1000;//1000000;
int matrixRows = limit * limitLoop;
int matrixColumns = 32;
//matrix for input data
int *arr = (int *)malloc(matrixRows* matrixColumns* sizeof(int *)); 
int *recvModes = (int *)malloc(matrixRows * sizeof(int *));
int *mpi_arr = (int *)malloc(matrixRows* matrixColumns* sizeof(int *));
int *cudaModes = (int *)malloc(KCLUSTERS* matrixColumns* sizeof(int *));


using namespace std;

int thread_count;
vector<vector <string> > Modes; 




void convertStrtoArr(string str,int *result,int index) 
{ 
    // get length of string str 
    int j = 0, i, sum = 0; 
    // Traverse the string 
    string tmp = "";
    int pos = 0;
    for (i = 0; str[i] != '\0'; i++) { 
        if (str[i] == ',') { 
          if(tmp == "20182"){
              tmp = '1';
          }else if(tmp == "20172"){
              tmp = '2';
          }
          stringstream geek(tmp); 
          int x = 0; 
          geek >> x; 
          *(result +index*matrixColumns +pos) = x;
          pos++;
          tmp ="";
        } 
        else { 
            tmp+= str[i];
        } 
    }
    //agregar el ultimo elemento
    stringstream geek(tmp); 
    int x = 0; 
    geek >> x; 
     *(result +index*matrixColumns +pos) = x;
    //como no tiene clase se le asigna un -1
     *(result +index*matrixColumns +pos+1) = -1;
    
} 


//asignacion de la moda
void splitParallel(int *data, int *tmp_mode,int matrixRows,int matrixColumns, int initIteration, int endIteration, int* modes, int clustersSize ){
    cout << "splitParallel "<<initIteration<<" "<<endIteration<<endl;
	int index = initIteration;

    //printf(" esta en el hilo %i con iteracion inicial %i y final %i\n",index, initIteration,endIteration);
    int pos = 0;
    int maxAccerts = 0;
    int accerts = 0;
    for(int i = initIteration; i< endIteration;i++){
      pos = 0;
      maxAccerts = 0;
      accerts = 0;
      for(int j = 0; j < clustersSize;j++){
        accerts = 0;
        for (int k = 0; k < matrixColumns-1; k++){
            if(*(modes + j*matrixColumns+ k) == *(data + i*matrixColumns+ k)){
              accerts++;
            }
        }
        if(accerts >= maxAccerts){
            maxAccerts = accerts;
            pos = j;
        } 
      }
      *(data + i*matrixColumns+ 31) = pos;
	  *(tmp_mode + (i%(endIteration-initIteration))) = pos;
    }

}

void newModes(int *data, int *frecuecny,int matrixRows,int matrixColumns, int totalThreads, int* modes, int index, int end, int clusters){
	cout<<"entra en new mode"<<endl;
	int repeticiones[clusters][32][34];
	for(int k=0;k < totalThreads;k++){
		for(int i =0;i<32;i++){
			for(int j=0;j<34;j++){
				repeticiones[k][i][j] = -1;
				*(frecuecny+k*32*34+i*32+j) = -1;
			}
		}
	}
	cout<<"termina asignacion en new mode"<<endl;
    
    //printf("entro\n");
    int initIteration = index, endIteration = end;
    //printf(" esta en el hilo %i con iteracion inicial %i y final %i\n",index, initIteration,endIteration);
    for(int i = initIteration; i<endIteration;i++){
		int clusterIndex = *(data + i*matrixColumns+ matrixColumns-1);
		for (int k = 0; k < matrixColumns-1; k++){
			int value = *(data + i*matrixColumns+ k);
			if(repeticiones[clusterIndex][k][value] == -1){
				//printf("entro\n");
				repeticiones[clusterIndex][k][value] = 1;
				*(frecuecny+clusterIndex*32*34+k*32+value) = 1;
				//printf("para el dato %i y atributo % i van %i\n",i,k,repeticiones[k][*(data + i*matrixColumns+ k)]);
			}else{
				repeticiones[clusterIndex][k][value] += 1;
				*(frecuecny+clusterIndex*32*34+k*32+value) += 1;
				//printf("para el dato %i y atributo % i en la opcion %i van %i\n",i,k,*(data + i*matrixColumns+ k),repeticiones[k][*(data + i*matrixRows+ k)]);
			}
		}
    }
    //printf("entro MI PERRO\n");
	/*
	for(int k=0;k<clusters;k++){
		for(int i =0;i<32;i++){
			int aux = -1;
            int aux2 = 0;
            for(int j = 0; j< 34;j++){
                 if(aux < repeticiones[k][i][j]){
                     aux = repeticiones[k][i][j];
                     aux2 = k;
                     //printf("entro\n");
                 }
            }
			*(modes + k*matrixColumns+ i) = aux2;
		}
	}
	*/
}


int main(int argc, char* argv[]) {
    	
	int rank, size;
	MPI_Init( &argc, &argv );
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	cout<<"kio padre world from process "<<rank<<" of " << size<<endl;
	
    
	cout<<"\n............................"<<endl;

	int iterations = ITERATIONS;
	string line;
	iterations = 2;
	thread_count = 5;
	
	if ( arr == NULL){
		fprintf(stderr, "Failed to allocate host vectors!\n");
		exit(EXIT_FAILURE);
	}
		
	//lectura de los datos
	int indexData = 0;
	for(int i=0; i<limitLoop;i++){ 
		bool omitFirst = false;
		int count = 0;
		ifstream myfile("./kModesData.csv");
		if(myfile.is_open()){
			while (getline(myfile,line))
			{
				if(omitFirst){
					convertStrtoArr(line,arr,indexData);
					indexData++;
					count++;
					if(count == limit)
					break;
				}else{
					omitFirst = true;
				}
			}
		}
		myfile.seekg(0);
		myfile.clear();
	}

      
	//Se termino la lectura de los datos
	//Creacion de las modas
	if(rank == 0){
		srand (time(NULL)); // pone las semillas en base al tiempo actual para la generacion de los numeros aleatorios
		int randI; // posicion aleatoria
		for(int i=0;i<KCLUSTERS;i++){
			randI = (rand() % matrixRows) + 1;
			for(int j=0; j<matrixColumns;j++){
				*(cudaModes+i*matrixColumns+j) = *(arr+randI*matrixColumns+j);
			}
		}
	}
	int modesSize = KCLUSTERS* matrixColumns;
	MPI_Bcast( cudaModes, modesSize, MPI_INT, 0, MPI_COMM_WORLD);
	
	/*
	*/
	
	if(rank == 0){
		cout<<"Modas antes:"<<endl;
		for(int j= 0; j< KCLUSTERS; j++ ){
			for(int i =0 ;i<32;i++)
				cout<<*(cudaModes+j*matrixColumns+i)<<"-";
			cout<<endl;
		}
	}
	

	int totalThreads = size;
	int initIteration = (matrixRows/size)*rank;
	int endIteration = (matrixRows/size)*(rank+1);
	int *tmpModes = (int *)malloc((endIteration- initIteration)*sizeof(int *));
	int *frecuency = (int *)malloc(KCLUSTERS* 32*34* sizeof(int *)); //[KCLUSTERS][32][34];
	int *recvFrecuency = (int *)malloc(KCLUSTERS* 32*34*size*sizeof(int *));
	int *(totalFrecuency) = (int *)malloc(KCLUSTERS* 32*34*sizeof(int *));
	cout<<"Matrix rows: "<<matrixRows<<endl;
	//cout<<"Size: "<<size<<endl;
	cout<<"El proceso "<<rank<<": inicia en "<<initIteration<<" y termina en "<<endIteration<<endl;
	
	splitParallel(arr,tmpModes,matrixRows,matrixColumns,initIteration,endIteration,cudaModes,KCLUSTERS);
	newModes(arr,frecuency,matrixRows,matrixColumns,totalThreads,cudaModes,initIteration,endIteration,KCLUSTERS);	
	cout<< "Obtener las modas"<<endl;
	MPI_Gather(frecuency, KCLUSTERS* 32*34, MPI_INT, recvFrecuency, KCLUSTERS* 32*34, MPI_INT, 0, MPI_COMM_WORLD);
	if(rank == 0){
		for (int i = 0; i < KCLUSTERS* 32*34; i++)
		{
			*(totalFrecuency +i ) = 0;
		}
		
		for(int task=0;task<size;task++){
			for(int i= 0; i<KCLUSTERS* 32*34; i++){
				*(totalFrecuency +i ) += *(recvFrecuency+task*KCLUSTERS* 32*34+i);
			}
		}

		for(int k=0;k<KCLUSTERS;k++){
			for(int i =0;i<32;i++){
				int aux = -1;
				int aux2 = 0;
				for(int j = 0; j< 34;j++){
					if(aux < *(totalFrecuency+ k*32*34 + 32*i +j)){
						aux = *(totalFrecuency+ k*32*34 + 32*i +j);
						aux2 = k;
						//printf("entro\n");
					}
				}
				*(modes + k*matrixColumns+ i) = aux2;
			}
		}

		cout<<"Modas nuevas:"<<endl;
		for(int j= 0; j< KCLUSTERS; j++ ){
			for(int i =0 ;i<32;i++)
				cout<<*(cudaModes+j*matrixColumns+i)<<"-";
			cout<<endl;
		}
	}

//PRINTS
/*
	cout<<"Resultado:"<<endl;
	
	for(int j= 0; j< KCLUSTERS; j++ ){
		for(int i =0 ;i<32;i++)
		cout<<*(arr+j*matrixColumns+i)<<"-";
		cout<<endl;
	}

	cout<<"Modas finales"<<endl;

	for(int j= 0; j< KCLUSTERS; j++ ){
		for(int i =0 ;i<32;i++)
		cout<<*(cudaModes+j*matrixColumns+i)<<"-";
		cout<<endl;
	}
*/
	MPI_Finalize();
	cout<<"\nTermino, Iteraciones = "<<iterations<<" hilos = "<<thread_count<<endl;
	cout<<"temino"<<endl;
}