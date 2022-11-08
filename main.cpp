/*
 * Soubor:  main.cpp
 * Datum:   2011/2012
 * Autor:   Tomas Smetka, xsmetk01@stud.fit.vutbr.cz
 * Login:   xsmetk01
 * Projekt: Simulator cislicovych obvodu
 */


#include <iostream>
#include <list>
#include <ctime>
#include "simclasses.h"
#include "parser.cpp"


using namespace std;


/* Chybová hlaseni odpovidajici chybovym kodum. */
const char *ARRAY_ERR[] =
{
	"**XML Syntax ERROR.\n", // ERR_XML_SYNTAX
  "Everything OK.\n",  // EOK
  "**Wrong argument!\n",  // ERR_PARAMS
  "**No such file!\n",  // ERR_FILE
  "**Undefined error!.\n"  // ERR_NEZNAMA
};
 
/* Pole pro pozdejsi indexaci a tisk napovedy */
const char *ARRAY_HELP =
  "simgate - Assignment for IMS - VUT FIT Brno\n"
  "Copyright (c) 2011 Knapovsky Martin, Smetka Tomas\n\n"
  //"THERE IS ABSOLUTELY NO WARRANTY FOR THIS PROGRAM.\n\n"
  "Usage: ./gatesim -t %simulation_time -f %input_file -o %output_type [-h]\n"
  "       -t %simulation_time - defines length of simulation - integer\n"
  "       -f %input_file      - circuit definition XML file\n"
  "       -o %output_type     - choose from \'text\' printed to STDOUT \n"
//  "                             \'gnuplot\' for gnuplot visualisation file\n"
  "       -h                  - prints this help\n";

/* Funkce pro tisk chybove hlasky */ 
void error_print(int state)
{

	fprintf(stderr, "%s", ARRAY_ERR[state]);  // tisk chybove hlasky na stderr
	//printf("%s\n", ARRAY_HELP);  // tisk napovedy

}


/* Funkce zpracovavajici parametry */
int getParams(int argc, char **argv, ParameterItem *parameters)
{

	int opt;
	int multiParS = 0, multiParF = 0, multiParP = 0;  // osetreni vicenasobneho zadani stejneho parametru

	while ((opt = getopt(argc, argv, "ht:o:f:")) != -1) {  // zpracovani parametru pomoci funkce getopt
		switch (opt) {
			case 'h':
				if (argc == 2) {
					return HELP;
        }
				else {
           return ERR_PARAMS;
        } 
				break;

			case 't':
				if (multiParS == 0) {  
					parameters->simTime = atoi(optarg);  // nacteni simulacniho casu
					multiParS = 1;  
        }
     	  else {
					return ERR_PARAMS;
				}
        break;

			case 'o':
				if (multiParP == 0) { 
					if (strcmp("text",optarg) == 0) {
						parameters->output = TERMINAL;
					}
					else if (strcmp("gnuplot",optarg) == 0) {
						parameters->output = SAVE;
					}
					else {
						return ERR_PARAMS;
					}
					multiParP = 1;  
        }
     	  else {
					return ERR_PARAMS;
				}
        break;

			case 'f':
				if (multiParF == 0) {  
					parameters->path = optarg;  // nacteni cesty k souboru s obvodem
					multiParF = 1;  
        }
     	  else {
					return ERR_PARAMS;
				}
        break;

      case '?':
        return ERR_PARAMS;

      default:
        break;
    }
	}

	return EOK;

}


/* Funkce testuje existenci a moznost cteni ze zadaneho souboru */
FILE* file_check(ParameterItem *parameters, FILE *pFile)
{

	FILE* pFile_1;

	pFile_1 = fopen (parameters->path, "r");

	if (pFile_1 != NULL) {
		return pFile_1;
	} 
	else {
		return NULL;
  }

}


/* funkce inicializuje kalendar udalosti - naplni jen generatory signalu */
int initialize_generators(list<generator> *generators, int simTime, list<EventCalendarItem> *event_calendar)
{

	// promenne pro vytazeni nastaveni generatoru signalu
	int g_id = 0, g_type = 0, g_lambda = 0, g_time = 0;

	// pomocny zaznam kalendare
	EventCalendarItem calendarRecord;

	// pruchod listem generatoru a zpracovani kazdeho zvlast
	generator *gen;

	for(list<generator>::iterator it=generators->begin(); it!=generators->end(); it++) {

    gen = &*it;
		gen->get_setting(&g_type, &g_id, &g_lambda, &g_time);
		//cout << "type: " << g_type << " id: " << g_id << " lambda: " << g_lambda << " time: " << g_time << endl;

		// vlozeni do kalendare
		calendarRecord.type = GENERATOR;
		calendarRecord.addr = gen;
		calendarRecord.lambda = 0;
		event_calendar->push_back(calendarRecord);

  }

	exit;
	return EOK;

}


// funkce ulozi vsem prvkum jejich stavy do historie
void save_history (list<gate> *gates, list<generator> *generators, int lambda)
{

	// vystupni hodnota generatoru predavana vystupnim portum hradel

	list<generator>::iterator p = generators->begin();
	
	while (p != generators->end()) {
		// ulozeni historie vystupu hradla
		p->set_history(lambda);
		p++;
	}

	// vystupni hodnota generatoru predavana vystupnim portum hradel

	list<gate>::iterator pi = gates->begin();
	
	while (pi != gates->end()) {
		// ulozeni historie vystupu hradla
		pi->set_history(lambda);
		pi++;
	}

}


/* Nastaveni vystupu pro generatory */
void sim_set_gen_output(list<EventCalendarItem> *event_calendar, list<generator> * generators, generator *gen, int lambda)			 
{
	
	//int output;
	int gLambda;

	list<EventCalendarItem>::iterator cp;

	// nastaveni hodnoty do output podle typu generatoru a jeho nastaveni
	gen->set_output(lambda);
	gLambda	= gen->get_lambda(lambda);  // ziskani casu, na kdy nastavit generator do kalendare udalosti

	// vlozeni generatoru do kalendare na novy cas
	// promenne pro vytazeni nastaveni generatoru signalu
	int g_id = 0, g_type = 0, g_lambda = 0, g_time = 0;

	EventCalendarItem calendarRecord;
	EventCalendarItem lambdaLast;

	gen->get_setting(&g_type, &g_id, &g_lambda, &g_time);

	// vlozeni do kalendare
	calendarRecord.type = GENERATOR;
	calendarRecord.addr = gen;
	calendarRecord.lambda = gLambda;

	// zjisteni lambdy posledniho prvku v seznamu
	lambdaLast = event_calendar->back();

	// pokud se jedna o impulsni generator, ktery dokoncil svuj impuls, nepridavej jej do kalendare udalosti
	if (gen->type == GEN_IMPULSE && ((gen->lambda + gen->time) < gLambda)) {
		return;
	}

	if (lambdaLast.lambda <= gLambda) {
		event_calendar->push_back(calendarRecord);  // posledni prvek ma mensi nebo roven cas nez aktualne planovany, vloz na konec
	}
	else {  // vyhledej stejnou hodnotu nebo vetsi a vloz pred ni
  	for (cp = event_calendar->begin(); cp != event_calendar-> end(); cp++) {
			if (cp->lambda >= gLambda) {
				event_calendar->insert(cp, calendarRecord);
				return;
			}
		}
	}

	return;

}


/* Funkce pro odeslani vystupnich hodnot z generatoru	na hradla */
void sim_gen_output_sent(list<gate *> *actGates, generator *gen, int lambda)				 
{

	// vystupni hodnota generatoru predavana vystupnim portum hradel
	int output = 0;

	// ukazatel na objekt typu gate
	gate *ga; 

	// ziskani vystupni hodnoty z hradla
	output = gen->get_output();  

	// ulozeni do historie
	gen->set_history(lambda);

	// ziskani ukazatele na list vystupnich propojeni (hradla + porty)
	list<OutputItem> outs;
	gen->get_connection(&outs);

	// nastaveni vsech vystupnich portu hradel na hodnotu vystupu generatoru
	list<OutputItem>::iterator point = outs.begin();
	
	while (point != outs.end()) {
		ga = (gate *) point->addr;  // ziskani adresy hradla
		ga->set_value_to_port(point->port, output);  // zapsani vystupni hodnoty generatoru na port hradla

		// kontrola zda se hradlo jiz nachazi v pomocnem seznamu
		if (ga->get_lock_add()) {	

			// zamknuti moznosti zapisu hradla do pomocneho listu hradel
			ga->lock_add_on();

			// zarazeni hradla do pomocneho listu
			actGates->push_back(ga);
		}
		point++;
	}

}	


/* Funkce pro odeslani vystupnich hodnot z generatoru	na hradla */
void sim_gate_output_sent(list<gate *> *actGates, gate *gat, int lambda)				 
{

	// vystupni hodnota generatoru predavana vystupnim portum hradel
	int output = 0;

	// ukazatel na objekt typu gate (pro zapisovani na vstupy hradel)
	gate *ga; 

	// ziskani vystupni hodnoty z hradla
	//if (gat->output.size() != 0) {
		output = gat->get_output();

		// ulozeni do historie
		gat->set_history(lambda);

		gat->delete_output(); 
	//}

	// ziskani ukazatele na list vystupnich propojeni (hradla + porty)
	list<OutputItem> outs;
	gat->get_connection(&outs);

	// nastaveni vsech vystupnich portu hradel na hodnotu vystupu generatoru
	list<OutputItem>::iterator point = outs.begin();
	
	while (point != outs.end()) {
		ga = (gate *) point->addr;  // ziskani adresy hradla
		ga->set_value_to_port(point->port, output);  // zapsani vystupni hodnoty generatoru na port hradla

		// kontrola zda se hradlo jiz nachazi v pomocnem seznamu
		if (ga->get_lock_add()) {	

			// zamknuti moznosti zapisu hradla do pomocneho listu hradel
			ga->lock_add_on();

			// zarazeni hradla do pomocneho listu
			actGates->push_back(ga);
		}
		point++;
	}

}	


/* Funkce zpracovava pomocny list hradel, kterym byla zapsana alespon jedna hodnota na vstupni port */
void proccess_act_gates(list<EventCalendarItem> *event_calendar, list<gate *> *actGates, int lambda)
{

	EventCalendarItem calendarRecord;  // pomocna struktura pro naplneni novym zaznamem
	EventCalendarItem lambdaLast;  // ziskani lambdy posledniho prvku v kalendari udalosti

	list<EventCalendarItem>::iterator cp;  // iterator pro pruchod kalendarem udalosti

	gate *ga;  // ukazatale na objekt typu gate

	int output;  // ziskani vystupu z nove nastavenych portu

	bool save = FALSE;  // pomocna promenna pro rozhodnuti, zda vlozit do kalendare

	// projdi vsechna hradla u kterych byla provedena zmena na vstupu
	while (actGates->size() != 0) {
		ga = actGates->front();  // ziskani adresy hradla

		output = ga->proccess();  // zpracovani vstupnich portu

		// odemkni moznost pridavat hradlo do pomocneho listu
		ga->lock_add_off();

		// pokud je list vystupu prazdny
		if (ga->output.size() == 0) {
			if (output != ga->hOutput) {  // a nova vystupni hodnota se nerovna pomocne
				// pokud se nerovnaji uloz hradlo do kalendare a pushni novy output
				save = TRUE;
			}
		} 
		else {
			if (ga->output.back() != output) {  // pokud neni prazdny, zkontroluj jeho nasledujici vystup
				// pokud se nerovnaji uloz hradlo do kalendare a pushni novy output
				save = TRUE;
			}
		}

		// pokud se nerovna vloz novou hodnotu na konec outputu
		if (save) {

			ga->output.push_back(output);

			// vlozeni do kalendare
			calendarRecord.type = GATE;
			calendarRecord.addr = ga;
			calendarRecord.lambda = ga->delay + lambda;  // vloz hradlo do kalendare udalosti na pozici aktualni lambda + jeho delay

			// zjisteni lambdy posledniho prvku v seznamu
			lambdaLast = event_calendar->back();
				
			if (lambdaLast.lambda <= ga->delay + lambda) {
				event_calendar->push_back(calendarRecord);  // posledni prvek ma mensi nebo roven cas nez aktualne planovany, vloz na konec
			}
			else {  // vyhledej stejnou hodnotu nebo vetsi a vloz pred ni
				for (cp = event_calendar->begin(); cp != event_calendar->end(); cp++) {
					if (cp->lambda >= ga->delay + lambda) {
						event_calendar->insert(cp, calendarRecord);
						break;
					}
				}
			}
		}

		// odstraneni zpracovaneho hradla z pomocneho seznamu
		actGates->pop_front();
	}

}


/* Funkce provadejici simulaci na zvolene topologii obvodu */
int simulation(list<gate> *gates, list<generator> *generators, ParameterItem *parameters)
{

	// navratova hodnota funkci
	int state = EOK;

	//vytvoreni kalendare udalosti
	list<EventCalendarItem> event_calendar;

	// pomocny seznam pro uchovani hradel, kterym prisla na port hodnota
	list<gate *> actGates;

	//naplneni kalendare udalosti generatory signalu
	state = initialize_generators(generators, parameters->simTime + 1, &event_calendar);
	if (state != EOK) {
		error_print(state);
		return state;
	}

	//simulacni cas
	int simTime = parameters->simTime;

	// iterator pro pruchod kalendarem udalosti
	list<EventCalendarItem>::iterator cp;
	list<EventCalendarItem> *calendar;
	calendar = &event_calendar;

	// pomocne promenne pro pristup k prvkum simulace
	gate *ga;
	generator *gen;


	//**************************************************
	// start simulace
	//**************************************************
	for (int lambda = 0; lambda < simTime+1; lambda++) {

		// nastav vystupy generatoru a hradel ktere jsou v aktualnim case lambda v kalendari udalosti
  	for (cp = calendar->begin(); cp != calendar->end(); cp++) {
			if (cp->lambda == lambda) {  // vybirani prvku z kalendare s odpovidajicim aktualnim casem lambda
				if (cp->type == GENERATOR) {
					sim_set_gen_output(&event_calendar, generators, (generator *) cp->addr, lambda);	 // nastaveni vystupu pro generatory				 
				}	
			}
			else {
				break;  // uz jsme v kalendari za simulacnim casem
			}
		}

		// odeslani hodnot z generatoru a hradel na prislusne porty
  	for (cp = calendar->begin(); cp != calendar->end(); cp++) {
			if (cp->lambda == lambda) {  // vybirani prvku z kalendare s odpovidajicim aktualnim casem lambda
				if (cp->type == GENERATOR) {
					sim_gen_output_sent(&actGates, (generator *) cp->addr, lambda);	 // odeslani vystupnich hodnot z generatoru
				}			
				else {  // jedna se o hradlo
					sim_gate_output_sent(&actGates, (gate *) cp->addr, lambda);	 // odeslani vystupnich hodnot z generatoru
				}	

					//calendar->pop_front();
			}
			else {
				break;
			}
		}
			
		// pruchod pomocnym listem hradel urcenym pro kontrolu jejich zmenenych vstupu
		proccess_act_gates(&event_calendar, &actGates, lambda);

		// ulozeni stavu vsech prvku do historie
		//save_history(gates, generators, lambda);

		// odstraneni zaznamu z kalendare v case lambda
		cp = calendar->begin();

 		while (cp != calendar->end()) {
			if (cp->lambda == lambda) {  // vybirani prvku z kalendare s odpovidajicim aktualnim casem lambda
				cp = calendar->erase(cp);  // odstraneni prvku	
			}
			else {
				break;
			}
		}
	}

	return EOK;

}


void gate_name(char *name, int type) 
{

	switch (type) {
		
		case AND_G:
			strcpy(name,"AND");
			break;

		case NAND_G:
			strcpy(name,"NAND");
			break;

		case OR_G:
			strcpy(name,"OR");
			break;

		case NOR_G:
			strcpy(name,"NOR");
			break;

		case NOT_G:
			strcpy(name,"NOT");
			break;

		default:
			break;
	}

}


FILE* create_gnuplot_files(string filename, int simtime, list<gate>* gates, list<generator>* generators, list<BlockItem>* blocks)
{
	
	/* Osetreni */
	if(gates == NULL || generators == NULL || blocks == NULL) return NULL;
	FILE* gnuplot = fopen(filename.c_str(), "w");
	FILE* simdata = fopen("simdata", "w");
	if(gnuplot == NULL || simdata == NULL){
		cout << "Cannot create file for gnuplot simulation!" << endl;
		return NULL;
	}
	
	list<gate>::iterator gate_iter = gates->begin();
	list<generator>::iterator generator_iter = generators->begin();
	int i = 0;
	int id = 0;
	int id_and = 0;
	int id_or = 0;
	int id_not = 0;
	int id_nand = 0;
	int id_nor = 0;
	int id_periodical = 0;
	int id_impulse = 0;
	int gate_counter = 0;
	
	fprintf(gnuplot, "#This is a gnuplot setting file for gatesim\n");
	fprintf(gnuplot, "#This file includes simulation.dat, that must be places in the same directory\n");
	fprintf(gnuplot, "set xlabel \"Simulation Time\"\n");
	fprintf(gnuplot, "set ylabel \"Gates and Generators\"\n");
	fprintf(gnuplot, "set xtic 0,1\n");
	fprintf(gnuplot, "set ytics (");
	while(generator_iter != generators->end()){
		string gen_name;
		int type = generator_iter->type;
		switch(type){
			case GEN_PERIODICAL: gen_name = "PER"; id_periodical++; id = id_periodical; break;
			case GEN_IMPULSE: gen_name = "IMP"; id_impulse++; id = id_impulse; break;
			default: return NULL; break;
		}
		fprintf(gnuplot, "\"%s_%d\" %f, ", gen_name.c_str(), id, i + 0.5);
		i++;
		generator_iter++;
	}
	while(gate_iter != gates->end()){
		string gatename;
		int type = gate_iter->type;
		switch(type){
			case AND_G: gatename = "AND"; id_and++; id = id_and; break;
			case OR_G: gatename = "OR"; id_or++; id = id_or; break;
			case NOT_G: gatename = "NOT"; id_not++; id = id_not; break;
			case NAND_G: gatename = "NAND"; id_nand++; id = id_nand; break;
			case NOR_G: gatename = "NOR"; id_nor++; id = id_nor; break;
			default: return NULL; break;
		} 
		fprintf(gnuplot, "\"%s_%d\" %f", gatename.c_str(), id, i + 0.5);
		i++;
		gate_iter++;
		if(gate_iter != gates->end()) fprintf(gnuplot, ", ");
		gate_counter++;
	}
	fprintf(gnuplot, ")\n");
	fprintf(gnuplot, "set grid\n");
	fprintf(gnuplot, "plot ");
	
	i = 0;
	id = 0;
	id_and = 0;
	id_or = 0;
	id_not = 0;
	id_nand = 0;
	id_nor = 0;
	id_periodical = 0;
	id_impulse = 0;
	gate_iter = gates->begin();
	generator_iter = generators->begin();
	while(generator_iter != generators->end()){
		string gen_name;
		int type = generator_iter->type;
		switch(type){
			case GEN_PERIODICAL: gen_name = "PER"; id_periodical++; id = id_periodical; break;
			case GEN_IMPULSE: gen_name = "IMP"; id_impulse++; id = id_impulse; break;
			default: return NULL; break;
		}
		fprintf(gnuplot, "\"simdata\" using 1:%d title \'%s_%d\' with steps%s", i+2, gen_name.c_str(), id, (gate_counter>0)?(", "):(" "));
		i++;
		generator_iter++;
	}
	while(gate_iter != gates->end()){
		string gatename;
		int type = gate_iter->type;
		switch(type){
			case AND_G: gatename = "AND"; id_and++; id = id_and; break;
			case OR_G: gatename = "OR"; id_or++; id = id_or; break;
			case NOT_G: gatename = "NOT"; id_not++; id = id_not; break;
			case NAND_G: gatename = "NAND"; id_nand++; id = id_nand; break;
			case NOR_G: gatename = "NOR"; id_nor++; id = id_nor; break;
			default: return NULL; break;
		} 
		fprintf(gnuplot, "\"simdata\" using 1:%d title \'%s_%d\' with steps", i+2, gatename.c_str(), id);
		i++;
		gate_iter++;
		if(gate_iter != gates->end()) fprintf(gnuplot, ", ");
	}
					//cout << "here2" << endl;
//return gnuplot;
	
	/* Prochazim vsechny casy lambda - > az do konce simulace */
	int current_time = -1;
	while(current_time < simtime){
		fprintf(simdata, "%d ", current_time + 1);
		/* Pruchod vsemy prvky a vytisknuti jejich hodnoty v danem case */
		list<generator>::iterator iter_gen = generators->begin();
		int x = 0;
		while(iter_gen != generators->end()){
			list<OutDataItem>::iterator p = iter_gen->history.begin();
			//int value = UNDEF;
			while(p != iter_gen->history.end()){
				
				cout << "time: " << p->lambda << "value: " << p->value << endl;
				p++;
			}
			return gnuplot;
			int value = iter_gen->get_history_gnu(current_time);
			float temp;
			if(value == 2) temp = 0.5 +x;
			else if(value == 1) temp = 0.9 +x;
			else temp = 0.1 +x;
			//cout << "time" << current_time << "valgen " << value << endl;
			fprintf(simdata, "%f ", temp);
			iter_gen++;
			x++;
		}
		list<gate>::iterator iter = gates->begin();
		//x = 0;
		while(iter != gates->end()){
			int value = iter->get_history_gnu(current_time);
			float temp;
			//cout << "time" << current_time << "val " << value << endl;
			if(value == 2) temp = 0.5 +x;
			else if(value == 1) temp = 0.9 +x;
			else temp = 0.1 +x;
			fprintf(simdata, "%f ", temp);
			iter++;
			x++;
		}
		fprintf(simdata, "\n");
		//cout <<  "cur time: " << current_time << endl;
		current_time++;
	}
	
	return gnuplot;
	
}
	
void wait_for_key ()
{
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__TOS_WIN__)  // every keypress registered, also arrow keys
    cout << endl << "Press any key to continue..." << endl;

    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
    _getch();
#elif defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
    cout << endl << "Press ENTER to continue..." << endl;

    std::cin.clear();
    std::cin.ignore(std::cin.rdbuf()->in_avail());
    std::cin.get();
#endif
    return;
}


/**/
void plot_data(list<gate> *gates, list<generator> *generators, list<BlockItem> *blocks, ParameterItem *parameters)
{

	int typeOfPrint = parameters->output;
	int lambda = parameters->simTime;

	bool bottom = false;

	list<BlockItem>::iterator b;	// blocks
	list<gate>::iterator g;	// hradla
	list<generator>::iterator n;	// generatory

	list<OutDataItem>::iterator h;	//history
	list<OutputItem> outs;

	string blockNameOld;  // uchovani predchoziho nazvu bloku
	char gateName[6];  // nazev hradla
	char genName[6];  // nazev hradla

	int helpPrint;

	switch (typeOfPrint) {
			case TERMINAL:

				// prochazeni vsech zaznamu z bloku
				for (b = blocks->begin(); b != blocks->end(); b++) {


					//cout << b->block << endl;


					// kontrola zda jiz byl tisten nazev obvodu, jinak tisk
					string blockName = b->block;
			
					if (strcmp(blockName.c_str(), blockNameOld.c_str()) != 0) {
						if (bottom) {  // tisk spodni linky tabulky
							for (int i = 0; i <= 4 * lambda + 12; i++) {
								if (i >= 8) {
									cout << "_";
								} 
								else {
									cout << " ";
								}
							}
							cout << endl << "lambda  "; 
							for (int i = 0; i <= lambda; i++) {
								if (i < 10) {
									printf("%3d ", i);
								}
								else {
									printf("%4d", i);
								}
							}
							cout << endl << endl;
						}

						cout << "***" << blockName << "***" << endl;
					
						// tisk horni linky tabulky
						for (int i = 0; i <= 4 * lambda + 12; i++) {
							if (i >= 8) {
								cout << "_";
							} 
							else {
								cout << " ";
							}
						}
						cout << endl;
						blockNameOld = blockName;
						bottom = true;
					} 

					// Rozhodnuti mezi hradlem a generatorem
					if (b->type == GATE) {
						for (g = gates->begin(); g != gates->end(); g++) {

							// bylo nalezeno hradlo
							if (b->id == g->id) {

								// ziskani jmena hradla
								gate_name(gateName, g->type);
								sprintf(gateName,"%s%d",gateName, g->id);
								printf("%s%*s", gateName, (int)(9 - strlen(gateName)),"-|");
								// ziskani historie vystupu hradla

								h = g->history.begin();
								// pokud nenajdes, tiskni FALSE, jinak tiskni historii
								for (int i = 0; i <= lambda; i++) {
									if (h->lambda == i) {
										cout << " " << h->value << " " << "|";
										helpPrint = h->value;
										g->activate_print_on();
										h++;	
									} 	
									else {
										if (g->get_activate_print()) {
											cout << " " << 2 << " " << "|";
										}
										else {
											cout << " " << helpPrint << " " << "|";
										}
									}
								}
								cout << endl;	
							}
						}
					}				
					else {	// generator
						for (n = generators->begin(); n != generators->end(); n++) {

							// byl nalezen generator
							if (b->id == n->id) { 

								// ziskani jmena generatoru
								
								sprintf(genName,"GEN%d", n->id);
								printf("%s%*s", genName, (int)(9 - strlen(genName)),"-|");

								// ziskani historie vystupu hradla
								h = n->history.begin();

								// pokud nenajdes, tiskni FALSE, jinak tiskni historii
								for (int i = 0; i <= lambda; i++) {
									if (h->lambda == i) {
										cout << " " << h->value << " " << "|";
										helpPrint = h->value;
										n->activate_print_on();
										h++;
									} 	
									else {
										if (n->get_activate_print()) {
											cout << " " << 0 << " " << "|";
										}
										else {
											cout << " " << helpPrint << " " << "|";
										}
									}
								}
								cout << endl;	
							}
						}
					}
				}
			
				// tiskspodni posledni linky tabulky
				for (int i = 0; i <= 4 * lambda + 12; i++) {
					if (i >= 8) {
						cout << "_";
					} 
					else {
						cout << " ";
					}
				}
				cout << endl << "lambda  "; 
				for (int i = 0; i <= lambda; i++) {
					if (i < 10) {
						printf("%3d ", i);
					}
					else {
						printf("%4d", i);
					}
				}
				cout << endl << endl;
				break;

			case SAVE:

				time_t rawtime;
				struct tm *timeinfo;

				time(&rawtime);
				timeinfo = localtime(&rawtime);

				char fileName[100];

				// pocet roku od 1900
				timeinfo->tm_year = timeinfo->tm_year + 1900;
				//cout << "here" << endl;gatesim:%d%d%d:%d:%d:%d.dat
				sprintf(fileName, "test",timeinfo->tm_mday, timeinfo->tm_mon, timeinfo->tm_year, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
				FILE* temp = create_gnuplot_files(fileName, parameters->simTime, gates, generators, blocks); 

				cout << endl << fileName << endl;

				break;


		}


}


int main(int argc, char **argv)
{

	// error na zacatek inicializuji jako 0 (bez chyb)
	int state = EOK;  

	// struktura parametru
	ParameterItem parameters;

	// zpracovani parametru
	if (argc == 2 || argc == 7) {  // bud vsechny parametry nebo napoveda
		if ((state = getParams(argc, argv, &parameters)) != EOK) {  // zpracovani parametru
			if (state == HELP) { // byla volana napoveda a ukonci program
        		printf("%s", ARRAY_HELP);
        		return EOK;
      		}
			else {  // chybne parametry
        		state = ERR_PARAMS;
			}
		}
	} 
	else {  // chybny pocet parametru
		state = ERR_PARAMS;
	}
	
	if (state != EOK) {
		error_print(state);
		return state;
	}

	// ukazatel na soubor
	FILE * pFile = NULL;

	// otestovani existence souboru
	pFile = file_check(&parameters, pFile);
	//cout << "STATE: " << state << endl;
	if (pFile == NULL) {
		cout << "Cannot open file " << endl;
		//error_print(state);
		return state;
	}
	
	// vytvoreni seznamu hradel
	list<gate> gates;

	// vytvoreni seznamu generatoru singalu
	list<generator> generators;

	// vytvoreni seznamu block pro poznamenani ktera hradla a generatory patri ke kteremu obvodu
	list<BlockItem> blocks;

	// predani prazdeho seznamu hradel, generatoru, blocku a ukazatele na otevreny soubor pro naplneni daty z XML
	//cout << "SOURCE MAIN " << pFile << endl;
	state = parse_xml(pFile, &gates, &generators, &blocks);
	if (state != EOK) {
		error_print(state);
		return state;
	}

	// zahajeni simulace
	state = simulation(&gates, &generators, &parameters);
	if (state != EOK) {
		error_print(state);
		return state;
	}
	
	// zpracovani ziskanych hodnot
	plot_data(&gates, &generators, &blocks, &parameters);



/******************************************
	//screensaver
	int iSecret, iGuess;
	srand (time(NULL));
	while (1) {
		for (int i = 0; i < 157; i++) {
			iSecret = rand() % 3;
			if (iSecret == 2)
				cout << " ";
			else 
				cout <<  iSecret;
		}
		cout << endl;
		usleep(150000);
	}
*******************************************/


	return state;

}
