
/*************************************************************************//**
 *****************************************************************************
 * @file        general.hpp
 * @brief       
 * @author      Frank Imeson
 * @date        
 *****************************************************************************
 ****************************************************************************/

// http://www.parashift.com/c++-faq-lite/
// http://www.emacswiki.org/emacs/KeyboardMacrosTricks

#ifndef GENERAL_H		// guard
#define GENERAL_H

/*****************************************************************************
 * INCLUDE
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <queue>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <sys/wait.h>
#include "sys/stat.h"
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>



using namespace std;


/*****************************************************************************
 * Defs
 ****************************************************************************/

// Added by Karl
/* Lifted attributes values */
#define Lifted 1
#define NotLifted 0

#define Original 1
#define NotOriginal 0

#define Removed 1
#define NotRemoved 0

#define kDummy 1
#define kNotDummy 0

#define Top 1
#define Bottom 0
////////////////


/*****************************************************************************
 * Prototypes
 ****************************************************************************/

void print_file (string filename);

//bool test_free_mem(int mb);
//bool test_free_mem(double percent);

#endif
