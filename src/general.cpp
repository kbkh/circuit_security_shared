
/*************************************************************************//**
 *****************************************************************************
 * @file        general.cpp
 * @brief       
 * @author      Frank Imeson
 * @date        
 *****************************************************************************
 ****************************************************************************/

// http://www.ros.org/wiki/CppStyleGuide

#include "general.hpp"
//#define DEBUG //


/*************************************************************************//**
 * @brief	
 * @version						v0.01b \n
 ****************************************************************************/
//bool test_free_mem(int mb) {
////    sg_init();
//    return sg_get_mem_stats()->free > mb*1024;
//}



/*************************************************************************//**
 * @brief	
 * @version						v0.01b \n
 ****************************************************************************/
//bool test_free_mem(double percent) {
////cout << "Total Mem = " << sg_get_mem_stats()->total << ", Free = " << sg_get_mem_stats()->free << endl;
////cout << "Percentage = " << (double) sg_get_mem_stats()->free/sg_get_mem_stats()->total << endl;
////cout << "return = " << ( ( (double) sg_get_mem_stats()->free/sg_get_mem_stats()->total ) > percent) << endl;
//    return ( (double) sg_get_mem_stats()->free/sg_get_mem_stats()->total ) > percent;
//}

/*************************************************************************//**
 * @brief	
 * @version						v0.01b \n
 ****************************************************************************/
void print_file (string filename) {
    ifstream file;
    file.open(filename.c_str());
    while (file.good()) {
        string line;
        getline(file, line);
        printf("%s\n", line.c_str());        
    }
}
