# PATH and MANPATH initialization -
# there are several different path subvariables
# changed by various files


OS=`/bin/uname`
ARCH=`/bin/uname -m`
BINTYPE=$ARCH.$OS

PATH=$HOME/bin
MANPATH=$HOME/man
NNTPSERVER=news.cs.arizona.edu
#LD_LIBRARY_PATH=$HOME/lib/$BINTYPE
#PERLLIB=$HOME/lib/perl5:$HOME/lib/perl5/$BINTYPE

# OS/Machine specific stuff
#if [ "$OS" == "Linux" ]
#then
  PATH=$PATH:/usr/local/bin:/sbin:/usr/sbin
  PATH=$PATH:/bin:/usr/bin:/usr/X11R6/bin:/usr/local/sbin
  PATH=$PATH:.
  MANPATH=$MANPATH:/usr/man:/usr/local/man:/usr/X11/man
#else
#  # assume lectura
#  PATH=$PATH:/usr/local/bin:/bin:/sbin:/usr/sbin
#  PATH=$PATH:/usr/ccs/bin:/opt/SUNWspro/bin
#  PATH=$PATH:/usr/ucb:/etc:/usr/X11/bin:/usr/openwin/bin
#  PATH=$PATH:.
#  MANPATH=$MANPATH:/usr/share/man:/usr/local/man:/usr/X11/man:
#  MANPATH=$MANPATH:/usr/openwin/man:/opt/unsupported/man
#  TEXINPUTS=.:$HOME/tex:/usr/local/texmf/tex/latex2e/base
#  TEXINPUTS=$TEXINPUTS:/usr/local/texmf/tex-unsupported/latex209/base
#  TEXINPUTS=$TEXINPUTS:/usr/local/texmf-Feb95/tex/latex2e/graphics
#  #stty erase '^H'
#  stty echoke
#fi


# this causes a script to run that requests the user change their password the
# first time that they log in - it then erases itself
/bin/sh .FIRST_TIME

