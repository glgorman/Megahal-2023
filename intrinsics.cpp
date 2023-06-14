
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


/*---------------------------------------------------------------------------*/

/*
 *		Function:	ExitHAL
 *
 *		Purpose:		Terminate the program.
 */
void intrinsics::exithal(void)
{
#ifdef __mac_os
    /*
     *		Must be called because it does use some system memory
     */
    if (gSpeechChannel) {
	StopSpeech(gSpeechChannel);
	DisposeSpeechChannel(gSpeechChannel);
	gSpeechChannel = nil;
    }
#endif

    exit(0);
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Read_Input
 *
 *		Purpose:		Read an input string from the user.
 */
char *intrinsics::read_input(char *prompt)
{
    static char *input=NULL;
    bool finish;
    int length;
    int c;

    /*
     *		Perform some initializations.  The finish boolean variable is used
     *		to detect a double line-feed, while length contains the number of
     *		characters in the input string.
     */
    finish=FALSE;
    length=0;
    if(input==NULL) {
	input=(char *)malloc(sizeof(char));
	if(input==NULL) {
	    error("read_input", "Unable to allocate the input string");
	    return(input);
	}
    }

    /*
     *		Display the prompt to the user.
     */
    fprintf(stdout, prompt);
    fflush(stdout);

    /*
     *		Loop forever, reading characters and putting them into the input
     *		string.
     */
    while(TRUE) {

	/*
	 *		Read a single character from stdin.
	 */
	c=getc(stdin);

	/*
	 *		If the character is a line-feed, then set the finish variable
	 *		to TRUE.  If it already is TRUE, then this is a double line-feed,
	 *		in which case we should exit.  After a line-feed, display the
	 *		prompt again, and set the character to the space character, as
	 *		we don't permit linefeeds to appear in the input.
	 */
	if((char)(c)=='\n') {
	    if(finish==TRUE) break;
	    fprintf(stdout, prompt);
	    fflush(stdout);
	    finish=TRUE;
	    c=32;
	} else {
	    finish=FALSE;
	}

	/*
	 *		Re-allocate the input string so that it can hold one more
	 *		character.
	 */
	++length;
	input=(char *)realloc((char *)input,sizeof(char)*(length+1));
	if(input==NULL) {
	    error("read_input", "Unable to re-allocate the input string");
	    return(NULL);
	}

	/*
	 *		Add the character just read to the input string.
	 */
	input[length-1]=(char)c;
	input[length]='\0';
    }

    while(isspace(abs(input[length-1])))
		--length;

    input[length]='\0';

    /*
     *		We have finished, so return the input string.
     */
    return(input);
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Initialize_Error
 *
 *		Purpose:		Close the current error file pointer, and open a new one.
 */
bool intrinsics::initialize_error(char *filename)
{
    if(config::errorfp!=stderr)
		fclose(config::errorfp);

    if(filename==NULL)
		return(TRUE);

    errno_t err = fopen_s(&config::errorfp,filename, "a");
    if(config::errorfp==NULL) {
	config::errorfp=stderr;
	return(FALSE);
    }
    return(print_header(config::errorfp));
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Error
 *
 *		Purpose:		Print the specified message to the error file.
 */
void intrinsics::error(char *title, char *fmt, ...)
{
    va_list argp;

    fprintf(config::errorfp, "%s: ", title);
    va_start(argp, fmt);
    vfprintf(config::errorfp, fmt, argp);
    va_end(argp);
    fprintf(config::errorfp, ".\n");
    fflush(config::errorfp);

    fprintf(stderr, "MegaHAL died for some reason; check the error log.\n");

    exit(1);
}

/*---------------------------------------------------------------------------*/

bool intrinsics::warn(char *title, char *fmt, ...)
{
    va_list argp;

    fprintf(config::errorfp, "%s: ", title);
    va_start(argp, fmt);
    vfprintf(config::errorfp, fmt, argp);
    va_end(argp);
    fprintf(config::errorfp, ".\n");
    fflush(config::errorfp);

    fprintf(stderr, "MegaHAL emitted a warning; check the error log.\n");

    return(TRUE);
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Initialize_Status
 *
 *		Purpose:		Close the current status file pointer, and open a new one.
 */
bool intrinsics::initialize_status(char *filename)
{
    if(config::statusfp!=stdout)
		fclose(config::statusfp);
    if(filename==NULL)
		return(FALSE);
	errno_t err = fopen_s (&config::statusfp,filename, "a");
	if(config::statusfp==NULL) {
		config::statusfp=stdout;
		return(FALSE);
    }
	return(print_header(config::statusfp));
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Status
 *
 *		Purpose:		Print the specified message to the status file.
 */
bool intrinsics::status(char *fmt, ...)
{
    va_list argp;

    va_start(argp, fmt);
	vfprintf(config::statusfp, fmt, argp);
    va_end(argp);
	fflush(config::statusfp);

    return(TRUE);
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Print_Header
 *
 *		Purpose:		Display a copyright message and timestamp.
 */
bool intrinsics::print_header(FILE *file)
{
    time_t clock;
    char timestamp[1024];
    struct tm *local;

    clock=time(NULL);
    local=localtime (&clock);
    strftime(timestamp, 1024, "Start at: [%Y/%m/%d %H:%M:%S]\n", local);

    fprintf(file, "MegaHALv8\n");
    fprintf(file, "Copyright (C) 1998 Jason Hutchens\n");
    fprintf(file, timestamp);
    fflush(file);

    return(TRUE);
}

/*---------------------------------------------------------------------------*/

/*
 *    Function:   Write_Output
 *
 *    Purpose:    Display the output string.
 */
void intrinsics::write_output(char *output)
{
    char *formatted;
	char *next_token;
    char *bit;

    capitalize(output);
    speak(output);

    config::width=75;
    formatted=format_output(output);
    delay(formatted);
    config::width=64;
    formatted=format_output(output);

    bit=strtok_s(formatted, "\n",&next_token);
    if(bit==NULL) (void)status("MegaHAL: %s\n", formatted);
    while(bit!=NULL) {
	(void)status("MegaHAL: %s\n", bit);
	bit=strtok_s(NULL, "\n",&next_token);
    }
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Capitalize
 *
 *		Purpose:		Convert a string to look nice.
 */
void intrinsics::capitalize(char *string)
{
    register int i;
    bool start=TRUE;
	unsigned char c1;
    for(i=0; i<(int)strlen(string); ++i)
	{
		c1 = string[i];
	if(isalpha(c1)) {
	    if(start==TRUE) string[i]=(char)toupper((int)string[i]);
	    else string[i]=(char)tolower((int)string[i]);
	    start=FALSE;
	}
	if((i>2)&&(strchr("!.?", string[i-1])!=NULL)&&(isspace(c1)))
	    start=TRUE;
    }
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Upper
 *
 *		Purpose:		Convert a string to its uppercase representation.
 */
void intrinsics::upper(char *string)
{
    register int i;

    for(i=0; i<(int)strlen(string); ++i) string[i]=(char)toupper((int)string[i]);
}

/*---------------------------------------------------------------------------*/

/*
 *    Function:   Write_Input
 *
 *    Purpose:    Log the user's input
 */
void intrinsics::write_input(char *input)
{
    char *formatted;
	char *next_token;
    char *bit;

    config::width=64;
    formatted=format_output(input);

	bit=strtok_s (formatted, "\n",&next_token);
    if(bit==NULL)
		(void)status ("User:    %s\n", formatted);
    while(bit!=NULL) {
	(void)status ("User:    %s\n", bit);
	bit=strtok_s (NULL, "\n",&next_token);
    }
}

/*---------------------------------------------------------------------------*/

/*
 *    Function:   Format_Output
 *
 *    Purpose:    Format a string to display nicely on a terminal of a given
 *                width.
 */
char *intrinsics::format_output(char *output)
{
    static char *formatted=NULL;
    register int i,j,c;
    int l;
    if(formatted==NULL) {
	formatted=(char *)malloc(sizeof(char));
	if(formatted==NULL) {
	    intrinsics::error("format_output", "Unable to allocate formatted");
	    return("ERROR");
	}
    }

    formatted=(char *)realloc((char *)formatted, sizeof(char)*(strlen(output)+2));
    if(formatted==NULL) {
	intrinsics::error("format_output", "Unable to re-allocate formatted");
	return("ERROR");
    }

    l=0;
    j=0;
    for(i=0; i<(int)strlen(output); ++i) {
	if((l==0)&&(isspace(output[i]))) continue;
	formatted[j]=output[i];
	++j;
	++l;
	if(!config::nowrap)
	    if(l>=config::width)
		for(c=j-1; c>0; --c)
		    if(formatted[c]==' ') {
			formatted[c]='\n';
			l=j-c-1;
			break;
		    }
    }
    if((j>0)&&(formatted[j-1]!='\n')) {
	formatted[j]='\n';
	++j;
    }
    formatted[j]='\0';

    return(formatted);
}

/*---------------------------------------------------------------------------*/
/*
 *		Function:	Boundary
 *
 *		Purpose:		Return whether or not a word boundary exists in a string
 *						at the specified location.
 */
bool intrinsics::boundary(char *string, int position)
{
	unsigned char c1,c2;
    if(position==0)
		return(FALSE);

    if(position==(int)strlen(string))
	return(TRUE);

    if(
	(string[position]=='\'')&&
	(isalpha(string[position-1])!=0)&&
	(isalpha(string[position+1])!=0)
	)
	return(FALSE);

    if(
	(position>1)&&
	(string[position-1]=='\'')&&
	(isalpha(string[position-2])!=0)&&
	(isalpha(string[position])!=0)
	)
	return(FALSE);

	c1 = string[position];
	c2 = string[position-1];

    if(
	(isalpha(c1)!=0)&&
	(isalpha(c2)==0)
	)
	return(TRUE);

    if(
	(isalpha(c1)==0)&&
	(isalpha(c2)!=0)
	)
	return(TRUE);

    if(isdigit(c1)!=isdigit(c2))
	return(TRUE);

    return(FALSE);
} 

void intrinsics::usleep (int period)
{
	Sleep (period);
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Delay
 *
 *		Purpose:		Display the string to stdout as if it was typed by a human.
 */
void intrinsics::delay(char *string)
{
//  register
	int i;

    /*
     *		Don't simulate typing if the feature is turned off
     */
    if(config::typing_delay==FALSE)	{
	fprintf(stdout, string);
	return;
    }

    /*
     *		Display the entire string, one character at a time
     */
    for(i=0; i<(int)strlen(string)-1; ++i) typein(string[i]);
    usleep((D_THINK+rnd(V_THINK)-rnd(V_THINK))/2);
    typein(string[i]);
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Typein
 *
 *		Purpose:		Display a character to stdout as if it was typed by a human.
 */
void intrinsics::typein(char c)
{
    /*
     *		Standard keyboard delay
     */
    usleep(D_KEY+rnd(V_KEY)-rnd(V_KEY));
    fprintf(stdout, "%c", c);
    fflush(stdout);

    /*
     *		A random thinking delay
     */
    if((!isalnum(c))&&((rnd(100))<P_THINK))
	usleep(D_THINK+rnd(V_THINK)-rnd(V_THINK));
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Ignore
 *
 *		Purpose:		Log the occurrence of a signal, but ignore it.
 */
void intrinsics::ignore(int sig)
{
    if(sig!=0)
		intrinsics::warn("ignore", "MegaHAL received signal %d", sig);

#if !defined(DOS)
    //    signal(SIGINT, saveandexit);
    //    signal(SIGILL, die);
    //    signal(SIGSEGV, die);
#endif
    //    signal(SIGFPE, die);
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Die
 *
 *		Purpose:		Log the occurrence of a signal, and exit.
 */
void intrinsics::die(int sig)
{
    error("die", "MegaHAL received signal %d", sig);
    exithal();
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Rnd
 *
 *		Purpose:		Return a random integer between 0 and range-1.
 */
int intrinsics::rnd(int range)
{
    static bool flag=FALSE;

/*
    if(flag==FALSE) {
#if defined(__mac_os) || defined(DOS)
	srand(time(NULL));
#else
	srand48(time(NULL));
#endif
    }
*/
    flag=TRUE;

    return(rand()%range);
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Usleep
 *
 *		Purpose:		Simulate the Un*x function usleep.  Necessary because
 *						Microsoft provide no similar function.  Performed via
 *						a busy loop, which unnecessarily chews up the CPU.
 *						But Windows '95 isn't properly multitasking anyway, so
 *						no-one will notice.  Modified from a real Microsoft
 *						example, believe it or not!
 */
#if defined(DOS) || defined(__mac_os)
void intrinsics::usleep(int period)
{
    clock_t goal;

    goal=(clock_t)(period*CLOCKS_PER_SEC)/(clock_t)1000000+clock();
    while(goal>clock());
}
#endif


/*---------------------------------------------------------------------------*/

/*
 *		Function:	Strdup
 *
 *		Purpose:		Provide the strdup() function for Macintosh.
 */
#ifdef __mac_os
char *intrinsics::strdup(const char *str)
{
    char *rval=(char *)malloc(strlen(str)+1);

    if(rval!=NULL) strcpy(rval, str);

    return(rval);
}
#endif

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Initialize_Speech
 *
 *		Purpose:		Initialize speech output.
 */
#ifdef __mac_os
bool intrinsics::initialize_speech(void)
{
    bool speechExists = false;
    long response;
    OSErr err;

    err = Gestalt(gestaltSpeechAttr, &response);

    if(!err) {
	if(response & (1L << gestaltSpeechMgrPresent)) {
	    speechExists = true;
	}
    }
    return speechExists;
}
#endif

/*---------------------------------------------------------------------------*/

/*
 *		Function:	changevoice
 *
 *		Purpose:		change voice of speech output.
 */
void DICTIONARY::changevoice(int position)
{
#ifdef __mac_os
    register int i, index;
    STRING word={ 1, "#" };
    char buffer[80];
    VoiceSpec voiceSpec;
    VoiceDescription info;
    short count, voiceCount;
    unsigned char* temp;
    OSErr err;
    /*
     *		If there is less than 4 words, no voice specified.
     */
    if(words->size<=4) return;

    for(i=0; i<words->size-4; ++i)
	if(wordcmp(word, words->entry[i])==0) {

	    err = CountVoices(&voiceCount);
	    if (!err && voiceCount) {
		for (count = 1; count <= voiceCount; count++) {
		    err = GetIndVoice(count, &voiceSpec);
		    if (err) continue;
		    err = GetVoiceDescription(&voiceSpec, &info,
					      sizeof(VoiceDescription));
		    if (err) continue;


		    for (temp= info.name; *temp; temp++) {
			if (*temp == ' ')
			    *temp = '_';
		    }

		    /*
		     *		skip command and get voice name
		     */
		    index = i + 3;
		    strcpy(buffer, words->entry[index].word);
		    c2pstr(buffer);
		    // compare ignoring case
		    if (EqualString((StringPtr)buffer, info.name, false, false)) {
			if (gSpeechChannel) {
			    StopSpeech(gSpeechChannel);
			    DisposeSpeechChannel(gSpeechChannel);
			    gSpeechChannel = nil;
			}
			err = NewSpeechChannel(&voiceSpec, &gSpeechChannel);
			if (!err) {
			    p2cstr((StringPtr)buffer);
			    printf("Now using %s voice\n", buffer);
			    c2pstr(buffer);
			    err = SpeakText(gSpeechChannel, &buffer[1], buffer[0]);
			}
		    }
		}
	    }
	}
#endif
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	listvoices
 *
 *		Purpose:		Display the names of voices for speech output.
 */
void intrinsics::listvoices(void)
{
#ifdef __mac_os
    VoiceSpec voiceSpec;
    VoiceDescription info;
    short count, voiceCount;
    unsigned char* temp;
    OSErr err;

    if(gSpeechExists) {
	err = CountVoices(&voiceCount);
	if (!err && voiceCount) {
	    for (count = 1; count <= voiceCount; count++) {
		err = GetIndVoice(count, &voiceSpec);
		if (err) continue;

		err = GetVoiceDescription(&voiceSpec, &info,
					  sizeof(VoiceDescription));
		if (err) continue;

		p2cstr(info.name);
		for (temp= info.name; *temp; temp++)
		    if (*temp == ' ')
			*temp = '_';
		printf("%s\n",info.name);
	    }
	}
    }
#endif
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Speak
 */
void intrinsics::speak(char *output)
{
    if(config::speech==FALSE) return;
#ifdef __mac_os
    if(gSpeechExists) {
	OSErr err;

	if (gSpeechChannel)
	    err = SpeakText(gSpeechChannel, output, strlen(output));
	else {
	    c2pstr(output);
	    SpeakString((StringPtr)output);
	    p2cstr((StringPtr)output);
	}
    }
#endif
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Progress
 *
 *		Purpose:		Display a progress indicator as a percentage.
 */
bool intrinsics::progress(char *message, int done, int total)
{
    static int last=0;
    static bool first=FALSE;

    /*
     *    We have already hit 100%, and a newline has been printed, so nothing
     *    needs to be done.
     */
    if((done*100/total==100)&&(first==FALSE)) return(TRUE);

    /*
     *    Nothing has changed since the last time this function was called,
     *    so do nothing, unless it's the first time!
     */
    if(done*100/total==last) {
	if((done==0)&&(first==FALSE)) {
	    fprintf(stderr, "%s: %3d%%", message, done*100/total);
	    first=TRUE;
	}
	return(TRUE);
    }

    /*
     *    Erase what we printed last time, and print the new percentage.
     */
    last=done*100/total;

    if(done>0) fprintf(stderr, "%c%c%c%c", 8, 8, 8, 8);
    fprintf(stderr, "%3d%%", done*100/total);

    /*
     *    We have hit 100%, so reset static variables and print a newline.
     */
    if(last==100) {
	first=FALSE;
	last=0;
	fprintf(stderr, "\n");
    }

    return(TRUE);
}

/*---------------------------------------------------------------------------*/

void intrinsics::help(void)
{
    int j;
	int sz = OPCODE::COMMAND_SIZE();
	for(j=0; j<sz; ++j) {
		OPCODE &w = config::commands[j];
		printf("#%-7s: %s\n", w.word, w.helpstring);
    }
}
