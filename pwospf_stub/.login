# Default login initialization file
#

# 
#  try to ascertain type of Operating System and then set path accordingly
#     examples of expected answers are  "5" = Solaris, "V3" = OSF, "4"=SunOS
set OSrelease=0
if ( -f /usr/bin/uname ) then
  set OSrelease=`/usr/bin/uname -r | awk -F. '{print $1}'`

  if ( "$OSrelease" == "5" ) then       # OS is System V - Solaris
    set path=( /usr/local/bin /bin /usr/ccs/bin /opt/SUNWspro/bin /usr/sbin /opt/cygnus/bin /usr/ccs/bin  /usr/etc  /etc  /usr/openwin/bin /usr/X11/bin /usr/bin /opt/unsupported/bin /opt/SUNWmotif/bin /usr/ucb . )
     setenv MANPATH /usr/share/man:/usr/local/man:/usr/X11R5/man:/usr/man:/opt/unsupported/man
#    Set terminal characteristic
	stty kill '^?'  erase '^H'  intr '^C' echoe  echoke

  else		# for non Solaris systems use the following settings

   set path=( /usr/local/bin  /etc /usr/etc /usr/hosts /usr/ucb /usr/bin /bin ~/bin /usr/local  /usr/openwin/bin /usr/bin/X11 . )
#   Set terminal characteristic
	stty kill '^?'  erase '^H'  intr '^C' 
  endif
endif
 

# set default permissions on file creation
umask 077

# set file completion
set filec

# set history feature
set history = 100

# set feature to stop ctrl-D from logging you out
set ignoreeof

# do not allow others to use talk to get to me
mesg n

# set variable HOST to be the current machine name
if ("$OSrelease" == "5" ) then
  set shorth="`uname -n|sed -e 's/\(...\).*/\1/'`"
else
  set shorth="`hostname|sed -e 's/\(...\).*/\1/'`"
endif

# set default mail handler
alias mail /usr/ucb/mail

# set a default printer for both System V and BSD type systems
setenv PRINTER lw27
setenv LPDEST lw27

set shortn=`whoami| awk '{print $1}'`
set prompt = "${shorth}:${shortn}>"
# set a prompt consisting of three letters of machine plus working directory
## REMOVE THE '#' from the next 3 lines if you want the pwd as your prompt
#set prompt = "${shorth}:`pwd` >"
#set noclobber
#alias cd 'cd \!*; set prompt = "${shorth}:`pwd` >"'

# set some useful alias's
alias rm "rm -i"
alias mv "mv -i"
alias cp "cp -i"
alias ls  /bin/ls -F
alias man "man -F"

# Core dumps limited to 0Mbytes.
limit coredumpsize 0
 
# Do screen initialization, default is xterm.  (Insert vt100, wyse75, or
# whatever you use the most for your default.)
## this variable already set by previos scripts. 
## uncomment if you want to change terminal type (for csh or tcsh)
## set noglob
#setenv TERM vt100
## unset noglob

# this causes a script to run that requests the user change their password the
# first time that they log in - it then erases itself
/bin/sh .FIRST_TIME

