# Ce makefile permet la construction de la DLL simgrid.

.autodepend

# R�pertoire d'installation du compilateur
!if !$d(BCB)
BCB = $(MAKEDIR)\..
!endif

# On utilise le compilateur BCC32
!if !$d(BCC32)
BCC32 = bcc32
!endif

# On utilise le linker ilink32
!if !$d(LINKER)
LINKER = ilink32
!endif

# Le nom du projet de compilation (ici la DLL)
PROJECT=C:\buildslave\projects\simgrid\builddir\build\build\builder6\simgrid\dll\debug\simgrid.dll

# Les options du compilateur BCC32
# -tWD		G�n�rer une DLL
# -tWM		G�n�rer une DLL multithread
# -c		Ne pas effectuer la liaison
# -n		Le r�pertoire est le r�pertoire obj
# -Od		D�sactiver toutes les optiomisations du compilateur
# -r-		D�sactivation de l'utilisation des variables de registre
# -b-		Les �num�rations ont une taille de un octet si possible
# -k-		Activer le cadre de pile standart
# -y		G�n�rer les num�ros de ligne pour le d�bogage
# -v		Activer le d�bogage du code source
# -vi-		Ne pas controler le d�veloppement des fonctions en ligne
# -a8		Aligner les donn�es sur une fronti�re de 8 octets
# -p-		Utiliser la convention d'appel C
	
 
#CFLAGS=-tWD -Vmp -X- -tWM- -c -nobj -Od -Vx -Ve -r- -b- -k -y -v -vi- 
CFLAGS=-tWD -X- -tWM -c -nsimgrid\obj -Od -r- -b- -k -y -v -vi- -a8 -p-

# Les options du linker
# -l		R�pertoire de sortie interm�diaire
# -I		Chemin du r�pertoire de sortie interm�diaire
# -c-		La liaison n'est pas sensible � la casse
# -aa		?
# -Tpd		?
# -x		Supprimer la cr�ation du fichier map
# -Gn		Ne pas g�n�rer de fichier d'�tat
# -Gi		G�n�rer la biblioth�que d'importation
# -w		Activer tous les avertissements		
# -v		Inclure les informations de d�bogage compl�tes

#LFLAGS = -l lib\debug -Iobj -D"" -c- -aa -Tpd -x -Gn -Gi -w -v
LFLAGS = -lsimgrid\lib\debug -Isimgrid\obj -c- -aa -Tpd -x -Gn -Gi -w -v

# Liste des avertissements d�sactiv�s
# -w-aus	Une valeur est affect�e mais n'est jamais utilis�e
# -w-ccc	Une condition est toujours vraie ou toujours fausse
# -w-csu	Comparaison entre une valeur sign�e et une valeur non sign�e
# -w-dup	La red�finition d'une macro n'est pas identique
# -w-inl	Les fonctions ne sont pas d�velopp�es inline
# -w-par	Le param�tre n'est jamais utilis�
# -w-pch	Impossible de cr�er l'en-t�te pr�compil�e
# -w-pia	Affectation incorrecte possible
# -w-rch	Code inatteignable
# -w-rvl	La fonction doit renvoyer une valeur
# -w-sus	Conversion de pointeur suspecte

# Chemins des r�pertoires contenant des librairies (importation ou de code objet)
LIBPATH = $(BCB)\Lib;$(BCB)\Lib\obj;simgrid\obj

WARNINGS=-w-aus -w-ccc -w-csu -w-dup -w-inl -w-par -w-pch -w-pia -w-rch -w-rvl -w-sus 

# Liste des chemins d'inclusion
INCLUDEPATH=..\..\src\amok\PeerManagement;..\..\src\amok;..\..\src\simdag;..\..\src\msg;..\..\src\surf;..\..\src\gras\Virtu;..\..\src\gras\Msg;..\..\src\gras\DataDesc;..\..\src\gras\Transport;..\..\src\gras;..\..\src\xbt;..\..\build\builder6\dll;$(BCB)\include;..\..\src;..\..\include;..\..\src\include;$(BCB)\include\dinkumware 

# Macro d�finies par l'utilisateur.
# Cette macro permet de d�finir l'ensemble des fonctions export�es dans la DLL
USERDEFINES=DLL_EXPORT	


# Macros du syst�me d�finies
# On utilise pas la VCL ni la RTL dynamique et on utilise le mode de controle de type NO_STRICT
SYSDEFINES=NO_STRICT;_NO_VCL;_RTLDLL

# Liste des chemins des r�pertoires qui contiennent les fichiers de code source .c
SRCDIR=simgrid;..\..\src\gras;..\..\src\msg;..\..\src\xbt;..\..\src\gras\Transport;..\..\src\gras\DataDesc;..\..\src\gras\Msg;..\..\src\gras\Virtu;..\..\src\surf;..\..\src\simdag;..\..\src\gras\Transport;..\..\src\gras\Virtu;..\..\src\gras\Msg;..\..\src\amok;..\..\src\amok\PeerManagement

# On demande au compilateur de rechercher les fichiers de code source c dans la liste des chemins d�finis ci-dessus
!if $d(SRCDIR)
.path.c   = $(SRCDIR)
!endif


# Liste des fichiers objets � g�n�r�s et dont d�pendent la dll
OBJFILES = simgrid.obj snprintf.obj graphxml_parse.obj heap.obj \
    log.obj log_default_appender.obj mallocator.obj set.obj \
    swag.obj sysdep.obj xbt_main.obj xbt_matrix.obj \
    asserts.obj config.obj cunit.obj dict.obj \
    dict_cursor.obj dict_elm.obj dict_multi.obj dynar.obj \
    ex.obj fifo.obj graph.obj gras.obj transport.obj \
    ddt_parse.yy.obj cbps.obj datadesc.obj ddt_convert.obj \
    ddt_create.obj ddt_exchange.obj ddt_parse.obj timer.obj \
    msg.obj rpc.obj process.obj gras_module.obj \
    surf.obj trace_mgr.obj maxmin.obj \
    workstation_KCCFLN05.obj cpu.obj network.obj \
    network_dassf.obj surf_timer.obj surfxml_parse.obj \
    workstation.obj deployment.obj environment.obj global.obj \
    gos.obj host.obj m_process.obj msg_config.obj task.obj \
    sd_workstation.obj sd_global.obj sd_link.obj sd_task.obj \
    transport_plugin_sg.obj sg_transport.obj sg_dns.obj \
    sg_emul.obj sg_process.obj sg_time.obj sg_msg.obj \
    xbt_peer.obj context.obj amok_base.obj peermanagement.obj 

LIBFILES =
ALLOBJ = c0d32.obj $(OBJFILES)

ALLLIB = $(LIBFILES) import32.lib cw32i.lib

# Compilation de la DLL
$(PROJECT): $(OBJFILES)
	$(BCB)\BIN\$(LINKER) @&&!
    $(LFLAGS) -L$(LIBPATH) +
    $(ALLOBJ), +
    $(PROJECT),, +
    $(ALLLIB)
!

# Comme implicite de compilation des fichiers de code source c en fichier objet (pas de liaison)	
.c.obj:
	$(BCB)\BIN\$(BCC32) $(CFLAGS) $(WARNINGS) -I$(INCLUDEPATH) -D$(USERDEFINES);$(SYSDEFINES) {$< }
