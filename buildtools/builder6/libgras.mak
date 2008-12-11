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
PROJECT=C:\buildslave\projects\simgrid\builddir\build\build\builder6\libgras\dll\debug\libgras.dll

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
	
 
CFLAGS=-tWD -X- -tWM -c -nlibgras\obj -Od -r- -b- -k -y -v -vi- -a8 -p-

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

LFLAGS = -llibgras\lib\debug -Ilibgras\obj -c- -aa -Tpd -x -Gn -Gi -w -v

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
LIBPATH = $(BCB)\Lib;$(BCB)\Lib\obj;libgras\obj

WARNINGS=-w-aus -w-ccc -w-csu -w-dup -w-inl -w-par -w-pch -w-pia -w-rch -w-rvl -w-sus 

# Liste des chemins d'inclusion
INCLUDEPATH=$(BCB)\include;..\..\src\amok\PeerManagement;..\..\src\amok\Bandwidth;..\..\src\amok;..\..\src\gras\Virtu;..\..\src\gras\Msg;..\..\src\gras\DataDesc;..\..\src\gras\Transport;..\..\src\gras;..\..\src\include;..\..\src;..\..\src\xbt;..\..\include

# Macro d�finies par l'utilisateur.
# Cette macro permet de d�finir l'ensemble des fonctions export�es dans la DLL
USERDEFINES=DLL_EXPORT	


# Macros du syst�me d�finies
# On utilise pas la VCL ni la RTL dynamique et on utilise le mode de controle de type NO_STRICT
SYSDEFINES=NO_STRICT;_NO_VCL;_RTLDLL

# Liste des chemins des r�pertoires qui contiennent les fichiers de code source .c
SRCDIR=libgras;..\..\src\xbt;..\..\src\gras;..\..\src\gras\Virtu;..\..\src\gras\Msg;..\..\src\gras\DataDesc;..\..\src\gras\Transport;..\..\src\amok;..\..\src\amok\Bandwidth;..\..\src\amok\PeerManagement

# On demande au compilateur de rechercher les fichiers de code source c dans la liste des chemins d�finis ci-dessus
!if $d(SRCDIR)
.path.c   = $(SRCDIR)
!endif


# Liste des fichiers objets � g�n�r�s et dont d�pendent la dll
OBJFILES = libgras.obj heap.obj log.obj mallocator.obj \
set.obj snprintf.obj swag.obj sysdep.obj xbt_main.obj \
xbt_matrix.obj xbt_peer.obj asserts.obj config.obj cunit.obj \
dict.obj dict_multi.obj dynar.obj ex.obj fifo.obj graph.obj \
graphxml_parse.obj rl_process.obj rl_emul.obj rl_dns.obj \
rl_time.obj process.obj gras_module.obj timer.obj rpc.obj \
rl_msg.obj msg.obj ddt_parse.yy.obj ddt_exchange.obj \
datadesc.obj ddt_create.obj ddt_convert.obj cbps.obj \
ddt_parse.obj transport_plugin_file.obj rl_transport.obj \
transport_plugin_tcp.obj transport.obj rl_stubs.obj gras.obj \
log_default_appender.obj dict_elm.obj dict_cursor.obj \
getline.obj xbt_thread.obj amok_base.obj saturate.obj \
bandwidth.obj peermanagement.obj

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
