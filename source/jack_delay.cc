// -----------------------------------------------------------------------------
//
//  Copyright (C) 2012-2018 Fons Adriaensen <fons@linuxaudio.org>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// -----------------------------------------------------------------------------


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>
#include <jack/jack.h>
#include "mtdm.h"


static MTDM           *mtdm = 0;
static jack_client_t  *jack_handle;
static jack_port_t    *jack_capt;
static jack_port_t    *jack_play;
static const char     *jack_name;
static volatile bool  active = false;


int jack_callback (jack_nframes_t nframes, void *arg)
{
    float *ip, *op;

    if (active)
    {
        ip = (float *)(jack_port_get_buffer (jack_capt, nframes));
        op = (float *)(jack_port_get_buffer (jack_play, nframes));
        mtdm->process (nframes, ip, op);
    }
    return 0;
}


static char *options = (char *)"hO:I:E";
static bool  E_opt = false;
static const char *I_val = 0;
static const char *O_val = 0;


static void help (void)
{
    fprintf (stderr, "\njack_delay %s\n", VERSION);
    fprintf (stderr, "(C) 2003-2013 Fons Adriaensen <fons@linuxaudio.org>\n");
    fprintf (stderr, "Measure round trip latency of a soundcard.\n");
    fprintf (stderr, "Usage: jack_delay <options>\n");
    fprintf (stderr, "Options:\n");
    fprintf (stderr, "  -h  Display this text\n");
    fprintf (stderr, "  -O  <playback port>   Connect output to named port.\n");
    fprintf (stderr, "  -I  <capture port>    Connect input to named port.\n");
    fprintf (stderr, "  -E  Report excess latency, requires both -I and -O.\n");
    fprintf (stderr, "The excess latency is the measured value minus the\n");
    fprintf (stderr, "expected value for the given ports including any\n");
    fprintf (stderr, "corrections set by Jack's -I and -O options.\n");
    exit (1);
}


static void procoptions (int ac, char *av [], const char *where)
{
    int k;
    
    optind = 1;
    opterr = 0;
    while ((k = getopt (ac, av, options)) != -1)
    {
        if (optarg && (*optarg == '-'))
        {
            fprintf (stderr, "\n%s\n", where);
	    fprintf (stderr, "  Missing argument for '-%c' option.\n", k); 
            fprintf (stderr, "  Use '-h' to see all options.\n");
            exit (1);
        }
	switch (k)
	{
        case 'h' :
            help ();
            exit (0);
 	case 'E' :
	    E_opt = true;
            break;
 	case 'I' :
	    I_val = optarg;
            break;
 	case 'O' :
	    O_val = optarg;
            break;
        case '?':
            fprintf (stderr, "\n%s\n", where);
            if (optopt != ':' && strchr (options, optopt))
	    {
                fprintf (stderr, "  Missing argument for '-%c' option.\n", optopt); 
	    }
            else if (isprint (optopt))
	    {
                fprintf (stderr, "  Unknown option '-%c'.\n", optopt);
	    }
            else
	    {
                fprintf (stderr, "  Unknown option character '0x%02x'.\n", optopt & 255);
	    }
            fprintf (stderr, "  Use '-h' to see all options.\n");
            exit (1);
        default:
            abort ();
 	}
    }
}


static void sigint_handler (int)
{
    signal (SIGINT, SIG_IGN);
    active = false;
}


int main (int ac, char *av [])
{
    double         d, dcapt, dplay, t;
    char           b [1024]; 
    jack_status_t  s;
    jack_latency_range_t range;

    procoptions (ac, av, "On command line:");
    if (E_opt)
    {
	if (! I_val || ! O_val)
	{
	    fprintf (stderr, "The -E option requires the use of -I and -O.\n");
	    return 1;
	}
    }

    jack_handle = jack_client_open ("jack_delay", JackNoStartServer, &s);
    if (jack_handle == 0)
    {
        fprintf (stderr, "Can't connect to Jack, is the server running ?\n");
        return 1;
    }

    jack_set_process_callback (jack_handle, jack_callback, 0);
    if (jack_activate (jack_handle))
    {
        fprintf(stderr, "Can't activate Jack");
        return 1;
    }
    jack_name = jack_get_client_name (jack_handle);
    jack_capt = jack_port_register (jack_handle, "in",  JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
    jack_play = jack_port_register (jack_handle, "out", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    mtdm = new MTDM (jack_get_sample_rate (jack_handle));

    if (O_val)
    {
	sprintf (b, "%s:out", jack_name);
	if (jack_connect (jack_handle, b, O_val))
	{
	    fprintf (stderr, "Can't connect '%s' to '%s'.\n", b, O_val);
	    exit (1);
	}
    }
    if (I_val)
    {
	sprintf (b, "%s:in", jack_name);
	if (jack_connect (jack_handle, I_val, b))
	{
	    fprintf (stderr, "Can't connect '%s' to '%s'.\n", I_val, b);
	    exit (1);
	}
    }
//    jack_port_get_latency_range (jack_capt, JackCaptureLatency, &range);
//    printf ("Latency on input port:     %6d %6d\n", range.min, range.max);
//    jack_port_get_latency_range (jack_play, JackPlaybackLatency, &range);
//    printf ("Latency on output port:    %6d %6d\n", range.min, range.max);
    
    if (E_opt)
    {
          jack_port_get_latency_range (jack_port_by_name (jack_handle, I_val), JackCaptureLatency, &range);
	  printf ("Latency on capture port:   %6d %6d\n", range.min, range.max);
	  dcapt = 0.5 * (range.max + range.min);
	  jack_port_get_latency_range (jack_port_by_name (jack_handle, O_val), JackPlaybackLatency, &range);
	  printf ("Latency on playback port:  %6d %6d\n", range.min, range.max);
	  dplay = 0.5 * (range.max + range.min);
    }
    else dcapt = dplay = 0;

    signal (SIGINT, sigint_handler);
    t = 1000.0 / jack_get_sample_rate (jack_handle);
    active = true;
    while (active)
    {
	usleep (250000);
	if (mtdm->resolve () < 0) printf ("Signal below threshold...\n");
	else 
	{
	    if (mtdm->err () > 0.35f) 
	    {
		mtdm->invert ();
		mtdm->resolve ();
	    }
	    d = mtdm->del ();
	    if (E_opt) d -= dcapt + dplay;
	    if (mtdm->err () > 0.30f) printf ("???  ");
            else if (mtdm->inv ()) printf ("Inv  ");
	    else printf ("     ");
	    printf ("%10.3lf frames %8.3lf ms\n", d, d * t);
	}
	fflush (stdout);
    }

    jack_deactivate (jack_handle);
    jack_client_close (jack_handle);
    return 0;
}

