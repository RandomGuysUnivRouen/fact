#include "fact.h"

#define NTHREADS 2

#define __mpz_inp_raw mpz_inp_raw
#define __mpz_out_raw mpz_out_raw

extern int input_type;

int level = 2;

double now()
{
    struct timeval t;
    gettimeofday(&t, NULL);
    return (double) t.tv_sec + (double) t.tv_usec / 1000000.;
}

// Initialisation du vecteur
void init_vec(vec_t *v, int count){
    assert(v);
    v->count = count;
    v->el = malloc(count * sizeof(mpz_t));
    assert(v->el);
    for (int i=0; i< v->count; i++)
        mpz_init(v->el[i]);
}

// Libération
void free_vec(vec_t *v){
    assert(v);
    for(int i=0; i < v->count ; i++) {
        mpz_clear(v->el[i]);
	}
    free(v->el);
}

// Récupération d'un fichier binaire et stockage dans vecteur
void input_bin_array(vec_t *v, char * filename){
	char tmp_out[100];
	sprintf(tmp_out, "%s%s", DIR, filename);
	FILE *in = fopen(tmp_out, "rb");
	assert(in);
	int count;
	int ret = fread(&count, sizeof(count), 1, in);
	assert(ret == 1);
	assert(count >= 0);
	init_vec(v, count);
	size_t bytes = 0;
	
	for (int i=0; i < count; i++){
		bytes += __mpz_inp_raw(v->el[i], in);
		
	}
		
		
	fclose(in);
}

// writes vec_t *v to the named file in binary format
void output_bin_array(vec_t *v, char *filename) {
	char tmp_out[100];
	sprintf(tmp_out, "%s%s", DIR, filename);
	FILE *out = fopen(tmp_out, "wb");
	if (out == NULL) {
		fprintf(stderr, "This file does not exist: %s\n", tmp_out);
		exit(EXIT_FAILURE);
	}
	fwrite(&v->count, sizeof(v->count), 1, out);
	size_t bytes = 0;
	for (int i=0; i < v->count; i++)
		bytes += __mpz_out_raw(out, v->el[i]);
	fclose(out);
}

// Fonction transformant un fichier texte clair en texte binaire
void transformFile(char *file_to_transform) {
	if (input_type != INPUT_DECIMAL && input_type != INPUT_HEXA) {
		fprintf (stderr, "Input type unknown\n");
		exit(EXIT_FAILURE);
	}
	FILE *in = fopen(file_to_transform, "r");
	if (in == NULL) {
		fprintf(stderr, "This file does not exist: %s\n", file_to_transform);
		exit(EXIT_FAILURE);
	}
	int count_l = 0;
	char* fileoutname = (char*) malloc (FILE_NAME * sizeof(char));
	sprintf(fileoutname, "%sPInterm1", DIR);

	FILE *out = fopen(fileoutname, "wb");
	mpz_t biginteger;
	mpz_init(biginteger);
	fwrite(&count_l, sizeof(count_l), 1, out);
	int l = 0;
	while(1) {
		int res = -1;
		if (input_type == INPUT_HEXA)
			res = gmp_fscanf(in, "%Zx", biginteger);
		else if (input_type == INPUT_DECIMAL)
			res = gmp_fscanf(in, "%Zd", biginteger);
		if (res == EOF) {
			break;
		} else if (res != 1) {
			fprintf(stderr, "Failed to read line #%d\n", l);
			exit(EXIT_FAILURE);
		} else {
			count_l++;
			mpz_out_raw(out, biginteger);	
		}
		l++;
	}
	int nearest_exp = (int) (ceil(log(count_l)/log(2)));
	int count_l2 = (int) pow (2., (double) nearest_exp);
	printf ("Padding with %d ones...\n", count_l2 - count_l);
	mpz_set_ui (biginteger, 1);
	for (int i = 0; i < count_l2 - count_l; i++) {
		mpz_out_raw (out, biginteger);
	}
	mpz_clear(biginteger);
	fclose(in);
	rewind(out);
	fwrite(&count_l2, sizeof(count_l2), 1, out);
	fclose(out);
}

// Affiche le produit final en clair
void printFinalProduct() {
	char tmp[100];
	level = level - 1;		// Final
	sprintf(tmp, "%sPInterm%d", DIR, level);
	level = level - 1;		// Last intermediate
	char final[50];
	sprintf(final, "mv %s %sPFinal", tmp, DIR);
	FILE* out2 = fopen(tmp, "rb");
	int count;
	int ret = fread(&count, sizeof(count), 1, out2);
	assert(ret == 1);
	mpz_t bigintegergmp;
	mpz_init(bigintegergmp);

	__mpz_inp_raw(bigintegergmp, out2);
	
	mpz_clear(bigintegergmp);
	fclose(out2);
	system(final);
}

// Fonction pour l'arbre produit (Attention : le fichier en entrée doit être un fichier clair)
int buildProductTree (char *moduli_filename) {
	// Ouverture du fichier
	FILE* moduli = fopen(moduli_filename, "r");
	if (moduli == NULL) {
		printf("buildProducTree: Fichier introuvable\n");
		exit(EXIT_FAILURE);
	}
	vec_t v;
	transformFile(moduli_filename);
	sprintf(moduli_filename, "PInterm1");
	input_bin_array(&v, moduli_filename);
	
	printf("Building product tree...\n");
	int start = now ();
	while (v.count > 1) {
		vec_t w;
		init_vec(&w,(v.count+1)/2);

		void mul_job(int i) {
			mpz_mul(w.el[i], v.el[2*i], v.el[2*i+1]);
		}
		iter_threads(0, v.count/2, mul_job);
		if (v.count & 1)
			mpz_set(w.el[v.count/2], v.el[v.count-1]); 

		char name[255];
		snprintf(name, sizeof(name)-1, "PInterm%d", level);
		output_bin_array(&w, name);

		free_vec(&v);
		v = w;
		level++;
	}
	printf ("Done in %0.10fs\n", now () - start);


	free_vec(&v);	
	return 0;
}

void iter_threads(int start, int end, void (*func)(int n)) {
	int n = start;
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

	void *thread_body(void *ptr) {
	while (1) {
		pthread_mutex_lock(&mutex);
		int i = n++;
		pthread_mutex_unlock(&mutex);
		if (i >= end)
			break;
		func(i);
	}

	return NULL;
	}

	pthread_t thread_id[NTHREADS];
	for (int i=0; i < NTHREADS; i++)
		pthread_create(&thread_id[i], NULL, thread_body, NULL);

	for (int i=0; i < NTHREADS; i++)
		pthread_join(thread_id[i], NULL);
}

int buildRemainderTree () {
	int secL;
	int sizeP=1; // à modifier, ceci peut se récupérer dans le fichier
	
	// On charge le premier élément P dans notre liste de previous


	vec_t modCur,gcd,v;
	vec_t modPre,prodL;
	init_vec(&modPre,sizeP);	
	input_bin_array(&modPre,"PFinal");
	
	// ETAPE 1 : calcul des modulos
	secL=level + 1;
	char str[15]="";
	printf("Building remainder tree...\n");
	int start = now ();
	do {
		// Création du vecteur current contenant les modulos calculés
		sizeP *= 2; 
		init_vec(&modCur,sizeP);	
	
		// Création du vecteur prod contenant la ligne des produits correspodant	
		init_vec(&prodL,sizeP);	
		sprintf(str, "PInterm%d",secL - 1);
		input_bin_array(&prodL,str);	

		void mul_job(int j){			
			mpz_pow_ui(prodL.el[j],prodL.el[j],2); // v= v^2
			mpz_mod(modCur.el[j],modPre.el[j/2],prodL.el[j]); // sol = produit mod v			
		}
		iter_threads(0, prodL.count, mul_job);
		
		// On assigne tous les éléments de modCur à modPre (on vide modPre avant)
		modPre = modCur;
		free_vec (&prodL);
	} while (--secL > 1);
	
	// ETAPE N : div, puis gcd
	init_vec(&gcd,modCur.count);
	
	input_bin_array (&v,"PInterm1");
	void mul_job(int j){
		mpz_divexact(modCur.el[j],modCur.el[j],v.el[j]); // sol = sol / item 
		mpz_gcd (gcd.el[j],modCur.el[j],v.el[j]); // gcd = pgcd(sol,item)
	}
	iter_threads(0, v.count, mul_job);
	
	printf ("Done in %0.10fs\n", now () - start);
	
	vec_t potentielVuln, premiers;
	init_vec (&potentielVuln, gcd.count);
	init_vec (&premiers, gcd.count);
	mpz_t q;
	mpz_init (q);
	int pvuln = 0, ppremier = 0;
	printf ("Identification des premiers et potentiels moduli vulnérables...\n");
	start = now ();
	for (int i = 0; i < gcd.count; i++) {
		if (mpz_cmp_ui (gcd.el[i], 1) != 0) {
			mpz_set (potentielVuln.el[pvuln++], v.el[i]);
			if (mpz_probab_prime_p (gcd.el[i], 25) != 0) {
				mpz_set (premiers.el[ppremier++], gcd.el[i]);
			}
		}
	}
	printf ("Done in %0.10fs\n", now () - start);
	potentielVuln.count = pvuln;
	premiers.count = ppremier;
	
	printf ("Identification des moduli vulnérables...\n");
	FILE *result = fopen ("moduli_p_q", "w");
	start = now ();
	for (int i = 0; i < potentielVuln.count; i++) {
		for (int j = 0; j < premiers.count; j++) {
			if (mpz_divisible_p (potentielVuln.el[i], premiers.el[j]) != 0) {
				mpz_divexact (q, potentielVuln.el[i], premiers.el[j]);
				if (mpz_probab_prime_p (q, 25) != 0) {
					if (input_type == INPUT_HEXA) 
						gmp_fprintf (result, "%ZX, %ZX, %ZX\n", potentielVuln.el[i], premiers.el[j], q);
					else
						gmp_fprintf (result, "%Zd, %Zd, %Zd\n", potentielVuln.el[i], premiers.el[j], q);
				}
			}
		}
	}
	printf ("Done in %0.10fs\n", now () - start);
	free_vec (&potentielVuln);
	free_vec (&premiers);
	fclose (result);
	free_vec (&gcd);
	free_vec (&v);

	return 0;
}
