#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

int MAX_ADDAND;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct add_param {   
  int a;           
  int b; 
  int result;     
};

/// @brief perform thread function (add a & b in arg together)
/// @param arg struct add_param containing addition parameters
/// @return struct add_param that has add_param.result filled in
void* thread_do( void* arg)
{
    struct add_param* curr_param = (struct add_param*) arg;

    printf("Thread %ld running add() with [%d + %d]\n", pthread_self(), curr_param->a, curr_param->b);

    pthread_mutex_lock( &mutex );
    curr_param->result = curr_param->a + curr_param->b;
    pthread_mutex_unlock( &mutex );
    
    return (void*) curr_param;
}

int main(int argc, char* argv[])
{
    if (argc != 2) {
        printf("wrong input arguments: ./a.out [MAX_ADDAND]\n");
        return EXIT_FAILURE;
    }

    setvbuf( stdout, NULL, _IONBF, 0 );

    MAX_ADDAND = atoi(argv[1]);

    // create array for all children
    pthread_t children[(MAX_ADDAND - 1) * MAX_ADDAND - 1];
    int child_num = 0;

    for (int a = 1; a < MAX_ADDAND; a++)
    {
        for (int b = 1; b < MAX_ADDAND + 1; b++)
        {
            struct add_param* curr_param = calloc(1, sizeof(struct add_param));
            curr_param->a = a;
            curr_param->b = b;

            pthread_t tid;
            printf("Main starting thread add() for [%d + %d]\n", curr_param->a, curr_param->b);
            int val = pthread_create(&tid, NULL, thread_do, (void *)curr_param);
            if (val < 0)
            {
                return -1;
            }
            else
            {
                children[child_num] = tid;
                child_num++;
            }
            
        }
    }

    // join all threads
    for (int i = 0; i < (MAX_ADDAND - 1) * MAX_ADDAND; i++)
    {
        struct add_param *ret_val;
        int val = pthread_join(children[i], (void**)&ret_val);
        printf("In main, collecting thread %ld computed [%d + %d] = %d\n", children[i], ((struct add_param*)ret_val)->a, ((struct add_param*)ret_val)->b, ((struct add_param*)ret_val)->result);

        free(ret_val);
    }

    return EXIT_SUCCESS;
}