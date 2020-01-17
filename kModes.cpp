%%cu

/* 
 Juan Camilo Lozano Mejia 1233898705
 David Santiago Gantiva Castro 1032494203
*/
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <time.h>
#include <bits/stdc++.h> 
#include <omp.h>
#include <stdio.h>
#include <sstream> 
// For the CUDA runtime routines (prefixed with "cuda_")
#include <cuda_runtime.h>
#include <memory>

#define KCLUSTERS 10
#define ITERATIONS 50
int limitLoop = 1;
int limit = 1000000;
int matrixRows = limit * limitLoop;
int matrixColumns = 32;
//matrix for input data
int *arr = (int *)malloc(matrixRows* matrixColumns* sizeof(int *)); 
int *d_arr; 
int *cudaModes = (int *)malloc(KCLUSTERS* matrixColumns* sizeof(int *));
int *d_cudaModes;


/************* add vector ******************************************************/

#if CUDART_VERSION < 5000

// CUDA-C includes
#include <cuda.h>

// This function wraps the CUDA Driver API into a template function
template <class T>
inline void getCudaAttribute(T *attribute, CUdevice_attribute device_attribute,
                             int device) {
  CUresult error = cuDeviceGetAttribute(attribute, device_attribute, device);

  if (CUDA_SUCCESS != error) {
    fprintf(
        stderr,
        "cuSafeCallNoSync() Driver API error = %04d from file <%s>, line %i.\n",
        error, __FILE__, __LINE__);

    exit(EXIT_FAILURE);
  }
}

#endif /* CUDART_VERSION < 5000 */

int getSPcores(cudaDeviceProp devProp)
{  
    int cores = 0;
    int mp = devProp.multiProcessorCount;
    switch (devProp.major){
     case 2: // Fermi
     printf("Fermi architecture");
      if (devProp.minor == 1) cores = mp * 48;
      else cores = mp * 32;
      break;
     case 3: // Kepler
     printf("Kepler architecture");
      cores = mp * 192;
      break;
     case 5: // Maxwell
     printf("Maxwell architecture");
      cores = mp * 128;
      break;
     case 6: // Pascal
     printf("Pascal architecture");
      if (devProp.minor == 1) cores = mp * 128;
      else if (devProp.minor == 0) cores = mp * 64;
      else printf("Unknown device type\n");
      break;
     case 7: // Volta and Turing
     printf("Volta and Turing");
      if ((devProp.minor == 0) || (devProp.minor == 5)) cores = mp * 64;
      else printf("Unknown device type\n");
      break;
     default:
      printf("Unknown device type\n"); 
      break;
      }
    return cores;
}

using namespace std;

int thread_count;
vector<vector <string> > Modes; 




void convertStrtoArr(string str,int *result,int index) 
{ 
    // get length of string str 
    int str_length = str.length(); 
    int arr[str_length]; 
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


__global__
void splitParallel(int *data,int matrixRows,int matrixColumns, int totalThreads, int* modes, int clustersSize ){
    
    int initIteration, endIteration;
    int index = (blockDim.x * blockIdx.x) + threadIdx.x;
    initIteration = (matrixRows/totalThreads) * index;
    endIteration = initIteration + (matrixRows/totalThreads) - 1;
    //printf(" esta en el hilo %i con iteracion inicial %i y final %i\n",index, initIteration,endIteration);
    int pos = 0;
    int maxAccerts = 0;
    int accerts = 0;

    for(int i = initIteration; i<endIteration;i++){
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
    }
}

__global__  
void newModes(int *data,int matrixRows,int matrixColumns, int totalThreads, int* modes, int clustersSize ){
    /*printf("Modas mal:\n");
      for(int i=0;i<KCLUSTERS;i++){
          for(int j= 0; j<matrixColumns;j++){  
            printf("%i ",*(modes+i*matrixColumns+j));
          }
          printf("\n");
      }*/
    __shared__ int repeticiones[32][34];
    if(threadIdx.x == 0){
        for(int i =0;i<32;i++){
            for(int j=0;j<34;j++){
                repeticiones[i][j] = -1;
            }
        }
    };
    //printf("entro\n");
    __syncthreads();
    int initIteration, endIteration;
    int index = threadIdx.x;
    initIteration = (matrixRows/blockDim.x) * index;
    endIteration = initIteration + (matrixRows/blockDim.x) - 1;
    //printf(" esta en el hilo %i con iteracion inicial %i y final %i\n",index, initIteration,endIteration);
    for(int i = initIteration; i<endIteration;i++){
        //printf("entro\n");
        //printf("para el dato %i se compararan %i y %i\n",i,*(data + i*matrixColumns+ 31),blockIdx.x);
        if(*(data + i*matrixColumns+ 31) == blockIdx.x){
            //printf("entro\n");
            for (int k = 0; k < matrixColumns-1; k++){
                //printf("%i\n",*(data + i*matrixColumns+ k));
                //printf("entro\n");
                if(repeticiones[k][*(data + i*matrixColumns+ k)] == -1){
                    //printf("entro\n");
                    repeticiones[k][*(data + i*matrixColumns+ k)] = 1;
                    //printf("para el dato %i y atributo % i van %i\n",i,k,repeticiones[k][*(data + i*matrixColumns+ k)]);
                }else{
                    repeticiones[k][*(data + i*matrixColumns+ k)] += 1;
                    //printf("para el dato %i y atributo % i en la opcion %i van %i\n",i,k,*(data + i*matrixColumns+ k),repeticiones[k][*(data + i*matrixRows+ k)]);
                }
            }
        }
    }
    __syncthreads();
    if(threadIdx.x == 0){
        for(int j = 0; j< 31;j++){
            int aux = -1;
            int aux2 = 0;
            for(int k = 0; k< 34;k++){
                 if(aux < repeticiones[j][k]){
                     aux = repeticiones[j][k];
                     aux2 = k;
                     //printf("entro\n");
                 }
            } 
            //printf("%i\n",aux2);
            *(modes + blockIdx.x*matrixColumns+ j) = aux2;
        }
    }
    //printf("entro MI PERRO\n");
}


int main(int arg, char* argv[]) {
    
    // CUDA DEVICE INIT
    int deviceCount = 0;
    cudaError_t error_id = cudaGetDeviceCount(&deviceCount);
    printf("devices that support cuda: %d\n",deviceCount);
    int dev, driverVersion = 0, runtimeVersion = 0;
    for (dev = 0; dev < deviceCount; ++dev){
      cudaSetDevice(dev);
      cudaDeviceProp deviceProp;
      cudaGetDeviceProperties(&deviceProp, dev);
      printf(" device cores %d",getSPcores(deviceProp));

    

      cout<<"\n............................"<<endl;

      int iterations = ITERATIONS;
      string line;
      
      iterations = 2;
      thread_count = 5;
      
      
      if ( arr == NULL){
        fprintf(stderr, "Failed to allocate host vectors!\n");
        exit(EXIT_FAILURE);
      }
          

      cout<<"se ejecuta "<<iterations<<" con "<<thread_count<<" hilos"<<endl;
      //lectura de los datos
      int indexData = 0;
      for(int i=0; i<limitLoop;i++){ 
        bool omitFirst = false;
        int count = 0;
        ifstream myfile("/content/drive/My Drive/Semestre 10/Data mining/Proyecto/kModesData.csv");
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
      srand (time(NULL)); // pone las semillas en base al tiempo actual para la generacion de los numeros aleatorios
      int randI; // posicion aleatoria
      for(int i=0;i<KCLUSTERS;i++){
        randI = (rand() % matrixRows) + 1;
        for(int j=0; j<matrixColumns;j++){
          *(cudaModes+i*matrixColumns+j) = *(arr+randI*matrixColumns+j);
        }
      }
      
      cudaError_t err = cudaSuccess;

      err = cudaMalloc((void **)&d_arr,matrixRows* matrixColumns* sizeof(int *) );
      if (err != cudaSuccess){
          fprintf(stderr, "Failed to allocate device vector C (error code %s)!\n", cudaGetErrorString(cudaGetLastError()));
          exit(EXIT_FAILURE);
      }
      
      err = cudaMemcpy(d_arr, arr, matrixRows* matrixColumns* sizeof(int *), cudaMemcpyHostToDevice);
      if (err != cudaSuccess){
          fprintf(stderr, "Failed to copy vector C from device to host (error code %s)!\n", cudaGetErrorString(cudaGetLastError()));
          exit(EXIT_FAILURE);
      }

      err = cudaMalloc((void **)&d_cudaModes,KCLUSTERS* matrixColumns* sizeof(int *));
      if (err != cudaSuccess){
          fprintf(stderr, "Failed to allocate device vector C (error code %s)!\n", cudaGetErrorString(cudaGetLastError()));
          exit(EXIT_FAILURE);
      }

      err = cudaMemcpy(d_cudaModes, cudaModes, KCLUSTERS* matrixColumns* sizeof(int *), cudaMemcpyHostToDevice);
      if (err != cudaSuccess){
          fprintf(stderr, "Failed to copy vector C from device to host (error code %s)!\n", cudaGetErrorString(cudaGetLastError()));
          exit(EXIT_FAILURE);
      }


      //CALCULO DE LOS CLUSTERS #######################################
      // Lanzar KERNEL
      int blocksPerGrid = 26;
      int threadsPerBlock = 96;
      int totalThreads = blocksPerGrid * threadsPerBlock;

      cout<<"Modas antes:"<<endl;
      for(int i =0 ;i<32;i++)
        cout<<*(arr+(matrixRows - 1)*matrixColumns+i)<<"-";
      cout<<endl;
      for(int i =0 ;i<32;i++)
        cout<<*(arr+(matrixRows/2 - 1)*matrixColumns+i)<<"-";
      cout<<endl;
      for(int i =0 ;i<32;i++)
        cout<<*(arr+(matrixRows/2)*matrixColumns+i)<<"-";
      cout<<endl;
      for(int i =0 ;i<32;i++)
        cout<<*(cudaModes+0*matrixColumns+i)<<"-";
      cout<<endl;
      splitParallel<<<blocksPerGrid, threadsPerBlock>>>(d_arr,matrixRows,matrixColumns,totalThreads,d_cudaModes,KCLUSTERS);
      cout<<"SPLIT PARALLEL"<<endl;
      newModes<<<1, threadsPerBlock>>>(d_arr,matrixRows,matrixColumns,totalThreads,d_cudaModes,KCLUSTERS);
      err = cudaMemcpy(arr, d_arr , matrixRows* matrixColumns* sizeof(int *), cudaMemcpyDeviceToHost);
      if (err != cudaSuccess){
          fprintf(stderr, "Failed to copy vector C from device to host (error code %s)!\n", cudaGetErrorString(cudaGetLastError()));
          exit(EXIT_FAILURE);
      }

      err = cudaMemcpy(cudaModes, d_cudaModes , KCLUSTERS* matrixColumns* sizeof(int *), cudaMemcpyDeviceToHost);
      if (err != cudaSuccess){
          fprintf(stderr, "Failed to copy modes from device to host (error code %s)!\n", cudaGetErrorString(cudaGetLastError()));
          exit(EXIT_FAILURE);
      }

      err = cudaFree(d_arr);
      if (err != cudaSuccess){
          fprintf(stderr, "Failed to free device vector C (error code %s)!\n", cudaGetErrorString(cudaGetLastError()));
          exit(EXIT_FAILURE);
      }

      err = cudaFree(d_cudaModes);
      if (err != cudaSuccess){
          fprintf(stderr, "Failed to free device vector C (error code %s)!\n", cudaGetErrorString(cudaGetLastError()));
          exit(EXIT_FAILURE);
      }

      err = cudaGetLastError();
      if (err != cudaSuccess){
          fprintf(stderr, "Failed to launch vectorAdd kernel (error code %s)!\n", cudaGetErrorString(err));
          exit(EXIT_FAILURE);
      }
      

      cout<<"Primeros 25 Datos:"<<endl;
      for(int i=0;i<25;i++){
          for(int j= 0; j<matrixColumns;j++){  
            cout<<*(arr+i*matrixColumns+j)<<" ";
          }
          cout<<"\n";
      }
      
      
  
      cout<<"Resultado:"<<endl;
      for(int i =0 ;i<32;i++)
        cout<<*(arr+(matrixRows - 1)*matrixColumns+i)<<"-";
      cout<<endl;
      for(int i =0 ;i<32;i++)
        cout<<*(arr+(matrixRows/2 - 1)*matrixColumns+i)<<"-";
      cout<<endl;
      for(int i =0 ;i<32;i++)
        cout<<*(arr+(matrixRows/2)*matrixColumns+i)<<"-";
      cout<<endl;
      for(int i =0 ;i<32;i++)
        cout<<*(cudaModes+0*matrixColumns+i)<<"-";
      cout<<endl;


      cout<<"\nTermino, Iteraciones = "<<iterations<<" hilos = "<<thread_count<<endl;
      cout<<blocksPerGrid<<"  "<<threadsPerBlock<<" "<<matrixRows<<endl;
    }
}