# parse-obj

A C++ and python parser for Wavefront .OBJ file formats. To install, clone the repository with `git clone https://github.com/JonathanZ837/parse-obj` and cd into the desired folder. Use make in the C++ folder to build and do ./Objparser <input_file> <output_file>.

## Testing Results

Testing on the bunny.obj file from https://casual-effects.com/data/ and takes the average of 3 runs using the `time` command in Linux.

Read & Write times:
| Date & Version                            | real  | user  | sys  |
|-------------------------------------------|-------|-------|------|
| 07/11 (OG)                                 | 1.668s | 0.728s | 0.137s |
| 07/11 (with emplace)                       | 1.669s | 0.758s | 0.113s |
| 07/11 (with std::format)                   | 2.780s | 1.842s | 0.142s |
| 07/11 (with vector.reserve)                | 1.431s | 0.683s | 0.090s |
| 07/12 (reduced assignments/temp variables) | 1.309s | 0.678s | 0.079s |
| 07/12 (reduced assignments/temp variables) | 1.309s | 0.678s | 0.079s |
| 07/17 (built with -O2)                   | 1.273s | 0.562s | 0.079s |
| 07/17 (built with -O3)                   | 1.285s | 0.566s | 0.064s |
| 07/21 (used mmap for reading/writing)    | 1.036s | 0.249s | 0.135s |

Additionally, on 7/21 I conducted a test by rewriting the parser using mmap to read and write, and compared the times separately. The results are shown below: 

Read times:

| Date & Version               | real  | user  | sys   |
|-------------------------|-------|-------|-------|
| 07/17 (w/o using mmap)  | 0.644s | 0.248s | 0.131s |
| 07/21 (using mmap)      | 0.358s | 0.249s | 0.057s |

Write times: 

| Date & Version                | Real |
|-----------------------------|------|
| 07/17 (w/o using mmap)      | 0.792s |
| 07/21 (using mmap)          | 0.715s |

Observations: 
- vector.emplace_back did not make much of a difference versus vector.push_back
- std:format made my program much slower than just using concatenations with << (but maybe my implementation is wrong)
- vector.reserve() did make the program faster using some approximate heuristics baesd on file size.
- mmap significantly improved the read times and slightly improved write times.

Comparisons to other obj loaders:
- reading the same file (bunny.obj) with fast_obj and tinyobjloader yields a read time of 0.20s and 0.71s, respectively, which is comparable to our read time of 0.358s.
- reading a larger file (rungholt.obj) with fast_obj and tinyobjloader yielded a read time of 4.73s and 14.74s. My program had an average time over 3 runs of 4.914s, meaning **my program has a comparable time to fastobj.**
  
Notes about using vector.reserve:
- Each file was about 40 bytes per line of code
- For each file, approximately half of the lines were faces, and the rest were v or vn (maybe 50/50 split)
- Thus, we find the file size in bytes, divide by 40, then divide by half to reserve space for faces, and ⅓ for vertices and ⅓ for normals.

Using `perf stat` yields a report:

     Performance counter stats for './Objparser_m ../testing/rungholt.obj ../testing/output.obj':

           6393.57 msec task-clock:u                     #    0.409 CPUs utilized
                 0      context-switches:u               #    0.000 /sec
                 0      cpu-migrations:u                 #    0.000 /sec
            121441      page-faults:u                    #   18.994 K/sec
     <not supported>      cycles:u
     <not supported>      instructions:u
     <not supported>      branches:u
     <not supported>      branch-misses:u

      15.640118635 seconds time elapsed

       3.626920000 seconds user
       2.924059000 seconds sys
  
       



