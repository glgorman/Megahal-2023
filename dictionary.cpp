
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
#include "megahal.h"

extern DICTIONARY *grt;

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Add_Word
 *
 *		Purpose:		Add a word to a dictionary, and return the identifier
 *						assigned to the word.  If the word already exists in
 *						the dictionary, then return its current identifier
 *						without adding it again.
 */
BYTE2 DICTIONARY::add_word(STRING word)
{
	DICTIONARY *dictionary = this;
    register int i;
    int position;
    bool found;

    /*
     *		If the word's already in the dictionary, there is no need to add it
     */
    position=search_dictionary(word, &found);
    if(found==TRUE) goto succeed;

    /*
     *		Increase the number of words in the dictionary
     */
    dictionary->size+=1;

    /*
     *		Allocate one more entry for the word index
     */
    if(dictionary->index==NULL) {
	dictionary->index=(BYTE2 *)malloc(sizeof(BYTE2)*
					  (dictionary->size));
    } else {
	dictionary->index=(BYTE2 *)realloc((BYTE2 *)
					   (dictionary->index),sizeof(BYTE2)*(dictionary->size));
    }
    if(dictionary->index==NULL) {
	intrinsics::error("add_word", "Unable to reallocate the index.");
	goto fail;
    }

    /*
     *		Allocate one more entry for the word array
     */
    if(dictionary->entry==NULL) {
	dictionary->entry=(STRING *)malloc(sizeof(STRING)*(dictionary->size));
    } else {
	dictionary->entry=(STRING *)realloc((STRING *)(dictionary->entry),
					    sizeof(STRING)*(dictionary->size));
    }
    if(dictionary->entry==NULL) {
	intrinsics::error("add_word", "Unable to reallocate the dictionary to %d elements.", dictionary->size);
	goto fail;
    }

    /*
     *		Copy the new word into the word array
     */
    dictionary->entry[dictionary->size-1].length=word.length;
    dictionary->entry[dictionary->size-1].word=(char *)malloc(sizeof(char)*
							      (word.length));
    if(dictionary->entry[dictionary->size-1].word==NULL) {
	intrinsics::error("add_word", "Unable to allocate the word.");
	goto fail;
    }
    for(i=0; i<word.length; ++i)
	dictionary->entry[dictionary->size-1].word[i]=word.word[i];

    /*
     *		Shuffle the word index to keep it sorted alphabetically
     */
    for(i=(dictionary->size-1); i>position; --i)
	dictionary->index[i]=dictionary->index[i-1];

    /*
     *		Copy the new symbol identifier into the word index
     */
    dictionary->index[position]=dictionary->size-1;

succeed:
    return(dictionary->index[position]);

fail:
    return(0);
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Search_Dictionary
 *
 *		Purpose:		Search the dictionary for the specified word, returning its
 *						position in the index if found, or the position where it
 *						should be inserted otherwise.
 */
int DICTIONARY::search_dictionary(STRING word, bool *find)
{
	DICTIONARY *dictionary = this; 
    int position;
    int min;
    int max;
    int middle;
    int compar;

    /*
     *		If the dictionary is empty, then obviously the word won't be found
     */
    if(dictionary->size==0) {
	position=0;
	goto notfound;
    }

    /*
     *		Initialize the lower and upper bounds of the search
     */
    min=0;
    max=dictionary->size-1;
    /*
     *		Search repeatedly, halving the search space each time, until either
     *		the entry is found, or the search space becomes empty
     */
    while(TRUE) {
	/*
	 *		See whether the middle element of the search space is greater
	 *		than, equal to, or less than the element being searched for.
	 */
	middle=(min+max)/2;
	compar=STRING::wordcmp(word, dictionary->entry[dictionary->index[middle]]);
	/*
	 *		If it is equal then we have found the element.  Otherwise we
	 *		can halve the search space accordingly.
	 */
	if(compar==0) {
	    position=middle;
	    goto found;
	} else if(compar>0) {
	    if(max==middle) {
		position=middle+1;
		goto notfound;
	    }
	    min=middle+1;
	} else {
	    if(min==middle) {
		position=middle;
		goto notfound;
	    }
	    max=middle-1;
	}
    }

found:
    *find=TRUE;
    return(position);

notfound:
    *find=FALSE;
    return(position);
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Find_Word
 *
 *		Purpose:		Return the symbol corresponding to the word specified.
 *						We assume that the word with index zero is equal to a
 *						NULL word, indicating an error condition.
 */
BYTE2 DICTIONARY::find_word(STRING word)
{
	DICTIONARY *dictionary = this;
    int position;
    bool found;

    position=search_dictionary(word, &found);

    if(found==TRUE)
		return(dictionary->index[position]);
    else
		return(0);
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Wordcmp
 *
 *		Purpose:		Compare two words, and return an integer indicating whether
 *						the first word is less than, equal to or greater than the
 *						second word.
 */
int STRING::wordcmp(STRING word1, STRING word2)
{
    register int i;
    int bound;

    bound=MIN(word1.length,word2.length);

    for(i=0; i<bound; ++i)
	if(toupper(word1.word[i])!=toupper(word2.word[i]))
	    return((int)(toupper(word1.word[i])-toupper(word2.word[i])));

    if(word1.length<word2.length) return(-1);
    if(word1.length>word2.length) return(1);

    return(0);
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Free_Dictionary
 *
 *		Purpose:		Release the memory consumed by the dictionary.
 */
void DICTIONARY::free_dictionary(DICTIONARY *dictionary)
{
    if(dictionary==NULL) return;
    if(dictionary->entry!=NULL) {
	free(dictionary->entry);
	dictionary->entry=NULL;
    }
    if(dictionary->index!=NULL) {
	free(dictionary->index);
	dictionary->index=NULL;
    }
    dictionary->size=0;
}



/*---------------------------------------------------------------------------*/

/*
 *		Function:	Initialize_Dictionary
 *
 *		Purpose:		Add dummy words to the dictionary.
 */
void DICTIONARY::initialize_dictionary()
{
	DICTIONARY *dictionary = this;
    STRING word={ 7, "<ERROR>" };
    STRING end={ 5, "<FIN>" };

    (void)dictionary->add_word(word);
    (void)dictionary->add_word(end);
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	New_Dictionary
 *
 *		Purpose:		Allocate room for a new dictionary.
 */
DICTIONARY *DICTIONARY::new_dictionary(void)
{
    DICTIONARY *dictionary=NULL;

    dictionary=(DICTIONARY *)malloc(sizeof(DICTIONARY));
    if(dictionary==NULL) {
		intrinsics::error("new_dictionary", "Unable to allocate dictionary.");
	return(NULL);
    }

    dictionary->size=0;
    dictionary->index=NULL;
    dictionary->entry=NULL;

    return(dictionary);
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Save_Dictionary
 *
 *		Purpose:		Save a dictionary to the specified file.
 */
void DICTIONARY::save_dictionary(FILE *file)
{
//  register
	int i;
	DICTIONARY *dictionary = this;

    fwrite(&(dictionary->size), sizeof(BYTE4), 1, file);
    intrinsics::progress("Saving dictionary", 0, 1);
    for(i=0; i<dictionary->size; ++i) {
	STRING::save_word(file, dictionary->entry[i]);
	intrinsics::progress(NULL, i, dictionary->size);
    }
    intrinsics::progress(NULL, 1, 1);
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Load_Dictionary
 *
 *		Purpose:		Load a dictionary from the specified file.
 */
void DICTIONARY::load_dictionary(FILE *file)
{
//  register
	int i;
    int size;
	DICTIONARY *dictionary = this;

    fread(&size, sizeof(BYTE4), 1, file);
    intrinsics::progress("Loading dictionary", 0, 1);
    for(i=0; i<size; ++i) {
		load_word(file, dictionary);
		intrinsics::progress(NULL, i, size);
    }
	intrinsics::progress(NULL, 1, 1);
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Save_Word
 *
 *		Purpose:		Save a dictionary word to a file.
 */
void STRING::save_word(FILE *file, STRING word)
{
//  register
	int i;

    fwrite(&(word.length), sizeof(BYTE1), 1, file);
    for(i=0; i<word.length; ++i)
	fwrite(&(word.word[i]), sizeof(char), 1, file);
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Load_Word
 *
 *		Purpose:		Load a dictionary word from a file.
 */
void DICTIONARY::load_word(FILE *file, DICTIONARY *dictionary)
{
//  register
	int i;
    STRING word;

    fread(&(word.length), sizeof(BYTE1), 1, file);
    word.word=(char *)malloc(sizeof(char)*word.length);
    if(word.word==NULL) {
		intrinsics::error("load_word", "Unable to allocate word");
	return;
    }
    for(i=0; i<word.length; ++i)
		fread(&(word.word[i]), sizeof(char), 1, file);

    dictionary->add_word(word);
    free(word.word);
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Execute_Command
 *
 *		Purpose:		Detect whether the user has typed a command, and
 *						execute the corresponding function.
 */
OPCODES DICTIONARY::execute_command(DICTIONARY *words, int *position)
{
	OPCODE *request;
//  register
	int i;
//  register
	int j;

    /*
     *		If there is only one word, then it can't be a command.
     */
    *position=words->size+1;
    if(words->size<=1)
		return(UNKNOWN);

    /*
     *		Search through the word array.  If a command prefix is found,
     *		then try to match the following word with a command word.  If
     *		a match is found, then return a command identifier.  If the
     *		Following word is a number, then change the judge.  Otherwise,
     *		continue the search.
     */
    for(i=0; i<words->size-1; ++i)
	/*
	 *		The command prefix was found.
	 */
	if(words->entry[i].word[words->entry[i].length - 1] == '#') {
	    /*
	     *		Look for a command word.
	     */
		int sz = OPCODE::COMMAND_SIZE();
		for(j = 0; j < sz ; ++j)
		{
			request = &config::commands[j];
			STRING &w = request->word;
			OPCODES id = request->command_id;
			if(STRING::wordcmp(w, words->entry[i + 1]) == 0)
		
		    *position = i + 1;
			return(id);
		}
	}

    return(UNKNOWN);
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Show_Dictionary
 *
 *		Purpose:		Display the dictionary for training purposes.
 */
void DICTIONARY::show_dictionary()
{
	DICTIONARY *dictionary = this;
//    register
	int i;
//    register
	int j;
    FILE *file;

    errno_t err = fopen_s (&file,"megahal.dic", "w");
    if(file==NULL) {
		intrinsics::warn("show_dictionary", "Unable to open file");
	return;
    }

    for(i=0; i<dictionary->size; ++i) {
	for(j=0; j<dictionary->entry[i].length; ++j)
	    fprintf(file, "%c", dictionary->entry[i].word[j]);
	fprintf(file, "\n");
    }

    fclose(file);
}

/*---------------------------------------------------------------------------*/

/*
 *    Function:   Make_Words
 *
 *    Purpose:    Break a string into an array of words.
 */
void DICTIONARY::make_words(char *input)
{
	DICTIONARY *words = this;
    int offset=0;

    /*
     *		Clear the entries in the dictionary
     */
    DICTIONARY::free_dictionary(words);

    /*
     *		If the string is empty then do nothing, for it contains no words.
     */
    if(strlen(input)==0)
		return;

    /*
     *		Loop forever.
     */
    while(1) {

	/*
	 *		If the current character is of the same type as the previous
	 *		character, then include it in the word.  Otherwise, terminate
	 *		the current word.
	 */
	if(intrinsics::boundary(input, offset)) {
	    /*
	     *		Add the word to the dictionary
	     */
	    if(words->entry==NULL)
		words->entry=(STRING *)malloc((words->size+1)*sizeof(STRING));
	    else
		words->entry=(STRING *)realloc(words->entry, (words->size+1)*sizeof(STRING));

	    if(words->entry==NULL) {
			intrinsics::error("make_words", "Unable to reallocate dictionary");
		return;
	    }

	    words->entry[words->size].length=offset;
	    words->entry[words->size].word=input;
	    words->size+=1;

	    if(offset==(int)strlen(input)) break;
	    input+=offset;
	    offset=0;
	} else {
	    ++offset;
	}
    }

    /*
     *		If the last word isn't punctuation, then replace it with a
     *		full-stop character.
     */
	unsigned char c1;
	c1 = words->entry[words->size-1].word[0];
    if(isalnum(c1)) {
	if(words->entry==NULL)
	    words->entry=(STRING *)malloc((words->size+1)*sizeof(STRING));
	else
	    words->entry=(STRING *)realloc(words->entry, (words->size+1)*sizeof(STRING));
	if(words->entry==NULL) {
	    intrinsics::error("make_words", "Unable to reallocate dictionary");
	    return;
	}

	words->entry[words->size].length=1;
	words->entry[words->size].word=".";
	++words->size;
    }
    else if(strchr("!.?", words->entry[words->size-1].word[words->entry[words->size-1].length-1])==NULL) {
	words->entry[words->size-1].length=1;
	words->entry[words->size-1].word=".";
    }

    return;
}

/*---------------------------------------------------------------------------*/
/*
 *		Function:	Make_Greeting
 *
 *		Purpose:		Put some special words into the dictionary so that the
 *						program will respond as if to a new judge.
 */
void DICTIONARY::make_greeting()
{
//  register
	int i;
	DICTIONARY *words = this;

    for(i=0; i<words->size; ++i)
		free(words->entry[i].word);
    DICTIONARY::free_dictionary(words);
    if(grt->size>0)
		(void)words->add_word(grt->entry[intrinsics::rnd(grt->size)]);
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Dissimilar
 *
 *		Purpose:		Return TRUE or FALSE depending on whether the dictionaries
 *						are the same or not.
 */
bool DICTIONARY::dissimilar(DICTIONARY *words1, DICTIONARY *words2)
{
//  register
	int i;

    if(words1->size!=words2->size)
		  return(TRUE);
    for(i=0; i<words1->size; ++i)
		if(STRING::wordcmp(words1->entry[i], words2->entry[i])!=0)
		return(TRUE);
    return(FALSE);
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Make_Output
 *
 *		Purpose:		Generate a string from the dictionary of reply words.
 */
char *DICTIONARY::make_output()
{
	DICTIONARY *words = this;
    static char *output=NULL;
//  register
	int i;
//  register
	int j;
    int length;
    static char *output_none=NULL;

    if(output_none==NULL)
		output_none=(char*)malloc(40);

    if(output==NULL) {
	output=(char *)malloc(sizeof(char));
	if(output==NULL) {
	    intrinsics::error("make_output", "Unable to allocate output");
	    return(output_none);
	}
    }

    if(words->size==0) {
	if(output_none!=NULL)
 	    strcpy_s(output_none,39, "I am utterly speechless!");
	return(output_none);
    }

    length=1;
    for(i=0; i<words->size; ++i)
		length+=words->entry[i].length;

    output=(char *)realloc(output, sizeof(char)*length);
    if(output==NULL) {
		intrinsics::error("make_output", "Unable to reallocate output.");
	if(output_none!=NULL)
	    strcpy_s(output_none,39,"I forgot what I was going to say!");
	return(output_none);
    }

    length=0;
    for(i=0; i<words->size; ++i)
	for(j=0; j<words->entry[i].length; ++j)
	    output[length++]=words->entry[i].word[j];

    output[length]='\0';

    return(output);
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Word_Exists
 *
 *		Purpose:		A silly brute-force searcher for the reply string.
 */
bool DICTIONARY::word_exists(STRING word)
{
//  register
	int i;
	DICTIONARY *dictionary = this;

    for(i=0; i<dictionary->size; ++i)
	if(STRING::wordcmp(dictionary->entry[i], word)==0)
	    return(TRUE);
    return(FALSE);
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Initialize_List
 *
 *		Purpose:		Read a dictionary from a file.
 */
DICTIONARY *DICTIONARY::initialize_list(char *filename)
{
    DICTIONARY *list;
    FILE *file=NULL;
    STRING word;
    char *string;
	char *next_token;
    char buffer[1024];

    list=DICTIONARY::new_dictionary();

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
	string = strtok_s(buffer, "\t \n#",&next_token);

	if((string!=NULL)&&(strlen(string)>0)) {
	    word.length=strlen(string);
	    word.word=_strdup(buffer);
	    list->add_word(word);
	}
    }

    fclose(file);
    return(list);
}

void DICTIONARY::free_words()
{
//  register
	int i;
	DICTIONARY *words = this;

    if(words == NULL)
		return;

    if(words->entry != NULL)
	for(i=0; i<words->size; ++i)
		STRING::free_word(words->entry[i]);
}

/*---------------------------------------------------------------------------*/

void STRING::free_word(STRING word)
{
    free(word.word);
}