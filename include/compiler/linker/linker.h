
#ifndef _LINKER_H_
#define _LINKER_H_


/*
argument: 
    An .ir source file

return:
    A NULL terminated set of paths, that form the set of dependencies of the provided source file

what does it do:
    It searches for imports in the file and extracts the path used and adds it to the set. 
*/
char** linker_get_dependencies(const char* const filename);


/*
argument:
    A NULL terminated list of filenames (relative path from project source) of the set of ir source code files
    whose set of dependencies to be calculated

return:
    a NULL terminated list of filenames, that is a weak superset of the input files. 
    The filenames will again be in relative path form
    returns NULL on error. 

What does it do:
    The function parses all the provided source files and searched for any and all "imports". 
    Every import is added to the set of total imports, includeing the source files themselves. 
    Any previously visited file will not be added again, hence the "set" of dependencies. 

Why use it?:
    This function is used to collect all necessary files needed for successful compilation using external libraries. 

*/
char** linker_build_dependency_set(char** source_files_ir);


#endif
