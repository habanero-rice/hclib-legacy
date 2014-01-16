---
title: HClib Tutorial
layout: hclib
---

# HClib Tutorial

HClib is composed of various constructs to express and control parallelism. Constructs such as `async`, `finish` and `forasync` allow fork-join style structured parallelism. Data-driven futures (DDFs) allow for data-flow like programming with tasks consuming and producing data.

Other features are orthogonal to the programming style. For instance, accumulators can be used to perform reductions safely, Phasers allow for lightweight point-to-point and collective synchronization among tasks.

Next sections describes HClib constructs in more details:

* [Async](#async)
* [Finish](#finish)
* [Forasync](#forasync)
* [Data-Driven Future](#ddf)
* [Accumulators](#accum)
* [Phasers](#phaser)
fin

# Getting Started

Refer to [installation instructions](installation.html) to set up your HClib distribution. Once installed you may have a look at this tutorial along side with the HClib [doxygen](api.html) documentation.

## Starting and shutting down HClib

The HClib runtime can be dynamically turned on and off through the `hclib_init` and `hclib_finalize` API calls. HClib APIs are callable only after the HClib runtime has been initialized and up to the point it is finalized.

```C
int main(int argc, char ** argv) {
    hclib_init(&argc, argv);
    printf("HClib running\n");
    hclib_finalize();
    return 0;
}
```

## <a name="async"> </a> Executing code asynchronously with async

Asynchronous code execution is composed of two steps: definition and instantiation.

A function definition of type `asyncFct_t` declares the code to execute. The `async` API call allows to create a task instance out of the code definition.

Example:

```C
// Function definition matching 'asyncFct_t' prototype
void hello(void * args) {
    printf("hello from async\n");
}

int main(int argc, char ** argv) {
    hclib_init(&argc, argv);
    async(&hello, NO_ARG, NO_DDF, NO_PHASER, NO_PROP);
    hclib_finalize();
}
```

## <a name="finish"> </a> Waiting for asyncs completion with finish

A finish scope delimitates a portion of code that must execute to completion before proceeding to the next statement. This definition includes any async spawned in the finish scope as well as all their transitively spawned children.

A finish scope is started by calling the `start_finish` API call and ends with the `end_finish` call. Finish scopes can be dynamically nested. In that case, a call to `end_finish` pairs with the most recent `start_finish` call.

Example:

```C
int main(int argc, char ** argv) {
    hclib_init(&argc, argv);
    start_finish();
    async(&hello, NO_ARG, NO_DDF, NO_PHASER, NO_PROP);
    end_finish();
    // here 'hello' async is guaranteed to have been executed.
    hclib_finalize();
}
```
Note that `hclib_init` and `hclib_finalize` act as an implicit global finish scope.

## <a name="forasync"> </a> Forasync parallel loop

Tutorial section coming soon.

## <a name="ddf"> </a> Coordinate asyncs execution: Data-Driven Future (DDF)

Data-Driven Futures (DDFs) allow to coordinate execution of asyncs in a macro-dataflow fashion. A DDF instance holds a value that can be set through a `put` operation and read through a `get` operation. A DDF value is initially undefined. Performing a `get` operation on a DDF that has an undefined value yields an error. DDFs are single-assignment meaning a unique `put` can occur on a DDF. When a `put` occurs, the DDF is satisfied and its value can be read through a `get` operation.

An async instance can depend on a list of DDFs. In that case, the async is scheduled for execution only when each of the DDF it depends on have been satisfied. We name an async that has DDFs dependences a Data-Driven Task (DDT).
A DDT typically reads its input from a set of DDFs and is executed only when all its inputs are available; i.e. a `put` has occurred on each DDF. A DDT may enable other DDTs to run by putting on DDFs they depend on.

```C
int main(int argc, char ** argv) {
    hclib_init(&argc, argv);
    struct ddf_st * ddf = ddf_create();
    async(&hello, NO_ARG, &ddf, NO_PHASER, NO_PROP);
    // async is not scheduled until a 'put' has occurred on ddf
    ddf_put(ddf, NO_DATUM);
    // The async now have all its DDFs satisfied 
    // and is eligible for scheduling
    hclib_finalize();
}
```

## Passing data to an async 

TODO: proper wording for fully-strict

To access data located at a certain memory address, the access must be both legal and safe. We define a memory address access to be legal if the memory pointed to is properly allocated either on stack or on the heap. We define a memory access to be safe if accessing the pointed memory is free of data races.

### Legal use of memory addresses

HClib enforces a terminally-strict model, meaning an async can outlive the context it has been created in. Typically, a function (or an async) that creates an async can execute to completion before the later has been executed.

It is always legal for an async to use the address of a heap-allocated variable for as long as the memory location pointed to has not been freed.

Additionally, a finish scope guarantees that memory addresses of all local variables that are visible in the lexical scope of the end_finish can be used by all asyncs transitively belonging to that scope.

Example: valid sharing of a stack variable
```C
int f() {
    int var = 12;
    async(&readValue, (void *) &var, NO_DDF, NO_PHASER, NO_PROP);
    // The following call ensures that all transitively created 
    // asyncs have executed to completion before proceeding. Thus,
    // each async that has var passed as an argument can legally 
    // read the memory location.
    end_finish(); 
}
```
Example: incorrect sharing of a stack variable
```C
int f() {
    int var = 12;
    async(&readValue, (void *) &var, NO_DDF, NO_PHASER, NO_PROP);
    // By the time the async is scheduled, the function 'f' might have
    // returned, thus reclaiming the stack-allocated memory for 'var'.
}
```
### Safe use of memory addresses

It is the responsibility of the programmer to ensure accesses to a memory location are properly coordinated. The finish and DDF constructs can help with such coordination. A finish can be used to ensure asyncs have executed to completion; i.e. they are done reading or writing to memory location they had access to. DDFs can be used to coordinate accesses, most notably in a producer-consumer style.


Example: Producer-Consumer with DDFs
```C
// Put on a DDF, transmitting data
void producer(void * args) {
    struct ddf_st * ddf = (struct ddf_st *) args;
    int * data = malloc(sizeof(int));
    data[0] = 5;
    printf("Producing value %d\n", data[0]);
    ddf_put(ddf,data);
}

// Get on a DDF dependence
void consumer(void * args) {
    struct ddf_st * ddf = (struct ddf_st *) args;
    int * data = (int *) ddf_get(ddf);
    printf("Consuming value %d\n", data[0]);
}

int main(int argc, char ** argv) {
    hclib_init(&argc, argv);
    struct ddf_st * ddf = ddf_create();

    // Create a DDT depending on ddf
    // Pass the ddf has an argument too for consumption
    async(&consumer, &ddf, &ddf, NO_PHASER, NO_PROP);

    // Pass the ddf down to the consumer
    async(&producer, &ddf, NO_DDF, NO_PHASER, NO_PROP);

    hclib_finalize();
}
```

## <a name="accum"> </a> Reductions with accumulators

Accumulators allow for parallel, concurrent, safe reductions. An accumulator typically registers to a finish scope where multiple asyncs can share it and contribute values to be reduced. The accumulator implementation ensures the reduction is race-free and it is safe to read the accumulator after the finish scope.

The `accum_create` API allows to instantiate and parameterize accumulators.
The type is specified by calling the relevant `accum_create` API implementation. HClib provides a default implementation for C primitive types (`accum_create_int`, `accum_create_double`, etc...). Parameters of the `accum_create` API allows to specify the accumulator's reduction kind, reduction strategy and the initial value of the reduction.

HClib provides a some (work in progress) accumulation operations:

  * PLUS
  * MAX

```C
//Accumulators example

void addAsyncFct(void * args) {
    accum_t * accum = (accum_t *) args;
    // contribue one to the accumulator
    accum_put_int(accum,1);
}

void function() {
    accum_t * accum = accum_create_int(ACCUM_OP_PLUS, ACCUM_MODE_LAZY, 0);
    start_finish();
        // register accumulator to current finish
        register_accum(&accum, 1);
        // create an activity and trickle the accumulator down through args
        async(addAsyncFct, (void *) accum, NO_DDF, NO_PHASER, NO_PROP);
        // contribue two to the accumulator
        accum_put_int(accum,2);
    end_finish();
    // retrieve final value after the finish
    int value = accum_get_int(accum);
    assert(value == 3);
    accum_destroy(accum);
}
```

## <a name="phaser"> </a> Collective and point-to-point synchronization with phasers

Tutorial section coming soon.
