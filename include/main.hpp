
/*************************************************************************//**
 *****************************************************************************
 * @file        main.hpp
 * @brief       
 * @author      Frank Imeson
 * @date        2012-01-02
 *****************************************************************************
 ****************************************************************************/

// http://www.parashift.com/c++-faq-lite/
// http://www.emacswiki.org/emacs/KeyboardMacrosTricks
// http://www.stack.nl/~dimitri/doxygen/


/*****************************************************************************
 * INCLUDE
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctime>
#include <queue>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <boost/program_options.hpp>

#include "general.hpp"
#include "circuit.hpp"
#include "security.hpp"
#include "subisosat.hpp"


using namespace std;
namespace po = boost::program_options;

// Added by Karl

void write_to_file(int lifted_edges, int G_vcount, int G_ecount, int G_v_lifted, int G_e_lifted, int H_vcount, int H_ecount, int H_v_no_dummy, int H_e_no_dummy, double took, int oneBond, int twoBonds, int Fvcount, int Fecount);
////////////////
