/*
 * 
 	parserInt.cpp
 	work in this file until new is given by prof
 *
*/
#include <cctype>
#include <map>
#include <string>
#include <queue>          // std::queue
#include "parse.h"
#include "lex.h"
#include "parserInt.h"
//#include "val.h"
#include "val.cpp"

map<string, bool> defVar;
map<string, Token> SymTable;
Queue ValQue;

static int error_count = 0;


int ErrCount()
{
    return error_count;
}

void ParseError(int line, string msg)
{
	++error_count;
	cout << line << ": " << msg << endl;
}

namespace Parser {
	bool pushed_back = false;
	LexItem	pushed_token;

	static LexItem GetNextToken(istream& in, int& line) {
		if( pushed_back ) {
			pushed_back = false;
			return pushed_token;
		}
		return getNextToken(in, line);
	}

	static void PushBackToken(LexItem & t) {
		if( pushed_back ) {
			abort();
		}
		pushed_back = true;
		pushed_token = t;	
	}

}

bool Prog(std::istream& in, int& line){
	bool status = true;
	LexItem t = Parser::GetNextToken(in, line);
	if(t!=PROGRAM){
		ParseError(line, "MISSING PROGRAM");
		status = false;
	}
	t = Parser::GetNextToken(in, line);
	if(t!=IDENT){
		ParseError(line, "Missing Program Name");
		status = false;
	}
    bool temp = true;
	temp = Decl(in, line);
    if(!temp){
        return false;   
    }
	temp = Stmt(in, line);
    if(!temp){
        return temp;   
    }
	t = Parser::GetNextToken(in, line);
	if(t!=END){
		ParseError(line, "Missing END of Program");
		status = false;
		return status;
	}
	t = Parser::GetNextToken(in, line);
	if(t!=PROGRAM){
		ParseError(line, "Missing PROGRAM at the End");
		status = false;
		return status;
	}
	t = Parser::GetNextToken(in, line);
	if(t!=IDENT){
		ParseError(line, "Missing Program Name");
		status = false;
	}
	return status;
}

bool Decl(std::istream& in, int& line){
	bool status = false;
	LexItem tok;
	LexItem t = Parser::GetNextToken(in, line);
	if(t == INTEGER || t == REAL || t == CHAR){
		//tok = t;    ??
		tok = Parser::GetNextToken(in, line);
		if(tok.GetToken() == COLON){
			status = IdList(in, line, t);
			if(status){
				status = Decl(in, line);
				return status;
			} else {
                return true;   
            }
		} else {
			ParseError(line, "Incorrect Declaration in Program");
			return false;
		
		}
	}
	Parser::PushBackToken(t);
	return true;
}

bool IdList(std::istream& in, int& line, LexItem tok){ 
	bool status = false;
	LexItem t = Parser::GetNextToken(in, line);
	if(t==IDENT){
		if (!(defVar.count(t.GetLexeme())==0)){
			ParseError(line, "Already declared var");
			return status;
		}
		defVar.insert(std::pair<std::string, bool>(t.GetLexeme(),true));
		SymTable.insert(std::pair<std::string, Token>(t.GetLexeme(),tok.GetToken()));
		LexItem z = Parser::GetNextToken(in, line);
		if(z.GetToken()==COMA){
			status = IdList(in, line, tok);
			if(status){
				return status;
			} else {
				return false;
			}
		} else {
			Parser::PushBackToken(z);
			return true;
		}
	} else {
		Parser::PushBackToken(t);
		ParseError(line, "Missing Variable");
		return false;
	}
	
}

bool Stmt(std::istream& in, int& line){
	bool status;
	//std::cout << "in Stmt" << std::endl;
	LexItem t = Parser::GetNextToken(in, line);
	switch(t.GetToken()){
		case PRINT:
			status = PrintStmt(in, line);
			if(status)
				status = Stmt(in, line);
			break;
		case IF:
			status = IfStmt(in, line); // must def
			if(status)
				status = Stmt(in, line);
			break;
		case READ:
			status = ReadStmt(in, line); // must def
			if(status)
				status = Stmt(in, line);
			break;
		case IDENT:
			if (defVar.count(t.GetLexeme())==0){
				ParseError(line, "not a declared var");
				ParseError(line, "Missing Left-Hand Side Variable in Assignment statement");
                ParseError(line, "Incomplete statement");
				return false;
			}
			status = AssignStmt(in, line); // must def
			if(status)
				status = Stmt(in, line);
			break;
		default:
			Parser::PushBackToken(t);
			return true;
	}
	return status;
}

bool PrintStmt(std::istream& in, int& line){
	//std::cout << "in PrintStmt" << std::endl;
	LexItem t;
	if( (t=Parser::GetNextToken(in, line)) != COMA ){
		ParseError(line, "Missing a Comma");
		return false;
	}
	
	ValQue = new queue<Value>;
	
	bool ex = ExprList(in, line);
	if( !ex ){
		ParseError(line, "Missing expression after print");
		// empty Q
		while(!(*ValQue).empty()){
			ValQue->pop();
		}
		delete ValQue;
		return false;
	}
	
	t = Parser::GetNextToken(in, line);
	if(t.GetToken() == SCOMA){
		while (!(*ValQue).empty()){
			Value nextVal = (*ValQue.front());
			std::cout<<nextVal;
			ValQue->pop();
		}	
		std::cout<<std::endl;
	} else {
		Parser::PushBackToken(t);
		return ex; // ? 
	}
	//Eval :: print out the list of expr vals
	return ex;
}

bool ExprList(istream& in, int& line){ 
	//std::cout << "in ExprList" << std::endl;
	bool status = false;
	LexItem t;
	status = Expr(in, line);
	if(!status){ // figure out errors l8er
		ParseError(line, "Inc Expr stmt");
		return false;
	} else if((t=Parser::GetNextToken(in, line)) == COMA){
		status = ExprList(in, line);
		return status;
	} else if(t==ERR){
        ParseError(line, "Unrecognized Input Pattern ("+t.GetLexeme()+")");
        ParseError(line, "Missing Expression");
        return false;
    } else {
		Parser::PushBackToken(t);
		return true;
	}
}

bool Expr(std::istream& in, int& line){
	//std::cout << "in Expr" << std::endl;
	bool status = false;
	status = Term(in, line);
	if(!status){
		ParseError(line, "inc Expr");
		return status;
	}
	LexItem t = Parser::GetNextToken(in, line);
	if(t == PLUS || t == MINUS){
		status = Expr(in, line);
		if(!status){
			ParseError(line, "inc Expr");
			return status;
		} else {
			return status;
		}
	} else {
		Parser::PushBackToken(t);
		return true;
	}
}

bool Term(std::istream& in, int& line){
	//std::cout << "in Term" << std::endl;
	bool status = false;
	status = SFactor(in, line);
	if(!status){
		//ParseError(line, "inc Term");
		return status;
	}
	LexItem t = Parser::GetNextToken(in, line);
	if(t == MULT || t == DIV){
		status = Term(in, line);
		if(!status){
			//ParseError(line, "inc Term");
			return status;
		} else {
			return status;
		}
	} else {
		Parser::PushBackToken(t);
		return true;
	}
}

bool SFactor(istream& in, int& line){
	//std::cout << "in SFactor" << std::endl;
	bool status = false;
	
	LexItem t = Parser::GetNextToken(in, line);
	if(t == PLUS){
		status = Factor(in, line, 1);
		if(!status){
			//ParseError(line, "inc sFactor");
			return status;
		}
	} else if (t == MINUS){
		status = Factor(in, line, -1);
		if(!status){
			//ParseError(line, "inc sFactor");
			return status;
		}
	} else {
		Parser::PushBackToken(t);
		
		status = Factor(in, line, 1); // we'll make 1 be default and positive
		if(!status){
			//ParseError(line, "inc sFactor");
			return status;
		}
	}
	return status;
}

bool Factor(istream& in, int& line, int sign){
	//std::cout << "in Factor" << std::endl;
	bool status = false;
	LexItem t = Parser::GetNextToken(in, line);
	// check if t in defVar map
	if(t==IDENT){
		if (defVar.count(t.GetLexeme())==0){
			ParseError(line, "Undeclared var");
			return status;
		}
		return true;
	} else if(t==ICONST){
		return true;
	} else if(t==RCONST){
		return true;
	} else if(t==SCONST){
		return true;
	} else if(t==LPAREN){
		status = Expr(in, line);
		if(!status){
			ParseError(line, "no expr in paren");
			return status;
		} else {
			t = Parser::GetNextToken(in, line);
			if(t==RPAREN){
				return true;
			} else {
				ParseError(line, "NO RPAREN");
				return false;
			}
		}
	} else {
		Parser::PushBackToken(t);
		ParseError(line, "Not a valid factor");
		return false;
	}
}

bool LogicExpr(istream& in, int& line){
	bool status = false;
	LexItem t;
	status = Expr(in, line);
	if(!status){
		ParseError(line, "inc expr in logicExpr");
		return status;
	}
	t = Parser::GetNextToken(in, line);
	if(t==EQUAL||t==LTHAN){
		status = Expr(in, line);
		if(!status){
			ParseError(line, "inc expr in logicExpr");
			return status;
		} else {
			return status;
		}		
	} else {
		Parser::PushBackToken(t);
		return true;
	}
}

bool IfStmt(istream& in, int& line){
	bool status = false;
	LexItem t = Parser::GetNextToken(in, line);
	if(t!=LPAREN){
		ParseError(line, "NO LPAREN IN IF");
		ParseError(line, "inc ifstmt");
		return status;
	}
	status = LogicExpr(in, line);
	if(!status){
		ParseError(line, "Incorrect LogicExpr");
		return status;
	}
	t = Parser::GetNextToken(in, line);
	if(t!=RPAREN){
		ParseError(line, "Missing Right Parenthesis");
		ParseError(line, "inc ifstmt");
		return false;
	}
	t = Parser::GetNextToken(in, line);
	if(t!=THEN){
		ParseError(line, "NO THEN IN IF");
		ParseError(line, "inc ifstmt");
		return false;
	}
	status = Stmt(in, line);
	if(!status){
		ParseError(line, "Incorrect Stmts in IF");
		return status;
	}
	t = Parser::GetNextToken(in, line);
	if(t!=END){
		ParseError(line, "NO END IN IF");
		ParseError(line, "inc ifstmt");
		return false;
	}
	t = Parser::GetNextToken(in, line);
	if(t!=IF){
		ParseError(line, "Missing IF at End of IF statement");
		ParseError(line, "inc ifstmt");
		return false;
	}
	
	return true;
}

bool Var(istream& in, int& line){
	//std::cout<<"inVAR"<<std::endl;
	bool status = false;
	LexItem t = Parser::GetNextToken(in, line);
	if(t!=IDENT){
		ParseError(line, "not an Identifier");
        ParseError(line, "Missing Variable");
		return status;	
	} else if(defVar.count(t.GetLexeme())==0){
		ParseError(line, "Undeclared Variable");
        ParseError(line, "Missing Variable");
		return status;
	}
	
	return true;
}

bool VarList(istream& in, int& line){
	//std::cout<<"inVARLIST"<<std::endl;
	bool status = false;
	status = Var(in, line);
	if(!status){
		ParseError(line, "Missing Variable after Read Statement");
		return status;
	}
	LexItem t = Parser::GetNextToken(in, line);
	if(t==COMA){
		status = VarList(in, line);
		if(status){
			return status;
		} else {
			return false;
		}
	} else {
		Parser::PushBackToken(t);
		return true;
	}
}

bool ReadStmt(istream& in, int& line){
	//std::cout<<"inREAD"<<std::endl;
	bool status = false;
	LexItem t;
	if( (t=Parser::GetNextToken(in, line)) != COMA ){
		ParseError(line, "Missing a Comma");
		ParseError(line, "Incorrect Statement in program");
		return status;
	}
	status = VarList(in, line);
	if( !status ){
		ParseError(line, "Incorrect Statement in program");
		return status;
	}
	
	return status;
}

bool AssignStmt(istream& in, int& line){
	bool status = false;
	LexItem t = Parser::GetNextToken(in, line);
	if(t!=ASSOP){
		ParseError(line, "Missing Assignment Operator =");
		ParseError(line, "Inc assign stmt");
		return status;
	}
	status = Expr(in, line);
	if(!status){
		ParseError(line, "Inc statement");
		return status;
	}
	return status;
}
