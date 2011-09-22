#!/bin/bash

#___________________________________________________________________________________________________
#Ubuntu 9.10________________________________________________________________________________________
ssh pipol pipol-sub esn i386-linux-ubuntu-karmic.dd.gz none 02:00 "~/Experimental_bindings.sh"
ssh pipol pipol-sub esn amd64-linux-ubuntu-karmic.dd.gz none 02:00 "~/Experimental_bindings.sh"

#Ubuntu 10.04
ssh pipol pipol-sub esn i386-linux-ubuntu-lucid.dd.gz none 02:00 "~/Experimental_bindings.sh"
ssh pipol pipol-sub esn amd64-linux-ubuntu-lucid.dd.gz none 02:00 "~/Experimental_bindings.sh"

#Ubuntu 10.10
ssh pipol pipol-sub esn amd64_2010-linux-ubuntu-maverick.dd.gz none 02:00 "~/Experimental_bindings.sh"

#___________________________________________________________________________________________________
#Fedora 12__________________________________________________________________________________________
ssh pipol pipol-sub esn i386-linux-fedora-core12.dd.gz none 02:00 "~/Experimental_bindings.sh"
ssh pipol pipol-sub esn amd64-linux-fedora-core12.dd.gz none 02:00 "~/Experimental_bindings.sh"

#Fedora 13
ssh pipol pipol-sub esn i386-linux-fedora-core13.dd.gz none 02:00 "~/Experimental_bindings.sh"
ssh pipol pipol-sub esn amd64-linux-fedora-core13.dd.gz none 02:00 "~/Experimental_bindings.sh"

#Fedora 14
ssh pipol pipol-sub esn amd64_2010-linux-fedora-core14.dd.gz none 02:00 "~/Experimental_bindings.sh"
ssh pipol pipol-sub esn i386_2010-linux-fedora-core14.dd.gz none 02:00 "~/Experimental_bindings.sh"

#___________________________________________________________________________________________________
#Debian Lenny 5.0___________________________________________________________________________________
ssh pipol pipol-sub esn i386_kvm-linux-debian-lenny none 02:00 "~/Experimental_bindings.sh"
ssh pipol pipol-sub esn amd64_kvm-linux-debian-lenny none 02:00 "~/Experimental_bindings.sh"

#Debian Lenny 6.0
ssh pipol pipol-sub esn amd64_2010-linux-debian-squeeze.dd.gz none 02:00 "~/Experimental_bindings.sh"
ssh pipol pipol-sub esn i386_2010-linux-debian-squeeze.dd.gz none 02:00 "~/Experimental_bindings.sh"

#Debian Testing
ssh pipol pipol-sub esn i386_kvm-linux-debian-testing none 02:00 "~/Experimental_bindings.sh"
ssh pipol pipol-sub esn amd64_kvm-linux-debian-testing none 02:00 "~/Experimental_bindings.sh"

#___________________________________________________________________________________________________
#MacOS Snow Leopard 10.6____________________________________________________________________________
ssh pipol pipol-sub esn x86_mac-mac-osx-server-snow-leopard-navarro-2011-09-22-113726.dd.gz none 02:00 "~/Experimental_bindings.sh"

#___________________________________________________________________________________________________
#windows-server-2008-64bits_________________________________________________________________________
#ssh pipol pipol-sub esn amd64-windows-server-2008-64bits.dd.gz none 02:00 "~/Experimental_bindings.sh"