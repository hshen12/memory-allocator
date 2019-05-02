# Project 3: Memory Allocator

See: https://www.cs.usfca.edu/~mmalensek/cs326/assignments/project-3.html 

To compile and use the allocator:

```bash
make
LD_PRELOAD=$(pwd)/allocator.so ls /
```

(in this example, the command `ls /` is run with the custom memory allocator instead of the default).

## Testing

To execute the test cases, use `make test`. To pull in updated test cases, run `make testupdate`. You can also run a specific test case instead of all of them:

```
# Run all test cases:
make test

# Run a specific test case:
make test run=4

# Run a few specific test cases (4, 8, and 12 in this case):
make test run='4 8 12'
```

This project is a custom memory allocator. We configure the active free space management algorithm via environment variables and use First fit, Best fit and Worst fit for different situation. We used mmap to provide regions of memory. We used a stucts to make blocks which includes how large it is, whether it has been freed or not, and includes a pointer to the next block in the chain. Users are able to allocate memory, free memory and reallocate memory. Plus, the print_memory will print out the current state of the regions and blocks and also allow an option of writing the result to a file. The output from print_memory() can be passed into visualize.sh to display the memory regions and blocks as a PDF diagram. Scribbling is also available, when the ALLOCATOR_SCRIBBLE environment variable is set to 1, you will fill any new allocation with 0xAA (10101010 in binary). This means that if a program assumes memory allocated by malloc is zeroed out.
