# parse-obj

A C++ and python parser for Wavefront .OBJ file formats. To install, clone the repository with `git clone https://github.com/JonathanZ837/parse-obj` and cd into the desired folder. Use make in the C++ folder to build and do ./Objparser <input_file> <output_file>.

## Testing Results

Testing on the bunny.obj file from [https://casual-effects.com/data/] and takes the average of 3 runs using the `time` command in Linux.

| Date & Version                            | real  | user  | sys  |
|-------------------------------------------|-------|-------|------|
| 07/11 (OG)                                 | 1.668s | 0.728s | 0.137s |
| 07/11 (with emplace)                       | 1.669s | 0.758s | 0.113s |
| 07/11 (with std::format)                   | 2.780s | 1.842s | 0.142s |
| 07/11 (with vector.reserve)                | 1.431s | 0.683s | 0.090s |
| 07/12 (reduced assignments/temp variables) | 1.309s | 0.678s | 0.079s |

Observations: 
- vector.emplace_back did not make much of a difference versus vector.push_back
- std:format made my program much slower than just using concatenations with << (but maybe my implementation is wrong)
- vector.reserve() did make the program faster using some approximate heuristics baesd on file size.
  
Notes about using vector.reserve:
- Each file was about 40 bytes per line of code
- For each file, approximately half of the lines were faces, and the rest were v or vn (maybe 50/50 split)
- Thus, we find the file size in bytes, divide by 40, then divide by half to reserve space for faces, and ⅓ for vertices and ⅓ for normals.

Using `perf stat` yields a report:

            745.42 msec task-clock:u                     #    0.499 CPUs utilized
                 0      context-switches:u               #    0.000 /sec
                 0      cpu-migrations:u                 #    0.000 /sec
              2032      page-faults:u                    #    2.726 K/sec
     <not supported>      cycles:u
     <not supported>      instructions:u
     <not supported>      branches:u
     <not supported>      branch-misses:u

       1.493394118 seconds time elapsed

       0.686664000 seconds user
       0.074011000 seconds sys](https://github.com/JonathanZ837/parse-obj/blob/main/README.md)

  
       



