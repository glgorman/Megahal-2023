#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#if 0
#include <unistd.h>
#include <getopt.h>
#endif 

#if !defined(AMIGA) && !defined(__mac_os)
#include <malloc.h>
#endif
#include <string.h>
#include <signal.h>
#include <math.h>
#include <time.h>
#include <ctype.h>
#if defined(__mac_os)
#include <types.h>
#include <Speech.h>
#else
#include <sys/types.h>
#endif
#include "calculator_test.h"
#include "megahal.h"
#if defined(DEBUG)
//#include "debug.h"
#endif

#ifdef __mac_os
#define bool Boolean
#endif

#ifdef DOS
#define SEP "\\"
#else
#define SEP "/"
#endif

#ifdef AMIGA
#undef toupper
#define toupper(x) ToUpper(x)
#undef tolower
#define tolower(x) ToLower(x)
#undef isalpha
#define isalpha(x) IsAlpha(_AmigaLocale,x)
#undef isalnum
#define isalnum(x) IsAlNum(_AmigaLocale,x)
#undef isdigit
#define isdigit(x) IsDigit(_AmigaLocale,x)
#undef isspace
#define isspace(x) IsSpace(_AmigaLocale,x)
#endif

namespace config
{
	bool typing_delay=FALSE;
	bool noprompt=FALSE;
	bool speech=FALSE;
	bool quiet=FALSE;
	bool nowrap=FALSE;
	bool nobanner=FALSE;

	int width = 75;
	int order = 5;
	FILE *errorfp = NULL;
	FILE *statusfp = NULL;

	DICTIONARY *ban = NULL;
	DICTIONARY *aux = NULL;
	DICTIONARY *fin = NULL;
	
	char *errorfilename = "megahal.log";
	char *statusfilename = "megahal.txt";
};

/*===========================================================================*/

static DICTIONARY *words=NULL;
static DICTIONARY *greets=NULL;
static MODEL *model0=NULL;

DICTIONARY *grt=NULL;
static SWAP *swp=NULL;
static bool used_key;
static char *directory=NULL;
static char *last=NULL;

OPCODE config::commands[] = 
{
    { { 4, "QUIT" }, "quits the program and saves MegaHAL's brain", QUIT },
    { { 4, "EXIT" }, "exits the program *without* saving MegaHAL's brain", EXIT },
    { { 4, "SAVE" }, "saves the current MegaHAL brain", SAVE },
    { { 5, "DELAY" }, "toggles MegaHAL's typing delay (off by default)", DELAY },
    { { 6, "SPEECH" }, "toggles MegaHAL's speech (off by default)", SPEECH },
    { { 6, "VOICES" }, "list available voices for speech", VOICELIST },
    { { 5, "VOICE" }, "switches to voice specified", VOICE },
    { { 5, "BRAIN" }, "change to another MegaHAL personality", BRAIN },
    { { 4, "HELP" }, "displays this message", HELP },
    { { 5, "QUIET" }, "toggles MegaHAL's responses (on by default)",QUIET},
	{ { 5, "DICT" }, "Print out the current dictionary",DICT},
    /*
      { { 5, "STATS" }, "Display stats", STATS},
      { { 5, "STATS-SESSION" }, "Display stats for this session only",STATS_SESSION},
      { { 5, "STATS-ALL" },"Display stats for the whole lifetime",STATS-ALL},
    */
};

int OPCODE::COMMAND_SIZE()
{
	int sz1 = sizeof (config::commands);
	int sz2 = sizeof (OPCODE);
	int result = sz1/sz2;
	return result;
}

#ifdef AMIGA
struct Locale *_AmigaLocale;
#endif

#ifdef __mac_os
Boolean gSpeechExists = false;
SpeechChannel gSpeechChannel = nil;
#endif

#ifdef __mac_os
static bool initialize_speech(void);
#endif

#ifdef __mac_os
static char *strdup(const char *);
#endif

#if defined(DOS) || defined(__mac_os)
static void usleep(int);
#endif

static char *format_output(char *);
#if 0
static void free_dictionary(DICTIONARY *);
#endif

/*
	Function: setnoprompt
	Purpose: Set noprompt variable.
*/

void megahal::setnoprompt(void)
{
	config::noprompt = TRUE;
}

void megahal::setnowrap (void)
{
    config::nowrap = TRUE;
}

void megahal::setnobanner (void)
{
    config::nobanner = TRUE;
}

void megahal::seterrorfile(char *filename)
{
    config::errorfilename = filename;
}

void megahal::setstatusfile(char *filename)
{
    config::statusfilename = filename;
}

/*
   megahal_initialize --

   Initialize various brains and files.

   Results:

   None.
*/

void megahal::initialize(void)
{
	config::errorfp = stderr;
    config::statusfp = stdout;

    intrinsics::initialize_error(config::errorfilename);
    intrinsics::initialize_status(config::statusfilename);
	intrinsics::ignore(0);

#ifdef AMIGA
    _AmigaLocale=OpenLocale(NULL);
#endif
#ifdef __mac_os
    gSpeechExists = initialize_speech();
#endif
    if(!config::nobanner)
	fprintf(stdout,
		"+------------------------------------------------------------------------+\n"
		"|                                                                        |\n"
		"|  #    #  ######   ####     ##    #    #    ##    #                     |\n"
		"|  ##  ##  #       #    #   #  #   #    #   #  #   #               ###   |\n"
		"|  # ## #  #####   #       #    #  ######  #    #  #              #   #  |\n"
		"|  #    #  #       #  ###  ######  #    #  ######  #       #   #   ###   |\n"
		"|  #    #  #       #    #  #    #  #    #  #    #  #        # #   #   #  |\n"
		"|  #    #  ######   ####   #    #  #    #  #    #  ######    #     ###r6 |\n"
		"|                                                                        |\n"
		"|                    Copyright(C) 1998 Jason Hutchens                    |\n"
		"+------------------------------------------------------------------------+\n"
		);

	words = DICTIONARY::new_dictionary();
    greets = DICTIONARY::new_dictionary();
	MODEL::change_personality(NULL, 0, &model0);
}

/*
   megahal_do_reply --

   Take string as input, and return allocated string as output.  The
   user is responsible for freeing this memory.

*/

char *megahal::do_reply(char *input, int log)
{
    char *output = NULL;

    if (log != 0)
		intrinsics::write_input(input);  /* log input if so desired */

    intrinsics::upper(input);
    words->make_words(input);
    model0->learn(words);
    output = model0->generate_reply(words);
    intrinsics::capitalize(output);
    return output;
}

/*
   megahal_initial_greeting --

   This function returns an initial greeting.  It can be used to start
   Megahal conversations, but it isn't necessary.

*/

char *megahal::initial_greeting(void)
{
    char *output;

    greets->make_greeting();
    output = model0->generate_reply(greets);
    return output;
}

/*
   megahal_output --

   This function pretty prints output.

   Wrapper function to have things in the right namespace.

*/

void megahal::output(char *output)
{
    if(!config::quiet)
	intrinsics::write_output(output);
}

/*
   megahal_input --

   Get a string from stdin, using a prompt.

*/

char *megahal::input(char *prompt)
{
    if (config::noprompt)
	return intrinsics::read_input("");
    else
	return intrinsics::read_input(prompt);
}

/*
   megahal_command --

   Check to see if input is a megahal command, and if so, act upon it.

   Returns 1 if it is a command, 0 if it is not.

*/

int megahal::command(char *input)
{
    int position = 0;
    char *output;

    words->make_words(input);
	OPCODES command_id;
	command_id = DICTIONARY::execute_command(words, &position);
	switch(command_id)
	{
	case EXIT:
		intrinsics::exithal();
		break;
    case QUIT:
		model0->save_model("megahal.brn");
		intrinsics::exithal();
		break;
    case SAVE:
		model0->save_model("megahal.brn");
		break;
    case DELAY:
		config::typing_delay=!config::typing_delay;
		printf("MegaHAL typing is now %s.\n", config::typing_delay?"on":"off");
		return 1;
    case SPEECH:
		config::speech=!config::speech;
		printf("MegaHAL speech is now %s.\n", config::speech?"on":"off");
		return 1;
    case HELP:
		intrinsics::help();
		return 1;
    case VOICELIST:
		intrinsics::listvoices();
		return 1;
    case VOICE:
		words->changevoice(position);
		return 1;
    case BRAIN:
		MODEL::change_personality(words, position, &model0);
		greets->make_greeting();
		output=model0->generate_reply(greets);
		intrinsics::write_output(output);
		return 1;
    case QUIET:
		config::quiet=!config::quiet;
		return 1;
    default:
		return 0;
    }
    return 0;
}

/*
   megahal_cleanup --

   Clean up everything. Prepare for exit.

*/

void megahal::cleanup(void)
{
     model0->save_model("megahal.brn");

#ifdef AMIGA
    CloseLocale(_AmigaLocale);
#endif
}

/*---------------------------------------------------------------------------*/

void MODEL::free_model(MODEL *model)
{
    if(model==NULL) return;
    if(model->forward!=NULL) {
		TREE::free_tree(model->forward);
    }
    if(model->backward!=NULL) {
	TREE::free_tree(model->backward);
    }
    if(model->context!=NULL) {
	free(model->context);
    }
    if(model->dictionary!=NULL) {
	DICTIONARY::free_dictionary(model->dictionary);
	free(model->dictionary);
    }
    free(model);
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	New_Model
 *
 *		Purpose:		Create and initialise a new ngram model.
 */
MODEL *MODEL::new_model(int order)
{
    MODEL *model=NULL;

//  model=(MODEL *)malloc(sizeof(MODEL));
	model = MODEL::allocate ();

    if(model==NULL) {
	intrinsics::error("new_model", "Unable to allocate model.");
	goto fail;
    }

    model->order=order;
	model->forward=TREE::new_node();
	model->backward=TREE::new_node();
    model->context=(TREE **)malloc(sizeof(TREE *)*(order+2));
    if(model->context==NULL) {
		intrinsics::error("new_model", "Unable to allocate context array.");
	goto
		fail;
    }
    model->initialize_context();
	model->dictionary=DICTIONARY::new_dictionary();
    model->dictionary->initialize_dictionary();

    return(model);

fail:
    return(NULL);
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Update_Model
 *
 *		Purpose:		Update the model with the specified symbol.
 */
void MODEL::update_model(int symbol)
{
	MODEL *model = this;
//  register
	int i;

    /*
     *		Update all of the models in the current context with the specified
     *		symbol.
     */
    for(i=(model->order+1); i>0; --i)
	if(model->context[i-1]!=NULL)
	{
	    model->context[i]=model->context[i-1]->add_symbol((BYTE2)symbol);
	}
    return;
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Update_Context
 *
 *		Purpose:		Update the context of the model without adding the symbol.
 */
void MODEL::update_context(int symbol)
{
//  register
	int i;
	MODEL *model = this;
	TREE *tptr;
    for(i=(model->order+1); i>0; --i)
	if(model->context[i-1]!=NULL)
	{
		tptr = model->context[i-1];
	    model->context[i]=tptr->find_symbol(symbol);
	}
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Initialize_Context
 *
 *		Purpose:		Set the context of the model to a default value.
 */
void MODEL::initialize_context()
{
//  register
	int i;
	MODEL *model = this;
    for(i=0; i<=model->order; ++i)
		model->context[i]=NULL;
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Learn
 *
 *		Purpose:		Learn from the user's input.
 */
void MODEL::learn(DICTIONARY *words)
{
	int i, sz;
	MODEL *model = this;
    BYTE2 symbol;

    /*
     *		We only learn from inputs which are long enough
     */
    if(words->size()<=(model->order)) return;

    /*
     *		Train the model in the forwards direction.  Start by initializing
     *		the context of the model.
     */
    model->initialize_context();
    model->context[0]=model->forward;
	sz = words->size();
    for(i=0; i<sz; ++i) {
	/*
	 *		Add the symbol to the model's dictionary if necessary, and then
	 *		update the forward model accordingly.
	 */
	DICTIONARY *d = model->dictionary; 
	symbol=d->add_word((*words)[i]);
	model->update_model(symbol);
    }
    /*
     *		Add the sentence-terminating symbol.
     */
    model->update_model(1);

    /*
     *		Train the model in the backwards direction.  Start by initializing
     *		the context of the model.
     */
    model->initialize_context();
    model->context[0]=model->backward;
	sz = words->size()-1;
    for(i=sz; i>=0; --i) {
	/*
	 *		Find the symbol in the model's dictionary, and then update
	 *		the backward model accordingly.
	 */
	symbol=model->dictionary->find_word((*words)[i]);
	model->update_model(symbol);
    }
    /*
     *		Add the sentence-terminating symbol.
     */
    model->update_model(1);

    return;
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Train
 *
 *		Purpose:	 	Infer a MegaHAL brain from the contents of a text file.
 */
void MODEL::train(char *filename)
{
	MODEL *model = this;
    FILE *file;
    char buffer[1024];
    DICTIONARY *words=NULL;
    int length;

    if(filename==NULL)
		return;

    errno_t err = fopen_s (&file,filename, "r");
    if(file==NULL) {
		printf("Unable to find the personality %s\n", filename);
	return;
    }

    fseek(file, 0, 2);
    length=ftell(file);
    rewind(file);

    words=DICTIONARY::new_dictionary();

    intrinsics::progress("Training from file", 0, 1);
    while(!feof(file)) {

	if(fgets(buffer, 1024, file)==NULL) break;
	if(buffer[0]=='#') continue;

	buffer[strlen(buffer)-1]='\0';

	intrinsics::upper(buffer);
	words->make_words(buffer);
	model->learn(words);

	intrinsics::progress(NULL, ftell(file), length);

    }
    intrinsics::progress(NULL, 1, 1);

    DICTIONARY::free_dictionary(words);
    fclose(file);
}


/*---------------------------------------------------------------------------*/

/*
 *		Function:	Save_Model
 *
 *		Purpose:		Save the current state to a MegaHAL brain file.
 */
void MODEL::save_model(char *modelname)
{
	MODEL *model = this;
    FILE *file;
	int sz;
    static char *filename=NULL;

    if(filename==NULL)
		filename=(char *)malloc(sizeof(char)*1);

	sz = sizeof(char)*(strlen(directory)+strlen(SEP)+12);
    /*
     *    Allocate memory for the filename
     */
    filename=(char *)realloc(filename,sz+1);
    if(filename==NULL)
		intrinsics::error("save_model","Unable to allocate filename");

    model->dictionary->show_dictionary();
    if(filename==NULL)
		return;

    sprintf_s (filename,sz, "%s%smegahal.brn", directory, SEP);
    errno_t err = fopen_s (&file,filename, "wb");
    if(file==NULL) {
	intrinsics::warn("save_model", "Unable to open file `%s'", filename);
	return;
    }

    fwrite(COOKIE, sizeof(char), strlen(COOKIE), file);
    fwrite(&(model->order), sizeof(BYTE1), 1, file);
	TREE::save_tree(file, model->forward);
    TREE::save_tree(file, model->backward);
    model->dictionary->save_dictionary(file);

    fclose(file);
}



/*---------------------------------------------------------------------------*/

/*
 *		Function:	Load_Model
 *
 *		Purpose:		Load a model into memory.
 */
bool MODEL::load_model(char *filename)
{
    FILE *file;
	MODEL *model = this;
    char cookie[16];

    if(filename==NULL)
		return(FALSE);

    errno_t err = fopen_s (&file,filename, "rb");

    if(file==NULL) {
	intrinsics::warn("load_model", "Unable to open file `%s'", filename);
	return(FALSE);
    }

    fread(cookie, sizeof(char), strlen(COOKIE), file);
    if(strncmp(cookie, COOKIE, strlen(COOKIE))!=0) {
	intrinsics::warn("load_model", "File `%s' is not a MegaHAL brain", filename);
	goto fail;
    }

    fread(&(model->order), sizeof(BYTE1), 1, file);
    model->forward->load_tree(file);
    model->backward->load_tree(file);
    model->dictionary->load_dictionary(file);

    return(TRUE);
fail:
    fclose(file);

    return(FALSE);
}

/*---------------------------------------------------------------------------*/
/*
 *    Function:   Generate_Reply
 *
 *    Purpose:    Take a string of user input and return a string of output
 *                which may vaguely be construed as containing a reply to
 *                whatever is in the input string.
 */
char *MODEL::generate_reply(DICTIONARY *words)
{
	MODEL *model = this;
    static DICTIONARY *dummy=NULL;
    DICTIONARY *replywords;
    DICTIONARY *keywords;
    MATH_TYPE surprise;
    MATH_TYPE max_surprise;
    char *output;
    static char *output_none=NULL;
    int count;
    int basetime;
    int timeout = TIMEOUT;

    /*
     *		Create an array of keywords from the words in the user's input
     */
    keywords=model->make_keywords(words);

    /*
     *		Make sure some sort of reply exists
     */
    if(output_none==NULL) {
	output_none=(char*)malloc(40);
	if(output_none!=NULL)
	    strcpy_s(output_none,39,"I don't know enough to answer you yet!");
    }
    output=output_none;
    if(dummy == NULL) 
		dummy = DICTIONARY::new_dictionary();
    replywords = model->reply(dummy);
	if(DICTIONARY::dissimilar(words, replywords) == TRUE)
		output = replywords->make_output();

    /*
     *		Loop for the specified waiting period, generating and evaluating
     *		replies
     */
    max_surprise=(MATH_TYPE)-1.0;
    count=0;
    basetime=time(NULL);
/*     progress("Generating reply", 0, 1);  */
    do {
	replywords= model->reply(keywords);
	surprise=model->evaluate_reply(keywords, replywords);
	++count;
	if((surprise>max_surprise)&&(DICTIONARY::dissimilar(words, replywords)==TRUE)) {
	    max_surprise=surprise;
	    output=replywords->make_output();
	}
/*  	progress(NULL, (time(NULL)-basetime),timeout); */
    } while((time(NULL)-basetime)<timeout);
    intrinsics::progress(NULL, 1, 1);

    /*
     *		Return the best answer we generated
     */
    return(output);
}


/*---------------------------------------------------------------------------*/

/*
 *		Function:	Make_Keywords
 *
 *		Purpose:		Put all the interesting words from the user's input into
 *						a keywords dictionary, which will be used when generating
 *						a reply.
 */
DICTIONARY *MODEL::make_keywords(DICTIONARY *words)
{
	MODEL *model = this;
    static DICTIONARY *keys=NULL;
	int i, j, c, sz1,sz2;

    if(keys==NULL)
		keys=DICTIONARY::new_dictionary();

	sz1 = keys->size();
    for(i=0; i<sz1; ++i)
		free((*keys)[i].word);
    DICTIONARY::free_dictionary(keys);

	sz1 = words->size();
    for(i=0; i<sz1; ++i) {
	/*
	 *		Find the symbol ID of the word.  If it doesn't exist in
	 *		the model, or if it begins with a non-alphanumeric
	 *		character, or if it is in the exclusion array, then
	 *		skip over it.
	 */
	c=0;
	sz2 = swp->size;
	for(j=0; j<sz2; ++j)
	    if(STRING::wordcmp(swp->from[j], (*words)[i])==0) {
		model->add_key(keys, swp->to[j]);
		++c;
	    }
	if(c==0) 
		model->add_key(keys, (*words)[i]);
    }

    if(keys->size()>0)
		for(i=0; i<words->size(); ++i) {

	c=0;
	for(j=0; j<swp->size; ++j)
	    if(STRING::wordcmp(swp->from[j], (*words)[i])==0) {
		model->add_aux(keys, swp->to[j]);
		++c;
	    }
	if(c==0)
		model->add_aux(keys, (*words)[i]);
    }
    return(keys);
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Add_Key
 *
 *		Purpose:		Add a word to the keyword dictionary.
 */
void MODEL::add_key(DICTIONARY *keys, STRING word)
{
    int symbol;
	MODEL *model = this;

    symbol=model->dictionary->find_word(word);
    if(symbol==0)
		return;
    if(isalnum(word.word[0])==0)
		return;
    symbol=config::ban->find_word(word);
    if(symbol!=0)
		return;
    symbol=config::aux->find_word(word);
    if(symbol!=0)
		return;

    keys->add_word(word);
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Add_Aux
 *
 *		Purpose:		Add an auxilliary keyword to the keyword dictionary.
 */
void MODEL::add_aux(DICTIONARY *keys, STRING word)
{
    int symbol;
	MODEL *model = this; 

    symbol=model->dictionary->find_word(word);
    if(symbol==0)
		return;
    if(isalnum(word.word[0])==0)
		return;
    symbol=config::aux->find_word(word);
    if(symbol==0)
		return;

    keys->add_word(word);
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Reply
 *
 *		Purpose:		Generate a dictionary of reply words appropriate to the
 *						given dictionary of keywords.
 */
DICTIONARY *MODEL::reply(DICTIONARY *keys)
{
	MODEL *model = this;
    static DICTIONARY *replies=NULL;
//  register
	int i;
    int symbol;
    bool start=TRUE;

    if(replies==NULL)
		replies=DICTIONARY::new_dictionary();
    
	DICTIONARY::free_dictionary(replies);

    /*
     *		Start off by making sure that the model's context is empty.
     */
    model->initialize_context();
    model->context[0]=model->forward;
    used_key=FALSE;

    /*
     *		Generate the reply in the forward direction.
     */
    while(TRUE) {
	/*
	 *		Get a random symbol from the current context.
	 */
	if(start==TRUE)
		symbol=model->seed(keys);
	else
		symbol=model->babble(keys, replies);
	
	if((symbol==0)||(symbol==1))
		break;
	start=FALSE;

	replies->append ((*model->dictionary)[symbol]);
	/*
	 *		Extend the current context of the model with the current symbol.
	 */
	model->update_context(symbol);
    }

    /*
     *		Start off by making sure that the model's context is empty.
     */
    model->initialize_context();
    model->context[0]=model->backward;

    /*
     *		Re-create the context of the model from the current reply
     *		dictionary so that we can generate backwards to reach the
     *		beginning of the string.
     */
    if(replies->size()>0) for(i=MIN(replies->size()-1, model->order); i>=0; --i) {
	symbol=model->dictionary->find_word((*replies)[i]);
	model->update_context(symbol);
    }

    /*
     *		Generate the reply in the backward direction.
     */
    while(TRUE) {
	/*
	 *		Get a random symbol from the current context.
	 */
	symbol=model->babble(keys, replies);
	if((symbol==0)||(symbol==1))
		break;

	replies->prepend ((*dictionary)[symbol]);

	/*
	 *		Extend the current context of the model with the current symbol.
	 */
	model->update_context(symbol);
    }

    return(replies);
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Evaluate_Reply
 *
 *		Purpose:		Measure the average surprise of keywords relative to the
 *						language model.
 */
MATH_TYPE MODEL::evaluate_reply(DICTIONARY *keys, DICTIONARY *words)
{
//  register
	int i;
//  register
	int j;
	MODEL *model = this;
    int symbol;
    MATH_TYPE probability;
    int count;
    MATH_TYPE entropy=(float)0.0;
    TREE *node;
    int num=0;

    if(words->size()<=0)
		return((MATH_TYPE)0.0);

	model->initialize_context();
    model->context[0]=model->forward;
    for(i=0; i<words->size(); ++i) {
	symbol=model->dictionary->find_word((*words)[i]);

	if(keys->find_word((*words)[i])!=0) {
	    probability=(MATH_TYPE)0.0;
	    count=0;
	    ++num;
	    for(j=0; j<model->order; ++j)
			if(model->context[j]!=NULL) {

		node=model->context[j]->find_symbol(symbol);
		probability+=(MATH_TYPE)(node->count)/
		    (MATH_TYPE)(model->context[j]->usage);
		++count;

	    }
	    if(count>0.0)
			entropy-=(float)log((float)probability/(float)count);
	}

	model->update_context(symbol);
    }

    model->initialize_context();
    model->context[0]=model->backward;
    for(i=words->size()-1; i>=0; --i) {
	symbol=model->dictionary->find_word((*words)[i]);

	if(keys->find_word((*words)[i])!=0) {
	    probability=(MATH_TYPE)0.0;
	    count=0;
	    ++num;
	    for(j=0; j<model->order; ++j)
			if(model->context[j]!=NULL) {

		node=model->context[j]->find_symbol(symbol);
		probability+=(float)(node->count)/
		    (float)(model->context[j]->usage);
		++count;

	    }

	    if(count>0.0)
			entropy-=(float)log((float)probability/(float)count);
	}

	model->update_context(symbol);
    }

    if(num>=8)
		entropy=((float)entropy)/sqrt(float(num-1));
    if(num>=16)
		entropy=((float)entropy)/(float)num;

    return(entropy);
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Babble
 *
 *		Purpose:		Return a random symbol from the current context, or a
 *						zero symbol identifier if we've reached either the
 *						start or end of the sentence.  Select the symbol based
 *						on probabilities, favouring keywords.  In all cases,
 *						use the longest available context to choose the symbol.
 */
int MODEL::babble(DICTIONARY *keys, DICTIONARY *words)
{
	MODEL *model =  this;
    TREE *node;
//  register
	int i;
    int count;
    int symbol;

    /*
     *		Select the longest available context.
     */
    for(i=0; i<=model->order; ++i)
	if(model->context[i]!=NULL)
	    node=model->context[i];

    if(node->branch==0)
		return(0);

    /*
     *		Choose a symbol at random from this context.
     */
    i=intrinsics::rnd(node->branch);
    count=intrinsics::rnd(node->usage);
    while(count>=0) {
	/*
	 *		If the symbol occurs as a keyword, then use it.  Only use an
	 *		auxilliary keyword if a normal keyword has already been used.
	 */
	symbol=node->tree[i]->symbol;

	if(
	    (keys->find_word((*model->dictionary)[symbol])!=0)&&
	    ((used_key==TRUE)||
	     (config::aux->find_word((*model->dictionary)[symbol])==0))&&
	    (words->word_exists((*model->dictionary)[symbol])==FALSE)
	    ) {
	    used_key=TRUE;
	    break;
	}
	count-=static_cast<TREE*>(node->tree[i])->count;
	i=(i>=(node->branch-1))?0:i+1;
    }

    return(symbol);
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Seed
 *
 *		Purpose:		Seed the reply by guaranteeing that it contains a
 *						keyword, if one exists.
 */
int MODEL::seed(DICTIONARY *keys)
{
//  register
	int i;
    int symbol;
    int stop;
	MODEL *model = this;
    /*
     *		Fix, thanks to Mark Tarrabain
     */
    if(model->context[0]->branch==0) symbol=0;
    else symbol=model->context[0]->tree[intrinsics::rnd(model->context[0]->branch)]->symbol;

    if(keys->size()>0) {
	i=intrinsics::rnd(keys->size());
	stop=i;
	while(TRUE) {
	    if(
		(model->dictionary->find_word((*keys)[i])!=0)&&
		(config::aux->find_word((*keys)[i])==0)
		) {
		symbol=model->dictionary->find_word((*keys)[i]);
		return(symbol);
	    }
	    ++i;
	    if(i==keys->size()) i=0;
	    if(i==stop)
			return(symbol);
	}
    }

    return(symbol);
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	New_Swap
 *
 *		Purpose:		Allocate a new swap structure.
 */
SWAP *SWAP::new_swap(void)
{
    SWAP *list;

    list=(SWAP *)malloc(sizeof(SWAP));
    if(list==NULL) {
	intrinsics::error("new_swap", "Unable to allocate swap");
	return(NULL);
    }
    list->size=0;
    list->from=NULL;
    list->to=NULL;

    return(list);
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Add_Swap
 *
 *		Purpose:		Add a new entry to the swap structure.
 */
void SWAP::add_swap(char *s, char *d)
{
	SWAP *list = this;
    list->size+=1;

    if(list->from==NULL) {
	list->from=(STRING *)malloc(sizeof(STRING));
	if(list->from==NULL) {
	    intrinsics::error("add_swap", "Unable to allocate list->from");
	    return;
	}
    }

    if(list->to==NULL) {
	list->to=(STRING *)malloc(sizeof(STRING));
	if(list->to==NULL) {
	    intrinsics::error("add_swap", "Unable to allocate list->to");
	    return;
	}
    }

    list->from=(STRING *)realloc(list->from, sizeof(STRING)*(list->size));
    if(list->from==NULL) {
	intrinsics::error("add_swap", "Unable to reallocate from");
	return;
    }

    list->to=(STRING *)realloc(list->to, sizeof(STRING)*(list->size));
    if(list->to==NULL) {
	intrinsics::error("add_swap", "Unable to reallocate to");
	return;
    }

    list->from[list->size-1].length=strlen(s);
    list->from[list->size-1].word=_strdup(s);
    list->to[list->size-1].length=strlen(d);
    list->to[list->size-1].word=_strdup(d);
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Initialize_Swap
 *
 *		Purpose:		Read a swap structure from a file.
 */
SWAP *SWAP::initialize_swap(char *filename)
{
    SWAP *list;
    FILE *file=NULL;
    char buffer[1024];
    char *from;
    char *to;
	char *next_token;

	list=SWAP::new_swap();

    if(filename==NULL)
		return(list);

	errno_t err = fopen_s (&file,filename, "r");
    if(file==NULL)
		return(list);

    while(!feof(file)) {

	if(fgets(buffer, 1024, file)==NULL)
		break;
	if(buffer[0]=='#')
		continue;
	from=strtok_s (buffer, "\t ",&next_token);
	to=strtok_s (NULL, "\t \n#",&next_token);

	list->add_swap(from, to);
    }

    fclose(file);
    return(list);
}

/*---------------------------------------------------------------------------*/

void SWAP::free_swap(SWAP *swap)
{
//  register
	int i;

    if(swap==NULL)
		return;

    for(i=0; i<swap->size; ++i) {
	STRING::free_word(swap->from[i]);
	STRING::free_word(swap->to[i]);
    }
    free(swap->from);
    free(swap->to);
    free(swap);
}

/*---------------------------------------------------------------------------*/

void MODEL::load_personality(MODEL **model)
{
	int sz;
    FILE *file;
    static char *filename=NULL;

    if(filename==NULL)
		filename=(char *)malloc(sizeof(char)*1);

    /*
     *		Allocate memory for the filename
     */
	sz = sizeof(char)*(strlen(directory)+strlen(SEP)+12);
    filename=(char *)realloc(filename,sz);
    if(filename==NULL)
		intrinsics::error("load_personality","Unable to allocate filename");

    /*
     *		Check to see if the brain exists
     */

    if(strcmp(directory, DEFAULT)!=0) {
	sprintf_s(filename, sz,"%s%smegahal.brn", directory, SEP);
	errno_t err = fopen_s (&file,filename, "r");

	if(file==NULL) {
	    sprintf_s(filename, sz,"%s%smegahal.trn", directory, SEP);
	    errno_t err = fopen_s(&file,filename, "r");
	    if(file==NULL) {
		fprintf(stdout, "Unable to change MegaHAL personality to \"%s\".\n"
			"Reverting to MegaHAL personality \"%s\".\n", directory, last);
		free(directory);
		directory=_strdup(last);
		return;
	    }
	}
	fclose(file);
	fprintf(stdout, "Changing to MegaHAL personality \"%s\".\n", directory);
    }

    /*
     *		Free the current personality
     */
	MODEL::free_model(*model);
    config::ban->free_words();
    DICTIONARY::free_dictionary(config::ban);
    config::aux->free_words();
    DICTIONARY::free_dictionary(config::aux);
    grt->free_words();
    DICTIONARY::free_dictionary(grt);
	SWAP::free_swap(swp);

    /*
     *		Create a language model.
     */
	*model=MODEL::new_model(config::order);

    /*
     *		Train the model on a text if one exists
     */

    sprintf_s(filename, sz,"%s%smegahal.brn", directory, SEP);
	bool status;
	status = (*model)->load_model (filename);
    if(status==FALSE) {
	sprintf_s(filename, sz, "%s%smegahal.trn", directory, SEP);
	(*model)->train(filename);
    }

    /*
     *		Read a dictionary containing banned keywords, auxiliary keywords,
     *		greeting keywords and swap keywords
     */
    sprintf_s(filename, sz,"%s%smegahal.ban", directory, SEP);
	config::ban = DICTIONARY::initialize_list(filename);
    sprintf_s(filename, sz,"%s%smegahal.aux", directory, SEP);
    config::aux = DICTIONARY::initialize_list(filename);
    sprintf_s(filename, sz,"%s%smegahal.grt", directory, SEP);
    grt = DICTIONARY::initialize_list(filename);
    sprintf_s(filename, sz,"%s%smegahal.swp", directory, SEP);
	swp = SWAP::initialize_swap(filename);
}

/*---------------------------------------------------------------------------*/

void MODEL::change_personality(DICTIONARY *command, int position, MODEL **model)
{
	int sz;
    if(directory == NULL)
	{
	sz = strlen(DEFAULT)+1;
	directory = (char *)malloc(sizeof(char)*(sz));
	if(directory == NULL) {
	    intrinsics::error("change_personality", "Unable to allocate directory");
	} else {
	    strcpy_s (directory,sz,DEFAULT);
	}
    }

    if(last == NULL) {
	last = _strdup(directory);
    }

    if((command == NULL)||((position+2)>=command->size())) {
	/* no dir set, so we leave it to whatever was set above */
    } else {
		sz = sizeof(char)*((*command)[position+2].length+1);
        directory=(char *)realloc(directory,sz);
        if(directory == NULL)
            intrinsics::error("change_personality", "Unable to allocate directory");
        strncpy_s (directory, sz, (*command)[position+2].word,
                (*command)[position+2].length);
        directory[(*command)[position+2].length]='\0';
    }

    load_personality(model);
}

/*---------------------------------------------------------------------------*/
