#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <cstring>
#include <list>
#include "simclasses.h"

#define LEX_ERR -1
#define SYN_ERR 0
#define SYN_OK 1
#define DUMMY 100

using namespace std;

int  generator_global = 0;

/** Funkce vrati maximalni id prvku v XML souboru.
*   Slouzi pro urceni velikosti pole struktur reprezentujici hradla obvodu
*@FILE* source - zdrojovy soubor
*@return - zjistene maximum
*/

/* Vstupni XML soubor */
FILE* source;
int line = 1;

#define KEYWORDS 16
#define TABLE_SIZE 20
#define SHIFT 30
const char* keywords[TABLE_SIZE] = {
	"OUTPUT"	, "PORT"	, "XML",
	"BLOCK"		, "GATE"	, "NAME",
	"GENERATOR"	, "TYPE"	, "VERSION",
	"LAMBDA"    , "TIME"    , "VALUE", "DELAY",
	"ID"		, "PORT"	, "INPUTS",
};

/** Funkce, ktera zjisti, zda je nacitany retezec
 *  klicovym slovem XML
 *  @param attr ukazatel na retezc
 */
int is_keyword(string attr){
  for(int i=0; i<KEYWORDS; i++)
  {
    if((strcmp(keywords[i], attr.c_str())) == 0)
      return i + SHIFT;
  }
  return 0;
}

/* Definice nazvu tokenu */
enum tokens {
	T_EOL,
	T_RULE_START,
	T_E,
	T_QUOTE,
	T_RULE_END,
	T_IDIV,
	T_EOF,
	T_INT,
	T_ID,
	T_Q,
	T_DOT,
	T_K
};

/* Definice nazvu stavu konecnych automatu */
enum states {
	S_START,
	S_EOL,
	S_RULE_START,
	S_E,
	S_QUOTE,
	S_RULE_END,
	S_IDIV,
	S_EOF,
	S_INT,
	S_ID,
	S_Q,
	S_DOT,
	S_K,
	S_ID_2,
	S_INT_2,
	S_QUOTE_2,
	S_Q_2,
	S_K_2,
	S_QUOTE_S,
	S_QUOTE_E,
	S_INNER_BLOCK_S,
	S_K_3,
	S_INNER_BLOCK_E_E,
	S_DIV,
	S_GATE_END,
	S_GATE_END_RULE_END,
	S_OUTPUT,
	S_NAME,
	S_OUTPUT_END_RULE_START,
	S_OUTPUT_END_DIV,
	S_OUTPUT_END_K,
	S_OUTPUT_END_RULE_END,
	S_VALUE,
	S_GENERATOR_END_K,
	S_GENERATOR_END_RULE_END
};

/* Definice indikatoru atributu */
enum indication {
	I_ID,
	I_TYPE,
	I_INPUTS,
	I_PORT,
	I_VALUE,
	I_LAMBDA,
	I_TIME, 
	I_DELAY
};

/** Zkontroluje, zda se jiz generator s danym id nenachazi v seznamu generatoru 
*@int id - id generatoru
*@list<generator>* generators - seznam generatoru
*/
int check_generator_id(int id, list<generator>* generators){
	if(generators == NULL) return false;
	list<generator>::iterator iter = generators->begin();
	while(iter != generators->end()){
		if(iter->id == id) return true;
		iter++;
	}
	return false;
}

/** Zkontroluje, zda se jiz hradlo s danym id nenachazi v seznamu hradel
*@int id - id hradla
*@list<gate>* gates - seznam hradel
*/
int check_gate_id(int id, list<gate>* gates){
	if(gates == NULL) return false;
	list<gate>::iterator iter = gates->begin();
	while(iter != gates->end()){
		if(iter->id == id) return true;
		iter++;
	}
	return false;
}

/** Zkontroluje, zda se block s danym nazvem nenachazi v seznamu bloku
*@string name - nazev bloku
*@list<BlockItem> - seznam bloku
*/
int check_block_name(string name, list<BlockItem>* blocks){
	if(blocks == NULL) { return 0; }
	list<BlockItem>::iterator iter = blocks->begin();
	while(iter != blocks->end()){
		if(name.compare(iter->block.c_str())){
			return 1;
		}
		iter++;
	}
	return 1;
}


/** Funkce prijima jako parametr vstupni soubor a provadi lexikalni analyzu
*@string* attr - ukazatel na atribut tokenu
*@return - vraci token s prectenym prvkem XML souboru
*/
int get_next_token(string* attr){

	/* Stav analyzy */
	int state = S_START;
	/* Promenna pro nacitani znaku */
	int c;
	/* Vycisteni atributu */
	*attr = "";
	/* Pomocna promenna */
	int x;
	char temp[1];

	/* Nekonecny cyklus lexikalniho analyzatoru */
	while(1){
	
		/* Nacteni znaku ze souboru XML */
		c = getc(source);
		
		#ifdef NDEBUG
			printf("%c\n", c);
		#endif
		
		switch(state){
			case S_START:
				if(isspace(c)){
					if(c == '\n'){
						line++;
						state = S_EOL;
						continue;
					}
					state = S_START;
				}
				else if(c == '<') return T_RULE_START; // zacatek pravidla
				else if(c == '=') return T_E;
				else if(c == '\"') return T_QUOTE;
				else if(c == '\'') return T_QUOTE;
				else if(c == '>') return T_RULE_END;
				else if(c == '/') return T_IDIV;
				else if(c == '?') return T_Q;
				else if(c == '.') return T_DOT;
				else if(c == EOF) return T_EOF;
				else if(c >= '0' && c <= '9'){
					sprintf(temp, "%c", c);
					attr->append(temp);
					state = S_INT;
				}
				else if((c>='a' && c<='z')||(c>='A' && c<='Z')||(c=='_')){ //identifikator
					sprintf(temp, "%c", toupper(c));
					attr->append(temp);
					state = S_ID;
				}
				else return LEX_ERR;
				break;
			/* Prazdny znak */
			case S_EOL:
				if(c == '\n'){
					line++;
				}
				else if(isspace(c));
				else{
					ungetc(c, source);
					return T_EOL;
				}
				break;
			/* Identifikator */
			case S_ID:
				/* Prevod na velky znak */
				x = toupper(c);
				if(c >= '0' && c <= '9'){
					sprintf(temp, "%c", c);
					/* Pripojeni znaku k atributu */
					attr->append(temp);
					state = S_ID;
				}
				else if((c>='a' && c<='z')||(c>='A' && c<='Z')){
					sprintf(temp, "%c", toupper(c));
					attr->append(temp);
					state = S_ID;
				}
				else if(c == '_'){
					sprintf(temp, "%c", c);
					attr->append(temp);
					state = S_ID;
				}
				else{
					ungetc(c, source);
					if((x=is_keyword(attr->c_str())) != 0) return T_K; // tady pak upravit na spravny keyword
					else return T_ID;
				}
				break;
			/* Cele cislo */
			case S_INT:
				if(c >= '0' && c <= '9'){
					sprintf(temp, "%c", c);
					attr->append(temp);
					state = S_INT;
				}
				else {
					ungetc(c, source);
					return T_INT;
				}
				break;
			/* Chyba */
			default:
				cout << "Undefined lexical error on line " << line << endl;
				return LEX_ERR;
				break;
		} /* switch */

	} /* while */
	return 0;
}

/** Projde hlavicku souboru a zjisti, zda je syntakticky spravna 
*@string* attr - atribut pro ulozeni parametru aktualne nacteneho tokenu
*/
int parse_xml_header(string* attr){

	int state = S_START;
	int token = 0;
	
	#ifdef NDEBUG
		cout << "PARSING HEADER" << endl;
	#endif
	
	while(1){
	
		token = get_next_token(attr);

		switch(state){
			/* Prvotni stav */
			case S_START:
				/* Ocekava se znak < */
				if(token != T_RULE_START){
					cout << "Expecting < on line " << line << endl;
					return SYN_ERR;
				}
				else if(token = T_EOL);
				else state = S_Q;
				break;
			/* Ocekava se znak " */
			case S_Q:
				if(token != T_Q){
					cout << "Expecting ? on line " << line << endl;
					return SYN_ERR;
				}
				else state = S_ID;
				break;
			/* Nacitani identifikatoru */
			case S_ID:
				if(token != T_K){
					cout << "Expecting 'XML' on line " << line << "'" << *attr << "' given" << endl;
					return SYN_ERR;
				}
				else if(token == T_K && !attr->compare("XML")){
					state = S_ID_2;
				}
				else{
					cout << "Expecting 'XML' on line " << line << " '" << *attr << "' given" << endl;
					return SYN_ERR;
				}
				break;
			/* Dalsi nacitani identifikatoru */
			case S_ID_2:
				if(token == T_K && !attr->compare("VERSION")){
					state = S_E;
				}
				else{
					cout << "Expecting 'VERSION' on line " << line << endl;
					return SYN_ERR;
				}
				break;
			/* Ocekava se znak = */
			case S_E:
				if(token == T_E){
					state = S_QUOTE;
				}
				else{
					cout << "Expecting '=' on line " << line << endl;
					return SYN_ERR;
				}
				break;
			/* Ocekava se znak " */
            case S_QUOTE:
				if(token == T_QUOTE){
					state = S_INT;
				}
				else{
					cout << "Expecting '\"' on line " << line << endl;
					return SYN_ERR;
				}
				break;
			/* Ocekava se znak 0-9 */
			case S_INT:
				if(token == T_INT && !attr->compare("1")){
					state = S_DOT;
				}
				else{
					cout << "Wrong XML version on line " << line << endl;
					return SYN_ERR;
				}
				break;
			/* Ocekava se znak . */
			case S_DOT:
				if(token == T_DOT){
					state = S_INT_2;
				}
				else{
					cout << "Wrong XML version on line " << line << endl;
					return SYN_ERR;
				}
				break;
			/* Ocekava se znak 1, nebo 0 */
			case S_INT_2:
				if(token == T_INT && (!attr->compare("1") || !attr->compare("0"))){
					state = S_QUOTE_2;
				}
				else{
					cout << "Expecting end of version string on line " << line << endl;
					return SYN_ERR;
				}
				break;
			/* Ocekava se znak " */
			case S_QUOTE_2:
				if(token == T_QUOTE){
					state = S_Q_2;
				}
				else{
					cout << "Expecting '\"' on line " << line << endl;
					return SYN_ERR;
				}
				break;
			/* Ocekava se znak ? */
			case S_Q_2:
				if(token == T_Q){
					state = S_RULE_END;
				}
				else{
					state = S_Q_2;
					/* dalsi parametry jsou nacitany bez kontroly syntaxe */
				}
				break;
			/* Ocekava se znak > */
			case S_RULE_END:
				if(token == T_RULE_END){
					return SYN_OK;
				}
				else{
					cout << "Expecting '>' on line " << line << endl;
					return SYN_ERR;
				}
				break;
			/* Chyba automatu */
			default:
				cout << "Syntax error on line " << line << endl;
				break;
		} /* switch */
	}	/* while */
}	/* parse_xml_header() */

/** Zkontoroluje syntaktickou a semantickou spravnost popisu propojeni v XML souboru 
*@string* attr - pomocny retezec pro ukladani atributu tokenu
*@int type - typ prvku - Hradlo/Generator
*@int itemid - id prvku
*@gate* new_gate - ukazatel na strukturu nove vytvoreneho hradla
*@generator* new_generator - ukazatel na strukturu nove vytvoreneho generatoru
*@string* blockname - ukazatel na retezec s nazvem bloku
*@list<gate>* gates - ukazatel na seznam hradel
*@list<generator>* generators - ukazatel na seznam generatoru
*@list<BlockItem>* blocks - ukazatel na seznam bloku
*/
int parse_xml_output(string* attr, int type, int itemid, gate* new_gate, generator* new_generator, string* blockname, list<gate>* gates, list<generator>* generators, list<BlockItem>* blocks){

	int state = S_START;
	int token = 0;
	int id_indicator = 0;
	int id = 0;
	int port = 0;
	int value = -1;
	int OUTPUT = 0;
	
	#ifdef NDEBUG
		cout << "PARSING OUTPUT" << endl;
	#endif
	
	while(1){

		token = get_next_token(attr);

		switch(state){
			
			/* Pocatecni stav */
			case S_START:
				if(token == T_RULE_START){
					state = S_OUTPUT;
					OUTPUT++;
				}
				else if(token == T_RULE_END){
					/* Spolecne povinne parametry pro GENERATOR i GATE */
					if(id == 0){
						cout << "ID value (output to gate id) not defined, line " << line << endl;
						return SYN_ERR;
					}
					if(port == 0){
						cout << "PORT value (output to gate id:PORT) not defined, line " << line << endl;
						return SYN_ERR;
					}
					/* vse v poradku - nacteni jmena prvku */
					state = S_NAME;
				}
				else if(token == T_EOL);
				else{
					cout << "Expecting '<' on line " << line << endl;
					return SYN_ERR;
				}
				break;
			/* Ocekava se klicove slovo OUTPUT */
			case S_OUTPUT:
				if(token == T_K && !attr->compare("OUTPUT")){
					state = S_K;
				}
				else if(token == T_IDIV){
					/* Parsing OUTPUT kompletni, navrat */
					return SYN_OK;
				}
				else{
					cout << "Expecting OUTPUT keyword on line " << line << endl;
					return SYN_ERR;
				}
				break;
			/* Ocekava se klicove slovo atributu */
			case S_K:
				if(token == T_K){
					if(!attr->compare("ID")){
						state = S_E;
						id_indicator = I_ID;
					}
					else if(!attr->compare("PORT")){
						state = S_E;
						id_indicator = I_PORT;
					}
					else{
						cout << "Expecting ID, PORT or VALUE(for GENERATOR) keyword on line " << line << endl;
						return SYN_ERR;
					}
				}
				else if(token == T_RULE_END){
					state = S_NAME;
				}
				else{
					cout << "Expecting ID, PORT or VALUE(for GENERATOR) keyword on line " << line << endl;
					return SYN_ERR;
				}
				break;
			/* Ocekava se znak = */
			case S_E:
				if(token == T_E){
					state = S_QUOTE_S;
				}
				else{
					cout << "Expecting '=' on line " << line << endl;
					return SYN_ERR;
				}
				break;
			/* Ocekava se znak " */
			case S_QUOTE_S:
				if(token == T_QUOTE){
					state = S_INT;
				}
				else{
					cout << "Expecting '\"' on line " << line << endl;
					return SYN_ERR;
				}
				break;
			/* Ocekava se znak 0-9 */
			case S_INT:
				if(token == T_INT){
					/* Rozliseni zda se jedna o nacitani hodnoty ID, nebo PORTU */
					if(id_indicator == I_ID){
						id = atoi(attr->c_str());
						state = S_QUOTE_E;
					}
					else if(id_indicator == I_PORT){
						port = atoi(attr->c_str());
						state = S_QUOTE_E;
					}
					else{
						cout << "Undefined error on line " << line << endl;
						return SYN_ERR;
					}
				}
				else{
					cout << "Expecting INTEGER on line " << line << endl;
					return SYN_ERR;
				}
				break;
			/* Ocekava se znak " */
			case S_QUOTE_E:
				if(token == T_QUOTE){
					/* Nacitani dalsiho parametru */
					state = S_K;
				}
				else{
					cout << "Expecting '\"' on line " << line << endl;
					return SYN_ERR;
				}
				break;
			/* Ocekava se Identifikator */
			case S_NAME:
				if(token == T_ID){
					/* Ulozeni jmena prvku */
					state = S_OUTPUT_END_RULE_START;
				}
				else if(token == T_RULE_START){
					state = S_OUTPUT_END_DIV;
				}
				else{
					cout << "Expecting IDENTIFIER or empty IDENTIFIER on line " << line << endl;
					return SYN_ERR;
				}
				break;
			/* Ocekava se ukonceni propojeni */
			case S_OUTPUT_END_RULE_START:
				if(token == T_RULE_START){
					state = S_OUTPUT_END_DIV;
				}
				else{
					cout << "Expecting '<' on line " << line << endl;
					return SYN_ERR;
				}
				break;
			/* Ocekava se znak / */
			case S_OUTPUT_END_DIV:
				if(token == T_IDIV){
					state = S_OUTPUT_END_K;
				}
				else{
					cout << "Expecting '/' on line " << line << endl;
					return SYN_ERR;
				}
				break;
			/* Ocekava se klicove slovo OUTPUT */
			case S_OUTPUT_END_K:
				if(token == T_K && !attr->compare("OUTPUT")){
					state = S_OUTPUT_END_RULE_END;
				}
				else{
					cout << "Expecting OUTPUT keyword on line " << line << endl;
					return SYN_ERR;
				}
				break;
			/* Ocekava se ukoncovaci znak > */
			case S_OUTPUT_END_RULE_END:
				if(token == T_RULE_END){
					/* Nulovani nactenych parametru - je mozno nacist vice vystupu */
					/* Kontrola parametru */
					if(port == 0){
						cout << "You must specify PORT of the output GATE, line " << line << endl;
						return SYN_ERR;
					}
					else if(id == 0){
						cout << "You must specify output GATE, line " << line << endl;
						return SYN_ERR;
					}
					if(type == GEN_IMPULSE || type == GEN_PERIODICAL){
						/* Pridani informaci o vystupu do generatoru nebo hradla */
						new_generator->set_out(id, port);
						#ifdef NDEBUG
							cout << "<------------Pridavam vystup do GENERATOR #" << new_generator->id << endl;
						#endif
					}
					/* Vsechny informace byli zadany a prvek je hradlem */
					else{
						#ifdef NDEBUG
							cout << "<---------------Pridavam vystup do GATE" << endl;
						#endif
						new_gate->set_out(id, port);
					}
					/* Nulovani parametru a navrat do stavu S_START - muze byt nacteno vice propojeni */
					state = S_START;
					id_indicator = 0;
					id = 0;
					port = 0;
					value = -1;
				}
				else{
					cout << "Expecting '>' on line " << line << endl;
					return SYN_ERR;
				}
				break;
			/* Chyba */
			default:
				cout << "Undefined error on line " << line << endl;
				return SYN_ERR;
				break;
		} /* switch */
	}	/* while */
}	/* parse_xml_output() */

/** Zkontoroluje syntaktickou a semantickou spravnost popisu generatoru v XML souboru 
*@string* attr - pomocny retezec pro ukladani atributu tokenu
*@string* blockname - ukazatel na retezec s nazvem bloku
*@int type - typ prvku - Hradlo/Generator
*@list<gate>* gates - ukazatel na seznam hradel
*@list<generator>* generators - ukazatel na seznam generatoru
*@list<BlockItem>* blocks - ukazatel na seznam bloku
*/
int parse_xml_generator(string* attr, string* blockname, list<gate>* gates, list<generator>* generators, list<BlockItem>* blocks){

	int state = S_START;
	int token = 0;
	int id = 0;
	int type = 0;
	int lambda = 0;
	int time = 0;
	int id_indicator = 0;
	
	/* Vytvoreni noveho prvku do bloku */
	BlockItem new_block;
	new_block.block = blockname->c_str();
	new_block.type = GENERATOR;
	
	/* Struktury pro ulozeni noveho generatoru, new_gate pouze pomocna (prazdna) struktura */	
	generator new_generator;
	gate new_gate;

	#ifdef NDEBUG
		cout << "PARSING GENERATOR" << endl;
	#endif
	
	/* Generator dostava ke zpracovani soubor s prectenymi tokeny <GENERATOR */
	while(1){

		token = get_next_token(attr);

		switch(state){
			/* Pocatecni stav */
			case S_START:
				/* Identifikar musi byt jedno z klicovych slov */
				if(token == T_K){
					state = S_E;
					if(!attr->compare("ID")){
						id_indicator = I_ID;
					}
					else if(!attr->compare("TYPE")){
						id_indicator = I_TYPE;
					}
					else if(!attr->compare("LAMBDA")){
						id_indicator = I_LAMBDA;
					}
					else if(!attr->compare("TIME")){
						id_indicator = I_TIME;
					}
					else{
						cout << "Expecting keyword (ID, TYPE, LAMBDA, TIME) on line " << line << " " << *attr << " is not a keyword" << endl;
						return SYN_ERR;
					}
				}
				/* Konec pravidla - kontrola */
				else if(token == T_RULE_END){
					/* Kontrola, zda byly zadany vsechny parametry */
					if(type == 0){
						cout << "Expecting keyword TYPE on line " << line << endl;
						return SYN_ERR;
					}
					if(id == 0){
						cout << "Expecting keyword ID or 0 entered on line " << line << endl;
						return SYN_ERR;
					}
					if(lambda == 0){
						cout << "Expecting keyword LAMBDA or 0 entered on line " << line << endl;
						return SYN_ERR;
					}
					/* Zapsani informaci o generatoru do struktury */
					#ifdef NDEBUG
						cout << "<---------------NASTAVENI GENERATORU--------------->" << endl;
						cout << "ID " << id << " TYPE " << type << " LAMBDA " << lambda << " TIME " << time << endl;
					#endif
					new_generator.set_generator(type, id, lambda, time);
					
					/* Nacitani vystupu generatoru */
					if(parse_xml_output(attr, type, id, &new_gate, &new_generator, blockname, gates, generators, blocks)){
						/* Naparsovany vystupy generatoru, pokracuje se na GENERATOR keywordu - </ bylo jiz nacteno */
						state = S_GENERATOR_END_K;
					}
					else{
						cout << "ERROR: OUTPUT syntax error on line " << line << endl;
						return SYN_ERR;
					}
				}
				else{
					cout << "Expecting keyword (ID, TYPE, LAMBDA, TIME) on line " << line << endl;
					return SYN_ERR;
				}
				break;
			/* Ocekava se znak = */
			case S_E:
				if(token = T_E){
					state = S_QUOTE_S;
				}
				else{
					cout << "Expecting '=' on line " << line << endl;
					return SYN_ERR;
				}
				break;
			/* Ocekava se znak " */
			case S_QUOTE_S:
				if(token = T_QUOTE){
					state = S_VALUE;
				}
				else{
					cout << "Expecting '\"' on line " << line << endl;
					return SYN_ERR;
				}
				break;
			/* Ocekava se identifikator typu generatoru */
			case S_VALUE:
				if(id_indicator == I_TYPE){
					if(token == T_ID){
						if(!attr->compare("PERIODICAL")){
							type = GEN_PERIODICAL;
							state = S_QUOTE_E;
						}
						else if(!attr->compare("IMPULSE")){
							type = GEN_IMPULSE;
							state = S_QUOTE_E;
						}
						else{
							cout << "Expecting PERIODICAL or IMPULSE type of GENERATOR on line " << line << " " << *attr << " is not a valid type of GENERATOR" << endl;
							return SYN_ERR;
						}
					}
					else{
						cout << "Expecting identifier on line " << line << " " << *attr << " is not a valid identifier" << endl;
						return SYN_ERR;
					}
				}
				/* Kontrola spravnosti parametru */
				else if(id_indicator == I_ID || id_indicator == I_LAMBDA || id_indicator == I_TIME){
					if(token == T_INT){
						if(id_indicator == I_ID){
							id = atoi(attr->c_str());
							if(id == 0){
								cout << "You cannot enter 0 as ID for GENERATOR on line " << line << endl;
								return SYN_ERR;
							}
							state = S_QUOTE_E;
						}
						else if(id_indicator == I_LAMBDA){
							lambda = atoi(attr->c_str());
							if(lambda == 0){
								cout << "You cannot enter 0 as LAMBDA for GENERATOR on line " << line << endl;
								return SYN_ERR;
							}
							state = S_QUOTE_E;
						}
						else if(id_indicator == I_TIME){
							/* Time muze byt 0 */
							time = atoi(attr->c_str());
							state = S_QUOTE_E;
						}
						else{
							cout << "Undefined syntax error on line " << line << endl;
							return SYN_ERR;
						}
					}
					else{
						cout << "Expecting INTEGER as parameter on line " << line << endl;
						return SYN_ERR;
					}
				}
				else{
					cout << "Undefined syntax error on line " << line << endl;
					return SYN_ERR;
				}
				break;
			/* Ocekava se znak " */
			case S_QUOTE_E:
				if(token == T_QUOTE){
					state = S_START;
				}
				else{
					cout << "Expecting '\"' on line " << line << endl;
					return SYN_ERR;
				}
				break;
			/* Ocekava se znak > */
			case S_RULE_END:
				if(token == T_RULE_END){
					if(parse_xml_output(attr, type, id, &new_gate, &new_generator, blockname, gates, generators, blocks)){
						/* Naparsovany vystupy generatoru, pokracuje se na GENERATOR keywordu - </ bylo jiz nacteno */
						state = S_GENERATOR_END_K;
					}
					else{
						cout << "ERROR: OUTPUT syntax error on line " << line << endl;
						return SYN_ERR;
					}
				}
				else{
					cout << "Expecting '>' on line " << line << endl;
					return SYN_ERR;
				}
				break;
			/* Ocekava se klicove slovo GENERATOR */
			case S_GENERATOR_END_K:
				if(token == T_K && !attr->compare("GENERATOR")){
					state = S_GENERATOR_END_RULE_END;
				}
				else{
					cout << "Expecting GENERATOR keyword on line " << line << endl;
					return SYN_ERR;
				}
				break;
			/* Ocekava se ukonceni pravidla generatoru */
			case S_GENERATOR_END_RULE_END:
				if(token == T_RULE_END){
					/* Analyza GENERATOR dokoncena */
					/* Zapsani GENERATOR do seznamu generatoru */
					#ifdef NDEBUG
						cout << "<---------------PRIDANI GENERATORU---------------->" << endl;
					#endif
					if(check_generator_id(id, generators)){
						cout << "Semantic error - generator id '" << new_generator.id << "' already exists. line " << line << endl;
						return SYN_ERR;
					} 
					generators->push_back(new_generator);
					#ifdef NDEBUG
						cout<< "<---------------PRIDANI GENERATORU DO BLOCKU------------>" << endl;
					#endif
					new_block.id = id;
					blocks->push_back(new_block);
					#ifdef NDEBUG
						cout << "GENERATOR #" << generator_global << " id: " << id << "WRITTEN" << endl;
					#endif
					return SYN_OK;
				}
				else{
					cout << "Expecting '>' on line " << line << endl;
					return SYN_ERR;
				}
				break;
			/* Chyba */
			default:
				cout << "Undefined syntax error on line " << line << endl;
				return SYN_ERR;
				break;
		} /* switch */
	} /* while */
} /* parse_xml_generator() */

/** Zkontoroluje syntaktickou a semantickou spravnost popisu hradla v XML souboru 
*@string* attr - pomocny retezec pro ukladani atributu tokenu
*@string* blockname - ukazatel na retezec s nazvem bloku
*@list<gate>* gates - ukazatel na seznam hradel
*@list<generator>* generators - ukazatel na seznam generatoru
*@list<BlockItem>* blocks - ukazatel na seznam bloku
*/
int parse_xml_gate(string* attr, string* blockname, list<gate>* gates, list<generator>* generators, list<BlockItem>* blocks){

	int state = S_START;
	int token = 0;
	/* Identifikace nacteneho id */
	int id_indicator = 0;
	int id = 0;
	int type = 0;
	int inputs = 0;
	int delay = 0;
	
	/* Struktura noveho prvku pro block */
	BlockItem new_block;
	new_block.block = blockname->c_str();
	new_block.type = GATE;
	
	/* Struktury noveho hradla a generatoru - zde generator pouze pomocny */
	gate new_gate;
	generator new_generator;

	#ifdef NDEBUG
		cout << "PARSING GATE" << endl;
	#endif
	
	/* Zacina se tokenem T_K - keyword, ktery obsahuje retezec id, nebo type - <GATE uz bylo nacteno pro rozpoznani */
	while(1){

		token = get_next_token(attr);

		switch(state){

			/* Ocekavame keyword id, type a inputs - vsechny povinne */
			case S_START:
				if(token == T_K){
					/* Token je keyword */
					if(!attr->compare("ID")){
						state = S_E;
						//cout << "id" << endl;
						id_indicator = I_ID;
					}
					else if(!attr->compare("TYPE")){
						state = S_E;
						//cout << "type" << endl;
						id_indicator = I_TYPE;
					}
					else if(!attr->compare("INPUTS")){
						state = S_E;
						//cout << "ins" << endl;
						id_indicator = I_INPUTS;
					}
					else if(!attr->compare("DELAY")){
						state = S_E;
						//cout << "del" << endl;
						id_indicator = I_DELAY;
					}
					else{
						cout << "Expecting ID, TYPE or INPUTS keyword on line " << line << " " << attr << " is not expected keyword." << endl;
						return SYN_ERR;
					}
				}
				else if(token == T_RULE_END){
					/* kontrola, zda byli nacteny vsechny potrebne parametry a parsovani vystupu hradla */
					if(id == 0) {
						cout << "ID parameter not specified, line " << line << endl;
						return SYN_ERR;
					}
					if(type == 0){
						cout << "TYPE parameter not specified, line " << line << endl;
						return SYN_ERR;
					}
					if(inputs == 0){
						cout << "INPUTS parameter not specified, line " << line << endl;
						return SYN_ERR;
					}
					if(delay == 0){
						cout << "DELAY 0 not allowed, line " << line << endl;
						return SYN_ERR;
					}
					/* Nastaveni hradla */
					#ifdef NDEBUG
						cout << "<---------------NASTAVENI HRADLA---------------->" << endl;
						cout << "TYPE " << type << " ID " << id << endl;
					#endif
					new_gate.set_gate(type, id, delay);
					/* Nastaveni vstupnich portu hradla */
					for(int i = 1; i <= inputs; i++){
						#ifdef NDEBUG
							cout << "<----Pridani vstupniho portu #" << i << endl;
						#endif
						new_gate.set_port(i);
					}
					/* potrebne parametry byli nacteny, parsovani OUTPUT */
					if(parse_xml_output(attr, type, id, &new_gate, &new_generator, blockname, gates, generators, blocks)){
						
						state = S_GATE_END;
					}
					else{
						cout << "ERROR: OUTPUT parsing error on line " << line << endl;
						return SYN_ERR;
					}
				}
				else{
					cout << "Expecting ID, TYPE or INPUTS keyword on line " << line << " " << attr << " is not a keyword." << endl;
					return SYN_ERR;
				}
				break;
			/* Ocekava se znak = */
			case S_E:
				if(token == T_E){
					state = S_QUOTE_S;
				}
				else{
					cout << "Expecting '=' on line " << line  << endl;
					return SYN_ERR;
				}
				break;
			/* Ocekava se znak " */
			case S_QUOTE_S:
				/* uvozovky " */
				if(token == T_QUOTE){
					state = S_INT;
				}
				else{
					cout << "Expecting '\"' on line " << line << endl;
					return SYN_ERR;
				}
				break;
			/* Ocekava se cislo typu integer nebo identifikator typu hradla */
			case S_INT:
				/* nacitani parametru identifikatoru */
				if(token == T_INT){
					if(id_indicator == I_ID){
						id = atoi(attr->c_str());						
						state = S_QUOTE_E;
					}
					else if(id_indicator == I_INPUTS){
						inputs = atoi(attr->c_str());
						state = S_QUOTE_E;
					}
					else if(id_indicator == I_DELAY){
						delay = atoi(attr->c_str());
						state = S_QUOTE_E;
					}
					else{
						cout << "Undefined error on line " << line << endl;
						return SYN_ERR;
					}
				}
				/* Jedna se o identifikator typu hradla */
				else if(token == T_ID){
					if(id_indicator = I_TYPE){
						if(!attr->compare("AND")){
							type = AND_G;
						}
						else if(!attr->compare("OR")){
							type = OR_G;
						}
						else if(!attr->compare("NOT")){
							type = NOT_G;
						}
						else if(!attr->compare("NAND")){
							type = NAND_G;
						}
						else if(!attr->compare("NOR")){
							type = NOR_G;
						}
						else{
							cout << "Unknown GATE type - use AND, OR, NOT, NAND and NOR, line " << line << endl;
							return SYN_ERR;
						}
						state = S_QUOTE_E;
					}
					else{
						cout << "Undefined syntax error on line " << line << endl;
					}
				}
				else{
					cout << "Expecting INTEGER OR IDENTIFIER on line " << line << endl;
					return SYN_ERR;
				}
				break;
			/* Ocekava se znak " */
			case S_QUOTE_E:
				/* ukoncovaci uvozovky */
				if(token == T_QUOTE){
					/* ok, nacitani dalsiho parametru */
					state = S_START;
				}
				else{
					cout << "Expecting \" on line " << line << endl;
					return SYN_ERR;
				}
				break;
			/* Ocekava se ukonceni popisu hradla */
			case S_GATE_END:
				/* ukonceni GATE tagu byli nacteny tokeny </ */
				if(token == T_K && !attr->compare("GATE")){
					state = S_GATE_END_RULE_END;
				}
				else{
					cout << "Expecting GATE keyword on line " << line << endl;
					return SYN_ERR;
				}
				break;
			case S_GATE_END_RULE_END:
				if(token == T_RULE_END){
					/* Syntakticka analyza GATE probehla v poradku */
					/* Zapsani hradla do seznamu hradel */
					#ifdef NDEBUG
						cout << "<---------------PRIDANI HRADLA---------------->" << endl;
					#endif
					if(check_gate_id(id, gates)){
						cout << "Semantic error - gate id '" << new_gate.id << "' already exists. line " << line << endl;
						return SYN_ERR;
					} 
					gates->push_back(new_gate);
					#ifdef NDEBUG
						cout<< "<---------------PRIDANI HRADLA DO BLOCKU------------>" << endl;
					#endif
					new_block.id = id;
					#ifdef NDEBUG
						cout << "BLOCKID: " << id << endl;
					#endif
					blocks->push_back(new_block);
					return SYN_OK;
				}
				else{
					cout << "Expecting '>' on line " << line << endl;
					return SYN_ERR;
				}
				break;
			/* Chyba */
			default:
				cout << "Undefined error on line " << line << endl;
				return SYN_ERR;
				break;
		} /* switch */
	} /* while */
}	/* parse_xml_gate */

/** Zkontoroluje syntaktickou a semantickou spravnost popisu vnitrniho bloku v XML souboru 
*@string* attr - pomocny retezec pro ukladani atributu tokenu
*@string* blockname - ukazatel na retezec s nazvem bloku
*@list<gate>* gates - ukazatel na seznam hradel
*@list<generator>* generators - ukazatel na seznam generatoru
*@list<BlockItem>* blocks - ukazatel na seznam bloku
*/
int parse_xml_inner_block(string *attr, string* blockname, list<gate>* gates, list<generator>* generators, list<BlockItem>* blocks){

	int state = S_START;
	int token = 0;
	
	#ifdef NDEBUG
		cout << "PARSING_INNER_BLOCK" << endl;
	#endif
	
	while(1){
		token = get_next_token(attr);

		switch(state){
			/* Pocatecni stav, ocekavani znaku < */
			case S_START:
				if(token == T_RULE_START){
					state = S_K;
				}
				else if(token == T_EOL);
				else{
					cout << "Expecting '<' on line " << line << endl;
					return SYN_ERR;
				}
				break;
			/* Ocekava se klicove slovo GATE, nebo GENERATOR */
			case S_K:
				if(token == T_K && !attr->compare("GATE")){
					if(parse_xml_gate(attr, blockname, gates, generators, blocks)){
						/* Pravidlo zpracovano, nacitani dalsiho */
						state = S_START;
					}
					else{
						cout << "ERROR: GATE syntax error on line " << line << endl;
						return SYN_ERR;
					}
				}
				else if(token == T_K && !attr->compare("GENERATOR")){
					if(parse_xml_generator(attr, blockname, gates, generators, blocks)){
						/* Pravidlo zpracovano, nacitani dalsiho */
						state = S_START;
					}
					else{
						cout << "ERROR: GENERATOR syntax error on line " << line << endl;
						return SYN_ERR;
					}
				}
				/* Konci nacitani vnitrnich prvku blocku a navrat na zpracovani vnejsiho ukonceni v parse_xml_block */
				else if(token == T_IDIV){
					return SYN_OK;
				}
				else{
					cout << "Expecting GENERATOR, GATE or / on line " << line << endl;
					return SYN_ERR;
				}
				break;
			/* Chyba */
			default:
				cout << "ERROR on line " << line << endl;
				return SYN_ERR;
				break;
		}	/* switch */
	}	/* while */
}	/* parse_xml_inner_block() */

/** Zkontoroluje syntaktickou a semantickou spravnost popisu bloku v XML souboru 
*@string* attr - pomocny retezec pro ukladani atributu tokenu
*@list<gate>* gates - ukazatel na seznam hradel
*@list<generator>* generators - ukazatel na seznam generatoru
*@list<BlockItem>* blocks - ukazatel na seznam bloku
*/
int parse_xml_block(string* attr, list<gate>* gates, list<generator>* generators, list<BlockItem>* blocks){

	int state = S_START;
	int token = 0;
	int idcounter = 0;
	
	/* Retezec pro ulozeni nazvu bloku */
	string blockname = "";
	
	BlockItem new_block;

	while(1){

		token = get_next_token(attr);

		switch(state){
			/* Pomocny stav, ktery neni pouzivan, ale do budoucna je zachovan */
			case DUMMY:
				if(token == T_RULE_START){
					state = S_K;
				}
				else if(token == T_EOL);
				else{
					cout << "Expecting '<' on line " << line << endl;
					return SYN_ERR;
				}
				break;
			/* Pocatecni stav - Ocekava se klicove slovo BLOCK */
			case S_START:
				if(token == T_K && !attr->compare("BLOCK")){
					state = S_K_2;
				}
				else{
					cout << "Expecting 'BLOCK' keyword on line " << line << endl;
					return SYN_ERR;
				}
				break;
			/* Ocekavani klicoveho slova NAME */
			case S_K_2:
				if(token == T_K && !attr->compare("NAME")){
					idcounter++;
					/* Naplneni id -> ne nutne */
					state = S_E;
				}
				else{
					cout << "Expecting NAME identifier on line " << line << endl;
					return SYN_ERR;
				}
				break;
			/* Ocekava se znak = */
			case S_E:
				if(token == T_E){
					state = S_QUOTE_S;
				}
				else{
					cout << "Expecting '=' on line " << line << endl;
					return SYN_ERR;
				}
				break;
			/* Ocekava se znak " */
			case S_QUOTE_S:
				if(token == T_QUOTE){
					state = S_ID;
				}
				else{
					cout << "Expecting '\"' on line " << line << endl;
					return SYN_ERR;
				}
				break;
			/* Ocekava se znak Identifikator s nazvem bloku */
			case S_ID:
				if(token == T_ID){
					/* naplneni hodnoty */
					blockname = attr->c_str();
					state = S_QUOTE_E;
				}
				else{
					cout << "Expecting identifier on line " << line << endl;
					return SYN_ERR;
				}
				break;
			/* Ocekava se znak " */
			case S_QUOTE_E:
				if(token == T_QUOTE){
					state = S_RULE_END;
				}
				else{
					cout << "Expecting '\"' on line " << line << endl;
					return SYN_ERR;
				}
				break;
			/* Ocekava se znak > */
			case S_RULE_END:
				if(token == T_RULE_END){
					state = S_INNER_BLOCK_S;
				}
				else{
					cout << "Expecting '>' on line " << line << endl;
					return SYN_ERR;
				}
				break;
			/* Ocekava se popis vnitrniho bloku */
			case S_INNER_BLOCK_S:
				/* Pred volanim parse_xml_inner_block() se provede zapis jmeno do blocku*/
				new_block.block = blockname.c_str();
				/* Volani zpracovani vnitrniho bloku */
				if(parse_xml_inner_block(attr, &blockname, gates, generators, blocks)){
					/* ve zpracovani parse_xml_inner_block() se nacetly tokeny </ - nasleduje BLOCK keyword */
					state = S_K_3; 
				}
				else{
					cout << "ERROR: INNER BLOCK XML syntax error on line " << line << endl;
					return SYN_ERR;
				}
				break;
			/* Ocekava se klicove slovo BLOCK */
			case S_K_3:
				if(token == T_K && !attr->compare("BLOCK")){
					state = S_INNER_BLOCK_E_E;
				}
				else{
					cout << "Expecting BLOCK on line " << line << endl;
					return SYN_ERR;
				}
				break;
			/* Ocekava se znak > ukoncujici popis bloku */
			case S_INNER_BLOCK_E_E:
				if(token == T_RULE_END){
					/* Analyza uspesna */
					return SYN_OK;
				}
				else{
					cout << "Expecting '>' on line " << line << endl;
					return SYN_ERR;
				}
				break;
			/* Chyba */
			default:
				cout << "Syntax error on line " << line << endl;
				return SYN_ERR;
		} /* switch */
	}	/* while */
} /* parse_xml_block() */

/** V prvnim pruchodu provede syntaktickou a semantickou analyzu a naplni seznamy
*   hradel, generatoru a bloku. V druhem pruchodu syntetizuje obvod naplnenim ukazatelu 
*	v propojenich s danymi prvky.
*@FILE *pFile - ukazatel na otevreny vstupni soubor s popisem obvodu ve formatu XML  
*@list<gate>* gates - ukazatel na seznam hradel
*@list<generator>* generators - ukazatel na seznam generatoru
*@list<BlockItem>* blocks - ukazatel na seznam bloku
*/
int parse_xml(FILE* pFile, list<gate>* gates, list<generator>* generators, list<BlockItem>* blocks){

	string attr = "";
	source = pFile;
	int token = 0;
	int status = 0;
	
	cout << "Parsing XML..." ;
	
	if(parse_xml_header(&attr)){
		// parsovani xml block
		// nacteni tokenu - pokud neni prazdny parsuji block
		// pokud je to EOL, nacitam dalsi
		// pokud je to EOF, koncim parsovani
		while(true){
			// kontrola, zda nasleduje dalsi block
			#ifdef NDEBUG
				cout << "PARSING BLOCK" << endl;
			#endif
			token = get_next_token(&attr);
			/* Ocekava se zacatek bloku znakem < */
			if(token == T_RULE_START){
				status = parse_xml_block(&attr, gates, generators, blocks);
				if(status == SYN_ERR){
					cout << "ERROR: XML BLOCK syntax error on line " << line << endl;
					return SYN_ERR;
				}
			}
			// bile znaky
			else if(token == T_EOL) ;
			else if(token == T_EOF){
				//ukonceni cyklu
				break;
			}
			// block nenalezen, kdyz byl ocekavan
			else{
				cout << "ERROR: Wrong XML BLOCK syntax on line " << line << endl;
				return SYN_ERR;
			}
		}
	}
	else{
		cout << "ERROR: Wrong XML header" << endl;
		return SYN_ERR;
	}
	
	/* Ukonceni retezce Parsing XML... na STDOUT */
	cout << "done" << endl;
	
	/* Pruchod naplnenymi listy hradel a naplneni ukazatelu podle na hradla podle ID */
	cout << "Building circuit..." ;
	
	/*---------------------------------------------------------------------------------------------------*/
	/* Pruchod seznamem hradel, precteni vystupu hradla a podle toho pridani adresy do struktury vystupu */
	/*---------------------------------------------------------------------------------------------------*/
	/* Iterator generatory */
	list<generator>::iterator generator_iterator = generators->begin();
	/* Iterator vystupy */
	list<OutputItem>::iterator output_iterator;
	/* Pruchod generatory */
	while(generator_iterator != generators->end()){
		#ifdef NDEBUG
			cout << "Generator ID: " << generator_iterator->id << endl;
		#endif
		/* Pruchod vystupy hradla */
		output_iterator = generator_iterator->outs.begin();
		while(output_iterator != generator_iterator->outs.end()){
			/* ID hradla na vystupu */
			int id = output_iterator->id;
			#ifdef NDEBUG
				cout << "--OUTPUT ID: " << id ;
			#endif
			gate* gate_pointer = NULL;
			/* Nalezeni hradla v seznamu hradel, ziskani jeho adresy a prirazeni */
			list<gate>::iterator find_gate_iterator = gates->begin();
			while(find_gate_iterator != gates->end()){
				if(find_gate_iterator->id == id){
					gate_pointer = &*find_gate_iterator;
					break; // id nalezeno - ukonceni while cyklu
				}
				find_gate_iterator++; // dalsi hradlo
			}
			/* Hradlo s danym ID nenalezeno - chyba */
			if(gate_pointer == NULL){
				cout << "ERROR - Gate id " << id << " not found" << endl;
				return SYN_ERR;
			} 
			/* Hradlo s ID nalezeno, ukazatel v gate_pointer */
			else{
				#ifdef NDEBUG
					cout << " Address: " << gate_pointer;
					cout << " TEST: Addr->ID: " << gate_pointer->id << endl;
				#endif
				/* Zapsani ukazatele na hradlo do vystupu aktualne prochazeneho hradla :) */
				output_iterator->addr = (void*) gate_pointer;
			}
			output_iterator++; // dalsi vystup
		}
		generator_iterator++; // dalsi hradlo
	}
	
	/* Iterator hradly */
	list<gate>::iterator gate_iterator = gates->begin();
	/* Hlavni pruchod hradly */
	while(gate_iterator != gates->end()){
		#ifdef NDEBUG
			cout << "GATE ID: " << gate_iterator->id << endl;
		#endif
		/* Pruchod vystupy hradla */
		output_iterator = gate_iterator->outs.begin();
		while(output_iterator != gate_iterator->outs.end()){
			/* ID hradla na vystupu */
			int id = output_iterator->id;
			#ifdef NDEBUG
				cout << "--OUTPUT ID: " << id ;
			#endif
			gate* gate_pointer = NULL;
			/* Nalezeni hradla v seznamu hradel, ziskani jeho adresy a prirazeni */
			list<gate>::iterator find_gate_iterator = gates->begin();
			while(find_gate_iterator != gates->end()){
				if(find_gate_iterator->id == id){
					gate_pointer = &*find_gate_iterator;
					break; // id nalezeno - ukonceni while cyklu
				}
				find_gate_iterator++; // dalsi hradlo
			}
			/* Hradlo s danym ID nenalezeno - chyba */
			if(gate_pointer == NULL){
				#ifdef NDEBUG
					cout << "ERROR - Gate id " << id << " not found" << endl;
				#endif
				return SYN_ERR;
			} 
			/* Hradlo s ID nalezeno, ukazatel v gate_pointer */
			else{
				#ifdef NDEBUG
					cout << " Address: " << gate_pointer;
					cout << " TEST: Addr->ID: " << gate_pointer->id << endl;
				#endif
				/* Zapsani ukazatele na hradlo do vystupu aktualne prochazeneho hradla :) */
				output_iterator->addr = (void*) gate_pointer;
			}
			output_iterator++; // dalsi vystup
		}
		gate_iterator++; // dalsi hradlo
	}
	/* Doplneni adres (druhy pruchod obvodem) dokoncen */
	cout << "done" << endl << endl;
	
	return SYN_OK;
}
