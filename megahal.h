#ifndef MEGAHAL_H
#define MEGAHAL_H 1

#define BYTE1 unsigned char
#define BYTE2 unsigned short
#define BYTE4 unsigned long
#define MATH_TYPE real
#define P_THINK 40
#define D_KEY 100000
#define V_KEY 50000
#define D_THINK 500000
#define V_THINK 250000
#define MIN(a,b) ((a)<(b))?(a):(b)
#define COOKIE "MegaHALv8"
#define TIMEOUT 1
#define DEFAULT "."

#ifdef SUNOS
extern double drand48(void);
extern void srand48(long);
#endif

typedef enum
{
	UNKNOWN, QUIT, EXIT, SAVE, DELAY, HELP, SPEECH,
	VOICELIST, VOICE, BRAIN, QUIET, DICT,
} OPCODES;

class STRING 
{
public:
	static void free_word(STRING);
	static void save_word(FILE *, STRING);
	static int wordcmp(STRING, STRING);

public:
    BYTE1 length;
    char *word;
};

class OPCODE
{
public:
    STRING word;
    char *helpstring;
    OPCODES command_id;
	static int COMMAND_SIZE();
};

class DICTIONARY;

namespace config
{
	extern OPCODE commands[];
	extern bool typing_delay;
	extern bool noprompt;
	extern bool speech;
	extern bool quiet;
	extern bool nowrap;
	extern bool nobanner;

	extern int width;
	extern int order;
	extern FILE *errorfp;
	extern FILE *statusfp;

	extern DICTIONARY *ban;
	extern DICTIONARY *aux;
	extern DICTIONARY *fin;

	extern char *errorfilename;
	extern char *statusfilename;
};

class real;

class megahal
{
public:
	static void setnoprompt(void);
	static void setnowrap (void);
	static void setnobanner (void);
	static void seterrorfile(char *filename);
	static void setstatusfile(char *filename);
	static void initialize(void);
	static char *do_reply(char *input, int log);
	static char *initial_greeting(void);
	static void output(char *output);
	static char *input(char *input);
	static int command(char *command);
	static void cleanup(void);
};

class DICTIONARY
{
public:
	static DICTIONARY *allocate ();
	static bool dissimilar(DICTIONARY *, DICTIONARY *);
	static OPCODES execute_command(DICTIONARY *, int *);
	static DICTIONARY *new_dictionary(void);
	static DICTIONARY *initialize_list(char *filename);
	static void free_dictionary(DICTIONARY *);
	static void load_word(FILE *, DICTIONARY *);
	
	BYTE2 add_word(STRING word);
	BYTE2 find_word(STRING);
	void changevoice(int);
	void free_words();
	void initialize_dictionary();
	void load_dictionary(FILE *file);
	void make_greeting ();
	char *make_output();
	void make_words (char *);
	void save_dictionary(FILE *file);
	bool word_exists(STRING word);
	void show_dictionary();

protected:
	int search_dictionary(STRING, bool *);
	
public:
//	BYTE4 size;
	int	 size;
    STRING *entry;
    BYTE2 *index;
};


class SWAP
{
protected:
	static SWAP *new_swap(void);
	void add_swap(char *, char *);

public:
	static void free_swap(SWAP *);
	static SWAP *initialize_swap(char *);
	
    BYTE2 size;
    STRING *from;
    STRING *to;
};

class TREE;

class NODE
{
public:
    BYTE2 symbol;
    BYTE4 usage;
    BYTE2 count;
    BYTE2 branch;
    NODE **tree;
	operator TREE* () { return reinterpret_cast<TREE*>(this); }
};

class TREE: public NODE
{
public:	
	static TREE *allocate ();
	static TREE *new_node(void);
	static int search_node(TREE *, int, bool *);
	static void free_tree(TREE *);
	void load_tree(FILE *file);
	static void save_tree(FILE *, TREE *);
	int search_node(int symbol, bool *found_symbol);
	TREE *find_symbol(int symbol);
	TREE *add_symbol(BYTE2 symbol);
	void add_node(TREE *node, int position);
	TREE *find_symbol_add(BYTE2 symbol);
	operator NODE* () { return reinterpret_cast<NODE*>(this); }
};

class MODEL
{
public:
	static void load_personality(MODEL **);
	static void change_personality(DICTIONARY *, int, MODEL **);
	static DICTIONARY *make_keywords(MODEL *, DICTIONARY *);
	static MODEL *allocate ();
	static MODEL *new_model(int);
	static void free_model(MODEL *);
	bool load_model(char *);
	void save_model(char *modelname);
	void add_aux(DICTIONARY *, STRING);
	void add_key(DICTIONARY *, STRING);
	char *generate_reply(DICTIONARY *);
	void initialize_context();
	void learn (DICTIONARY *);
	void train(char *filename);
	DICTIONARY *make_keywords(DICTIONARY *words);

protected:
	int babble(DICTIONARY *, DICTIONARY *);
	MATH_TYPE evaluate_reply(DICTIONARY *, DICTIONARY *);
	DICTIONARY *reply(DICTIONARY *keys);
	int seed(DICTIONARY *keys);
	void update_context(int symbol);
	void update_model(int);

public:
    BYTE1 order;
    TREE *forward;
    TREE *backward;
    TREE **context;
    DICTIONARY *dictionary;
};

namespace intrinsics
{
	bool boundary(char *, int);
	void capitalize(char *);
	void delay(char *);
	void die(int);
	void error(char *, char *, ...);
	char *format_output(char *output);
	void help(void);
	void ignore(int);
	bool initialize_error(char *);
	bool initialize_status(char *);
	void listvoices(void);
	bool print_header(FILE *);
	bool progress(char *, int, int);
	char *read_input(char *);
	void speak(char *);
	bool status(char *, ...);
	void typein(char);
	void upper(char *);
	bool warn(char *, char *, ...);
	void write_input(char *);
	void write_output(char *);
	int rnd(int);
	void exithal(void);
	void usleep (int period);
};

#endif /* MEGAHAL_H  */
