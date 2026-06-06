

/*
Orchestrates the compilation
takes the files to compile, together with the target level (ir, asm, bin). 

From CLI it should be possible to compile any amount of files from any format to any other (lower) format. 
Equal formats do not cause errors, it just passes through. 

It should also be possible to link the files from here. 



so, from CLI I want the following options: 

./main -s file.ir -o file2.ir
this would compiler file.ir to assembly and file2.ir to binary


*/
