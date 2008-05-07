#ifndef __GETOPT_H
#define	__GETOPT_H

#define	no_argument			0
#define required_argument	1
#define optional_argument	2

struct option
{
	const char *name;	/* nom de l'option longue	*/
	
	int has_arg;		/*
						 * has_arg vaut : no_argument (ou 0), si l'option ne prend pas d'argument, 
						 * required_argument (ou 1) si l'option prend un argument,
						 * ou optional_argument (ou 2) si l'option prend un argument optionnel. 
						 */
	
	int *flag;			/* sp�cifie la mani�re de renvoyer les r�sultats pour une option longue
						 * si flag vaut NULL, alors getopt_long() renvoie val
						 * (un programme peut par exemple initialiser val avec le caract�re de 
						 * l'option courte correspondante)
						 * dans le cas contraire getopt_long() renvoie 0, et flag pointe sur la  
						 * variable dont le contenu est sp�cifi� dans le champ val lorsque l'option 
						 * est trouv�e mais reste inchang�e si l'option n'est pas trouv�e.
						 */
						 				
	int val;			/* val est la valeur renvoy�e par getopt_long() lorsque le pointeur flag
						 * vaut NULL ou la valeur de la variable r�f�renc�e par le pointeur flag
						 * lorsque l'option est trouv�e.
						 */
};

extern int 
optind;

extern char* 
optarg;

extern int 
opterr;

extern int 
optopt;


int 
getopt (int argc, char * const argv[], const char *optstring);

int
getopt_long(int argc, char * const argv[], const char *optstring, const struct option *longopts, int *longindex);

int
getopt_long_only (int argc, char * const argv[], const char *optstring, const struct option *longopts, int *longindex);


#endif /* !__GETOPT_H */
