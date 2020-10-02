#include <iostream>
#ifdef HCL_ENABLE_THALLIUM_TCP
#include <thallium.hpp>
#include <mpi.h>
#include <zconf.h>

namespace tl = thallium;

void hello(const tl::request& req) {
    std::cout << "Hello World!" << std::endl;
}

int main(int argc, char** argv) {
    int provided;
    MPI_Init_thread(&argc,&argv, MPI_THREAD_MULTIPLE, &provided);
    if (provided < MPI_THREAD_MULTIPLE) {
        printf("Didn't receive appropriate MPI threading specification\n");
        exit(EXIT_FAILURE);
    }
    int comm_size,my_rank;
    MPI_Comm_size(MPI_COMM_WORLD,&comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD,&my_rank);
    if(my_rank == 0){
        tl::engine myEngine("tcp://127.0.0.1:1234", THALLIUM_SERVER_MODE);
        myEngine.define("hello", hello).disable_response();
    }
    sleep(5);
    if(my_rank !=0){
        tl::engine myEngine("tcp", THALLIUM_CLIENT_MODE);
        tl::remote_procedure hello = myEngine.define("hello").disable_response();
        tl::endpoint server = myEngine.lookup("tcp://127.0.0.1:1234");
        hello.on(server)();
    }
    MPI_Finalize();
    return 0;
}
#else
int main(int argc, char** argv) {
    return 0;
}
#endif

