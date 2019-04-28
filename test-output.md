## Test 01: Unix Utilities [1 pts]

Runs 'ls /', 'df', and 'w' with custom memory allocator

```

# Check to make sure the library exists
[[ -e "./allocator.so" ]] || test_end 1

expected=$(ls /)
actual=$(LD_PRELOAD=./allocator.so ls /) || test_end
compare <(echo "${expected}") <(echo "${actual}") || test_end

Expected Output                        | Actual Output
---------------------------------------V---------------------------------------
bin                                       bin
boot                                      boot
dev                                       dev
etc                                       etc
home                                      home
lib                                       lib
lib64                                     lib64
mnt                                       mnt
opt                                       opt
proc                                      proc
root                                      root
run                                       run
sbin                                      sbin
srv                                       srv
sys                                       sys
tmp                                       tmp
usr                                       usr
var                                       var
---------------------------------------^---------------------------------------

LD_PRELOAD=./allocator.so   df  || test_end
df: memory exhausted
```

## Test 02: free() tests [1 pts]

Makes a large amount of random allocations and frees them

```

# Check to make sure the library exists
[[ -e "./allocator.so" ]] || test_end 1

# If this crashes, you likely have an error in your free() / reuse logic that is
# causing memory to leak and eventually run out.
LD_PRELOAD=./allocator.so tests/progs/free
Virtual Memory Size at startup (pages): 583
[0] Virtual Memory Size (pages): 35102
[1] Virtual Memory Size (pages): 56691
ERROR: Virtual memory has increased more than 2440 pages
since the first round of execution!
free() is not functioning correctly.

test_end
```

## Test 03: Basic First Fit [1 pts]

```

output=$( \
    ALLOCATOR_ALGORITHM=first_fit \
    tests/progs/allocations-1 2> /dev/null)

echo "${output}"


# Just get the block ordering from the output. We ignore the last allocation
# that is caused by printing to stdout.
block_order=$(grep '\[BLOCK\]' <<< "${output}" \
    | sed 's/.*(\([0-9]*\)).*/\1/g' \
    | head --lines=-1)

compare <(echo "${expected_order}") <(echo "${block_order}") || test_end

Expected Output                        | Actual Output
---------------------------------------V---------------------------------------
0                                      |
6                                      <
2                                      <
3                                      <
4                                      <
5                                      <
---------------------------------------^---------------------------------------
```

## Test 04: Basic Best Fit [1 pts]

```

output=$( \
    ALLOCATOR_ALGORITHM=best_fit \
    tests/progs/allocations-1 2> /dev/null)

echo "${output}"


# Just get the block ordering from the output. We ignore the last allocation
# that is caused by printing to stdout.
block_order=$(grep '\[BLOCK\]' <<< "${output}" \
    | sed 's/.*(\([0-9]*\)).*/\1/g' \
    | head --lines=-1)

compare <(echo "${expected_order}") <(echo "${block_order}") || test_end

Expected Output                        | Actual Output
---------------------------------------V---------------------------------------
0                                      |
1                                      <
2                                      <
6                                      <
4                                      <
5                                      <
---------------------------------------^---------------------------------------
```

## Test 05: Basic Worst Fit [1 pts]

```

output=$( \
    ALLOCATOR_ALGORITHM=worst_fit \
    tests/progs/allocations-1 2> /dev/null)

echo "${output}"


# Just get the block ordering from the output. We ignore the last allocation
# that is caused by printing to stdout.
block_order=$(grep '\[BLOCK\]' <<< "${output}" \
    | sed 's/.*(\([0-9]*\)).*/\1/g' \
    | head --lines=-1)

compare <(echo "${expected_order}") <(echo "${block_order}") || test_end

Expected Output                        | Actual Output
---------------------------------------V---------------------------------------
0                                      |
1                                      <
2                                      <
3                                      <
4                                      <
5                                      <
6                                      <
---------------------------------------^---------------------------------------
```

## Test 06: Testing first_fit [1 pts]

```

# Check to make sure there were no extra pages
ALLOCATOR_ALGORITHM=${algo} \
tests/progs/allocations-2 2> /dev/null || test_end
Pages at start = 578; pages after allocations = 579

output=$( \
    ALLOCATOR_ALGORITHM=${algo} \
    tests/progs/allocations-3 2> /dev/null)

echo "${output}"


# Just get the block ordering from the output. We ignore the last allocation
# that is caused by printing to stdout.
block_order=$(grep '\[BLOCK\]' <<< "${output}" \
    | sed 's/.*(\([0-9]*\)).*/\1/g' \
    | head --lines=-1)

compare <(echo "${expected_order}") <(echo "${block_order}") || test_end

Expected Output                        | Actual Output
---------------------------------------V---------------------------------------
0                                      |
5                                      <
6                                      <
7                                      <
2                                      <
3                                      <
4                                      <
---------------------------------------^---------------------------------------
```

## Test 07: Testing best_fit [1 pts]

```

# Check to make sure there were no extra pages
ALLOCATOR_ALGORITHM=${algo} \
tests/progs/allocations-2 2> /dev/null || test_end
Pages at start = 578; pages after allocations = 579

output=$( \
    ALLOCATOR_ALGORITHM=${algo} \
    tests/progs/allocations-3 2> /dev/null)

echo "${output}"


# Just get the block ordering from the output. We ignore the last allocation
# that is caused by printing to stdout.
block_order=$(grep '\[BLOCK\]' <<< "${output}" \
    | sed 's/.*(\([0-9]*\)).*/\1/g' \
    | head --lines=-1)

compare <(echo "${expected_order}") <(echo "${block_order}") || test_end

Expected Output                        | Actual Output
---------------------------------------V---------------------------------------
0                                      |
5                                      <
2                                      <
6                                      <
7                                      <
4                                      <
---------------------------------------^---------------------------------------
```

## Test 08: Testing worst_fit [1 pts]

```

# Check to make sure there were no extra pages
ALLOCATOR_ALGORITHM=${algo} \
tests/progs/allocations-2 2> /dev/null || test_end
Pages at start = 578; pages after allocations = 579

output=$( \
    ALLOCATOR_ALGORITHM=${algo} \
    tests/progs/allocations-3 2> /dev/null)

echo "${output}"


# Just get the block ordering from the output. We ignore the last allocation
# that is caused by printing to stdout.
block_order=$(grep '\[BLOCK\]' <<< "${output}" \
    | sed 's/.*(\([0-9]*\)).*/\1/g' \
    | head --lines=-1)

compare <(echo "${expected_order}") <(echo "${block_order}") || test_end

Expected Output                        | Actual Output
---------------------------------------V---------------------------------------
0                                      |
6                                      <
7                                      <
2                                      <
3                                      <
4                                      <
5                                      <
---------------------------------------^---------------------------------------
```

## Test 09: Memory Scribbling [1 pts]

```

# Check to make sure the library exists
[[ -e "./allocator.so" ]] || test_end 1

actual=$(LD_PRELOAD=./allocator.so tests/progs/scribble) || test_end
--------------------------------------
ERROR: program terminated with SIGSEGV
--------------------------------------
```

## Test 10: Thread Safety [1 pts]

Performs allocations across multiple threads in parallel

```

# Check to make sure the library exists
[[ -e "./allocator.so" ]] || test_end 1

# If this crashes or times out, your allocator is not thread safe!
LD_PRELOAD=./allocator.so tests/progs/thread-safety
thread-safety: allocatestack.c:624: allocate_stack: Assertion `errno == ENOMEM' failed.
./tests/10-Thread-Safety-1.sh: line 10: 20796 Aborted                 (core dumped) LD_PRELOAD=./allocator.so tests/progs/thread-safety

test_end
```

## Test 11: print_memory() function [1 pts]

```

output=$(tests/progs/print-test 2> /dev/null)

echo "${output}"
-- Current Memory State --
[REGION] 0x7fecd4888000-0x7fecd4889000 4096
[BLOCK]  0x7fecd4888000-0x7fecd4889000 (0) 4096 4048 4000
[REGION] 0x7fecd4887000-0x7fecd4888000 4096
[BLOCK]  0x7fecd4887000-0x7fecd4888000 (1) 4096 4048 4000
[REGION] 0x7fecd4886000-0x7fecd4887000 4096
[BLOCK]  0x7fecd4886000-0x7fecd4887000 (2) 4096 4048 4000
[REGION] 0x7fecd4884000-0x7fecd4886000 8192
[BLOCK]  0x7fecd4884000-0x7fecd4886000 (3) 8192 4144 4096

# Must have 3 or more regions
regions=$(grep '\[REGION\]' <<< "${output}" | wc -l)
[[ ${regions} -ge 3 ]] || test_end 1

# Must have 3 or more allocations of req size 4000
# We also check that the request size is in column 6
blocks=$(grep '\[BLOCK\]' <<< "${output}" \
    | awk '{print $6}' \
    | grep '4000' \
    | wc -l)
[[ ${blocks} -ge 3 ]] || test_end 1

test_end
```

## Test 12: write_memory() function [1 pts]

```

rm -f test-memory-output-file.txt

tests/progs/write-test test-memory-output-file.txt 2> /dev/null

output=$(cat test-memory-output-file.txt)
cat: test-memory-output-file.txt: No such file or directory

# Must have 3 or more regions
regions=$(grep '\[REGION\]' <<< "${output}" | wc -l)
[[ ${regions} -ge 3 ]] || test_end 1
```

## Test 13: You get a point just for TRYING! [1 pts]

```

test_end 0
```

