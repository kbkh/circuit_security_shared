
/****************************************************************************//**
 ********************************************************************************
 * @file        formula.cpp
 * @brief       
 * @author      Frank Imeson
 * @date        
 ********************************************************************************
 ********************************************************************************/

#include "formula.hpp"
#include <sstream>
using namespace formula; //

//#define DEBUG

/********************************************************************************
 * 
 * Functions
 * 
 ********************************************************************************/         


/************************************************************//**
 * @brief	
 * @version						v0.01b
 ****************************************************************/
int formula::get_solution (const Solver &solver, vec<Lit> &lits, Var max_var) {
    if (max_var < 0)
        max_var = solver.model.size()-1;
    for (unsigned int i=0; i<=max_var; i++)
        lits.push( mkLit(i, solver.model[i] == l_False) );
    return 0;
}



/************************************************************//**
 * @brief	
 * @version						v0.01b
 ****************************************************************/
int formula::negate_solution (const vec<Lit> &lits, Solver &solver) {
    vec<Lit> nlits;
    for (unsigned int i=0; i<lits.size(); i++)
        nlits.push( ~lits[i] );
    solver.addClause(nlits);
    return 0;
}



/************************************************************//**
 * @brief	
 * @version						v0.01b
 ****************************************************************/
Lit formula::translate (const int &lit) {
    return mkLit(abs(lit)-1, (lit<0));
}



/************************************************************//**
 * @brief	
 * @version						v0.01b
 ****************************************************************/
int formula::translate (const Lit &lit) {
    return sign(lit) ? -var(lit)-1:var(lit)+1;
}



/************************************************************//**
 * @brief	
 * @return            string representation of connective	
 * @version						v0.01b
 ****************************************************************/
string formula::str (const Con &connective) {
    switch (connective) {
        case F_AND:
            return "^";
        case F_OR:
            return "v";
	case F_XOR:
		return "x";
	case F_XNOR:
		return "n";
        default:
            return " ";
    }
}



/************************************************************//**
 * @brief	
 * @return            string representation of connective	
 * @version						v0.01b
 ****************************************************************/
string formula::str (const Lit &lit) {
    stringstream out;
    if (sign(lit))
        out << "-";
    else
        out << " ";
    out << (var(lit)+1);
    return out.str();
}




/************************************************************//**
 * @brief	
 * @return            string representation of connective	
 * @version						v0.01b
 ****************************************************************/
string formula::str (const vec<Lit> &lits) {
    string result;
    for (unsigned int i=0; i<lits.size(); i++)
        result += str(lits[i]) + " ";
    return result;
}



/************************************************************//**
 * @brief	
 * @return            negated connective, -1 on error
 * @version						v0.01b
 ****************************************************************/
Con formula::negate (const Con &connective) {
    switch (connective) {
        case F_AND:
            return F_OR;
        case F_OR:
            return F_AND;
	case F_XOR:
		return F_XNOR;
	case F_XNOR:
		return F_XOR;
        default:
            return -1;
    }
}



/************************************************************//**
 * @brief
 * @return            negated litteral	
 * @version						v0.01b
 ****************************************************************/
Lit formula::negate (const Lit &lit) {
    return ~lit;
}



/********************************************************************************
 * 
 * Formula Class
 *
 ********************************************************************************/         




/************************************************************//**
 * @brief	
 * @version						v0.01b
 ****************************************************************/
Formula::~Formula () {
    clear();
	//cout << "destructing formula";
}



/************************************************************//**
 * @brief	
 * @version						v0.01b
 ****************************************************************/
Formula::Formula ()
    : connective(F_AND)
    , max_var(0)
{}



/************************************************************//**
 * @brief	
 * @version						v0.01b
 ****************************************************************/
Formula::Formula (Con _connective)
    : connective(_connective)
    , max_var(0)
{}


Formula::Formula (const Formula &formula)
	: max_var(formula.max_var), connective(formula.connective)
{
	for (int i=0; i < formula.formuli.size(); i++)
	{
		formuli.push(new Formula(*formula.formuli[i]));
	}

	for (int i=0; i < formula.lits.size(); i++)
		lits.push(formula.lits[i]);
}

Formula& Formula::operator=(const Formula& formula)
{
	max_var = formula.max_var;
	connective = formula.connective;
	for (int i=0; i < formula.formuli.size(); i++)
	{
		formuli.push(new Formula(*formula.formuli[i]));
	}

	for (int i=0; i < formula.lits.size(); i++)
		lits.push(formula.lits[i]);
}
/************************************************************//**
 * @brief	            Clear all contents of Formula recursively
 * @version						v0.01b
 ****************************************************************/
int Formula::clear () {
    try {
        for (unsigned int i=0; i<formuli.size(); i++) {
            if (formuli[i] != NULL) {
                delete formuli[i];
                formuli[i] = NULL;
            }
        }
        formuli.clear();
    } catch(...) {
        perror("error Formula::clear(): ");
        exit(1);
    }
	return 0;
}



/************************************************************//**
 * @brief	            Negate entire formula
 * @return            0 on succession, < 0 on error
 * @version						v0.01b
 ****************************************************************/
int Formula::negate () {

	if (connective == F_XOR || connective == F_XNOR) {
		if (lits.size() + formuli.size() == 1) {
			if (lits.size() > 0) lits[0] = ~lits[0];
			if (formuli.size() > 0) formuli[0]->negate();
			return 0;
		}
		if (lits.size() + formuli.size() == 2) { this->connective = ::negate(connective); return 0; }
		Formula* temp = new Formula(*this);
		
		this->clear();
		this->lits.clear();
		this->connective = ::negate(connective);

		if (temp->lits.size() > 0) { 
			this->add(temp->lits.last());
			temp->lits.pop();
			this->add(temp);
		}
		else if (temp->formuli.size() > 0) {
			this->add(temp->formuli.last());
			temp->formuli.pop();
			this->add(temp);
		}
		return 0;
	}

    connective = ::negate(connective);
	
    if (connective < 0)
        return connective;
    
    for (unsigned int i=0; i<lits.size(); i++)
        lits[i] = ::negate(lits[i]);
        
    for (unsigned int i=0; i<formuli.size(); i++) {
        int rtn = formuli[i]->negate();
        if (rtn < 0)
            return rtn;
    }
    return 0;
}






/************************************************************//**
 * @brief	
 * @version						v0.01b
 ****************************************************************/
void Formula::track_max (const Var &var) {
    if (var > max_var)
        max_var = var;
}



/************************************************************//**
 * @brief	
 * @version						v0.01b
 ****************************************************************/
int Formula::add (const Lit &lit) {
    track_max(var(lit));
    lits.push(lit);
    return 0;
}



/************************************************************//**
 * @brief	
 * @version						v0.01b
 ****************************************************************/
int Formula::add (const int &lit) {
    return add(translate(lit));
}



/************************************************************//**
 * @brief	
 * @version						v0.01b
 ****************************************************************/
int Formula::add (const vec<Lit> &lits) {
    for (unsigned int i=0; i<lits.size(); i++)
        add(lits[i]);
    return 0;
}



/************************************************************//**
 * @brief	
 * @version						v0.01b
 ****************************************************************/
int Formula::add (Formula *formula) {
	if (formula != NULL)
{
    track_max(formula->maxVar());
    
        formuli.push(formula);
}
    return 0;
}




/************************************************************//**
 * @brief	
 * @version						v0.01b
 ****************************************************************/
Var Formula::newVar () {
    max_var++;
    return max_var;
}



/************************************************************//**
 * @brief	
 * @version						v0.01b
 ****************************************************************/
Var Formula::maxVar () {
    return max_var;
}


bool Formula::evaluate(Solver *solver) {
	switch (connective) {
		case F_AND:
		if (lits.size() == 0 && formuli.size() == 0) cout << "YRYl;sjpror";
		for (int i = 0; i < lits.size(); i++)
				if (solver->modelValue(lits[i]) == l_False) return false;
			
		for (int i = 0; i < formuli.size(); i++)
				if (!formuli[i]->evaluate(solver)) return false;
		
		return true;			
		break;
		case F_OR:
if (lits.size() == 0 && formuli.size() == 0) cout << "YRYl;sjpror";
		for (int i = 0; i < lits.size(); i++)
				if (solver->modelValue(lits[i]) == l_True) return true;
			
		for (int i = 0; i < formuli.size(); i++)
				if (formuli[i]->evaluate(solver)) return true;
		
		return false;			
		
		break;
	}
}

/************************************************************//**
 * @brief	
 * @version						v0.01b
 ****************************************************************/
int Formula::export_cnf (Lit &out, Formula *formula, Solver *solver) {

    if ( (formula == NULL && solver == NULL) || (formula != NULL && solver != NULL) )
        return -1;

    // setup vars in solver
    // translation requires a new var to replace nested phrases
    if (solver != NULL) {
        while (solver->nVars() < max_var+1)
            solver->newVar();
        out = mkLit( solver->newVar(), false );
    } else {
        while (formula->maxVar() < max_var)
            formula->newVar();
        out = mkLit( formula->newVar(), false );
    }

    vec<Lit> big_clause;
    switch (connective) {
        case F_OR:

            // Variables
            for (unsigned int i=0; i<lits.size(); i++) {
                big_clause.push(lits[i]);
                if (solver != NULL) {
                    solver->addClause(~lits[i], out);
                } else {
                    Formula *phrase = new Formula(F_OR);
                    phrase->add(~lits[i]);
                    phrase->add(out);
                    formula->add(phrase);
                }
            }

            // Nested formuli
            for (unsigned int i=0; i<formuli.size(); i++) {
                Lit cnf_out;
                if (int err = formuli[i]->export_cnf(cnf_out, formula, solver) < 0) 
                    return err;
                big_clause.push(cnf_out);
                if (solver != NULL) {
                    solver->addClause(~cnf_out, out);
                } else {
                    Formula *phrase = new Formula(F_OR);
                    phrase->add(~cnf_out);
                    phrase->add(out);
                    formula->add(phrase);
                }
            }

            big_clause.push(~out);
            break;
    
        case F_AND:

            // Variables
            for (unsigned int i=0; i<lits.size(); i++) {
                big_clause.push(~lits[i]);
                if (solver != NULL) {
                    solver->addClause(lits[i], ~out);
                } else {
                    Formula *phrase = new Formula(F_OR);
                    phrase->add(lits[i]);
                    phrase->add(~out);
                    formula->add(phrase);
                }
            }

            // Nested formuli
            for (unsigned int i=0; i<formuli.size(); i++) {
                Lit cnf_out;
                if (int err = formuli[i]->export_cnf(cnf_out, formula, solver) < 0) 
                    return err;
                big_clause.push(~cnf_out);
                if (solver != NULL) {
                    solver->addClause(cnf_out, ~out);
                } else {
                    Formula *phrase = new Formula(F_OR);
                    phrase->add(cnf_out);
                    phrase->add(~out);
                    formula->add(phrase);                    
                }

            }            

            big_clause.push(out);
            break;
    
	case F_XOR:
	{
		if (lits.size() + formuli.size() == 1) { connective = F_AND; return export_cnf(out, formula, solver); return 0; }	
		Formula* temp = new Formula(); Var cur_var;

		vec<Var> formula_vars;

		while (temp->maxVar() < max_var)
            		temp->newVar();
	    	for (unsigned int i=0; i<formuli.size(); i++) {
			Formula* formula_0 = new Formula(*formuli[i]);
			Formula* neg_formula_0 = new Formula(*formuli[i]); neg_formula_0->negate();

Var formula_var;
if (solver != NULL) {
       	formula_var = solver->newVar();
    } else {
        formula_var = formula->newVar();
    }
			Formula* impl = new Formula(F_OR);
			Formula* one_one = new Formula(F_AND); one_one->add(mkLit(formula_var)); one_one->add(formula_0); impl->add(one_one);
			Formula* zero_zero = new Formula(F_AND); zero_zero->add(mkLit(formula_var, true)); zero_zero->add(neg_formula_0); impl->add(zero_zero);
			temp->add(impl);
			formula_vars.push(formula_var);
		}
	
		vec<Lit> xor_lits;
		lits.copyTo(xor_lits);
		for (int i=0; i < formula_vars.size(); i++) xor_lits.push(mkLit(formula_vars[i]));

		if (xor_lits.size() > 0) {

			Formula* sub_xor = new Formula(F_OR);
			Formula* one_zero = new Formula(F_AND);
			Formula* zero_one = new Formula(F_AND);
			one_zero->add(xor_lits[0]); one_zero->add(~xor_lits[1]);	sub_xor->add(one_zero);
			zero_one->add(~xor_lits[0]); zero_one->add(xor_lits[1]);	sub_xor->add(zero_one);

			if (xor_lits.size() <= 2) temp->add(sub_xor); else {
			Formula* neg_sub_xor = new Formula(*sub_xor); neg_sub_xor->negate();
if (solver != NULL) {
       	cur_var = solver->newVar();
    } else {
        cur_var = formula->newVar();
    }

//			cur_var = temp->newVar();
			Formula* impl = new Formula(F_OR);
			Formula* one_one = new Formula(F_AND); one_one->add(mkLit(cur_var)); one_one->add(sub_xor); impl->add(one_one);
			Formula* zero_zero = new Formula(F_AND); zero_zero->add(mkLit(cur_var, true)); zero_zero->add(neg_sub_xor); impl->add(zero_zero);
			temp->add(impl); }
		    	for (unsigned int i=2; i<xor_lits.size(); i++) {
				cout << "once";
				Formula* sub_xor = new Formula(F_OR);
				Formula* one_zero = new Formula(F_AND);
				Formula* zero_one = new Formula(F_AND);
				one_zero->add(xor_lits[i]); one_zero->add(mkLit(cur_var,true));	sub_xor->add(one_zero);
				zero_one->add(~xor_lits[i]); zero_one->add(mkLit(cur_var));	sub_xor->add(zero_one);
	
				if (i == xor_lits.size()-1) { cout << "here"; temp->add(sub_xor); continue; }
				Formula* neg_sub_xor = new Formula(*sub_xor); neg_sub_xor->negate();

if (solver != NULL) {
       	cur_var = solver->newVar();
    } else {
        cur_var = formula->newVar();
    }

				Formula* impl = new Formula(F_OR);
				Formula* one_one = new Formula(F_AND); one_one->add(mkLit(cur_var)); one_one->add(sub_xor); impl->add(one_one);
				Formula* zero_zero = new Formula(F_AND); zero_zero->add(mkLit(cur_var, true)); zero_zero->add(neg_sub_xor); impl->add(zero_zero);
				temp->add(impl);
		    	}
			//temp->add(cur_var + 1);
		}
		
		int error = temp->export_cnf(out, formula, solver);
		temp->clear();
		return error;
	   break;
	}

	case F_XNOR:
	{		
		if (lits.size() + formuli.size() == 1) { connective = F_AND; return export_cnf(out, formula, solver); return 0; }	
		Formula* temp = new Formula(); Var cur_var;

		vec<Var> formula_vars;

		while (temp->maxVar() < max_var)
            		temp->newVar();
	    	for (unsigned int i=0; i<formuli.size(); i++) {
			Formula* formula_0 = new Formula(*formuli[i]);
			Formula* neg_formula_0 = new Formula(*formuli[i]); neg_formula_0->negate();

Var formula_var;
if (solver != NULL) {
       	formula_var = solver->newVar();
    } else {
        formula_var = formula->newVar();
    }

			Formula* impl = new Formula(F_OR);
			Formula* one_one = new Formula(F_AND); one_one->add(mkLit(formula_var)); one_one->add(formula_0); impl->add(one_one);
			Formula* zero_zero = new Formula(F_AND); zero_zero->add(mkLit(formula_var, true)); zero_zero->add(neg_formula_0); impl->add(zero_zero);
			temp->add(impl);
			formula_vars.push(formula_var);
		}
	
		vec<Lit> xnor_lits;
		lits.copyTo(xnor_lits);
		for (int i=0; i < formula_vars.size(); i++) xnor_lits.push(mkLit(formula_vars[i]));

		if (xnor_lits.size() > 0) {

			Formula* sub_xnor = new Formula(F_OR);
			Formula* one_zero = new Formula(F_AND);
			Formula* zero_one = new Formula(F_AND);
			one_zero->add(xnor_lits[0]); one_zero->add(xnor_lits[1]);	sub_xnor->add(one_zero);
			zero_one->add(~xnor_lits[0]); zero_one->add(~xnor_lits[1]);	sub_xnor->add(zero_one);

			if (xnor_lits.size() <= 2) temp->add(sub_xnor); else {
			Formula* neg_sub_xnor = new Formula(*sub_xnor); neg_sub_xnor->negate();

if (solver != NULL) {
       	cur_var = solver->newVar();
    } else {
        cur_var = formula->newVar();
    }

			Formula* impl = new Formula(F_OR);
			Formula* one_one = new Formula(F_AND); one_one->add(mkLit(cur_var)); one_one->add(sub_xnor); impl->add(one_one);
			Formula* zero_zero = new Formula(F_AND); zero_zero->add(mkLit(cur_var, true)); zero_zero->add(neg_sub_xnor); impl->add(zero_zero);
			temp->add(impl); }

		    	for (unsigned int i=2; i<xnor_lits.size(); i++) {
				Formula* sub_xnor = new Formula(F_OR);
				Formula* one_zero = new Formula(F_AND);
				Formula* zero_one = new Formula(F_AND);
				one_zero->add(xnor_lits[i]); one_zero->add(mkLit(cur_var));	sub_xnor->add(one_zero);
				zero_one->add(~xnor_lits[i]); zero_one->add(mkLit(cur_var,true));	sub_xnor->add(zero_one);
	
				if (i == xnor_lits.size()-1) { temp->add(sub_xnor); continue; }

				Formula* neg_sub_xnor = new Formula(*sub_xnor); neg_sub_xnor->negate();

if (solver != NULL) {
       	cur_var = solver->newVar();
    } else {
        cur_var = formula->newVar();
    }

				Formula* impl = new Formula(F_OR);
				Formula* one_one = new Formula(F_AND); one_one->add(mkLit(cur_var)); one_one->add(sub_xnor); impl->add(one_one);
				Formula* zero_zero = new Formula(F_AND); zero_zero->add(mkLit(cur_var, true)); zero_zero->add(neg_sub_xnor); impl->add(zero_zero);
				temp->add(impl);
		    	}
			//temp->add(cur_var + 1);
		}

		int error = temp->export_cnf(out, formula, solver);
		temp->clear();
		return error;
	   break;
	}


       default:
            break;
    }

    if (solver != NULL) {
        solver->addClause(big_clause);
    } else {
        Formula *phrase = new Formula(F_OR);
        phrase->add(big_clause);
        formula->add(phrase);         
    }

//	formula->clear();
    return 0;
}



/************************************************************//**
 * @brief	
 * @version						v0.01b
 ****************************************************************/
string Formula::str () {
    return str(string(""));
}



/************************************************************//**
 * @brief	
 * @version						v0.01b
 ****************************************************************/
string Formula::str (string prefix) {
    string result = prefix;

    if (formuli.size() == 0) {
//        result += "( ";
        for (unsigned int i=0; i<lits.size(); i++)
            result += ::str(lits[i]) + ", " + ::str(connective) + " ";
        result.resize(result.size()-2);
//        result += ")\n";
        result += "\n";
    } else {
        result += prefix + ::str(connective) + '\n';

        for (unsigned int i=0; i<lits.size(); i++)
            result += prefix + " " + ::str(lits[i]) + '\n';        

        for (unsigned int i=0; i<formuli.size(); i++)
            result += formuli[i]->str(prefix+" ");
    }

    return result;
}

string Formula::toOpb () {
    string result = "";

	for (unsigned int i=0; i<lits.size(); i++) {
		// char buffer[33];
		ostringstream buffer;
		buffer << var(lits[i])+1;
		result += sign(lits[i])? "-1*x": "+1*x";
		result += buffer.str();
		result += " = ";
		result += sign(lits[i])? "0": "1";
		result += ";\n";
	}
		
	for (unsigned int i=0; i<formuli.size(); i++) {
		int count = 0;
		for (unsigned int j=0; j<formuli[i]->lits.size(); j++) {
			ostringstream buffer;
			buffer << var(formuli[i]->lits[j])+1;
			result += sign(formuli[i]->lits[j])? "-1*x": "+1*x";
			if (sign(formuli[i]->lits[j])) count++;
			result += buffer.str();
			result += " ";
		}
		ostringstream buffer;
		buffer << 1-count;
		result += " >= ";
		result += buffer.str();
		result += ";\n";
	}

    return result;
}







