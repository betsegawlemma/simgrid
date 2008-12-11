//---------------------------------------------------------------------------

#include <windows.h>
//---------------------------------------------------------------------------
//   Remarque importante concernant la gestion de m�moire de DLL lorsque votre DLL utilise la
//   version statique de la biblioth�que d'ex�cution :
//
//   Si votre DLL exporte des fonctions qui passent des objets String (ou des
//   structures/classes contenant des cha�nes imbriqu�es) comme param�tre
//   ou r�sultat de fonction, vous devrez ajouter la biblioth�que MEMMGR.LIB
//   � la fois au projet DLL et � tout projet qui utilise la DLL.  Vous devez aussi
//   utiliser MEMMGR.LIB si un projet qui utilise la DLL effectue des op�rations
//   new ou delete sur n'importe quelle classe non d�riv�e de TObject qui est
//   export�e depuis la DLL. Ajouter MEMMGR.LIB � votre projet forcera la DLL et
//   ses EXE appelants � utiliser BORLNDMM.DLL comme gestionnaire de m�moire.
//   Dans ce cas, le fichier BORLNDMM.DLL devra �tre d�ploy� avec votre DLL.
//
//   Pour �viter d'utiliser BORLNDMM.DLL, passez les cha�nes comme param�tres "char *"
//   ou ShortString.
//
//   Si votre DLL utilise la version dynamique de la RTL, vous n'avez pas besoin
//   d'ajouter MEMMGR.LIB, car cela est fait automatiquement.
//---------------------------------------------------------------------------

#pragma argsused
int WINAPI DllEntryPoint(HINSTANCE hinst, unsigned long reason, void* lpReserved)
{
	return 1;
}
//---------------------------------------------------------------------------
