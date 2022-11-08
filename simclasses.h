#ifndef SIMCLASSES_H
#define SIMCLASSES_H

#include <iostream>
#include <list>
#include <cstdio>

using namespace std;

/* Urceni konstant vyctovych typu chyby */
enum error
{
	ERR_XML_SYNTAX,  // syntakticka chyba v xml souboru
	EOK,  // vse OK
	ERR_PARAMS,  // chyba v parametrech
	ERR_FILE,  // neexistujici soubor
	ERR_IFACE,  // chybne rozhrani
	HELP  // napoveda
};

/* Struktura parametru programu */
typedef struct Parameter {
	char *path;  // cesta k souboru pravidel
	int simTime;  // simulacni cas
	int output;  // typ zobrazeni vysledku
} ParameterItem;

/* Struktura portu hradla */
typedef struct Port {
	int id;
	int state;
} PortItem;


/* Struktura parametru programu */
typedef struct Output {
	int id;
	void *addr;
	int port;
} OutputItem;


/* Struktura pro zaznam do kalendare udalosti */
typedef struct EventCalendar {
	int type;	// typ prvku
	void *addr;	// id prvku
	int lambda;	// cas (lambda)
} EventCalendarItem;


/* Struktura pro zaznam do kalendare udalosti */
typedef struct Block {
	string block;	// nazev bloku
	int type;	// typ prvku
	int id;	// id prvku
} BlockItem;


/* Struktura pro uchovani vystupu v prubehu simulace pro vsechny prvky obvodu */
typedef struct OutData {
	int value;
	int lambda;
} OutDataItem;


/* Typy vystupu - zobrazeni vystupnich hodnot */
enum output_types
{
	TERMINAL,
	SAVE,
	GNUPLOT
};


/* Typy hradel */
enum gate_types
{
	AND_G = 30,
	NAND_G,
	OR_G,
	NOR_G,
	NOT_G
};


/* Typy prvku */
enum element_types
{
	GENERATOR = 20,
	GATE
};


/* Typy generatoru */
enum generator_types
{
	GEN_PERIODICAL = 10,
	GEN_IMPULSE
};


/* Typy logickych hodnot */
enum logical
{
	FALSE,
	TRUE,
	UNDEF
};


/* Zakladni trida hradla */
class generator {
public:

	int id;	// identifikacni cislo generatoru
	int type;	// typ generatoru
	int output;  // vystup generatoru
	int lambda;  // pocet intervalu, ve kterych se generuje signal
	int time;	// cas pro spusteni generatoru
	bool activatePrint;	

	// list vystupu
	list<OutputItem> outs;  
	OutputItem new_out;  // pomocny out

	// list historie
	list<OutDataItem> history;
	OutDataItem new_history;	// pomocna struktura pro naplneni noveho zaznamu

	// inicializace
	generator() {
		id = 0;
    type = 0;
		output = FALSE;
		lambda = 0;
		time = 0;
		activatePrint = TRUE;
	}
	
	// vytiskne nastaveni generatoru na STDOUT
	void print(){
		cout << "<------------------NASTAVENI GENERATORU------------------>" << endl;
		cout << "TYPE: " << type << endl;
		cout << "ID: " << id << endl;
		cout << "OUTPUT: " << output << endl;
		cout << "LAMBDA: " << lambda << endl;
		cout << "TIME: " << time << endl; 
		/*cout << "INPUTS: " << endl;
		list<PortItem>::iterator p = ports.begin();
		while(p != ports.end()){
			cout << "--ID: " << p->id << " STATE: " << p->state << endl;
			p++;
		}*/
		cout << "OUTPUTS: " << endl;
		list<OutputItem>::iterator p1 = outs.begin();
		while(p1 != outs.end()){
			cout << "--ID: " << p1->id << " PORT: " << p1->port << endl;
			p1++;
		} 
		cout << "HISTORY: " << endl;
		list<OutDataItem>::iterator p2 = history.begin();
		//while(p2 != history.end()){
			//cout << "--VALUE: " << p2->value << " LAMBDA: " << p2->lambda << endl;
			//p2++;
		//} 
		cout << "<-------------KONEC NASTAVENI GENERATORU------------->" << endl; 	
	}

	void print_history(FILE* output){
		list<OutDataItem>::iterator p = history.begin();
		while(p != history.end()){
			fprintf(output, "%d %d\n", p->lambda, p->value);
		}
	}

	// nastaveni zakladnich parametru (id, typ generatoru)
	void set_generator(int n_type, int n_id, int n_lambda, int n_time) {
		type = n_type;
		id = n_id;
		lambda = n_lambda;
		time = n_time;
	}

	// metoda vrati zakladni nastaveni generatoru
	void get_setting(int *n_type, int *n_id, int *n_lambda, int *n_time) {
		*n_type = type;
		*n_id = id;
		*n_lambda = lambda;
		*n_time = time;
	}

	// nastaveni vystupu (hradla a porty) 
	void set_out(int n_id, int n_port) {
		new_out.id = n_id;
		new_out.port = n_port;
		outs.push_back(new_out);
	}

	// ziskani historie
	int get_history_gnu(int time_1){
		list<OutDataItem>::iterator p = history.begin();
		int value = UNDEF;
		if(time_1 == -1){
			if(time == 1) return 1;
			else return 0;
		}
		while((time_1 >= p->lambda) && (p!=history.end())){
						//cout << "value gen: " << value << endl;
			if(time_1 == p->lambda) return p->value;
			value = p->value;
			p++;
		}
		return p->value;
	}

	// ziska cas, na ktery je potreba generatoru umistit do kalendare udalosti
	int get_lambda(int n_lambda) {
		if (type == GEN_PERIODICAL) {
			if (n_lambda == 0) {
				if (time == 0) {
					return lambda;
				}
				else {
					return time;
				}
			}
			else {
				return lambda + n_lambda;
			}
		}

		if (type == GEN_IMPULSE) {
			if (n_lambda == time && time == 0) {
				return lambda;
			}

			if (n_lambda < time) {
				return time;
			}
			else {
				return lambda + n_lambda;
			}
		}
	}


	// metoda nastavujici vystupni signal generatoru podle aktualniho simulacniho casu a typu generatoru
	void set_output(int n_lambda) {
	
		if (type == GEN_PERIODICAL) {

				if (n_lambda == 0 && time != 0) {
					output = FALSE;
					return;
				}

				if (output == TRUE) {
					output = FALSE;
				}
				else {
					output = TRUE;
				}
			
		}

		if (type == GEN_IMPULSE) {

			// pokud je pocatecni cas mensi roven lambda & pocatecni cas+perioda je vetsi nez lambda -> nastav 1
			if ((time <= n_lambda) && ((time + lambda) > n_lambda)) {
				output = TRUE;
			}
			else {
				output = FALSE;
			}
		}
	}

  // metoda vraci aktualne generovany signal generatorem
	int get_output() {
		return output;
	}

	// metoda pro ziskani vsech napojeni na prislusne hradla a jejich porty
	void get_connection(list<OutputItem> *n_out) {
		*n_out = outs;
	}

	// metoda pro uchovani historie vystupu generatoru
	void set_history(int n_lambda) {
		new_history.value = output;
		new_history.lambda = n_lambda;
		history.push_back(new_history);
	}

	// metoda pro pro ziskani historie vystupu
	void get_history(list<OutDataItem> *n_history) {
		*n_history = history;
	}

	void activate_print_on() {
		activatePrint = FALSE;
	}

	bool get_activate_print() {
		return activatePrint;
	}

	// vrati aktualni nastaveni prvku
	void print_gen(void ){
		cout << "GENERATOR SETTINGS" << endl;
		cout << "0TYPE: " << type << endl;
		cout << "ID: " << id << endl;
		cout << "LAMBDA: " << lambda << endl;
		cout << "TIME: " << time << endl;
		cout << endl;
	}
};


/* Zakladni trida hradla */
class gate {
public:

	int type;	// typ hradla
	int id;	// identifikacni cislo hradla
	int delay; // zpozdeni hradla
	int act_lambda;
	bool activatePrint;

	list<int> output;  // seznam vystupu hradla
	int hOutput;  // pomocny vystup

	bool getLock;  // stav, kdy je mozne hradlo pridat do pomocneho seznamu pro zpracovani vstupu

	// list vstupnich portu
	list<PortItem> ports;  
	PortItem new_port;  // pomocny port

	// list vystupu
	list<OutputItem> outs;  
	OutputItem new_out;  // pomocny out

	// list historie
	list<OutDataItem> history;
	OutDataItem new_history;	// pomocna struktura pro naplneni noveho zaznamu

	// inicializace
	gate() {
		id = 0;
    type = 0;
		//output.push_back(UNDEF);  // vlozeni pocatecni undef hodnoty na vystup hradla
		act_lambda = 0;
		hOutput = UNDEF;
		getLock = TRUE;
		activatePrint = TRUE;
	}

	// odemceni moznosti pridavat hradlo do pomocneho listu pro zpracovani jejich vystupu
	void lock_add_off() {
		getLock = TRUE;
	}

	// zamceni moznosti pridavat hradlo do pomocneho listu pro zpracovani jejich vystupu
	void lock_add_on() {
		getLock = FALSE;
	}

	// ziskani stavu moznosti pridavat hradlo do pomocneho listu pro zpracovani jejich vystupu
	bool get_lock_add() {
		return getLock;
	}
	
	// vytiskne nastaveni hradla na STDOUT
	void print(){
		cout << "<------------------NASTAVENI HRADLA------------------>" << endl;
		cout << "TYPE: " << type << endl;
		cout << "ID: " << id << endl;
		cout << "OUTPUT: " << output.front() << endl;
		cout << "INPUTS: " << endl;
		list<PortItem>::iterator p = ports.begin();
		while(p != ports.end()){
			cout << "--ID: " << p->id << " STATE: " << p->state << endl;
			p++;
		}
		cout << "OUTPUTS: " << endl;
		list<OutputItem>::iterator p1 = outs.begin();
		while(p1 != outs.end()){
			cout << "--ID: " << p1->id << " PORT: " << p1->port << endl;
			p1++;
		} 
		cout << "HISTORY: " << endl;
		list<OutDataItem>::iterator p2 = history.begin();
		while(p2 != history.end()){
			cout << "--VALUE: " << p2->value << " LAMBDA: " << p2->lambda << endl;
			p2++;
		} 
		cout << "<-------------KONEC NASTAVENI HRADLA------------->" << endl; 	
	}
	
	// nastaveni pristupovych portu hradla
	void set_port(int port) {  	
		new_port.id = port;  // cislo portu
		new_port.state = UNDEF;	// inicializace vzdy na nedefinovanou hodnotu
		ports.push_back(new_port);  // vlozeni do listu
	}

	// nastaveni zakladnich parametru (id, typ hradla)
	void set_gate(int n_type, int n_id, int n_delay) {
		type = n_type;
		id = n_id;
		delay = n_delay;
	}

	// nastaveni vystupu (hradla a porty) 
	void set_out(int n_id, int n_port) {
		new_out.id = n_id;
		new_out.port = n_port;
		outs.push_back(new_out);
	}

	void delete_output() {
		output.pop_front();  // odstraneni vystupu
	}

  // metoda vraci hodnotu na vystupu z hradla
	int get_output() {
		hOutput = output.front();
		return hOutput;
	}

	void print_history(FILE* output, float shift){
		int step = 2;
		list<OutDataItem>::iterator p = history.begin();
		int i = 0;
		while(p != history.end()){
			i++;
			fprintf(output, "%d %f\n", p->lambda, float(p->value+1) + shift);
			p++;
		}
	}

	int get_history_gnu(int time){
		list<OutDataItem>::iterator p = history.begin();
		int value = UNDEF;
		//cout << "value gate: " << value << endl;
		if(time == -1) return UNDEF;
		while((time >= p->lambda) && (p!=history.end())){
			//cout << "lambda: " << p->lambda << "time: " << time << "value: " << p->value  << endl;
			if(time == p->lambda) return p->value;
			value = p->value;
			p++;
		}
		return p->value;
	}

	int get_first_output() {
		return output.front();
	}

	// zpracovani hodnot na portech podle logicke operace AND
	int proccess() {

		list<PortItem>::iterator p = ports.begin();
		int h_state = -1;

		switch(type) {

			// zpracovani logickeho clenu AND podle jeho nastaveni vstupnich portu
			case AND_G:
				// pruchod listem portu a jejich hodnot
				while(p != ports.end()) {
					if (p->state == UNDEF) {
						h_state = UNDEF;
					}
					if (p->state == FALSE) {
						return FALSE;
					}
					p++;  // posun v listu
				}
        if(h_state == UNDEF){
					return UNDEF;
				}
				else {
					return TRUE;
				}			
	
				break;

			// zpracovani logickeho clenu OR podle jeho nastaveni vstupnich portu
			case OR_G:
				// pruchod listem portu a jejich hodnot
				while(p != ports.end()) {

					if (p->state == UNDEF) {
						h_state = UNDEF;
					}
					if (p->state == TRUE) {
						return TRUE;
					}
					p++;  // posun v listu
				}

				if (h_state == UNDEF) {
					return UNDEF;
				}
				else {
					return FALSE;
				}			
	
				break;

			// zpracovani logickeho clenu NAND podle jeho nastaveni vstupnich portu
			case NAND_G:
				// pruchod listem portu a jejich hodnot
				while(p != ports.end()) {

					if (p->state == UNDEF) {
						h_state = UNDEF;
					}
					if (p->state == FALSE) {
						return TRUE;
					}

					p++;  // posun v listu
				}

				if (h_state == UNDEF) {
					return UNDEF;
				}
				else {
					return FALSE;
				}			
	
				break;

			// zpracovani logickeho clenu NOR podle jeho nastaveni vstupnich portu
			case NOR_G:
				// pruchod listem portu a jejich hodnot
				while(p != ports.end()) {

					if (p->state == UNDEF) {
						h_state = UNDEF;
					}
					if(p->state == TRUE) {
            return FALSE;
          }
					p++;  // posun v listu
				}

				if (h_state == UNDEF) {
					return UNDEF;
				}
				else {
					return TRUE;
				}			
	
				break;

			// zpracovani logickeho clenu NOT podle jeho nastaveni vstupnich portu
			case NOT_G:
				// pruchod listem portu a jejich hodnot
				while(p != ports.end()) {

					if (p->state == UNDEF) {
						return UNDEF;
					}
					if (p->state == TRUE) {
						return FALSE;
					}
					if (p->state == FALSE) {
						return TRUE;
					}					

					p++;  // posun v listu
				}		
	
				break;

			default:
				break;

		}
	}
	
  // metoda pro nastaveni typu hradla
	void set_type(int type) {
		switch(type) {

			case AND_G:
				type = AND_G;
				break;

			case OR_G:
				type = OR_G;
				break;

			case NAND_G:
				type = NAND_G;
				break;

			case NOR_G:
				type = NOR_G;
				break;

			case NOT_G:
				type = NOT_G;
				break;

			default:
				break;

		}
	}

	void activate_print_on() {
		activatePrint = FALSE;
	}

	bool get_activate_print() {
		return activatePrint;
	}

	// metoda pro ziskani vsech napojeni na prislusne hradla a jejich porty
	void get_connection(list<OutputItem> *n_out) {
		*n_out = outs;
	}

	// metoda pro uchovani historie vystupu hradla
	void set_history(int n_lambda) {
		new_history.value = hOutput;
		new_history.lambda = n_lambda;
		history.push_back(new_history);
	}

	// metoda pro pro ziskani historie vystupu
	void get_history(list<OutDataItem> *n_history) {
		*n_history = history;
	}

	// nastaveni hodnoty na prislusny port
	void set_value_to_port(int n_port, int n_value) {  	

		list<PortItem>::iterator p = ports.begin();

		// pruchod listem portu a jejich hodnot
		while(p != ports.end()) {
			if (p->id == n_port) {	// vyhledani portu
				p->state = n_value;	// nastaveni hodnoty
				break;
			}
			p++;  // posun v listu
		}
	}

};

#endif
